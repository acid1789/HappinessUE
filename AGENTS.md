# Happiness — guide for AI agents

Shared by every agent working on this project (Claude Code reads it through `CLAUDE.md`, Codex reads it directly).
Keep it true: when you learn something that would have saved you time, add it here or to the guide it belongs in.

## The project

- **Happiness** is a logic-grid puzzle game for Android and iOS, built with **Unreal Engine 5.8.3** and **UMG**.
  Project root: `E:\HappinessUE` (`Happiness.uproject`).
- **Puzzle logic is C++** in `Source/Happiness/HappinessClassic/` (generation, clues, hints, rating, campaign,
  free play, save data).
- **The UI is UMG Widget Blueprints** in `Content/`, some with C++ parent classes in `Source/Happiness/UI/`.
- Map of the game's systems, screens and save data: [Docs/Agents/GameStructure.md](Docs/Agents/GameStructure.md).

## Hard rules

1. **Never modify the engine.** `E:\HappinessUE\Engine` is a symlink to Epic's installed engine
   (`E:\UE4\UE_5.8\Engine`). Anything under it is Epic's. Project tooling belongs in `Plugins/` or `Tools/`.
2. **Don't commit.** The user reviews and commits their own changes.
3. **`.uasset` files are binary.** Change them only through the editor (the tools below), never as files.
   Two agents can't merge edits to the same asset: only one agent should edit a given asset at a time.
4. **Only one agent drives the editor at a time.** The editor tools share one editor instance and one
   UmgMcp client id. Coordinate through the user before starting, stopping or editing in it.
5. **Leave the editor running** when you finish. Close it only to rebuild C++ (`Tools/editor.sh stop`), then
   start it again (`Tools/editor.sh start`).
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
- Test puzzle logic changes with the headless harness (`Tools/puzzle.sh`) before handing them over.
