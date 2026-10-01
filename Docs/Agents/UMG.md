# Editing widget layouts (UMG)

Widget layouts (the designer hierarchy and each widget's properties) are edited through the **UmgMcp** plugin
(`Plugins/UmgMcp`, third-party, reviewed) with `python Tools/umg.py <command> '<json>'`. Epic's MCP tools only
cover Blueprint graphs, not the designer.

Take your checkout's editor first with `Tools/editor.sh start`, and keep `HAPPINESS_AGENT_ID` set for
all calls. Instance selection verifies this checkout's full project path and live PID, so another checkout's
editor running at the same time is never targeted; client IDs are agent- and checkout-specific. See
[EditorTooling.md](EditorTooling.md#editor-gate-one-editor-per-checkout).

Lock a widget before changing it (`python Tools/editor_gate.py lock /Game/... --task "..."`). Commands that
change the current `set_target_umg_asset` target are refused without its lock; reading the tree and querying
properties need none ([EditorTooling.md](EditorTooling.md#file-locks)).

## Basics

```bash
python Tools/umg.py set_target_umg_asset '{"asset_path": "/Game/Happiness/UI/WBP_EndScreen"}'
python Tools/umg.py get_widget_tree '{}'
python Tools/umg.py query_widget_properties '{"widget_name": "TotalLabel", "properties": ["Text", "Font", "Slot.LayoutData"]}'
python Tools/umg.py set_widget_properties  '{"widget_name": "TotalLabel", "properties": {"Text": "Total EXP:"}}'
python Tools/umg.py create_widget '{"widget_type": "TextBlock", "new_widget_name": "LevelLabel", "parent_name": "Content"}'
python Tools/umg.py delete_widget '{"widget_name": "OldLabel", "confirm_delete": true}'
python Tools/umg.py reorder_widget_tree '{"root": "Content", "tree": "[\"TitleText\", \"ClassicButton\", \"LessonsButton\"]"}'
```

- **Always call `set_target_umg_asset` first.** Without a target the plugin creates a junk asset
  `/Game/DefaultAndCanBeDelete`; if that appears, delete the file with the editor closed.
- **Each `umg.py` run connects first.** For a series of commands, write a small Python script that imports
  `Tools/umg.py`, sends `connect` once (`{"display_name": "...", "exclusive": false}`), then sends the commands.
  Calling `umg.send` without connecting fails with `not_connected`.
- **Save afterwards** with Epic's `AssetTools.save_assets` (see [EditorTooling.md](EditorTooling.md)). UmgMcp's
  own save can fail when the editor was started `-unattended`.
- **Compile the widget** (`BlueprintTools.compile_blueprint`) after structural changes, and before reparenting.

## Property formats

- **Slot properties use dotted keys:** `"Slot.LayoutData"`, `"Slot.ZOrder"`, `"Slot.Padding"`,
  `"Slot.HorizontalAlignment"`, `"Slot.bAutoSize"`, `"Slot.Size"`. A nested `"Slot": {...}` object is silently
  ignored.
- **Canvas slot layout:**
  ```json
  "Slot.LayoutData": {"offsets": {"left": -150, "top": 110, "right": 100, "bottom": 30},
                      "anchors": {"minimum": {"x": 0.5, "y": 0}, "maximum": {"x": 0.5, "y": 0}},
                      "alignment": {"x": 0.5, "y": 0}}
  ```
  With point anchors, `left`/`top` are the position and `right`/`bottom` are the **width and height**.
- **Copying a look:** query a widget's properties (`Font`, `ColorAndOpacity`, `ShadowOffset`,
  `ShadowColorAndOpacity`, `Justification`, `WidgetStyle`, `FillColorAndOpacity`, ...) and set the same values
  on another widget. The values round-trip as returned.
- **Button styles:** `WidgetStyle` has `normal`/`hovered`/`pressed`/`disabled` brushes. The menu buttons'
  `disabled` brush is `NoDrawType` (no background at all), so a disabled button shows only grey text; give it a
  dimmed brush if it should still look like a button.
- **Text:** `"Text": "..."` sets the literal text. A `\n` inside the text makes a line break.

## Creating widgets

- `widget_type` takes engine widget names (`TextBlock`, `Button`, `Image`, `ProgressBar`, `SizeBox`,
  `HorizontalBox`, ...). Project widgets need their class path:
  - Widget Blueprints: `/Game/Happiness/UI/WBP_CampaignEndScreen.WBP_CampaignEndScreen_C`
  - C++ widgets: `/Script/Happiness.LessonProgressTicks`
- New widgets can be referenced from the Blueprint graph after the widget Blueprint is compiled (they show up as
  `Variables|<WidgetBlueprint>|Get<Name>` node types).
- **There is no rename.** To rename, create a new widget, copy the old one's properties onto it, then delete
  the old one.
- A C++ parent class binds child widgets **by name** (`meta = (BindWidget)`); a required binding missing from
  the layout is a compile error. Create the widgets first, then reparent.

## Gotchas

- **Draw order in a CanvasPanel:** children with the same `Slot.ZOrder` share a starting layer, so deeply nested
  content (text) of an earlier child can draw over a later overlay. Overlays and popups need a higher
  `Slot.ZOrder`. Current values in `WBP_Happiness`: `HintInfo` 10, end screens 20. In `WBP_GameSelect`:
  `HappinessMode` 5, `CampaignTree` 10.
- **A Collapsed widget has no layout.** Anything that measures geometry (like the hint panel placing itself)
  must keep its own root laid out and collapse only an inner box.
- **Freshly added widgets have no geometry until the next frame.** Re-place things that depend on another
  widget's position on the following tick.
- **Widget Blueprint class defaults:** changing a property on a Widget Blueprint's defaults shows up in
  instances only after that Widget Blueprint is compiled. Instances placed inside other widgets keep their own
  saved values (delta serialization); set important defaults in the C++ constructor instead.
- **Fonts with drop shadows:** wrapping text in a `ScaleBox` dropped the text shadows in this project; set the
  font size directly instead.
- Hidden clue widgets are removed from their panel (`RemoveFromParent`), not collapsed.
