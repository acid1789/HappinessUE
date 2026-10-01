"""Run: python -m unittest discover -s Tools/tests -v (does not drive Unreal)."""
from contextlib import contextmanager
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import editor_gate as gate
import editor
import umg
import mcp_call


class GateTestCase(unittest.TestCase):
    """Two checkouts in a temp folder, a private gate database, no real editors."""

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        root = Path(self.temp.name)
        self.a = root / "Art Checkout"
        self.b = root / "Gameplay Checkout"
        for folder in (self.a, self.b):
            folder.mkdir()
            (folder / "Happiness.uproject").write_text('{}')
        self.old_cwd = Path.cwd()
        os.chdir(self.a)
        self.env = mock.patch.dict(os.environ, {"HAPPINESS_EDITOR_GATE_DIR": str(root / "state"),
                                                "HAPPINESS_AGENT_ID": "codex-art"})
        self.env.start()
        self.processes = mock.patch.object(gate, "editor_processes", return_value=[])
        self.processes.start()

    def tearDown(self):
        os.chdir(self.old_cwd)
        self.env.stop()
        self.processes.stop()
        self.temp.cleanup()

    @contextmanager
    def claude(self):
        with mock.patch.dict(os.environ, {"HAPPINESS_AGENT_ID": "claude-gameplay"}):
            os.chdir(self.b)
            try:
                yield
            finally:
                os.chdir(self.a)

    @contextmanager
    def agent(self, identity):
        with mock.patch.dict(os.environ, {"HAPPINESS_AGENT_ID": identity}):
            yield

    def entry(self, folder):
        return gate.status()["checkouts"][gate.canonical((folder / "Happiness.uproject").resolve())]


class EditorGateTests(GateTestCase):
    def test_checkouts_are_owned_independently(self):
        gate.acquire("Art")
        with self.claude():
            owner = gate.acquire("Gameplay")
            self.assertEqual(owner["project"], str((self.b / "Happiness.uproject").resolve()))
            with gate.operation("compile"):
                pass
        self.assertEqual(self.entry(self.a)["owner"]["agent"], "codex-art")
        self.assertEqual(self.entry(self.b)["owner"]["agent"], "claude-gameplay")
        with gate.operation("render"):
            pass
        gate.release()
        self.assertIsNone(self.entry(self.a)["owner"])
        self.assertEqual(self.entry(self.b)["owner"]["agent"], "claude-gameplay")

    def test_second_agent_in_same_checkout_queues(self):
        gate.acquire("Art")
        with self.agent("ron-playtest"):
            with self.assertRaises(gate.GateError):
                gate.acquire("Playtest")
            with self.assertRaises(gate.GateError):
                gate.release()
        entry = self.entry(self.a)
        self.assertEqual(entry["owner"]["agent"], "codex-art")
        self.assertEqual(entry["requests"][0]["agent"], "ron-playtest")

    def test_handoff_within_checkout_keeps_editor(self):
        gate.acquire()
        with self.agent("ron-playtest"):
            gate.request("Playtest")
        # Releasing never needs the editor closed: the next owner takes it over
        with mock.patch.object(gate, "project_editors", return_value=[{"pid": 42}]):
            gate.release()
        with self.agent("ron-playtest"):
            gate.acquire("Playtest")
        entry = self.entry(self.a)
        self.assertEqual(entry["owner"]["agent"], "ron-playtest")
        self.assertEqual(entry["requests"], [])

    def test_inflight_operation_blocks_release_and_parallel_calls(self):
        gate.acquire()
        with gate.operation("import texture"):
            with self.assertRaises(gate.GateError):
                gate.release()
            with self.assertRaises(gate.GateError):
                with gate.operation("another call"):
                    pass
        self.assertEqual(self.entry(self.a)["operations"], [])
        gate.release()

    def test_each_checkout_has_its_own_mcp_port(self):
        port_a = gate.mcp_port()
        with self.claude():
            port_b = gate.mcp_port()
        self.assertEqual(port_a, gate.FIRST_MCP_PORT)
        self.assertNotEqual(port_a, port_b)
        self.assertEqual(gate.mcp_port(), port_a)
        self.assertEqual(gate.mcp_url(), f"http://127.0.0.1:{port_a}/mcp")

    def test_old_single_owner_state_is_upgraded(self):
        project = str((self.a / "Happiness.uproject").resolve())
        old = {"owner": {"agent": "codex-art", "project": project, "task": "", "acquired": 0, "heartbeat": 0,
                         "editor_pid": None},
               "requests": [{"agent": "claude-gameplay", "project": str((self.b / "Happiness.uproject").resolve()),
                             "task": "", "requested": 0}],
               "operations": []}
        upgraded = gate.upgrade(old)
        self.assertEqual(upgraded["checkouts"][gate.canonical(project)]["owner"]["agent"], "codex-art")
        other = gate.canonical((self.b / "Happiness.uproject").resolve())
        self.assertEqual(upgraded["checkouts"][other]["requests"][0]["agent"], "claude-gameplay")

    def test_failed_call_removes_operation(self):
        gate.acquire()
        with self.assertRaises(ValueError):
            with gate.operation("failed operation"):
                raise ValueError("network failure")
        gate.release()

    def test_exact_project_from_subdirectory(self):
        nested = self.a / "Source"
        nested.mkdir()
        os.chdir(nested)
        self.assertEqual(gate.project_file(), (self.a / "Happiness.uproject").resolve())

    def test_no_identity_refuses_acquire(self):
        with mock.patch.dict(os.environ, {"HAPPINESS_AGENT_ID": ""}):
            with self.assertRaises(gate.GateError):
                gate.acquire()

    def test_same_agent_name_cannot_use_other_checkout(self):
        gate.acquire()
        os.chdir(self.b)
        with self.assertRaises(gate.GateError):
            gate.acquire()

    def test_other_checkouts_editor_is_ignored(self):
        processes = [{"pid": 10, "project": str(self.a / "Happiness.uproject")},
                     {"pid": 20, "project": str(self.b / "Happiness.uproject")}]
        with mock.patch.object(gate, "editor_processes", return_value=processes):
            self.assertEqual(gate.verify_single_editor(), 10)
            self.assertEqual(gate.project_editors()[0]["pid"], 10)

    def test_second_editor_on_same_checkout_rejected(self):
        # e.g. a Standalone Game launched from this checkout's editor
        processes = [{"pid": 10, "project": str(self.a / "Happiness.uproject")},
                     {"pid": 11, "project": str(self.a / "Happiness.uproject")}]
        with mock.patch.object(gate, "editor_processes", return_value=processes):
            with self.assertRaises(gate.GateError):
                gate.verify_single_editor()

    def test_discovery_filters_project_and_pid(self):
        instance_folder = Path(self.temp.name) / "local" / "UmgMcp" / "instances"
        instance_folder.mkdir(parents=True)
        for name, project, pid, port in [("correct", self.a, 10, 1010),
                                          ("stale", self.a, 9, 1009),
                                          ("foreign", self.b, 20, 1020)]:
            (instance_folder / f"{name}.json").write_text(json.dumps({
                "project_file": str(project / "Happiness.uproject"),
                "process_id": pid, "host": "127.0.0.1", "port": port}))
        with mock.patch.dict(os.environ, {"LOCALAPPDATA": str(instance_folder.parents[1])}):
            self.assertEqual(gate.endpoint_for(self.a / "Happiness.uproject", 10), ("127.0.0.1", 1010))

    def test_clients_block_network_without_gate(self):
        with mock.patch.object(umg.socket, "create_connection") as connect:
            with self.assertRaises(gate.GateError):
                umg.send("127.0.0.1", 1234, "get_widget_tree", {})
            connect.assert_not_called()
        with mock.patch.object(mcp_call.urllib.request, "urlopen") as post:
            with self.assertRaises(gate.GateError):
                mcp_call.post({"method": "tools/call"})
            post.assert_not_called()

    def test_mcp_rejects_wrong_listener_pid(self):
        with mock.patch.object(gate, "verify_single_editor", return_value=10), \
                mock.patch.object(gate.subprocess, "run", return_value=mock.Mock(stdout='[20]')):
            with self.assertRaises(gate.GateError):
                gate.verify_mcp_listener("http://127.0.0.1:8000/mcp")

    def test_recovery_never_steals_running_editor(self):
        gate.acquire()
        with mock.patch.object(gate, "editor_processes", return_value=[
                {"pid": 10, "project": str(self.a / "Happiness.uproject")}]):
            with self.assertRaises(gate.GateError):
                gate.recover(True)
        with self.assertRaises(gate.GateError):
            gate.recover(False)
        # Another checkout's editor doesn't block recovering this one
        with mock.patch.object(gate, "editor_processes", return_value=[
                {"pid": 20, "project": str(self.b / "Happiness.uproject")}]):
            gate.recover(True)
        self.assertIsNone(self.entry(self.a)["owner"])

    def test_atomic_acquisition_across_processes(self):
        # Two agents racing for the same checkout: exactly one wins
        script_folder = str(Path(gate.__file__).resolve().parent)
        code = ("import sys; sys.path.insert(0, sys.argv.pop(1)); import editor_gate; "
                "editor_gate.editor_processes=lambda: []; editor_gate.main()")
        contenders = []
        for identity, cwd in [("agent-one", self.a), ("agent-two", self.a)]:
            env = dict(os.environ, HAPPINESS_AGENT_ID=identity)
            contenders.append(subprocess.Popen([sys.executable, "-c", code, script_folder, "acquire"], cwd=cwd,
                                              env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE))
        results = [process.communicate(timeout=15) for process in contenders]
        self.assertEqual(sorted(process.returncode for process in contenders), [0, 1], results)

    @unittest.skipUnless(os.name == "nt", "Windows command-line parser")
    def test_windows_paths_with_spaces(self):
        command = r'"E:\Epic Games\UnrealEditor.exe" "E:\Art Checkout\Happiness.uproject" -unattended'
        self.assertEqual(gate.windows_argv(command)[1], r"E:\Art Checkout\Happiness.uproject")

    def test_stop_never_targets_foreign_checkout(self):
        gate.acquire()
        with mock.patch.object(gate, "editor_processes", return_value=[
                {"pid": 20, "project": str(self.b / "Happiness.uproject")}]), \
                mock.patch.object(editor.subprocess, "run") as kill:
            editor.stop(1)
            kill.assert_not_called()

    def test_start_launches_own_editor_beside_another_checkouts(self):
        engine = Path(self.temp.name) / "Engine"
        (engine / "Binaries" / "Win64").mkdir(parents=True)
        (engine / "Binaries" / "Win64" / "UnrealEditor.exe").write_text("")
        with mock.patch.object(gate, "editor_processes", return_value=[
                {"pid": 20, "project": str(self.b / "Happiness.uproject")}]), \
                mock.patch.dict(os.environ, {"UE_ENGINE_DIR": str(engine)}), \
                mock.patch.object(editor.subprocess, "Popen", return_value=mock.Mock(pid=30)) as launch:
            with self.assertRaises(gate.GateError):  # never becomes ready: nothing really launched
                editor.start(0)
            args = launch.call_args[0][0]
        port = gate.mcp_port()
        self.assertIn(f"-ModelContextProtocolPort={port}", args)
        self.assertIn("-ModelContextProtocolStartServer", args)
        settings = (self.a / "Saved" / "Config" / "WindowsEditor" / "EditorPerProjectUserSettings.ini").read_text()
        self.assertIn(f"ServerPortNumber={port}", settings)
        self.assertEqual(self.entry(self.a)["owner"]["agent"], "codex-art")

    def test_start_queues_behind_other_agent_in_same_checkout(self):
        with self.agent("ron-playtest"):
            gate.acquire("Playtest")
        with mock.patch.object(editor.subprocess, "Popen") as launch:
            with self.assertRaises(gate.GateError):
                editor.start(1)
            launch.assert_not_called()
        entry = self.entry(self.a)
        self.assertEqual(entry["owner"]["agent"], "ron-playtest")
        self.assertEqual(entry["requests"][0]["agent"], "codex-art")

    def test_mcp_settings_keep_other_lines(self):
        path = self.a / "Saved" / "Config" / "WindowsEditor" / "EditorPerProjectUserSettings.ini"
        path.parent.mkdir(parents=True)
        path.write_text("[Other]\r\nKeep=1\r\n\r\n" + editor.MCP_SECTION + "\r\nbAutoStartServer=True\r\n"
                        "ServerPortNumber=8000\r\nbEnableToolSearch=True\r\n\r\n[Later]\r\nAlso=2\r\n", newline="")
        editor.write_mcp_settings(self.a / "Happiness.uproject", 8001)
        text = path.read_text()
        self.assertIn("Keep=1", text)
        self.assertIn("Also=2", text)
        self.assertIn("bEnableToolSearch=True", text)
        self.assertIn("ServerPortNumber=8001", text)
        self.assertNotIn("ServerPortNumber=8000", text)
        self.assertEqual(text.count(editor.MCP_SECTION), 1)


class FileLockTests(GateTestCase):
    """File locks are shared by every checkout and enforced by the editor tool clients."""

    def setUp(self):
        super().setUp()
        # No real git: each test sets what it needs
        self.unshared = mock.patch.object(gate, "unshared_changes", return_value="")
        self.behind = mock.patch.object(gate, "behind_upstream", return_value=False)
        self.unshared.start()
        self.behind.start()

    def tearDown(self):
        self.unshared.stop()
        self.behind.stop()
        super().tearDown()

    def test_lock_keys_are_checkout_independent(self):
        for path in ["/Game/Happiness/UI/WBP_Happiness", "/Game/Happiness/UI/WBP_Happiness.WBP_Happiness",
                     "/Game/Happiness/UI/WBP_Happiness.WBP_Happiness_C",
                     "/Game/Happiness/UI/WBP_Happiness.WBP_Happiness:DoHint.K2Node_CallFunction_7",
                     "Content/Happiness/UI/WBP_Happiness.uasset",
                     str(self.b / "Content" / "Happiness" / "UI" / "WBP_Happiness.uasset")]:
            self.assertEqual(gate.lock_key(path), "/Game/Happiness/UI/WBP_Happiness", path)
        self.assertEqual(gate.lock_key("Source\\Happiness\\UI\\HintInfoWidget.cpp"),
                         "Source/Happiness/UI/HintInfoWidget.cpp")

    def test_lock_conflicts_across_checkouts(self):
        gate.lock(["/Game/Happiness/UI/WBP_Happiness"], "Hint button art")
        gate.lock(["/Game/Happiness/UI/WBP_Happiness"])  # relocking your own lock is fine
        with self.claude():
            with self.assertRaises(gate.GateError):
                gate.lock(["Content/Happiness/UI/WBP_Happiness.uasset"])
            with self.assertRaises(gate.GateError):
                gate.unlock(["/Game/Happiness/UI/WBP_Happiness"])
            gate.lock(["/Game/Happiness/UI/WBP_EndScreen"])
        self.assertEqual(gate.locks()["/Game/Happiness/UI/WBP_Happiness"]["agent"], "codex-art")
        self.assertEqual(gate.locks()["/Game/Happiness/UI/WBP_EndScreen"]["agent"], "claude-gameplay")

    def test_lock_needs_latest_version(self):
        with mock.patch.object(gate, "behind_upstream", return_value=True):
            with self.assertRaises(gate.GateError):
                gate.lock(["/Game/Happiness/UI/WBP_Happiness"])
        self.assertEqual(gate.locks(), {})

    def test_unlock_waits_for_shared_history(self):
        gate.lock(["/Game/Happiness/UI/WBP_Happiness"])
        with mock.patch.object(gate, "unshared_changes", return_value="uncommitted changes"):
            with self.assertRaises(gate.GateError):
                gate.unlock(["/Game/Happiness/UI/WBP_Happiness"])
            self.assertIn("/Game/Happiness/UI/WBP_Happiness", gate.locks())
            gate.unlock(["/Game/Happiness/UI/WBP_Happiness"], force=True)
        self.assertEqual(gate.locks(), {})

    def test_mcp_changes_need_locks(self):
        def call(tool, arguments):
            return {"method": "tools/call", "params": {"name": "call_tool", "arguments": {
                "toolset_name": "x", "tool_name": tool, "arguments": arguments}}}
        graph = {"graph": {"refPath": "/Game/Happiness/UI/WBP_Happiness.WBP_Happiness:DoHint"}}
        self.assertEqual(mcp_call.changed_assets(call("read_graph_dsl", graph)), [])
        self.assertEqual(mcp_call.changed_assets(call("RenderWidget", {"widgetBlueprintPath": "/Game/A/WBP_X"})), [])
        self.assertEqual([gate.lock_key(p) for p in mcp_call.changed_assets(call("create_node", graph))],
                         ["/Game/Happiness/UI/WBP_Happiness"])
        imported = mcp_call.changed_assets(call("import_file", {"folder_path": "/Game/Art/", "asset_name": "T_Sky",
                                                                "source_file": "E:/x.png"}))
        self.assertEqual([gate.lock_key(p) for p in imported], ["/Game/Art/T_Sky"])

        gate.acquire()
        with mock.patch.object(gate, "checked_endpoint"), mock.patch.object(gate, "verify_mcp_listener"), \
                mock.patch.object(mcp_call, "_post", return_value=({}, None)) as send:
            with self.assertRaises(gate.GateError):
                mcp_call.post(call("create_node", graph))
            send.assert_not_called()
            mcp_call.post(call("get_node_infos", graph))  # reading needs no lock
            gate.lock(["/Game/Happiness/UI/WBP_Happiness"])
            mcp_call.post(call("create_node", graph))
            self.assertEqual(send.call_count, 2)

    def test_umg_changes_need_lock_on_target(self):
        gate.acquire()
        target = {"status": "success", "asset_path": "/Game/Happiness/UI/WBP_EndScreen.WBP_EndScreen"}
        with mock.patch.object(umg, "find_endpoint", return_value=("127.0.0.1", 1)), \
                mock.patch.object(umg, "_send", return_value=target) as send:
            with self.assertRaises(gate.GateError):  # no target yet
                umg.send("127.0.0.1", 1, "set_widget_properties", {"widget_name": "TotalLabel"})
            umg.send("127.0.0.1", 1, "set_target_umg_asset", {"asset_path": "/Game/Happiness/UI/WBP_EndScreen"})
            umg.send("127.0.0.1", 1, "get_widget_tree", {})
            with self.assertRaises(gate.GateError):
                umg.send("127.0.0.1", 1, "set_widget_properties", {"widget_name": "TotalLabel"})
            gate.lock(["/Game/Happiness/UI/WBP_EndScreen"])
            umg.send("127.0.0.1", 1, "set_widget_properties", {"widget_name": "TotalLabel"})
            self.assertEqual(send.call_count, 3)


if __name__ == "__main__":
    unittest.main()
