"""Compact dump of a Blueprint graph's nodes: title, and each linked pin with its links.
    python Tools/bp_dump.py <graph refPath> [title filter]
"""
import json
import sys

from bp_swap_node import Session


def short(ref):
    return ref.rsplit(".", 1)[-1]


def main():
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    graph = sys.argv[1]
    title = sys.argv[2] if len(sys.argv) > 2 else ""
    s = Session()
    nodes = s.call("find_nodes", {"graph": {"refPath": graph}, "title": title})
    infos = s.call("get_node_infos", {"nodes": nodes})
    for ref, info in zip(nodes, infos):
        name = info.get("title") or info.get("name") or ""
        print(f"{short(ref['refPath'])}: {name}".replace("\n", " "))
        for kind in ("input_pins", "output_pins"):
            for pin in info.get(kind, []):
                links = [f"{short(c['node']['refPath'])}#{c['index_id']}" for c in pin.get("connected_pins", [])]
                if links or (kind == "input_pins" and pin.get("value") not in ("", None)):
                    arrow = "<-" if kind == "input_pins" else "->"
                    val = f" ={pin['value']}" if pin.get("value") else ""
                    print(f"    {pin['name']}#{pin['pin_id']['index_id']}{val} {arrow} {', '.join(links)}")


if __name__ == "__main__":
    main()
