# Game structure

## Modes

From the game select screen, **Happiness** opens the mode screen (`WBP_HappinessMode`), unless a puzzle is in
progress, in which case it goes straight back into that puzzle.

| Mode | What it is | Key code |
|---|---|---|
| **Free Play** | Any size 3–8 and difficulty, with a choice of clue types. Scored with EXP against a par time; EXP fills the player's level. | `UFreePlaySubsystem` (`HappinessClassic/FreePlaySettings.*`) |
| **Lessons** | A tree of clue-type lessons (`WBP_CampaignTree`). Each lesson: 3x3 Easy, 3x3 Normal, 4x4 Easy, 4x4 Normal, then a 4x4 Hard final. Points per puzzle (3, minus hints) unlock stages. | `UCampaignSubsystem` (`HappinessClassic/CampaignProgress.*`), tree in `CampaignTree.cpp` |
| **Campaign** | Unlocked by completing every lesson. The same tree on bigger boards: per clue 4×5x5 Normal, 5x5 Hard, 4×6x6 Normal, 6x6 Hard, 4×7x7 Normal, 7x7 Hard, then an 8x8 Easy final. One step per solved puzzle; scored with EXP like Free Play. | Same subsystem, `ECampaignMode::Campaign` |

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
