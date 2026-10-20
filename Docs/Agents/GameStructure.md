# Game structure

## Modes

From the game select screen, **Happiness** opens the mode screen (`WBP_HappinessMode`), unless a puzzle is in
progress, in which case it goes straight back into that puzzle.

| Mode | What it is | Key code |
|---|---|---|
| **Free Play** | Any size 3–8 and difficulty, with a choice of clue types. Scored with EXP against a par time; EXP fills the player's level. | `UFreePlaySubsystem` (`HappinessClassic/FreePlaySettings.*`) |
| **Lessons** | A tree of clue-type lessons (`WBP_CampaignTree`). Each lesson: 3x3 Easy, 3x3 Normal, 4x4 Easy, 4x4 Normal, then a 4x4 Hard final. Points per puzzle (3, minus hints) unlock stages. | `UCampaignSubsystem` (`HappinessClassic/CampaignProgress.*`), tree in `CampaignTree.cpp` |
| **Campaign** | Unlocked by completing every lesson. The same tree on bigger boards: per clue 4×5x5 Normal, 5x5 Hard, 4×6x6 Normal, 6x6 Hard, 4×7x7 Normal, 7x7 Hard, then an 8x8 Easy final. One step per solved puzzle; scored with EXP like Free Play. | Same subsystem, `ECampaignMode::Campaign` |
| **Master Mode** | Per clue, once its Campaign final is cleared: 4×8x8 Normal, then 4×8x8 Hard. Stages 7 and 8 of the clue's track (`GetFirstMasterStage()`), with their own count (`FLessonProgress::MasterPoints`) that doesn't affect Campaign progression. | Same subsystem (`IsMasterStage`, `GetCurrentMasterStage`, ...) |

`UCampaignSubsystem::GetMode()` says which track the tree, popup and end screens are showing; the stage
functions (`GetNumStages`, `GetStageSize`, `GetMaxPoints`, ...) follow it.

## Screens

| Widget (`Content/...`) | C++ parent (`Source/Happiness/...`) | Role |
|---|---|---|
| `General/UI/WBP_GameSelect` | — | Main menu; hosts the mode screen, campaign tree and Free Play config |
| `Happiness/UI/WBP_HappinessMode` | `UI/HappinessModeWidget` | Free Play / Lessons / Campaign / Back |
| `Happiness/UI/WBP_HappinessConfig` | — | Free Play setup: size, difficulty, clue selection (`WBP_ClueSelect`, `UI/ClueSelectWidget`) |
| `Happiness/UI/WBP_CampaignTree` | `UI/CampaignTreeWidget` | The lesson tree (Lessons and Campaign) |
| `Happiness/UI/WBP_LessonPopup` | `UI/LessonPopupWidget` | A lesson's description, progress bar with stage ticks, stage buttons |
| `Happiness/UI/WBP_Happiness` | — | The puzzle screen: board, clue panels, buttons, overlays |
| `Happiness/UI/WBP_GamePanel` | `UI/GamePanelWidget` | Centers the board and sizes columns to the height of its two candidate-icon rows; the Blueprint still builds and updates the grid |
| `Happiness/UI/WBP_Cell`, `WBP_Icon` | — | Board cells and candidate icons |
| `Happiness/UI/WBP_VerticalCluePanel` | — | The **bottom** clue strip (vertical clues) |
| `Happiness/UI/WBP_HorizontalCluePanel` | — | The **right** clue column (horizontal clues) |
| `Happiness/UI/WBP_ButtonPanel` | `UI/ButtonPanelWidget` | Side buttons; the hint button wiggles after 5 s idle |
| `Happiness/UI/WBP_HintInfo` | `UI/HintInfoWidget` | Hint explanation panel over the bottom clue strip |
| `Happiness/UI/WBP_EndScreen` | — | Free Play end screen: times, EXP, level bar |
| `Happiness/UI/WBP_LessonEndScreen` | `UI/LessonEndScreenWidget` | Lessons end screen: points breakdown, lesson progress |
| `Happiness/UI/WBP_CampaignEndScreen` | `UI/CampaignEndScreenWidget` | Campaign end screen: Free Play scoring plus campaign progress |
| `Happiness/UI/WBP_PauseMenu`, `WBP_ConfirmDialog`, `WBP_CellDialog`, `WBP_HelpPanel` | — | Overlays in the puzzle screen |

The player controller `General/Blueprints/PC_Happiness` starts, loads and saves puzzles and holds the player's
level and EXP (`HappinessLevel`, `HappinessExp`).

## Puzzle code (`Source/Happiness/HappinessClassic/`)

- `Puzzle.*`: generation, solving, rating (`ComputeRating`/`GetRating`), hint selection (`GenerateHint`).
- `Clue.*`: every clue type, its analysis and its help text.
- `Hint.*`: one hint (`UHint`): the action and its plain-English explanation (`GetExplanation`). Rule from the
  user: a hint is **one action** that follows directly from the clue and the current board, explained as
  "reason, then result".
- `CampaignTree.*`, `CampaignProgress.*`, `FreePlaySettings.*`, `HappinessSaveGame.*`: modes and progress.

## Save data

Everything lives in one save slot, `HappinessSave`, written by `SG_Happiness` (parent class
`UHappinessSaveGame`). It holds the puzzle in progress (resumed on relaunch), the lesson and campaign progress,
the Free Play settings and pace, and the player's level.


### Lessons tree presentation

`WBP_CampaignTree` keeps the shared villa visible: its own Background is collapsed. The framed heading,
Back button and compact OverallStatus range plaque stay fixed. `UCampaignTreeWidget::RebuildTree` creates the
blurred horizontal viewport and all medallions inside TreeCanvas; do not place persistent children there.
The complete viewport border stays fixed. Desktop left-drag and touch scrolling pan the tree, and it centers
on the first unfinished group when opened, clamped to the content bounds without blank end gutters.
A drag suppresses lesson clicks. Group unlocks still require every lesson in all preceding groups.

`ULessonClueDiagram` draws abstract circle/square/triangle examples in C++; these are not puzzle icon assets.
`ULessonMedallion` draws the opaque painted face, wrapped label and completion/lock mark, including disabled
states. The frame and heading textures are T_LessonMedallion and T_LessonsHeading. OverallStatus shows
the visible group range (for example, "4 - 8 of 12"); the legacy OverallProgress is collapsed.
The active group has its own completion plaque beneath its stack.
Its sage halo follows the actual stack and plaque bounds, with the stronger inner glow extending beyond
the opaque medallion. Keep TreeScroll at Z=1 above the blur/tint; within NodeCanvas use connectors/glow
at Z=0, number/count plaques at Z=1 and lesson buttons at Z=2. Equal canvas Z values can share Slate
paint layers. Verify halo contrast in PIE over the villa; the isolated widget render has no villa background. Medallions use 292x214 design units,
320-unit column spacing and rounded ivory/gold group brackets with flower junctions. A ScaleBox fits the complete
888-unit design canvas to the available viewport height. Column spacing stays fixed by expanding the
design-space X positions by the reciprocal scale, while frames, diagrams and labels scale together,
so scroll bounds, centering, range text and hit targets remain aligned at wide phone aspect ratios.
Painted group badges and range/count plaques use T_LessonsGroupBadge and T_LessonsStatusPlaque.
The bundled Crimson Text Semibold font (LessonsSerif_Font) carries its OFL license beside the font assets.

Locked lessons use a bottom-center cream/gold-rimmed sage badge with an ivory lock. Row spacing
reserves room for this badge beneath each medallion; keep it clear of the next row and count plaque.

The painted Back button uses T_LessonsBack. Lock badges use T_LessonsLock; the collapsed
LessonLockArt image in WBP_CampaignTree preserves the texture reference for cooking.

`WBP_LessonPopup` now uses the option-B notebook presentation, built by
`ULessonPopupWidget::BuildNotebookLayout` around its existing bound widgets. T_LessonNotebook and
T_LessonNotebookButtons supply the blank surfaces; all labels, progress, stage states and actions stay live.
The right page scrolls long descriptions. The design canvas grows when the Campaign Master row appears,
preserving the existing reveal animation and stage/replay routing. The shared ULessonClueDiagram captures
its lesson when its Slate widget is constructed: recreate it when changing the notebook to another clue.
Learn More broadcasts `OnLearnMoreRequested(ECampaignLesson)`; the future clue-specific rules navigation
is intentionally left for its consumer to connect. It does not open a separate rules screen yet.
