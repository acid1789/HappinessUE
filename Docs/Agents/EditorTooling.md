# Editor tooling

Run from Git Bash in **your own checkout** (or a subdirectory); most tools need the editor open unless noted.
The gated Python tools also work from PowerShell. Every checkout needs an `Engine` symlink to the installed
engine (`editor.py` can also use Unreal's registry or an explicit `UE_ENGINE_DIR`).

## Starting, stopping, building

| Task | Command |
|---|---|
| Start the editor and wait until its tool servers answer | `Tools/editor.sh start` |
| Close only this checkout's editor (no forced shutdown) | `Tools/editor.sh stop` |
| Build the C++ (editor must be **closed**, it locks the DLL: LNK1104) | `Tools/puzzle.sh --build -seed=1` |

The build log is `Saved/PuzzleBuild.log`; look for `Result: Succeeded` and `error C`/`error LNK` lines.
After building, start the editor again and recompile any Widget Blueprints whose C++ parent changed.

## Editor gate: one editor per checkout

Each checkout runs **its own editor**, so agents in different checkouts (Claude in `E:\HappinessUE`, Codex in
`E:\Happiness_Art`) work at the same time. Within a checkout, one owner at a time uses that editor: an agent,
or the user taking it for a playtest (e.g. `ron-playtest`).

State lives in `%LOCALAPPDATA%/Happiness/AgentGate/gate.sqlite3`, one record per checkout: owner (agent, task,
heartbeat, editor PID), waiting requests, the tool call in progress, and the checkout's **Epic MCP port**. SQLite
transactions make acquisition atomic. Gate files are local runtime data, not Git artifacts.
`HAPPINESS_EDITOR_GATE_DIR` can override the directory; every agent must use the same absolute directory.

Choose one identity per active agent session, and keep it for subsequent shell calls:

```bash
export HAPPINESS_AGENT_ID=codex-art          # Claude uses claude-gameplay
Tools/editor.sh start --task "Inspect menu"  # takes this checkout's editor, launching it if needed
Tools/editor.sh status                       # every checkout: owner, requests, port
python Tools/umg.py set_target_umg_asset '{"asset_path":"/Game/General/UI/WBP_GameSelect"}'
# ...inspect/edit, compile, visually verify, save...
Tools/editor.sh release                      # the editor stays open for the next owner
```

In PowerShell use `$env:HAPPINESS_AGENT_ID = 'codex-art'` and `python Tools/editor.py start`.
Environment settings must be supplied again if your shell tool starts a fresh process each call.
`--agent` sets identity only for that invocation; UMG/MCP still need the environment variable.
An identity owns at most one checkout's editor at a time.

**Ports.** Each checkout is given its own MCP port the first time it's used (`E:\HappinessUE` 8000,
`E:\Happiness_Art` 8001). `start` launches the editor with `-ModelContextProtocolPort=<port>
-ModelContextProtocolStartServer` and also writes the port into the checkout's local
`Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini`, so an editor opened by hand on that checkout
serves on the same port. `mcp_call.py` targets its checkout's port automatically (`UE_MCP_URL` overrides it).
The UmgMcp plugin already picks a free port per editor and publishes it with the project path and PID.

**Waiting for an editor.** If another owner holds your checkout's editor, `start` fails without touching the
editor and records a request (or use `Tools/editor.sh request --task "..."`). Work on source/art meanwhile.
The owner checks `status` at least every 60 seconds and between asset edits; when someone is waiting, it
finishes the current operation, ends PIE, saves, and releases. Handoff is cooperative. A heartbeat is
refreshed by each tool call; use `heartbeat` while doing independent work.

**Rebuilding C++.** `Tools/editor.sh stop` closes only this checkout's editor and keeps ownership; build, then
`start` again. Other checkouts' editors keep running.

**Playtesting.** Take the checkout's editor before playing in it (`HAPPINESS_AGENT_ID=ron-playtest`,
`start --task "Playtest"`, `release` afterwards) so no agent edits under you. A **Standalone Game** launched
from an editor is a second `UnrealEditor.exe` for the same checkout, which blocks tool calls there until it
closes. A packaged build or a device doesn't affect the gate at all. Check for PIE with
`python Tools/mcp_call.py EditorToolset.EditorAppToolset IsPIERunning '{}'`.

**Routing checks.** UMG routing requires **both** exact `project_file` and live process ID in instance
metadata; stale discovery files and other checkouts' instances are ignored. Client IDs include agent and
checkout. Epic MCP requests verify that the checkout's port is owned by that checkout's editor PID. Tool calls
are serialized per checkout and ownership can't be released during a call. These wrappers are a cooperative
guard, not an OS security boundary: don't bypass them with raw sockets or start a second editor on the same
checkout.

**Recovery.** Heartbeats never automatically expire ownership. For a crashed agent/tool, first confirm that it
is gone and arrange a safe shutdown of that checkout's editor; then
`python Tools/editor_gate.py recover --confirm-abandoned` (from that checkout). Recovery refuses while that
checkout's editor or a recorded tool process is running. Never reclaim merely because a heartbeat is old.

## File locks

Each checkout has its own copy of every file, and git can't merge two checkouts' edits to the same `.uasset`.
So before changing an asset, lock it. Text files (source, docs, config) merge like any code: don't lock them. Locks live in the same gate database and are shared by every checkout;
each is held by one agent identity.

```bash
python Tools/editor_gate.py lock /Game/Happiness/UI/WBP_Happiness --task "Hint button art"
python Tools/editor_gate.py locks                     # every lock: path, agent, task, checkout
python Tools/editor_gate.py unlock /Game/Happiness/UI/WBP_Happiness
```

- **Names:** an asset as `/Game/Folder/Asset` (object paths like `.../WBP_X.WBP_X` and graph paths like
  `...:DoHint` count as the same asset) or as its file `Content/Folder/Asset.uasset`. Any other file by its path
  in the checkout, e.g. `Source/Happiness/UI/HintInfoWidget.cpp`. Git Bash rewriting `/Game/...` arguments into
  `C:/Program Files/Git/Game/...` is handled.
- **Enforced for assets.** `mcp_call.py` refuses any Epic MCP call that names an asset you haven't locked,
  unless the tool only reads (names starting `get`/`find`/`read`/`list`/`describe`/`search`/`query`/`is`, plus
  `RenderWidget` and the PIE tools). An import (`folder_path` + `asset_name`) needs the lock on the new asset.
  `umg.py` refuses commands that change a widget unless you hold the lock on the current
  `set_target_umg_asset` target; reading the tree and querying properties are free.
- **Keep the lock until your change is shared.** `unlock` refuses while the file has uncommitted or unpushed
  changes in your checkout, so the next agent never edits a stale copy. The user commits; unlock after the
  commit is pushed. `unlock --force` drops a lock regardless (for an abandoned change you've reverted).
- **Start from the latest version.** `lock` fetches and refuses if GitHub has a newer version of the file than
  your checkout: pull first.
- Locks don't expire. When you're blocked by a lock, do other work or ask the user; never edit around it.

All these scripts are versioned and work on the checkout they live in. `puzzle.sh` and `mcp.sh` get the
project and `Saved` paths from `Tools/checkout_env.sh`, and use the checkout's `Engine` symlink (every checkout
needs one, pointing at the installed engine). `puzzle.sh --build` refuses while this checkout's editor is open;
each agent builds only its own checkout.

- **One build at a time on this machine.** UnrealBuildTool keeps a single log for every checkout
  (`%LOCALAPPDATA%/UnrealBuildTool/Log.txt`), so a build started while another checkout is building fails at
  once. `puzzle.sh --build` waits (up to 30 minutes) for any other build to finish first.
- **Builds use the engine's real path.** `checkout_env.sh` resolves the `Engine` symlink (to
  `E:/UE4/UE_5.8/Engine`). Building through the symlink path one time and the real path another includes engine
  headers by two paths, which fails with redefinition errors (`'FGenericPlatformTypes': 'struct' type
  redefinition`, `PLATFORM_32BITS should not be defined`). If a checkout shows those, delete its `Intermediate`
  folders (project and `Plugins/*`) and build again.

## The two tool servers

The editor hosts two servers, each with a small command-line client in `Tools/`:

| Server | Client | Use it for |
|---|---|---|
| **Epic MCP** (HTTP, `http://127.0.0.1:<checkout port>/mcp`) | `python Tools/mcp_call.py <toolset> <tool> '<json>'` | Blueprint graphs, compiling, reparenting, saving, assets, importing, rendering |
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
- For a populated board layout check, pass `previewPuzzleSize: 3` (3–8) to `RenderWidget` with `WBP_GameSelect`, `WBP_Happiness`, or `WBP_GamePanel`. It creates sample cells and icons only in the render instance, using the actual widget classes and native panel sizing. This checks layout, not puzzle generation or interaction; no asset visibility needs to be changed.
- For hint placement, render WBP_Happiness with previewPuzzleSize: 4 and bPreviewHint: true. This hides popup overlays and shows the hint only in the render instance.
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
- `Happiness.Lessons.*` set lesson progress in the current mode (the full list and arguments are at the top of
  `Source/Happiness/HappinessClassic/CampaignDebugCommands.cpp`): `List`, `Reset`, `Clear <Lesson>`,
  `Points <Lesson> <N>`, `Stage <Lesson> <Stage>`, `ToFinal [Lesson]`, `Complete [Lesson]`, `ToColumn <Column>`.
  A lesson is its enum name (`NextTo`) or number; left out, the first unfinished one. `Complete` with the lessons
  screen open plays the completion animation, as if back from its final. They save the progress: back up
  `Saved/SaveGames/HappinessSave.sav` first if it matters. Development builds only.
- To see a fast UI animation, slow it on the live PIE widget (`ObjectTools.set_properties` on its transient path
  from `obj list class=<Class>`) and take screenshots with the PIE console command `shot showui`: full
  resolution, in `Saved/Screenshots/WindowsEditor/`, far faster than `CaptureEditorImage` (~3.5 s per call).
- `Happiness.SolvePuzzle` fills the open puzzle with its solution and finishes it as if the player had: the
  end screen, scoring, progress and save all run normally. Development builds only.

### Driving PIE from a tool

Start PIE, run console commands in it, click, capture what's on screen, and stop it, all without the editor UI:

```bash
A=EditorToolset.EditorAppToolset
I=HappinessMCPInput.HappinessMCPInputToolset
python Tools/mcp_call.py $A StartPIE '{"options": {"bSimulate": false, "playMode": "PlayMode_InViewPort"}}'
python Tools/mcp_call.py $A IsPIERunning '{}'          # poll until true, then give the game a few seconds to load
python Tools/mcp_call.py $I ExecPIECommand '{"command": "Happiness.SolvePuzzle"}'
python Tools/mcp_call.py $I ClickViewport '{"x": 0.18, "y": 0.08, "button": "Left"}'   # normalized viewport coords
python Tools/mcp_call.py $A CaptureEditorImage '{}'    # the whole editor window as a base64 PNG ("data")
python Tools/mcp_call.py $A StopPIE '{}'
```

- **`ExecPIECommand`** runs a console command in the PIE world, as if typed in the game's console (`~`): the game's
  dev commands, engine commands (`slomo 0.5`, `stat fps`), console variables (`Happiness.PreviewAdLayout 1`) and
  log verbosity (`log LogTemp Verbose`). It returns false when PIE isn't running.
- **The game's dev commands** (development builds only): `Happiness.SolvePuzzle`, `Happiness.CompleteAllLessons`, `Happiness.Lessons.*`,
  `Happiness.DailyDate`, `Happiness.ResetDaily`, `Happiness.StreakSaver.Set`, `Happiness.Reminder.Test`, `Happiness.PreviewAdLayout`, `Happiness.Ads`,
  `Happiness.LargeBanner`, `Happiness.RemoveAdsToken`, `Happiness.TestAd`. Find them all with
  `grep -rhn 'TEXT("Happiness\.' Source/Happiness`.
- **`CaptureEditorImage` is for screens, not motion.** Each capture takes long enough that an animation or effect
  lasting under a second or so is over before it's taken: a 0.5 s Niagara pulse never showed in any capture. Check
  short effects in the logs, or ask the user to look.
- **`ClickViewport`** uses coordinates normalized to the PIE viewport (0 to 1). To aim from a capture, subtract the
  viewport's top-left corner in the capture and divide by the viewport's size there.
- Moves made this way change the player's save. Undo them before stopping PIE.
- Stop PIE before saving assets. Dev test code must not keep PIE objects alive past PIE (a global
  `TStrongObjectPtr` to a widget in the play world): ending PIE then fails with a fatal "world leaked" error.

### Niagara effects in UMG

Niagara draws inside widgets through `Plugins/NiagaraUIRenderer`, a **project fork** of Epic's experimental UE 5.8
plugin. It has the same name, so it replaces the engine copy. The fork adds a material user parameter binding to
the UI sprite renderer (each component can draw with its own material instance), exports `UNiagaraUIWidget` with
`SetNiagaraSystem` / `GetNiagaraComponent`, fixes sprite sizes (scaled to the screen, and full size rather than
double), and takes `SpriteRotation` in degrees like the other renderers (the engine version used radians). Each change is marked "Happiness fork" in the source; reapply them if the plugin is updated from the
engine.

The engine's `NiagaraToolsets` plugin (enabled, editor-only) adds MCP tools to create and edit Niagara systems
(`NiagaraToolsets.NiagaraToolset_System.*`). Its guidance lives in
`Engine/Plugins/Experimental/Toolsets/NiagaraToolsets/Content/Python/skills/`. Example: `NS_IconPulse`, the
icon pulse each grid cell plays when its icon is set (`UGameCellWidget`).

- A UI sprite needs a **UI-domain** material, or nothing is drawn and nothing is reported.
- Templates made from `DirectionalBurst` and similar systems play **once for 1 s**: check the system's Loop
  Behavior and Loop Duration.
- `UNiagaraComponent::ResetSystem` doesn't restart a component that has finished; use `Activate(true)`.
- **`AddSetParameterEntry` crashed the editor** (assertion `!IsRooted()` in `UNiagaraNodeAssignment::GenerateScript`)
  when adding to an existing SetParameters module. Add a new SetParameters module with `AddSetParametersModule` instead.
- The particle VM has no integer `%`: use `x - n * floor(x / n)` on floats in HLSL expressions.
- Examples: `NS_IconShatter` breaks an eliminated candidate icon into a 6×6 grid of falling, spinning pieces (SubUV
  tiles of the icon). `NS_IconFirework`, a copy of it on a 10�10 grid, bursts a solved puzzle's icons outward (velocity away from
  the icon's center, a Drag module ending the burst) and lets them fall slowly as confetti (gravity over drag sets
  the fall speed; each piece's width swings with a cosine so it seems to flip).
- **Unity builds put several `.cpp` files in one translation unit,** so names in their anonymous namespaces can
  clash (C2374 "redefinition"): two end screens both defined `Ink`. Give file-level constants distinctive names.
