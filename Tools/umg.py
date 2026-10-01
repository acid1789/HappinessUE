"""Send one command to the UmgMcp editor plugin over its local socket, without the MCP server.

    python Tools/umg.py <command> ['{"json": "params"}'] [--max N] [--timeout SECONDS]

Finds the exact checkout's editor from %LOCALAPPDATA%/UmgMcp/instances, uses an agent-specific client id,
sends the command, and prints the JSON reply (compact, truncated to --max characters, default 4000).
Command names and params are the plugin's own, e.g. set_target_umg_asset, get_widget_tree,
create_widget, set_widget_properties; see Plugins/UmgMcp/Resources/Python/UmgMcpServer.py.
"""
import json
import socket
import sys
import uuid
import hashlib
import editor_gate as gate

def client_id():
    checkout = hashlib.sha256(gate.canonical(gate.project_file()).encode()).hexdigest()[:12]
    return f"happiness-{gate.agent_id()}-{checkout}"


def find_endpoint():
    return gate.checked_endpoint()


READ_ONLY_COMMANDS = {"connect", "set_target_umg_asset", "get_target_umg_asset"}


def changed_assets(command, params):
    """The widget assets a command may change: the current target (and any asset it names), unless it only
    reads. Changing a widget needs its lock (Tools/editor_gate.py lock)."""
    if command in READ_ONLY_COMMANDS or command.startswith(("get_", "query_", "list_")):
        return []
    target = gate.umg_target()
    if not target:
        raise gate.GateError("No UMG target recorded; call set_target_umg_asset first")
    return [target] + gate.game_paths(params)


def send(host, port, command, params, timeout=30):
    with gate.operation(f"UMG {command}"):
        if (host, port) != find_endpoint():
            raise gate.GateError("UMG endpoint changed; reconnect to this checkout's editor")
        gate.require_locks(changed_assets(command, params))
        reply = _send(host, port, command, params, timeout)
        if command == "set_target_umg_asset" and isinstance(reply, dict) and reply.get("status") == "success":
            gate.set_umg_target(reply.get("asset_path") or params.get("asset_path", ""))
        return reply


def _send(host, port, command, params, timeout):
    request = {"command": command, "params": params, "client_id": client_id(), "request_id": str(uuid.uuid4())}
    with socket.create_connection((host, port), timeout=timeout) as s:
        s.sendall(json.dumps(request).encode("utf-8") + b"\0")
        s.shutdown(socket.SHUT_WR)
        chunks = []
        while True:
            chunk = s.recv(65536)
            if not chunk:
                break
            if b"\0" in chunk:
                chunks.append(chunk[: chunk.index(b"\0")])
                break
            chunks.append(chunk)
    return json.loads(b"".join(chunks).decode("utf-8") or "null")


def main():
    args = sys.argv[1:]
    max_chars = 4000
    timeout = 30
    if "--timeout" in args:
        i = args.index("--timeout")
        timeout = float(args[i + 1])
        del args[i : i + 2]
    if "--max" in args:
        i = args.index("--max")
        max_chars = int(args[i + 1])
        del args[i : i + 2]
    if not args:
        sys.exit(__doc__)

    command = args[0]
    params = json.loads(args[1]) if len(args) > 1 else {}

    host, port = find_endpoint()
    connected = send(host, port, "connect", {"display_name": gate.agent_id(), "exclusive": False})
    if connected and connected.get("status") == "error":
        sys.exit("connect failed: " + json.dumps(connected))

    reply = json.dumps(send(host, port, command, params, timeout), separators=(",", ":"), ensure_ascii=False)
    if len(reply) > max_chars:
        reply = reply[:max_chars] + f"... [{len(reply) - max_chars} more chars; use --max]"
    print(reply)


if __name__ == "__main__":
    try:
        main()
    except gate.GateError as error:
        sys.exit(f"Editor gate: {error}")
