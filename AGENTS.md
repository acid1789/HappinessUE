# Happiness — guide for AI agents

Shared by every agent working on this project (Claude Code reads it through `CLAUDE.md`, Codex reads it directly).
Keep it true: when you learn something that would have saved you time, add it here or to the guide it belongs in.

## The project

- **Happiness** is a logic-grid puzzle game for Android and iOS, built with **Unreal Engine 5.8.3** and **UMG**.
  Each agent has its own checkout (`Happiness.uproject` in that checkout's root).
  Claude: `E:\HappinessUE`; Codex art: `E:\Happiness_Art`. Run tooling from your own checkout.
- **Puzzle logic is C++** in `Source/Happiness/HappinessClassic/` (generation, clues, hints, rating, campaign,
  free play, save data).
- **The UI is UMG Widget Blueprints** in `Content/`, some with C++ parent classes in `Source/Happiness/UI/`.
- Map of the game's systems, screens and save data: [Docs/Agents/GameStructure.md](Docs/Agents/GameStructure.md).
- To find where something happens in the Blueprints, search the Blueprint map (`Saved/BlueprintMap`, made by
  `python Tools/bp_map.py`) rather than reading graphs through the editor. See
  [Blueprints.md](Docs/Agents/Blueprints.md#finding-things-the-blueprint-map).

## Hard rules

1. **Never modify the engine.** `E:\HappinessUE\Engine` is a symlink to Epic's installed engine
   (`E:\UE4\UE_5.8\Engine`). Anything under it is Epic's. Project tooling belongs in `Plugins/` or `Tools/`.
2. **Don't commit.** The user reviews and commits their own changes.
3. **Lock a file before changing it.** Each checkout has its own copy of every file, and git can't merge two
   checkouts' edits to the same `.uasset`. Before changing an asset (or any file another agent might edit),
   run `python Tools/editor_gate.py lock <path> --task "..."`; if another agent holds it, pick other work
   or ask the user. The editor tools refuse to change an asset you haven't locked. Keep the lock until the
   user has committed and pushed your change (`unlock` refuses before that), and pull before locking a file
   that changed upstream. `.uasset` files are binary: change them only through the editor, never as files.
   See [EditorTooling.md](Docs/Agents/EditorTooling.md#file-locks).
4. **Each checkout runs its own editor; one owner per editor.** Agents in different checkouts work at the
   same time, each in its own editor with its own MCP port. Set `HAPPINESS_AGENT_ID` to your unique session
   name (`codex-art` or `claude-gameplay` if only one session of each is active) and take your checkout's
   editor with `Tools/editor.sh start`. Within a checkout only the owner may use the editor: anyone else
   (including the user, e.g. `ron-playtest` for a playtest) queues with a request. UMG/MCP clients enforce
   ownership and exact project/PID/port routing. See
   [EditorTooling.md](Docs/Agents/EditorTooling.md#editor-gate-one-editor-per-checkout).
5. **Leave the editor running** when you finish unless rebuilding C++. Release ownership with
   `Tools/editor.sh release`; the editor stays open for the next owner. Check `Tools/editor.sh status` at
   least every 60 seconds while holding your editor and between asset edits; if someone is waiting on your
   checkout, finish your current operation, end PIE, save and release. Never stop another checkout's editor
   or force-close unsaved work. After rebuilding, restart your editor.
6. **Don't save while Play-In-Editor is running.** Saving fails during PIE ("Asset does not exist").
   Look for an open PIE session in `Saved/Logs/Happiness.log` (`Creating play world` without a later
   `Shutting down PIE`) before saving.
7. **Verify visually.** After a UI change, render the widget to a PNG and look at it
   ([EditorTooling.md](Docs/Agents/EditorTooling.md#rendering-a-widget-to-png)). The user can't see images an
   agent displays in chat, so describe what the render shows and give its path.

## Guides

| Guide | What it covers |
|---|---|
| [Docs/Agents/EditorTooling.md](Docs/Agents/EditorTooling.md) | Starting/stopping the editor, building C++, the MCP tools, saving, rendering, importing images, the puzzle test harness |
| [Docs/Agents/UMG.md](Docs/Agents/UMG.md) | Editing widget layouts with the UmgMcp tool: commands, property formats, gotchas |
| [Docs/Agents/Blueprints.md](Docs/Agents/Blueprints.md) | Reading and editing Blueprint graphs with Epic's BlueprintTools: workflow, node types, gotchas |
| [Docs/Agents/GameStructure.md](Docs/Agents/GameStructure.md) | Game modes, screens, which C++ class backs which widget, save data |
| [Docs/Agents/LegacyArtAndUI.md](Docs/Agents/LegacyArtAndUI.md) | **Legacy:** the game's current (old) look, where art assets live, how to bring new art into a screen. Not the art direction going forward. |

## Working conventions

- Match the surrounding code: naming, comment density, idioms. Several C++ files use **CRLF** line endings
  (`Hint.cpp`, `Hint.h`, `Puzzle.cpp`, `Clue.cpp`, `CampaignTree.cpp`, ...); keep each file's endings.
- When writing C++ through a shell heredoc, an escaped `\n` inside a string literal can turn into a real line
  break. Check string literals after scripted edits.
- `Clue.cpp` and `Puzzle.cpp` start with `#pragma optimize("", off)` and end with `#pragma optimize("", on)`.
  Keep both: without the closing one it leaks into the next file of the unity build (warning C4426).
- C++ widget defaults (sounds, colors) are more reliable set in the C++ constructor than on a Widget
  Blueprint's class defaults; instances placed inside other widgets keep their own saved values.
- **Don't load Blueprint classes in a C++ constructor** (`ConstructorHelpers::FClassFinder`) when that Blueprint
  can reach, through its references, a widget of the class being constructed: the circular load hangs the editor
  (and the puzzle commandlet) at startup, right after loading the Happiness module. Use a `TSoftClassPtr` and
  load it when first needed (see `UPauseMenuWidget::OptionsClass`). Sounds and textures are safe to find there.
- Test puzzle logic changes with the headless harness (`Tools/puzzle.sh`) before handing them over.
- **After a pull that changes `Source/`, rebuild C++ before touching assets** (`Tools/puzzle.sh --build`, editor
  closed). Pulled assets may use C++ classes your editor's DLL doesn't have yet: their Blueprints then fail to load
  and odd errors follow elsewhere ("Could not find a variable named ...", crashes while reading graphs). Check with
  `git log -1 --format=%cd` against the date of `Binaries/Win64/UnrealEditor-Happiness.dll`.
