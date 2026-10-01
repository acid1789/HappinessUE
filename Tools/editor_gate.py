"""Machine-local ownership gate shared by all Happiness checkouts (stdlib only).

Set HAPPINESS_AGENT_ID to a unique session name, e.g. codex-art or claude-gameplay.
python Tools/editor_gate.py acquire --task "Inspect menu"
python Tools/editor_gate.py status
python Tools/editor_gate.py request --task "Need editor for UI"
python Tools/editor_gate.py release
"""
import argparse
from contextlib import contextmanager
import ctypes
from ctypes import wintypes
import glob
import json
import os
from pathlib import Path
import sqlite3
import subprocess
import time
import uuid
from urllib.parse import urlparse


class GateError(RuntimeError):
    pass


def project_file():
    # Resolve from the caller's checkout, never from a hardcoded clone.
    for folder in (Path.cwd(), *Path.cwd().parents):
        candidates = list(folder.glob("*.uproject"))
        if len(candidates) == 1:
            return candidates[0].resolve()
        if len(candidates) > 1:
            raise GateError(f"Multiple .uproject files in {folder}")
    raise GateError("Run from a project checkout containing a .uproject file")


def canonical(path):
    return os.path.normcase(os.path.abspath(str(path))).replace("\\", "/").casefold()


def agent_id():
    value = os.environ.get("HAPPINESS_AGENT_ID", "").strip()
    if not value:
        raise GateError("Set HAPPINESS_AGENT_ID to a unique agent/session name first")
    return value


def state_dir():
    override = os.environ.get("HAPPINESS_EDITOR_GATE_DIR")
    if override:
        return Path(override).resolve()
    local = os.environ.get("LOCALAPPDATA")
    if not local:
        raise GateError("LOCALAPPDATA is missing; set HAPPINESS_EDITOR_GATE_DIR")
    return Path(local) / "Happiness" / "AgentGate"


@contextmanager
def transaction():
    folder = state_dir()
    folder.mkdir(parents=True, exist_ok=True)
    db = sqlite3.connect(folder / "gate.sqlite3", timeout=10)
    try:
        db.execute("BEGIN IMMEDIATE")
        db.execute("CREATE TABLE IF NOT EXISTS state (id INTEGER PRIMARY KEY, data TEXT NOT NULL)")
        row = db.execute("SELECT data FROM state WHERE id=1").fetchone()
        state = json.loads(row[0]) if row else {"owner": None, "requests": [], "operations": []}
        yield state
        db.execute("INSERT OR REPLACE INTO state VALUES (1, ?)", (json.dumps(state),))
        db.commit()
    except BaseException:
        db.rollback()
        raise
    finally:
        db.close()


def require_owner(state):
    owner = state["owner"]
    if not owner or owner["agent"] != agent_id() or canonical(owner["project"]) != canonical(project_file()):
        who = f"{owner['agent']} in {owner['project']}" if owner else "nobody"
        raise GateError(f"Editor gate owned by {who}. Acquire it before using editor tools")
    owner["heartbeat"] = time.time()
    return owner


def acquire(task=""):
    project = project_file()
    editors = editor_processes()
    foreign = [e for e in editors if not e["project"] or canonical(e["project"]) != canonical(project)]
    if foreign:
        request(task)
        raise GateError(f"Another editor is open: {foreign}. Handoff requested; its owner must save and close it")
    with transaction() as state:
        if state["owner"]:
            owner = require_owner(state)
            if task:
                owner["task"] = task
        else:
            owner = {"agent": agent_id(), "project": str(project), "task": task,
                     "acquired": time.time(), "heartbeat": time.time(), "editor_pid": None}
            state["owner"] = owner
        state["requests"] = [r for r in state["requests"] if r["agent"] != owner["agent"]]
        return dict(owner)


def request(task=""):
    with transaction() as state:
        identity = agent_id()
        state["requests"] = [r for r in state["requests"] if r["agent"] != identity]
        state["requests"].append({"agent": identity, "project": str(project_file()),
                                  "task": task, "requested": time.time()})
        return state


def status():
    with transaction() as state:
        return state


def release():
    with transaction() as state:
        owner = require_owner(state)
        if state["operations"]:
            raise GateError("An editor operation is in progress; finish it before releasing")
        other_checkout = any(canonical(r["project"]) != canonical(owner["project"]) for r in state["requests"])
        if other_checkout and project_editors(owner["project"]):
            raise GateError("Another checkout is waiting. Save, stop YOUR editor, then release")
        state["owner"] = None
        return state


@contextmanager
def operation(label):
    token = uuid.uuid4().hex
    with transaction() as state:
        require_owner(state)
        # Serialize operations even when several tool processes use the same owner identity.
        if state["operations"]:
            raise GateError("Another editor tool call is in progress; wait for it to finish")
        state["operations"].append({"token": token, "pid": os.getpid(), "label": label, "started": time.time()})
    try:
        yield
    finally:
        with transaction() as state:
            state["operations"] = [op for op in state["operations"] if op["token"] != token]
            if state["owner"]:
                state["owner"]["heartbeat"] = time.time()


def record_editor(pid):
    with transaction() as state:
        require_owner(state)["editor_pid"] = pid


def windows_argv(command):
    if os.name != "nt":
        raise GateError("Editor instance discovery currently supports Windows")
    count = ctypes.c_int()
    parse = ctypes.windll.shell32.CommandLineToArgvW
    parse.argtypes = [wintypes.LPCWSTR, ctypes.POINTER(ctypes.c_int)]
    parse.restype = ctypes.POINTER(wintypes.LPWSTR)
    pointer = parse(command, ctypes.byref(count))
    if not pointer:
        raise GateError("Could not parse an editor process command line")
    try:
        return [pointer[i] for i in range(count.value)]
    finally:
        free = ctypes.windll.kernel32.LocalFree
        free.argtypes = [ctypes.c_void_p]
        free.restype = ctypes.c_void_p
        free(pointer)


def editor_processes():
    command = ("$ErrorActionPreference='Stop'; "
               "@(Get-CimInstance Win32_Process -Filter \"Name = 'UnrealEditor.exe'\" | "
               "Select-Object ProcessId,CommandLine,ExecutablePath) | ConvertTo-Json -Compress")
    result = subprocess.run(["powershell.exe", "-NoProfile", "-NonInteractive", "-Command", command],
                            capture_output=True, text=True, check=True,
                            creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    rows = json.loads(result.stdout.strip() or "[]")
    if isinstance(rows, dict):
        rows = [rows]
    editors = []
    for row in rows:
        if not row.get("CommandLine"):
            raise GateError(f"Cannot identify editor PID {row['ProcessId']}; refusing ambiguous targeting")
        args = windows_argv(row["CommandLine"])
        projects = [arg for arg in args[1:] if arg.lower().endswith(".uproject")]
        editors.append({"pid": int(row["ProcessId"]), "project": projects[0] if len(projects) == 1 else None})
    return editors


def project_editors(project=None):
    target = canonical(project or project_file())
    return [e for e in editor_processes() if e["project"] and canonical(e["project"]) == target]


def verify_single_editor():
    target = project_file()
    editors = editor_processes()
    # Unknown projects are unsafe, as they may own the shared MCP port.
    foreign = [e for e in editors if not e["project"] or canonical(e["project"]) != canonical(target)]
    if foreign:
        raise GateError(f"Another editor is open: {foreign}. Its owner must save and close it before handoff")
    own = [e for e in editors if e["project"] and canonical(e["project"]) == canonical(target)]
    if len(own) != 1:
        raise GateError(f"Expected one editor for {target}, found {len(own)}")
    return own[0]["pid"]


def endpoint_for(project, pid):
    pattern = str(Path(os.environ["LOCALAPPDATA"]) / "UmgMcp" / "instances" / "*.json")
    matches = []
    for filename in glob.glob(pattern):
        try:
            info = json.loads(Path(filename).read_text(encoding="utf-8"))
            if (canonical(info.get("project_file", "")) == canonical(project)
                    and int(info.get("process_id", -1)) == pid):
                matches.append(info)
        except (OSError, ValueError, TypeError):
            continue
    if len(matches) != 1:
        raise GateError(f"Expected one live UMG endpoint for project {project} PID {pid}, found {len(matches)}")
    return matches[0]["host"], int(matches[0]["port"])


def checked_endpoint():
    with transaction() as state:
        require_owner(state)
    pid = verify_single_editor()
    host, port = endpoint_for(project_file(), pid)
    record_editor(pid)
    return host, port


def verify_mcp_listener(url):
    parsed = urlparse(url)
    if parsed.scheme != "http" or parsed.hostname not in ("127.0.0.1", "localhost", "::1"):
        raise GateError("UE_MCP_URL must point to the local gated editor")
    port = parsed.port or 80
    expected = verify_single_editor()
    command = (f"$ErrorActionPreference='Stop'; @(Get-NetTCPConnection -State Listen -LocalPort {port} "
               "-ErrorAction SilentlyContinue | Select-Object -ExpandProperty OwningProcess) | ConvertTo-Json -Compress")
    result = subprocess.run(["powershell.exe", "-NoProfile", "-NonInteractive", "-Command", command],
                            capture_output=True, text=True, check=True,
                            creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    listeners = json.loads(result.stdout.strip() or "[]")
    if isinstance(listeners, int):
        listeners = [listeners]
    if not listeners or set(listeners) != {expected}:
        raise GateError(f"MCP port {port} is not exclusively owned by editor PID {expected}: {listeners}")


def recover(confirmed):
    if not confirmed:
        raise GateError("Recovery needs --confirm-abandoned, after confirming the old agent is gone")
    with transaction() as state:
        if editor_processes():
            raise GateError("Recovery refused while an editor is running; arrange a safe handoff")
        if any(pid_alive(op["pid"]) for op in state["operations"]):
            raise GateError("Recovery refused while an editor tool process is still running")
        state["owner"] = None
        state["operations"] = []
        return state


def pid_alive(pid):
    open_process = ctypes.windll.kernel32.OpenProcess
    open_process.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    open_process.restype = wintypes.HANDLE
    close_handle = ctypes.windll.kernel32.CloseHandle
    close_handle.argtypes = [wintypes.HANDLE]
    handle = open_process(0x1000, False, pid)
    if handle:
        close_handle(handle)
        return True
    # Access denied is not evidence that a process is gone.
    return ctypes.windll.kernel32.GetLastError() == 5


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=["status", "acquire", "request", "release", "heartbeat", "recover"])
    parser.add_argument("--task", default="")
    parser.add_argument("--agent", help="Override HAPPINESS_AGENT_ID for this call")
    parser.add_argument("--confirm-abandoned", action="store_true")
    args = parser.parse_args()
    if args.agent:
        os.environ["HAPPINESS_AGENT_ID"] = args.agent
    try:
        if args.command == "recover":
            result = recover(args.confirm_abandoned)
        elif args.command == "heartbeat":
            with transaction() as state:
                result = dict(require_owner(state))
        else:
            function = globals()[args.command]
            result = function(args.task) if args.command in ("acquire", "request") else function()
        print(json.dumps(result, indent=2))
    except (GateError, OSError, sqlite3.Error, subprocess.SubprocessError) as error:
        parser.exit(1, f"Editor gate: {error}\n")


if __name__ == "__main__":
    main()
