"""Replace one Blueprint graph node with a node of another type, keeping every wire (Epic MCP BlueprintTools).

    python Tools/bp_swap_node.py <graph refPath> <old node refPath> <new type_id> ['{"oldPin": "newPin"}']

Creates the new node next to the old one, reconnects each of the old node's links to the new node's pin with the
same name (or the mapped name), deletes the old node, and prints the new node's pins and links.
Find nodes with BlueprintTools.find_nodes and type ids with find_node_types.
"""
import json
import sys

sys.path.insert(0, __import__("os").path.dirname(__file__))
from mcp_call import post  # noqa: E402

BT = "editor_toolset.toolsets.blueprint.BlueprintTools"


class Session:
    def __init__(self):
        _, self.session = post({"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {
            "protocolVersion": "2025-03-26", "capabilities": {}, "clientInfo": {"name": "bp_swap_node", "version": "1.0"}}})
        post({"jsonrpc": "2.0", "method": "notifications/initialized"}, self.session)
        self.next_id = 2

    def call(self, tool, arguments):
        self.next_id += 1
        reply, _ = post({"jsonrpc": "2.0", "id": self.next_id, "method": "tools/call", "params": {
            "name": "call_tool", "arguments": {"toolset_name": BT, "tool_name": tool, "arguments": arguments}}}, self.session)
        result = reply["result"]
        text = result["content"][0]["text"]
        if result.get("isError"):
            raise RuntimeError(f"{tool}: {text[:600]}")
        return json.loads(text).get("returnValue")


def node_info(s, node_ref):
    return s.call("get_node_infos", {"nodes": [{"refPath": node_ref}]})[0]


def main():
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    if len(sys.argv) < 4:
        sys.exit(__doc__)
    graph, old_ref, new_type = sys.argv[1], sys.argv[2], sys.argv[3]
    pin_map = json.loads(sys.argv[4]) if len(sys.argv) > 4 else {}

    s = Session()
    old = node_info(s, old_ref)
    pos = old["position"]
    new_node = s.call("create_node", {"graph": {"refPath": graph}, "type_id": new_type,
                                      "pos": {"x": pos["x"], "y": pos["y"] + 220}, "declaring_class": None})
    new_ref = new_node["refPath"] if isinstance(new_node, dict) else new_node
    new = node_info(s, new_ref)
    new_pins = {(p["name"], p["pin_id"]["direction"]): p["pin_id"] for p in new["input_pins"] + new["output_pins"]}

    wired = []
    for pin in old["input_pins"] + old["output_pins"]:
        direction = pin["pin_id"]["direction"]
        target_name = pin_map.get(pin["name"], pin["name"])
        target = new_pins.get((target_name, direction))
        for other in pin.get("connected_pins", []):
            if target is None:
                raise RuntimeError(f"new node has no {direction} pin '{target_name}' for old pin '{pin['name']}'")
            if direction == "EGPD_Input":
                s.call("connect_pins", {"output_pin": other, "input_pin": target})
            else:
                s.call("connect_pins", {"output_pin": target, "input_pin": other})
            wired.append(f"{pin['name']}->{target_name}")

    s.call("delete_node", {"node": {"refPath": old_ref}})

    result = node_info(s, new_ref)
    print(f"new node {new_ref} ({new_type}); rewired: {', '.join(wired)}")
    for p in result["input_pins"] + result["output_pins"]:
        links = [c["node"]["refPath"].split(".")[-1] + f"[{c['index_id']}]" for c in p.get("connected_pins", [])]
        print(f"  {p['pin_id']['direction'][5:]:6} {p['name']:12} {p.get('value', '')!s:6} {links}")


if __name__ == "__main__":
    main()
