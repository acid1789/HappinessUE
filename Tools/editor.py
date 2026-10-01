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


def mcp_ready():
    request = urllib.request.Request(os.environ.get("UE_MCP_URL", "http://127.0.0.1:8000/mcp"),
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
    try:
        gate.acquire(task)
    except gate.GateError:
        gate.request(task or "Need editor")
        raise
    with gate.operation("start editor"):
        editors = gate.editor_processes()
        if any(not e["project"] or gate.canonical(e["project"]) != gate.canonical(project) for e in editors):
            raise gate.GateError(f"Another checkout's editor is open: {editors}. Request a handoff")
        if len(editors) > 1:
            raise gate.GateError("Multiple editors are open for this checkout; resolve before continuing")
        if not editors:
            executable = engine_directory(project) / "Binaries" / "Win64" / "UnrealEditor.exe"
            child = subprocess.Popen([str(executable), str(project), "-nosplash", "-unattended"],
                                     cwd=project.parent, stdin=subprocess.DEVNULL,
                                     stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                     creationflags=subprocess.CREATE_NO_WINDOW)
            gate.record_editor(child.pid)
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            try:
                gate.checked_endpoint()
                url = os.environ.get("UE_MCP_URL", "http://127.0.0.1:8000/mcp")
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
