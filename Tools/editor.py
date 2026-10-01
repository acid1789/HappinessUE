"""Checkout-aware editor lifecycle. Does not save assets or force-close an editor."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import time
import urllib.request
import winreg

import editor_gate as gate


def engine_directory(project):
    override = os.environ.get("UE_ENGINE_DIR")
    if override:
        candidates = [Path(override)]
    else:
        candidates = [project.parent / "Engine"]
        association = json.loads(project.read_text(encoding="utf-8-sig")).get("EngineAssociation", "")
        for hive, keypath, value_name in [
            (winreg.HKEY_CURRENT_USER, r"Software\Epic Games\Unreal Engine\Builds", association),
            (winreg.HKEY_LOCAL_MACHINE, rf"SOFTWARE\EpicGames\Unreal Engine\{association}", "InstalledDirectory"),
        ]:
            try:
                with winreg.OpenKey(hive, keypath) as key:
                    install = Path(winreg.QueryValueEx(key, value_name)[0])
                    candidates.append(install / "Engine")
            except OSError:
                continue
    for candidate in candidates:
        if (candidate / "Binaries" / "Win64" / "UnrealEditor.exe").is_file():
            return candidate.resolve()
    raise gate.GateError("Engine not found. Set UE_ENGINE_DIR to the installed Engine directory")


MCP_SECTION = "[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]"


def write_mcp_settings(project, port):
    """Store this checkout's MCP port and auto-start in its local (unversioned) editor settings, so an editor
    opened by hand on this checkout serves on the same port. Only call while this checkout's editor is closed:
    a running editor rewrites the file from memory when it exits."""
    path = project.parent / "Saved" / "Config" / "WindowsEditor" / "EditorPerProjectUserSettings.ini"
    lines = path.read_text(encoding="utf-8").splitlines() if path.is_file() else []
    wanted = {"ServerPortNumber": str(port), "bAutoStartServer": "True"}
    try:
        start = lines.index(MCP_SECTION) + 1
    except ValueError:
        lines += ["", MCP_SECTION]
        start = len(lines)
    end = start
    while end < len(lines) and not lines[end].startswith("["):
        end += 1
    body = [line for line in lines[start:end] if line.split("=", 1)[0] not in wanted and line.strip()]
    body += [f"{key}={value}" for key, value in wanted.items()]
    lines[start:end] = body + [""]
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\r\n".join(lines) + "\r\n", encoding="utf-8", newline="")


def mcp_ready():
    request = urllib.request.Request(gate.mcp_url(),
                                     data=b'{}', headers={"Content-Type": "application/json"}, method="POST")
    try:
        with urllib.request.urlopen(request, timeout=2):
            return True
    except urllib.error.HTTPError as error:
        return error.code in (400, 406)  # JSON-RPC parse error / missing Accept header: server is alive.
    except (OSError, urllib.error.URLError):
        return False


def start(timeout, task=""):
    project = gate.project_file()
    gate.acquire(task or "Need editor")  # records a request and raises if another agent holds this checkout
    with gate.operation("start editor"):
        # Other checkouts' editors don't matter: each checkout has its own editor and MCP port
        editors = gate.project_editors(project)
        if len(editors) > 1:
            raise gate.GateError("Multiple editors are open for this checkout; resolve before continuing")
        if not editors:
            port = gate.mcp_port(project)
            write_mcp_settings(project, port)
            executable = engine_directory(project) / "Binaries" / "Win64" / "UnrealEditor.exe"
            child = subprocess.Popen([str(executable), str(project), "-nosplash", "-unattended",
                                      f"-ModelContextProtocolPort={port}", "-ModelContextProtocolStartServer"],
                                     cwd=project.parent, stdin=subprocess.DEVNULL,
                                     stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                     creationflags=subprocess.CREATE_NO_WINDOW)
            gate.record_editor(child.pid)
            gate.set_umg_target(None)
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            try:
                gate.checked_endpoint()
                url = gate.mcp_url()
                gate.verify_mcp_listener(url)
                if mcp_ready():
                    print(f"Editor ready: {project} (PID {gate.verify_single_editor()})")
                    return
            except gate.GateError:
                pass
            time.sleep(2)
        raise gate.GateError(f"Editor tools not ready after {timeout}s. Editor and gate retained; inspect Saved/Logs")


def stop(timeout):
    with gate.operation("stop editor"):
        project = gate.project_file()
        editors = gate.project_editors(project)
        if not editors:
            gate.record_editor(None)
            print(f"Editor not running for {project}")
            return
        if len(editors) != 1:
            raise gate.GateError("Multiple matching editors; refusing ambiguous stop")
        pid = editors[0]["pid"]
        # Check identity again immediately before asking Windows to close this PID.
        if gate.project_editors(project) != editors:
            raise gate.GateError("Editor processes changed; retry after checking status")
        subprocess.run(["taskkill.exe", "/PID", str(pid)], check=True, capture_output=True,
                       creationflags=subprocess.CREATE_NO_WINDOW)
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if not gate.project_editors(project):
                gate.record_editor(None)
                print(f"Editor closed: {project}")
                return
            time.sleep(2)
        raise gate.GateError("Editor did not close (possibly an unsaved-assets dialog). Resolve it manually; no force kill")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=["start", "stop", "status", "request", "release", "heartbeat"])
    parser.add_argument("--agent")
    parser.add_argument("--task", default="")
    parser.add_argument("--timeout", type=int, default=None)
    args = parser.parse_args()
    if args.agent:
        os.environ["HAPPINESS_AGENT_ID"] = args.agent
    try:
        if args.command == "start":
            start(args.timeout or 360, args.task)
        elif args.command == "stop":
            stop(args.timeout or 60)
        elif args.command == "heartbeat":
            with gate.transaction() as state:
                print(json.dumps(gate.require_owner(state), indent=2))
        else:
            function = getattr(gate, args.command)
            print(json.dumps(function(args.task) if args.command == "request" else function(), indent=2))
    except (gate.GateError, OSError, subprocess.SubprocessError) as error:
        parser.exit(1, f"Editor: {error}\n")


if __name__ == "__main__":
    main()
