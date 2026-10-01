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


class EditorGateTests(unittest.TestCase):
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

    def test_second_agent_cannot_acquire_or_release(self):
        gate.acquire("Art")
        with self.claude():
            with self.assertRaises(gate.GateError):
                gate.acquire("Gameplay")
            with self.assertRaises(gate.GateError):
                gate.release()
            gate.request("Need Blueprint editor")
        state = gate.status()
        self.assertEqual(state["owner"]["agent"], "codex-art")
        self.assertEqual(state["requests"][0]["agent"], "claude-gameplay")

    def test_handoff_requires_outgoing_editor_closed(self):
        gate.acquire()
        with self.claude():
            gate.request()
        with mock.patch.object(gate, "project_editors", return_value=[{"pid": 42}]):
            with self.assertRaises(gate.GateError):
                gate.release()
        with mock.patch.object(gate, "project_editors", return_value=[]):
            gate.release()
        with self.claude():
            owner = gate.acquire()
            self.assertEqual(owner["project"], str((self.b / "Happiness.uproject").resolve()))
            self.assertEqual(gate.status()["requests"], [])

    def test_inflight_operation_blocks_release_and_parallel_calls(self):
        gate.acquire()
        with gate.operation("import texture"):
            with self.assertRaises(gate.GateError):
                gate.release()
            with self.assertRaises(gate.GateError):
                with gate.operation("another call"):
                    pass
        self.assertEqual(gate.status()["operations"], [])
        gate.release()

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

    def test_foreign_editor_rejected(self):
        processes = [{"pid": 10, "project": str(self.a / "Happiness.uproject")},
                     {"pid": 20, "project": str(self.b / "Happiness.uproject")}]
        with mock.patch.object(gate, "editor_processes", return_value=processes):
            with self.assertRaises(gate.GateError):
                gate.verify_single_editor()
            self.assertEqual(gate.project_editors()[0]["pid"], 10)

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
        with mock.patch.object(gate, "editor_processes", return_value=[{"pid": 10}]):
            with self.assertRaises(gate.GateError):
                gate.recover(True)
        with self.assertRaises(gate.GateError):
            gate.recover(False)

    def test_atomic_acquisition_across_processes(self):
        script_folder = str(Path(gate.__file__).resolve().parent)
        code = ("import sys; sys.path.insert(0, sys.argv.pop(1)); import editor_gate; "
                "editor_gate.editor_processes=lambda: []; editor_gate.main()")
        contenders = []
        for identity, cwd in [("agent-one", self.a), ("agent-two", self.b)]:
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

    def test_start_does_not_launch_when_foreign_editor_open(self):
        with mock.patch.object(gate, "editor_processes", return_value=[
                {"pid": 20, "project": str(self.b / "Happiness.uproject")}]), \
                mock.patch.object(editor.subprocess, "Popen") as launch:
            with self.assertRaises(gate.GateError):
                editor.start(1)
            launch.assert_not_called()
        self.assertIsNone(gate.status()["owner"])
        self.assertEqual(len(gate.status()["requests"]), 1)


if __name__ == "__main__":
    unittest.main()
