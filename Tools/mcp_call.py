"""Call one Unreal MCP (Epic ModelContextProtocol) toolset tool over HTTP, without an MCP client.

    python Tools/mcp_call.py <toolset> <tool> ['{"json": "arguments"}'] [--max N]

Example:
    python Tools/mcp_call.py editor_toolset.toolsets.blueprint.BlueprintTools get_parent \
        '{"blueprint": {"refPath": "/Game/Happiness/UI/WBP_CampaignTree.WBP_CampaignTree"}}'

Use Tools/mcp.sh -find / -describe first to learn a tool's arguments. The editor must be running with
the MCP server started (Tools/editor.sh start does that). Each checkout's editor serves on its own port
(Tools/editor_gate.py mcp_port); UE_MCP_URL overrides the URL.
"""
import json
import sys
import urllib.request
import editor_gate as gate


def changed_assets(payload):
    """The assets a tools/call may change: every /Game path in its arguments, unless the tool only reads.
    An import names a folder and a new asset; the lock is on the new asset. A duplicate only reads its source."""
    if payload.get("method") != "tools/call":
        return []
    call = payload.get("params", {}).get("arguments", {})
    tool, arguments = call.get("tool_name", ""), call.get("arguments", {})
    if gate.is_read_only(tool) or not isinstance(arguments, dict):
        return []
    arguments = dict(arguments)
    if tool == "duplicate":
        arguments.pop("path", None)
    paths = []
    if isinstance(arguments.get("folder_path"), str) and isinstance(arguments.get("asset_name"), str):
        paths.append(arguments.pop("folder_path").rstrip("/") + "/" + arguments.pop("asset_name"))
    return paths + gate.game_paths(arguments)


def post(payload, session=None):
    with gate.operation("Epic MCP " + payload.get("method", "request")):
        gate.checked_endpoint()
        gate.require_locks(changed_assets(payload))
        url = gate.mcp_url()
        gate.verify_mcp_listener(url)
        return _post(url, payload, session)


def _post(url, payload, session=None):
    headers = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream"}
    if session:
        headers["Mcp-Session-Id"] = session
    request = urllib.request.Request(url, data=json.dumps(payload).encode("utf-8"), headers=headers, method="POST")
    with urllib.request.urlopen(request, timeout=300) as response:
        session = response.headers.get("Mcp-Session-Id") or session
        body = response.read().decode("utf-8")

    if not body.strip():
        return None, session
    # Streamable HTTP may answer as server-sent events; take the last data line
    if body.lstrip().startswith("event:") or body.lstrip().startswith("data:"):
        data = [line[5:].strip() for line in body.splitlines() if line.startswith("data:")]
        body = data[-1] if data else "null"
    return json.loads(body), session


def main():
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    args = sys.argv[1:]
    max_chars = 4000
    if "--max" in args:
        i = args.index("--max")
        max_chars = int(args[i + 1])
        del args[i : i + 2]
    if len(args) < 2:
        sys.exit(__doc__)

    toolset, tool = args[0], args[1]
    arguments = json.loads(args[2]) if len(args) > 2 else {}

    _, session = post({"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {
        "protocolVersion": "2025-03-26", "capabilities": {}, "clientInfo": {"name": "claude-code-cli", "version": "1.0"}}})
    post({"jsonrpc": "2.0", "method": "notifications/initialized"}, session)

    reply, _ = post({"jsonrpc": "2.0", "id": 2, "method": "tools/call", "params": {
        "name": "call_tool", "arguments": {"toolset_name": toolset, "tool_name": tool, "arguments": arguments}}}, session)

    text = json.dumps(reply, separators=(",", ":"), ensure_ascii=False)
    if len(text) > max_chars:
        text = text[:max_chars] + f"... [{len(text) - max_chars} more chars; use --max]"
    print(text)


if __name__ == "__main__":
    try:
        main()
    except gate.GateError as error:
        sys.exit(f"Editor gate: {error}")
