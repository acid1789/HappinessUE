# Editor tooling

Run from Git Bash in **your own checkout** (or a subdirectory); most tools need the editor open unless noted.
The gated Python tools also work from PowerShell. Engine discovery uses the checkout's `Engine` link,
Unreal's Windows installation registry, or an explicit `UE_ENGINE_DIR` pointing to the installed `Engine` directory.

## Starting, stopping, building

| Task | Command |
|---|---|
| Start the editor and wait until its tool servers answer | `Tools/editor.sh start` |
| Close only this checkout's editor (no forced shutdown) | `Tools/editor.sh stop` |
| Build the C++ (editor must be **closed**, it locks the DLL: LNK1104) | `Tools/puzzle.sh --build -seed=1` |

The build log is `Saved/PuzzleBuild.log`; look for `Result: Succeeded` and `error C`/`error LNK` lines.
After building, start the editor again and recompile any Widget Blueprints whose C++ parent changed.

## Shared editor gate

All checkouts share `%LOCALAPPDATA%/Happiness/AgentGate/gate.sqlite3`. SQLite transactions make acquisition
atomic; state includes the owning agent, full project path, task, heartbeat, editor PID, handoff requests,
and any tool operation in progress. Gate files are local runtime data, not Git artifacts.
`HAPPINESS_EDITOR_GATE_DIR` can override the directory; every agent must use the same absolute directory.

Choose one identity per active agent session, and keep it for subsequent shell calls:

```bash
export HAPPINESS_AGENT_ID=codex-art          # Claude uses claude-gameplay
Tools/editor.sh start --task "Inspect menu"
Tools/editor.sh status
python Tools/umg.py set_target_umg_asset '{"asset_path":"/Game/General/UI/WBP_GameSelect"}'
# ...inspect/edit, compile, visually verify, save...
Tools/editor.sh release
```

In PowerShell use `$env:HAPPINESS_AGENT_ID = 'codex-art'` and `python Tools/editor.py start`.
Environment settings must be supplied again if your shell tool starts a fresh process each call.
`--agent` sets identity only for that invocation; UMG/MCP still need the environment variable.

If occupied, `start` fails without starting/stopping an editor and records a handoff request.
You can also request explicitly: `Tools/editor.sh request --task "Need editor for logo placement"`.
Work on source/art independently while waiting. Check status at least every 60 seconds while holding
ownership and between asset edits. A heartbeat is refreshed by each tool call; use `heartbeat` while
doing independent work. Handoff is cooperative, not an automatic interruption.

The owner finishes its current operation, ends PIE, saves every changed asset, then:

```bash
Tools/editor.sh stop                       # retains ownership; closes only this checkout
Tools/editor.sh release
```

The waiting agent retries `start` to open its own checkout. If nobody in another checkout is waiting,
`release` leaves the editor open; a later request still requires the previous agent to reacquire,
save/stop, and release before the requester can start. Requests remain visible after release.
An agent can never acquire while a different checkout's editor is open.

UMG routing requires **both** exact `project_file` and live process ID in instance metadata; stale
discovery files and another checkout's newer instance are ignored. Client IDs include agent and checkout.
Epic MCP requests verify that the local HTTP listening port belongs to that same editor PID.
Tool calls are serialized and ownership cannot be released during a call. These wrappers are a cooperative
guard, not an OS security boundary: don't bypass them with raw sockets or manually start a second editor.

Heartbeats never automatically expire ownership. For a crashed agent/tool, first confirm that it is gone
and arrange a safe editor shutdown; then `python Tools/editor_gate.py recover --confirm-abandoned`.
Recovery refuses while an editor or recorded tool process is running. Never reclaim merely because a
heartbeat is old. Asset ownership still needs coordination: separate checkouts cannot merge `.uasset` edits.

All these scripts are versioned and work on the checkout they live in. `puzzle.sh` and `mcp.sh` get the
project and `Saved` paths from `Tools/checkout_env.sh`, and use the checkout's `Engine` symlink (every checkout
needs one, pointing at the installed engine). `puzzle.sh --build` refuses while this checkout's editor is open;
each agent builds only its own checkout.

## The two tool servers

The editor hosts two servers, each with a small command-line client in `Tools/`:

| Server | Client | Use it for |
|---|---|---|
| **Epic MCP** (HTTP, `http://127.0.0.1:8000/mcp`) | `python Tools/mcp_call.py <toolset> <tool> '<json>'` | Blueprint graphs, compiling, reparenting, saving, assets, importing, rendering |
| **UmgMcp** (third-party plugin, local socket) | `python Tools/umg.py <command> '<json>'` | Widget layouts: the designer hierarchy and widget properties (see [UMG.md](UMG.md)) |

Both clients take `--max N` to limit how much of the reply is printed.

### Finding Epic MCP tools and their arguments

Never call MCP `describe_toolset`: its schemas are huge (BlueprintTools alone is ~72K characters). Use the
lookup script instead:

```
Tools/mcp.sh -toolsets                                   # toolset names
Tools/mcp.sh -find=import                                # search tools by keyword
Tools/mcp.sh -list=editor_toolset.toolsets.asset.AssetTools
Tools/mcp.sh -describe=editor_toolset.toolsets.asset.AssetTools.save_assets
```

Toolset names are long, e.g. `editor_toolset.toolsets.blueprint.BlueprintTools`. Object arguments are
`{"refPath": "/Game/Path/Asset.Asset"}`. In the project's own tools (`HappinessMCPLookup.*`), optional string
arguments still have to be passed (use `""`).

### Common calls

```bash
B=editor_toolset.toolsets.blueprint.BlueprintTools
python Tools/mcp_call.py $B compile_blueprint '{"blueprint": {"refPath": "/Game/Happiness/UI/WBP_Happiness.WBP_Happiness"}}'
python Tools/mcp_call.py $B get_parent       '{"blueprint": {"refPath": "/Game/Happiness/UI/WBP_X.WBP_X"}}'
python Tools/mcp_call.py $B set_parent       '{"blueprint": {"refPath": "/Game/Happiness/UI/WBP_X.WBP_X"}, "parent_class": {"refPath": "/Script/Happiness.MyWidgetClass"}}'
python Tools/mcp_call.py editor_toolset.toolsets.asset.AssetTools save_assets '{"asset_paths": ["/Game/Happiness/UI/WBP_X"]}'
python Tools/mcp_call.py editor_toolset.toolsets.asset.AssetTools duplicate   '{"path": "/Game/Happiness/UI/WBP_A", "new_path": "/Game/Happiness/UI/WBP_B"}'
```

- **Compile results:** `compile_blueprint` returns nothing useful. Check the log instead: note the line count of
  `Saved/Logs/Happiness.log` before compiling and grep the new lines for `error|warning`.
- **Saving:** save every asset you changed with `AssetTools.save_assets`. A duplicated asset exists only in
  memory until saved. Saving fails while PIE is running.

## Rendering a widget to PNG

```bash
python Tools/mcp_call.py HappinessMCPLookup.HappinessUIToolset RenderWidget \
  '{"widgetBlueprintPath": "/Game/Happiness/UI/WBP_EndScreen", "width": 1920, "height": 1080, "outputFile": ""}'
# -> E:/HappinessUE/Saved/WidgetRenders/WBP_EndScreen.png
```

- The game is landscape, so render at **1920 × 1080** (and check wider mobile aspect ratios when relevant).
- It renders the **designer** state: placeholder texts, default visibility, no game running. Anything set at
  runtime (scores, which buttons are locked, mode-dependent tick marks) won't show.

`Tools/render_ui.sh /Game/Path/WBP_Name [w] [h]` does the same through the gate in the current checkout: it starts
the editor if needed, defaults to landscape, and leaves the editor open.

## Work without the editor UI

Source artwork (PNG generation, illustration, typography, palette studies) needs no Unreal process or gate.
Unreal assets must still be loaded/edited/saved through Unreal APIs, never patched as binary files.

Unreal supports headless Python asset processing through `UnrealEditor-Cmd.exe <project.uproject>
-run=pythonscript -script=<script.py>` when the Python Editor Script plugin is enabled. The project's
`.uproject` currently does not explicitly enable that plugin; availability needs checking before using it.
See [Epic's Python scripting documentation](https://dev.epicgames.com/documentation/unreal-engine/scripting-the-unreal-editor-using-python).

Texture import/settings and Blueprint asset inspection/compilation can be implemented as commandlet jobs.
Arbitrary Blueprint graph and UMG widget-tree authoring needs editor APIs/custom project tooling; our current
`umg.py` and Blueprint MCP wrappers talk to live editor servers and do not have a headless backend.
Our `RenderWidget` implementation currently depends on `GEditor`, an editor world, Slate and a GPU render
target. A rendering commandlet would need explicit world/Slate/RHI setup and visual verification; `-nullrhi`
cannot render PNG previews. None of those new authoring/rendering commandlets has been implemented or tested.

For asset-writing or rendering commandlets, use the shared gate as well and keep the same checkout's editor
closed during the job. Read-only puzzle logic jobs can run separately when they don't write assets.

## Importing images

```bash
python Tools/mcp_call.py editor_toolset.toolsets.texture.TextureTools import_file \
  '{"folder_path": "/Game/Happiness/UI/Textures", "asset_name": "T_TitleBackground", "source_file": "E:/path/to/image.png"}'
```

Then save the new asset. Use a `T_` prefix for textures.

## Assets to handle with care

- **Never put copies of existing Blueprint classes into `Content/`** for comparison (for example an old
  version extracted from git). Deleting such copies through `AssetTools.delete` crashed the editor (World
  Partition class registry assertion). Compare old versions some other way.
- `AssetTools.delete` permanently removes an asset; only delete what you created.

## Logs and crashes

- Editor log: `Saved/Logs/Happiness.log` (runtime Blueprint errors show as `Blueprint Runtime Error` /
  `Accessed None`).
- Crash reports: `Saved/Crashes/`.

## Puzzle test harness (no editor needed)

`Tools/puzzle.sh` runs the puzzle commandlet headless and prints only its result:

```
Tools/puzzle.sh -seeds=1-200 -size=6 -diff=2              # batch: failures + SUMMARY line
Tools/puzzle.sh -seed=42 -size=5 -mode=show|trace          # one puzzle in detail
Tools/puzzle.sh -seeds=1-40 -size=6 -diff=2 -explain -mode=trace   # hint explanations per clue type
Tools/puzzle.sh -lesson=all -stage=0 -seeds=0-9            # lesson puzzles for every clue type
Tools/puzzle.sh -lesson=all -campaignmode -stage=6 -seeds=0-4      # campaign puzzles
Tools/puzzle.sh -seeds=1-50 -size=6 -only=NextTo,Span | -exclude=Chain   # free play clue selection
```

Each puzzle is solved twice (full clue analysis, and hint by hint with every hint checked against the
solution). A healthy run shows `hintFail=0 withErrors=0`; with `-explain`, the `EXPLAIN` line should report 0
unexplained hints. Difficulty: 0 easy, 1 normal, 2 hard. Sizes 3–8.

## Testing in the game

- PIE console (`~`) command `Happiness.CompleteAllLessons` marks every lesson complete, which unlocks Campaign
  mode. Development builds only.
