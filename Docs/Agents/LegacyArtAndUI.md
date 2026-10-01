# Legacy art and UI

> **This is legacy information, not the art direction.** It records how the game looks today so you know what
> you are replacing. Don't copy these styles into new work or treat them as conventions to follow. The new art
> direction is being set separately with the user; follow that wherever it differs from this page.
>
> Still current: the format, the asset locations, and how to bring new art into a screen.

## Format

- Phone game, **portrait**. Preview renders at **1080 × 1920**.
- Screens are UMG widgets (see [GameStructure.md](GameStructure.md) for the list). Check any visual change by
  rendering the widget ([EditorTooling.md](EditorTooling.md#rendering-a-widget-to-png)); remember the render
  shows designer placeholders, not live values.

## Where art lives

| What | Where |
|---|---|
| Fonts (Comic Sans family: regular, bold, italic, bold italic) | `Content/Happiness/UI/Fonts/` |
| UI textures | `Content/Happiness/UI/Textures/` |
| General textures (`Logo`, `Clouds4`) | `Content/General/Textures/` |
| Puzzle icon sets (`IS_Balls`, `IS_Cars`, `IS_Flowers`, `IS_Fruits`, `IS_Hats`, `IS_Instruments`, `IS_Toys`, ...) | `Content/Happiness/IconSets/`; each is a `DA_IconSet` data asset with an `Icons` array of textures |
| Sounds (menu clicks, end screen steps, fills, unlocks) | `Content/General/Audio/Cues/` |

Bring a new image in with `TextureTools.import_file` (see [EditorTooling.md](EditorTooling.md#importing-images)),
then point a widget's `Image` brush (or a button's `WidgetStyle` brushes) at it with UMG `set_widget_properties`.

## The legacy look (as of 2026-10-01, being replaced)

- **Two visual styles are in use:**
  - End screens (`WBP_EndScreen`, `WBP_LessonEndScreen`, `WBP_CampaignEndScreen`): a 600-wide framed panel
    (three `BG` images), Comic fonts, pale yellow text with dark drop shadows, a green SUCCESS / red INCORRECT
    headline, green progress bars, metallic-looking buttons.
  - Menus (`WBP_HappinessMode`, `WBP_LessonPopup`): plain light-grey rounded buttons with dark Roboto text on
    black or white backgrounds.
- **End screen layout pattern:** title line, result, two columns "Your Time" / "Par Time" (label above value),
  right-aligned score lines with a yellow underline before the total, then bars with a small label above and a
  "x / y" count below, and three buttons along the bottom.
- **State colors used in code:** solved `(0, 0.5, 0)`, incorrect `(1, 0, 0)`, hint removal tint red on the icon.
- Locked buttons (e.g. Campaign before lessons are done) are disabled; give them a visibly dimmed style.
