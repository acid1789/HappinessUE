# Reading and editing Blueprint graphs

Blueprint graphs are edited with Epic's MCP **BlueprintTools** (`editor_toolset.toolsets.blueprint.BlueprintTools`).
`Tools/bp_swap_node.py` provides a `Session` class that wraps the HTTP calls, so a Python script can make many
calls in a row:

```python
import sys; sys.path.insert(0, r"E:\HappinessUE\Tools")
from bp_swap_node import Session, node_info
s = Session()
G = {"refPath": "/Game/Happiness/UI/WBP_Happiness.WBP_Happiness:DoHint"}   # graph = <asset>:<graph name>
r = s.call("find_node_types", {"graph": G, "type_id_filter": "IsValid", "context_pins": []})
```

## Reading

| What | How |
|---|---|
| A graph's names | `list_graphs {"blueprint": {"refPath": ".../WBP_X.WBP_X"}}` |
| A graph as compact pseudo-code | `read_graph_dsl {"graph": {"refPath": ".../WBP_X.WBP_X:GraphName"}}` |
| Every node with its pins and links | `python Tools/bp_dump.py "/Game/.../WBP_X.WBP_X:GraphName"` |
| One node's pins, links and position | `node_info(s, "<graph refPath>.K2Node_CallFunction_7")` |

Node references are the graph path plus the node name, e.g.
`/Game/Happiness/UI/WBP_Happiness.WBP_Happiness:RefreshPuzzle.K2Node_CallFunction_7`.

**The DSL can mislabel calls.** When several classes have a function of the same name, it may print the wrong
class (`CampaignEndScreen|Show` for the classic `EndScreen.Show`, `HintInfo|ShowHint` for a cell's `ShowHint`).
Check what a node really is with `bp_dump.py` / `node_info` before relying on the label.

## Editing

Lock the Blueprint first (`python Tools/editor_gate.py lock /Game/... --task "..."`): every call that changes
it, compiling included, is refused without the lock. Reading needs none
([EditorTooling.md](EditorTooling.md#file-locks)).

| Step | Call |
|---|---|
| Find a node type id | `find_node_types {"graph": G, "type_id_filter": "...", "context_pins": []}` |
| Create a node | `create_node {"graph": G, "type_id": "...", "pos": {"x": 0, "y": 0}, "declaring_class": null}` |
| Connect / disconnect | `connect_pins` / `break_pins {"output_pin": <pin_id>, "input_pin": <pin_id>}` |
| Set a literal pin value | `set_pin_value {"pin": <pin_id>, "value": "true"}` (enums by name, e.g. `"Campaign"`) |
| Delete a node | `delete_node {"node": {"refPath": "..."}}` |
| Add a local variable | `add_variable {"blueprint": {...}, "name": "bRetried", "type_name": "bool", "graph": G, "container_type": null}` |
| Swap a node for another type, keeping wires | `python Tools/bp_swap_node.py <graph> <old node> <new type id> ['{"oldPin": "newPin"}']` |

Pin ids come from `node_info` (`input_pins` / `output_pins`, each with `name`, `pin_id`, `connected_pins`).
Then compile, check the log for errors and save (see [EditorTooling.md](EditorTooling.md)). Read the graph back
with `read_graph_dsl` to confirm the result.

### Type ids that are easy to get wrong

- Branch: `Utilities|FlowControl|Branch` (pins `execute`, `Condition`, `then`, `else`).
- Is Valid with exec pins: `Utilities|IsValid` (pins `exec`, `InputObject`, outputs `Is Valid` / `Is Not Valid`).
- Object equality: `Utilities|Operators|Equal(==)`; enum equality: `Utilities|Enum|Equal(Enum)`.
- Variable getters/setters: `Variables|Default|GetX`, a component widget `Variables|<WidgetBlueprint>|GetX`.
  A new local variable shows its type id **without** its `b` prefix (`Variables|Default|GetRetried`) while its
  pin keeps the full name (`bRetried`).
- Function on another class: `Class|<ClassName>|<Function>`; on this Blueprint: `CallFunction|<Function>`.
- Reverse loop: `Utilities|Array|ReverseforEachLoop` (pins `ArrayElement`, `ArrayIndex`, no spaces; the normal
  ForEachLoop's pin is `Array Element`).

### Events from a widget's delegates

- Bound events ("On Clicked (MyButton)" style nodes) can't be created through these tools.
- Use the **Assign** node instead: `create_node` with `<Widget>|Assign<Delegate>` creates the bind node **and** a
  matching, properly named custom event (e.g. `OnNextPuzzle_Event`). Connect the widget getter to its `self`,
  put it in the Event Construct chain of `BindEventto...` nodes, and wire the event's `then`.
- Custom events can't be renamed through ObjectTools, so prefer the Assign route when the name matters.
- An exec input can take several incoming wires: two events can feed the same chain of nodes.

## Gotchas

- **ForEachLoop re-reads a pure array input every iteration.** Removing items from that array inside the loop
  (e.g. `RemoveFromParent` over `GetAllChildren`) skips every other item. Iterate with `ReverseforEachLoop`.
- **Local variables are per call,** so a function can call itself (used by `WBP_Happiness.DoHint`).
- After C++ changes to a widget's parent class, recompile the Widget Blueprint and the Blueprints that use it.
