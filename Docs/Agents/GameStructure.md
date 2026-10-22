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
| (`PuzzleFailed` in `WBP_Happiness`) | `UI/PuzzleFailedWidget` | Completed with mistakes: Undo / Restart over the board |
| `Happiness/UI/WBP_PauseMenu`, `WBP_ConfirmDialog`, `WBP_CellDialog`, `WBP_HelpPanel` | — | Overlays in the puzzle screen |

A correctly solved puzzle celebrates before its end screen. `WBP_Happiness.RefreshPuzzle` asks the board
(`UGamePanelWidget::PlaySolvedFireworks`) first: if it returns true, every icon bursts into confetti
(`NS_IconFirework`) in a random order, spread evenly over `FireworksDuration` (1 s). `EndScreenDelay` (0.5 s) after
the first burst the board fires
`OnFireworksFinished`, bound in `WBP_Happiness`'s Construct chain to call `RefreshPuzzle` again, which then shows
the end screen; the remaining bursts go off behind it. The burst icons stay hidden until the puzzle is no longer complete
(restart or a new puzzle).

A board completed **with mistakes** gets no end screen and records no result. `RefreshPuzzle` shows `PuzzleFailed`
(`UI/PuzzleFailedWidget`, Z 20), `ShowFinalErrors` marks the wrong icons (if that option is on) and `UnhideClues` brings back every hidden clue. It is a small
translucent panel over the bottom clue strip; the rest of it is clear but blocks the board. Undo (`OnUndo`) runs
`Undo` then `RefreshPuzzle`, so with nothing to undo it simply shows again; Restart (`OnRestart`) resets like the pause menu's Reset Puzzle. A **reset** (`UPuzzle::ResetBoard`) keeps the clock and the hints used; only an end screen's Restart (`RestartPuzzle`, `UPuzzle::Reset`) starts over at 0:00 with no hints.
The widget builds its own layout in C++ (`RebuildWidget`); its look is the Puzzle Failed properties.

The player controller `General/Blueprints/PC_Happiness` starts, loads and saves puzzles and holds the player's
level and EXP (`HappinessLevel`, `HappinessExp`).

## Puzzle code (`Source/Happiness/HappinessClassic/`)

- `Puzzle.*`: generation, solving, rating (`ComputeRating`/`GetRating`), hint selection (`GenerateHint`).
- `Clue.*`: every clue type, its analysis and its help text.
- `Hint.*`: one hint (`UHint`): the action and its plain-English explanation (`GetExplanation`). Rule from the
  user: a hint is **one action** that follows directly from the clue and the current board, explained as
  "reason, then result".
- `CampaignTree.*`, `CampaignProgress.*`, `FreePlaySettings.*`, `HappinessSaveGame.*`: modes and progress.

## Streak savers

Bought days that cover missed daily puzzles (`UDailySubsystem`, `Ads/StreakSaverStore.*`, `UI/StreakSaverPanel.*`).
Days missed since the streak's last day use saver days up first: a broken streak comes back (the missed days don't
count toward it) and what's left protects later misses. Saver days are only used when there are enough for every
missed day; otherwise the streak stays broken, the mode screen says it can be saved, and more days still bring it
back until the next daily solve starts a new streak (the saver days left then protect that one). Saved in
`FDailySaveData`: `SaverDays`, `CoveredThrough` (the last day savers covered), `SaverTransactions` (purchases already
granted, so a purchase the store reports again isn't counted twice).

Products are consumables `streak_saver_1`, `streak_saver_7`, `streak_saver_30` (1, 7, 30 days; ids are config on
`UStreakSaverStoreSubsystem`, prices come from the stores). A purchase is granted, then consumed (Google Play) or
finished (StoreKit); unfinished ones are picked up on the next store check. Editor and PC development builds have no
store: buying grants the days at once. The mode screen makes the Streak Saver button (just above the Daily tile, never over a tile) and the
panel itself (`UHappinessModeWidget::MakeStreakSaverButton`); a `StreakSaverButton` in the layout replaces the made one.
Test with `Happiness.DailyDate` (a later date = missed days) and `Happiness.StreakSaver.Set <days>`.

## Daily reminder

`UDailyReminderSubsystem` (`HappinessClassic/DailyReminder`) schedules a phone notification for each of the next
`DaysAhead` (7) days at `ReminderHour:ReminderMinute` local time (1:00; config `[/Script/Happiness.DailyReminderSubsystem]`
in `DefaultGame.ini`), using the engine's local notifications (`UBlueprintPlatformLibrary`). It reschedules on startup,
on coming back to the foreground and on `UDailySubsystem::OnChanged`, so a solved day gets no reminder and reminders
stop a week after the app was last opened. The first one says "Keep your N-day streak going!" while the streak is
still going. With a streak of 8+ days (`EveningMinStreak`) it also schedules an evening reminder at
20:00 (`EveningHour`) on the next day not yet solved; solving that day reschedules and drops it. Permission is asked once, after the first daily solve (`bAskedPermission` in GameUserSettings): iOS asks
itself (alerts and sound, no push); Android 13+ asks for `POST_NOTIFICATIONS` through
`AndroidThunkJava_Happiness_RequestNotifications` in `Happiness_UPL.xml`, which also declares the permission.
`bRestoreNotificationsOnReboot` brings Android's back after a restart. On Android the small icon is the
`ic_notification` drawable if there is one, else the app icon. Test on a phone with `Happiness.Reminder.Test [seconds]`.
Android and iOS only; it does nothing in the editor.

## Android back button

`UBackButtonSubsystem` (`Source/Happiness/BackButton.*`, Android only) catches the back button before the UI does (a
Slate input preprocessor) and sends the app to the background with `moveTaskToBack` (Java side
`AndroidThunkJava_Happiness_MoveToBack` in `Happiness_UPL.xml`), like Home: the game keeps its state. Mid-puzzle it
first calls WBP_Happiness's `Pause` (saves the time, shows the pause menu), unless the pause menu or an end screen is
already showing. No screen handles back itself.

## Save data

Everything lives in one save slot, `HappinessSave`, written by `SG_Happiness` (parent class
`UHappinessSaveGame`). It holds the puzzle in progress (resumed on relaunch), the lesson and campaign progress,
the Free Play settings and pace, and the player's level.


### Lessons tree presentation

`WBP_CampaignTree` keeps the shared villa visible: its own Background is collapsed. The framed heading,
Back button and compact OverallStatus progress plaque stay fixed. `UCampaignTreeWidget::RebuildTree` creates the
blurred horizontal viewport and all medallions inside TreeCanvas; do not place persistent children there.
The complete viewport border stays fixed. Desktop left-drag and touch scrolling pan the tree, and it centers
on the first unfinished group when opened, clamped to the content bounds without blank end gutters.
A drag suppresses lesson clicks. Group unlocks still require every lesson in all preceding groups.

`ULessonClueDiagram` draws abstract circle/square/triangle examples in C++; these are not puzzle icon assets.
`ULessonMedallion` draws the opaque painted face, wrapped label and completion/lock mark, including disabled
states. The frame and heading textures are T_LessonMedallion and T_LessonsHeading. OverallStatus shows
how many groups are fully completed (for example, "3 / 12 complete"); the legacy OverallProgress is collapsed.
The active group has its own completion plaque beneath its stack.
Its sage halo follows the actual stack and plaque bounds, with the stronger inner glow extending beyond
the opaque medallion. Keep TreeScroll at Z=1 above the blur/tint; within NodeCanvas use connectors/glow
at Z=0, number/count plaques at Z=1 and lesson buttons at Z=2. Equal canvas Z values can share Slate
paint layers. Verify halo contrast in PIE over the villa; the isolated widget render has no villa background. Medallions use 292x214 design units,
320-unit column spacing and rounded ivory/gold group brackets with flower junctions. A ScaleBox fits the complete
888-unit design canvas to the available viewport height. Column spacing stays fixed by expanding the
design-space X positions by the reciprocal scale, while frames, diagrams and labels scale together,
so scroll bounds, centering, and hit targets remain aligned at wide phone aspect ratios.
Painted group badges and progress/count plaques use T_LessonsGroupBadge and T_LessonsStatusPlaque.
The bundled Crimson Text Semibold font (LessonsSerif_Font) carries its OFL license beside the font assets.

Back from a lesson's final (first completion), the tree skips the lesson popup and celebrates instead
(`UCampaignTreeWidget::StartCelebration`, handed the lesson once by `UCampaignSubsystem::ConsumeLessonCompletion`):
the lesson's check pops in (small, past full size, settling) with a glow and a shimmer; then, if that completed
its group, the halo slides to the next group while the tree scrolls to center it, and that group unlocks.
Timings are the Lesson Completion properties on WBP_CampaignTree.

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
Learn More broadcasts `OnLearnMoreRequested(ECampaignLesson)`. `UCampaignTreeWidget::HandleLearnMore` opens
WBP_GameRules (`RulesClass`) over the tree and popup (Z 300 in its root canvas), selected at that clue's subject
with the index scrolled to it (`UGameRulesWidget::ShowLesson`); its Close returns to the popup. A clue type with
no rules subject, and Game Basics (VerticalTwo, which teaches the game itself), open at Basics > Objective.
