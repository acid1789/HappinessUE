# Editor tooling

Everything here runs from Git Bash in `E:\HappinessUE` and needs the editor open unless noted.

## Starting, stopping, building

| Task | Command |
|---|---|
| Start the editor and wait until its tool servers answer | `Tools/editor.sh start` |
| Close the editor (forced after 60 s) | `Tools/editor.sh stop` |
| Build the C++ (editor must be **closed**, it locks the DLL: LNK1104) | `Tools/puzzle.sh --build -seed=1` |

The build log is `Saved/PuzzleBuild.log`; look for `Result: Succeeded` and `error C`/`error LNK` lines.
After building, start the editor again and recompile any Widget Blueprints whose C++ parent changed.

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
  '{"widgetBlueprintPath": "/Game/Happiness/UI/WBP_EndScreen", "width": 1080, "height": 1920, "outputFile": ""}'
# -> E:/HappinessUE/Saved/WidgetRenders/WBP_EndScreen.png
```

- The game is portrait, so render at **1080 × 1920**.
- It renders the **designer** state: placeholder texts, default visibility, no game running. Anything set at
  runtime (scores, which buttons are locked, mode-dependent tick marks) won't show.
- `Tools/render_ui.sh /Game/Path/WBP_Name [w] [h]` does the same and starts the editor if needed.

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
