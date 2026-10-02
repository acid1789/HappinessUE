"""Machine-local editor ownership gate, one per Happiness checkout (stdlib only).

Each checkout runs its own editor, so agents in different checkouts work at the same time. Within a checkout,
one agent (or the user, e.g. for a playtest) owns that checkout's editor at a time. Each checkout also gets its
own Epic MCP port, so the editors' servers don't collide.

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


FIRST_MCP_PORT = 8000


@contextmanager
def transaction():
    folder = state_dir()
    folder.mkdir(parents=True, exist_ok=True)
    db = sqlite3.connect(folder / "gate.sqlite3", timeout=10)
    try:
        db.execute("BEGIN IMMEDIATE")
        db.execute("CREATE TABLE IF NOT EXISTS state (id INTEGER PRIMARY KEY, data TEXT NOT NULL)")
        row = db.execute("SELECT data FROM state WHERE id=1").fetchone()
        state = upgrade(json.loads(row[0]) if row else {})
        yield state
        db.execute("INSERT OR REPLACE INTO state VALUES (1, ?)", (json.dumps(state),))
        db.commit()
    except BaseException:
        db.rollback()
        raise
    finally:
        db.close()


def upgrade(state):
    # State is kept per checkout: {"checkouts": {canonical project: {project, owner, requests, operations,
    # mcp_port, umg_target}}, "locks": {asset key: lock}}. The first version had one global
    # owner/requests/operations; file each under its checkout.
    if "checkouts" in state:
        state.setdefault("locks", {})
        return state
    upgraded = {"checkouts": {}, "locks": {}}
    owner = state.get("owner")
    if owner:
        entry = checkout(upgraded, owner["project"])
        entry["owner"] = owner
        entry["operations"] = state.get("operations", [])
    for waiting in state.get("requests", []):
        checkout(upgraded, waiting["project"])["requests"].append(waiting)
    return upgraded


def checkout(state, project=None):
    project = project or project_file()
    key = canonical(project)
    if key not in state["checkouts"]:
        state["checkouts"][key] = {"project": str(project), "owner": None, "requests": [], "operations": [],
                                   "mcp_port": None}
    return state["checkouts"][key]


def require_owner(state):
    entry = checkout(state)
    owner = entry["owner"]
    if not owner or owner["agent"] != agent_id():
        who = owner["agent"] if owner else "nobody"
        raise GateError(f"This checkout's editor is owned by {who}. Acquire it before using editor tools")
    owner["heartbeat"] = time.time()
    return owner


def acquire(task=""):
    project = project_file()
    identity = agent_id()
    with transaction() as state:
        elsewhere = [e["project"] for key, e in state["checkouts"].items()
                     if key != canonical(project) and e["owner"] and e["owner"]["agent"] == identity]
        if elsewhere:
            raise GateError(f"{identity} already owns the editor of {elsewhere[0]}; release it there first")
        entry = checkout(state, project)
        holder = entry["owner"]
        if not holder:
            entry["owner"] = {"agent": identity, "project": str(project), "task": task,
                              "acquired": time.time(), "heartbeat": time.time(), "editor_pid": None}
        elif holder["agent"] == identity:
            holder["heartbeat"] = time.time()
            if task:
                holder["task"] = task
        if not holder or holder["agent"] == identity:
            entry["requests"] = [r for r in entry["requests"] if r["agent"] != identity]
            return dict(entry["owner"])
    # Someone else holds this checkout's editor: queue behind them
    request(task)
    raise GateError(f"This checkout's editor is owned by {holder['agent']} ({holder['task'] or 'no task'}). "
                    "Request recorded; retry when they release")


def request(task=""):
    with transaction() as state:
        identity = agent_id()
        entry = checkout(state)
        entry["requests"] = [r for r in entry["requests"] if r["agent"] != identity]
        entry["requests"].append({"agent": identity, "project": entry["project"], "task": task,
                                  "requested": time.time()})
        return state


def status():
    with transaction() as state:
        return state


def release():
    with transaction() as state:
        require_owner(state)
        entry = checkout(state)
        if entry["operations"]:
            raise GateError("An editor operation is in progress; finish it before releasing")
        # The editor stays open for whoever takes this checkout next
        entry["owner"] = None
        return state


@contextmanager
def operation(label):
    token = uuid.uuid4().hex
    with transaction() as state:
        require_owner(state)
        entry = checkout(state)
        # Serialize calls to this checkout's editor, even from several tool processes of the same owner. An entry
        # whose process is gone (killed mid-call) no longer holds the editor.
        entry["operations"] = [op for op in entry["operations"] if pid_alive(op["pid"])]
        if entry["operations"]:
            raise GateError("Another editor tool call is in progress; wait for it to finish")
        entry["operations"].append({"token": token, "pid": os.getpid(), "label": label, "started": time.time()})
    try:
        yield
    finally:
        with transaction() as state:
            entry = checkout(state)
            entry["operations"] = [op for op in entry["operations"] if op["token"] != token]
            if entry["owner"]:
                entry["owner"]["heartbeat"] = time.time()


def record_editor(pid):
    with transaction() as state:
        require_owner(state)["editor_pid"] = pid


def mcp_port(project=None):
    # Each checkout keeps its own Epic MCP port: the lowest from FIRST_MCP_PORT no other checkout uses
    with transaction() as state:
        entry = checkout(state, project)
        if not entry["mcp_port"]:
            taken = {e["mcp_port"] for e in state["checkouts"].values() if e["mcp_port"]}
            port = FIRST_MCP_PORT
            while port in taken:
                port += 1
            entry["mcp_port"] = port
        return entry["mcp_port"]


def mcp_url():
    return os.environ.get("UE_MCP_URL") or f"http://127.0.0.1:{mcp_port()}/mcp"


# ---- File locks, shared by all checkouts ----
# Each checkout has its own copy of every file, and git can't merge two checkouts' edits to the same .uasset.
# An agent locks a file before changing it; the editor tools refuse to change an asset without its lock.
# A lock is held until the change is committed and pushed, so the next agent edits the latest version.

def lock_key(path):
    """The checkout-independent name of a file: /Game/Folder/Asset for Unreal assets (from a /Game object or
    graph path, or a Content/... .uasset/.umap file), else the path relative to the checkout root."""
    text = str(path).strip().replace("\\", "/")
    if text.startswith("/Game/"):
        text = text.split(":", 1)[0]
        folder, _, name = text.rpartition("/")
        return f"{folder}/{name.split('.', 1)[0]}"
    file = Path(text)
    if file.is_absolute():
        # A path inside any checkout: relative to that checkout's root
        for folder in file.parents:
            if len(list(folder.glob("*.uproject"))) == 1:
                file = file.relative_to(folder)
                break
        else:
            # Git Bash turns a /Game/... argument into <Git install>/Game/...; take it back
            if "/Game/" in text:
                return lock_key(text[text.index("/Game/"):])
            raise GateError(f"{path} is not inside a project checkout")
    parts = file.as_posix().lstrip("./").split("/")
    if parts[0] == "Content" and file.suffix.lower() in (".uasset", ".umap"):
        return "/Game/" + "/".join(parts[1:])[: -len(file.suffix)]
    return "/".join(parts)


def lock_file(key, root=None):
    """The file a lock key names in a checkout (an existing .umap or else the .uasset for /Game keys)."""
    root = Path(root or project_file().parent)
    if key.startswith("/Game/"):
        base = root / "Content" / key[len("/Game/"):]
        level = base.with_name(base.name + ".umap")
        return level if level.exists() else base.with_name(base.name + ".uasset")
    return root / key


def git(*args, root=None):
    result = subprocess.run(["git", "-C", str(root or project_file().parent), *args], capture_output=True,
                            text=True, creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    return result.returncode, result.stdout.strip()


def unshared_changes(key):
    """Why this checkout's copy isn't in the shared history yet: uncommitted or unpushed changes ('' if none)."""
    file = lock_file(key).relative_to(project_file().parent).as_posix()
    code, changes = git("status", "--porcelain", "--", file)
    if code == 0 and changes:
        return "uncommitted changes"
    code, unpushed = git("log", "--oneline", "@{u}..HEAD", "--", file)
    if code == 0 and unpushed:
        return "unpushed commits"
    return ""


def behind_upstream(key):
    """True if the shared history has a newer version of the file than this checkout (after fetching)."""
    git("fetch", "--quiet")
    file = lock_file(key).relative_to(project_file().parent).as_posix()
    code, newer = git("log", "--oneline", "HEAD..@{u}", "--", file)
    return code == 0 and bool(newer)


def lock(paths, task=""):
    identity = agent_id()
    keys = [lock_key(p) for p in paths]
    stale = [k for k in keys if behind_upstream(k)]
    if stale:
        raise GateError(f"Your checkout is behind GitHub for {stale}; pull before locking")
    with transaction() as state:
        taken = [f"{k} ({state['locks'][k]['agent']}: {state['locks'][k]['task'] or 'no task'})"
                 for k in keys if k in state["locks"] and state["locks"][k]["agent"] != identity]
        if taken:
            raise GateError(f"Locked by another agent: {', '.join(taken)}")
        for key in keys:
            state["locks"][key] = {"agent": identity, "project": str(project_file()), "task": task,
                                   "locked": time.time()}
        return {k: state["locks"][k] for k in keys}


def unlock(paths, force=False):
    identity = agent_id()
    keys = [lock_key(p) for p in paths]
    if not force:
        pending = [f"{k} ({why})" for k in keys if (why := unshared_changes(k))]
        if pending:
            raise GateError(f"Not in the shared history yet: {', '.join(pending)}. Keep the lock until the user "
                            "commits and pushes (or unlock --force to abandon the change)")
    with transaction() as state:
        foreign = [k for k in keys if k in state["locks"] and state["locks"][k]["agent"] != identity]
        if foreign:
            raise GateError(f"Not your locks: {foreign}")
        for key in keys:
            state["locks"].pop(key, None)
        return state["locks"]


def locks():
    with transaction() as state:
        return state["locks"]


def require_locks(paths):
    """Refuse unless this agent holds the lock on every file named."""
    keys = sorted({lock_key(p) for p in paths})
    if not keys:
        return
    identity = agent_id()
    with transaction() as state:
        missing = [k for k in keys if state["locks"].get(k, {}).get("agent") != identity]
    if missing:
        raise GateError(f"Lock before changing: {missing}  (python Tools/editor_gate.py lock <path> --task ...)")


def game_paths(value):
    """Every /Game/... path mentioned anywhere in a JSON-like value."""
    if isinstance(value, str):
        return [value] if value.startswith("/Game/") else []
    if isinstance(value, dict):
        return [p for v in value.values() for p in game_paths(v)]
    if isinstance(value, (list, tuple)):
        return [p for v in value for p in game_paths(v)]
    return []


# Tools that only read: everything else that names an asset needs its lock
READ_ONLY_PREFIXES = ("get", "find", "read", "list", "describe", "search", "query", "is")
READ_ONLY_TOOLS = {"renderwidget", "startpie", "stoppie", "clickviewport"}


def is_read_only(tool):
    name = tool.lower()
    return name.startswith(READ_ONLY_PREFIXES) or name in READ_ONLY_TOOLS


def set_umg_target(path):
    # None when a fresh editor starts: UmgMcp forgets its target then
    with transaction() as state:
        checkout(state)["umg_target"] = lock_key(path) if path else None


def umg_target():
    with transaction() as state:
        return checkout(state).get("umg_target")


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
    # Other checkouts' editors are fine (each has its own MCP port; UmgMcp routes by project and PID).
    # This checkout must have exactly one, e.g. not also a "Standalone Game" launched from it.
    target = project_file()
    own = project_editors(target)
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
        entry = checkout(state)
        if project_editors(entry["project"]):
            raise GateError("Recovery refused while this checkout's editor is running; arrange a safe handoff")
        if any(pid_alive(op["pid"]) for op in entry["operations"]):
            raise GateError("Recovery refused while an editor tool process is still running")
        entry["owner"] = None
        entry["operations"] = []
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
    parser.add_argument("command", choices=["status", "acquire", "request", "release", "heartbeat", "recover",
                                            "lock", "unlock", "locks"])
    parser.add_argument("paths", nargs="*", help="lock/unlock: /Game/... asset paths or files in the checkout")
    parser.add_argument("--task", default="")
    parser.add_argument("--agent", help="Override HAPPINESS_AGENT_ID for this call")
    parser.add_argument("--confirm-abandoned", action="store_true")
    parser.add_argument("--force", action="store_true", help="unlock: drop the lock even with unshared changes")
    args = parser.parse_args()
    if args.agent:
        os.environ["HAPPINESS_AGENT_ID"] = args.agent
    try:
        if args.command in ("lock", "unlock") and not args.paths:
            raise GateError(f"{args.command} needs at least one path")
        if args.command == "lock":
            result = lock(args.paths, args.task)
        elif args.command == "unlock":
            result = unlock(args.paths, args.force)
        elif args.command == "locks":
            result = locks()
        elif args.command == "recover":
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
