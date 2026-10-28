# Game structure

## Modes

The game always starts on the mode screen (`WBP_HappinessMode`). Each mode (Daily, Free Play, Lessons, Campaign)
keeps its own puzzle in progress: choosing a mode with one asks whether to resume or abandon it (a `WBP_ConfirmDialog`
the mode screen makes, `UHappinessModeWidget::AskResumeOrAbandon`), otherwise it goes into that mode's menus (see
[Save data](#save-data)). Abandoning drops the puzzle from its slot (`UPuzzleSlotSubsystem::AbandonPuzzle`), then
goes on as if there were none: Daily starts over, Free Play opens its settings, Lessons and Campaign open the tree.

| Mode | What it is | Key code |
|---|---|---|
| **Free Play** | Any size 3–8 and difficulty, with a choice of clue types. Scored with EXP against a par time; EXP fills the player's level. | `UFreePlaySubsystem` (`HappinessClassic/FreePlaySettings.*`) |
| **Lessons** | A tree of clue-type lessons (`WBP_CampaignTree`). Each lesson: 3x3 Easy, 3x3 Normal, 4x4 Easy, 4x4 Normal, then a 4x4 Hard final. Points per puzzle (3, minus hints) unlock stages. | `UCampaignSubsystem` (`HappinessClassic/CampaignProgress.*`), tree in `CampaignTree.cpp` |
| **Campaign** | Unlocked by completing every lesson. The same tree on bigger boards: per clue 4×5x5 Normal, 5x5 Hard, 4×6x6 Normal, 6x6 Hard, 4×7x7 Normal, 7x7 Hard, then an 8x8 Easy final. One step per solved puzzle; scored with EXP like Free Play. | Same subsystem, `ECampaignMode::Campaign` |
| **Master Mode** | Per clue, once its Campaign final is cleared: 4×8x8 Normal, then 4×8x8 Hard. Stages 7 and 8 of the clue's track (`GetFirstMasterStage()`), with their own count (`FLessonProgress::MasterPoints`) that doesn't affect Campaign progression. Each Master puzzle is the best of `UCampaignSubsystem::MasterCandidates` (3) generated from its seed: `UPuzzle::InitCampaign` keeps the one whose solve takes the most deductions (`ComputeRating().Steps`), as plain generation can give Master puzzles far easier than their size suggests. No hints or Fix Puzzle (`UCampaignSubsystem::AreAssistsAllowed`): the button panel (`UI/ButtonPanelWidget`) disables the hint and fix buttons and stops the hint button's wiggle, and the pause menu fades and disables Fix Puzzle. | Same subsystem (`IsMasterStage`, `GetCurrentMasterStage`, ...) |

`UCampaignSubsystem::GetMode()` says which track the tree, popup and end screens are showing; the stage
functions (`GetNumStages`, `GetStageSize`, `GetMaxPoints`, ...) follow it.

## Screens

| Widget (`Content/...`) | C++ parent (`Source/Happiness/...`) | Role |
|---|---|---|
| `General/UI/WBP_GameSelect` | — | Main menu; hosts the mode screen, campaign tree and Free Play config. A new player (no `HappinessSave` at startup, `UCampaignSubsystem::ConsumeNewPlayerStart`) starts on the Lessons tree with Game Basics' popup open (`UHappinessModeWidget::StartNewPlayer`, on the mode screen's first tick); everyone else starts on the mode screen |
| `Happiness/UI/WBP_HappinessMode` | `UI/HappinessModeWidget` | Free Play / Lessons / Campaign / Back ; top right, the player level badge (`UI/PlayerLevelBadge`, made in code): the level on `T_PlayerLevelSun`, ringed by twelve shaped rewards from `T_MasterRewardGems` (4×3 atlas). Each reward appears once every clue in that tree column has completed Campaign Master Mode (`UCampaignSubsystem::IsMasterColumnCompleted`). `SunTexture` and `IconTextures` remain art override hooks. The mode screen moves the badge into WBP_GameSelect's MainCanvas at Z 0 (`MoveBadgeToMenu`, after construction), so it stays up behind every menu screen (mode Z 5, tree Z 10, Free Play settings added later); it goes transparent while the puzzle screen (`WBP_Happiness`, in the same canvas) is visible (`UPlayerLevelBadge::SetCoveredBy`) and refreshes when it comes back. Tapping the sun opens the player info (`UI/PlayerInfoPanel`, made in code): the mode screen puts a clear `SunButton` over the sun, in the menu just above the badge (so the menu's screens still cover it; hidden with the badge, `SetTapTarget`). The panel goes over the whole menu (Z 50) with the badge raised above its dim (Z 55, `SetBadgeRaised`), so the sun stays up; its window keeps clear of the sun on both sides. It shows Mastered Clues (the badge's twelve rewards in a row, at `LockedRewardOpacity` (25%) until earned) and Options (`WBP_Options`), Game Rules (`WBP_GameRules`) and Icon Sets (`UI/IconSetsPanel`, see Icon sets). The three open over the whole menu (Z 300, above the raised sun) and close back to the panel; its close button (top right, `UPauseMenuGlyph` symbol 9), tapping outside the window or the sun closes it. |
| `Happiness/UI/WBP_HappinessConfig` | — | Free Play setup: size, difficulty, clue selection (`WBP_ClueSelect`, `UI/ClueSelectWidget`) |
| `Happiness/UI/WBP_CampaignTree` | `UI/CampaignTreeWidget` | The lesson tree (Lessons and Campaign). Bottom left in Lessons mode until every lesson is completed: `WBP_CompleteLessons` (`UI/CompleteLessonsPanel`), which asks with the screen's own `ConfirmDialog` (`UConfirmDialogWidget::Ask`), then completes every lesson, unlocking Campaign. When every lesson has just been completed (the last final, or that button), the tree closes to the mode screen, which opens the congratulations (`UI/LessonsCompletePopup`, made in code: the Garden Visitors and Campaign unlocks) with fireworks over them (`UI/FireworksWidget`, code-drawn). `UCampaignSubsystem::ConsumeLessonsCompletion` says when; like the lesson completion, it isn't saved |
| `Happiness/UI/WBP_LessonPopup` | `UI/LessonPopupWidget` | A lesson's description, progress bar with stage ticks, stage buttons. When a clue's Master Mode has just been completed, the popup (Mastered) holds its reward back from the sun's badge (`UPlayerLevelBadge::GetPendingMasterReward`) and plays fireworks over itself, round after round (`UFireworksWidget::AddBursts`), until Return to Campaign collapses it into the reward's place on the sun (`RevealMasterReward`). Preview: `Happiness.PreviewMasterReward` |
| `Happiness/UI/WBP_Happiness` | — | The puzzle screen: board, clue panels, buttons, overlays |
| `Happiness/UI/WBP_GamePanel` | `UI/GamePanelWidget` | Centers the board and sizes columns to the height of its two candidate-icon rows; the Blueprint still builds and updates the grid |
| `Happiness/UI/WBP_Cell`, `WBP_Icon` | — | Board cells and candidate icons |
| `Happiness/UI/WBP_VerticalCluePanel` | — | The **bottom** clue strip (vertical clues) |
| `Happiness/UI/WBP_HorizontalCluePanel` | — | The **right** clue column (horizontal clues) |
| `Happiness/UI/WBP_ButtonPanel` | `UI/ButtonPanelWidget` | Side buttons; the hint button wiggles after 5 s idle |
| `Happiness/UI/WBP_HintInfo` | `UI/HintInfoWidget` | Hint explanation panel over the bottom clue strip |
| `Happiness/UI/WBP_Icon` | `UI/HintSparkleIconWidget` | One icon of a cell or clue. While its `ShowingHint` is set, sparkles orbit it (a clue's orbit its whole clue, run by its first icon `_0`). When a hint starts, its sparkles stream in to lead the player's eye: out of the hint button (`UButtonPanelWidget::Button_H`) to the clue, then, `CellStreamDelay` later, off the clue's ring to the board icon (`ShownClue`). Each sparkle flies into its place in the orbit (`StreamSeconds`, `StreamSpawns`, `TravelSeconds`). A clue's sparkles run from a core ticker, since the clue panels' scroll boxes stop ticking clues scrolled out of view: the hint scrolls its clue into view, the ring follows the clue as the panel scrolls (its place comes from arranging the scroll box), and sparkles outside the panel are hidden. A board icon's sparkles stop once the icon stops ticking (`TickBoardWatch`): placing a hinted icon ends the hint and collapses the cell's candidates in the same frame, before the icon could see it. |
| `Happiness/UI/WBP_EndScreen` | — | Free Play end screen: times, EXP, level bar |
| `Happiness/UI/WBP_LessonEndScreen` | `UI/LessonEndScreenWidget` | Lessons end screen: points breakdown, lesson progress |
| `Happiness/UI/WBP_CampaignEndScreen` | `UI/CampaignEndScreenWidget` | Campaign end screen: Free Play scoring plus campaign progress |
| (`PuzzleFailed` in `WBP_Happiness`) | `UI/PuzzleFailedWidget` | Completed with mistakes: Undo / Restart over the board |
| `Happiness/UI/WBP_PauseMenu`, `WBP_ConfirmDialog`, `WBP_CellDialog`, `WBP_HelpPanel` | — | Overlays in the puzzle screen |
| `Happiness/UI/WBP_Options` | — | Options: volumes, checkboxes, `WBP_RemoveAds` (`UI/RemoveAdsPanel`). Opened from the pause menu and from the player info; Apply refreshes the puzzle screen only when there is one and it is visible, and sets the puzzle's AutoSetIcons only when there is a puzzle |

A correctly solved puzzle celebrates before its end screen. `WBP_Happiness.RefreshPuzzle` asks the board
(`UGamePanelWidget::PlaySolvedFireworks`) first: if it returns true, every icon bursts into confetti
(`NS_IconFirework`) in a random order, spread evenly over `FireworksDuration` (1 s). `EndScreenDelay` (0.5 s) after
the first burst the board fires
`OnFireworksFinished`, bound in `WBP_Happiness`'s Construct chain to call `RefreshPuzzle` again, which then shows
the end screen; the remaining bursts go off behind it. The burst icons stay hidden until the puzzle is no longer complete
(restart or a new puzzle).

A board completed **with mistakes** gets no end screen and records no result. `RefreshPuzzle` shows `PuzzleFailed`
(`UI/PuzzleFailedWidget`, Z 20), `ShowFinalErrors` marks the wrong icons (if that option is on) and `UnhideClues` brings back every hidden clue. It is a small
ivory banner across the top edge of the grid; the rest of it is clear but blocks the board. Undo (`OnUndo`) runs
`Undo` then `RefreshPuzzle`, so with nothing to undo it simply shows again; Restart (`OnRestart`) resets like the pause menu's Reset Puzzle. A **reset** (`UPuzzle::ResetBoard`) keeps the clock and the hints used; only an end screen's Restart (`RestartPuzzle`, `UPuzzle::Reset`) starts over at 0:00 with no hints.
The widget builds its own layout in C++ (`RebuildWidget`) with the existing hint-reading frame and notebook button art.

The player controller `General/Blueprints/PC_Happiness` starts, loads and saves puzzles and holds the player's
level and EXP (`HappinessLevel`, `HappinessExp`). Its `InputMode` (mouse, or touch on Android and iOS) must be set
before any puzzle screen exists, so BeginPlay sets it first: `WBP_GamePanel` wires each cell for the mode when it
builds the grid (touch: `Button_Cell` opens the cell dialog; mouse: the icons take clicks). Set after that, a grid
takes no taps.

## Puzzle code (`Source/Happiness/HappinessClassic/`)

- `Puzzle.*`: generation, solving, rating (`ComputeRating`/`GetRating`), hint selection (`GenerateHint`).
- `Clue.*`: every clue type, its analysis and its help text.
- `Hint.*`: one hint (`UHint`): the action and its plain-English explanation (`GetExplanation`). Rule from the
  user: a hint is **one action** that follows directly from the clue and the current board, explained as
  "reason, then result".
- `CampaignTree.*`, `CampaignProgress.*`, `FreePlaySettings.*`, `HappinessSaveGame.*`: modes and progress.

## Icon sets

Each puzzle row uses one icon set (`DA_IconSet` data assets in `Content/Happiness/IconSets/`: `Id` and `Icons`, each a
texture and an Id; set and icon IDs are what a saved puzzle stores). `PC_Happiness.SoftIconSetCollection` names the
default collection (`KatrinaCollection`, 8 sets). At BeginPlay `PC_Happiness` swaps in the player's own collection from
`UIconSetSubsystem::GetPlayerCollection` (`HappinessClassic/IconSets.*`): the default sets, then each unlocked set from
`DA_IconSetUnlocks` (`UIconSetUnlocks`: set + `EIconSetUnlock` condition, `bHiddenUntilUnlocked`, optional
`LockedReason`). It updates in place when campaign progress changes. `IS_GardenVisitors` unlocks once every lesson is
completed. To add an unlockable set, add it to `DA_IconSetUnlocks` (new conditions go in `EIconSetUnlock` /
`UIconSetSubsystem::IsUnlocked` / the default reason in `GetEntries`). `DA_IconSetUnlocks` is always cooked (Asset
Manager rule in `DefaultGame.ini`), so the sets it lists ship. Its `SetNames` (by set Id) are the names the icon sets
screen shows; a set without one shows its Id spaced out.

The player can turn unlocked sets off on the icon sets screen (`UI/IconSetsPanel`, from the player info), down to
`UIconSetSubsystem::MinEnabledSets` (8, the biggest puzzle's rows). The sets turned off (their Ids) are saved in
`FIconSetSaveData` (`UHappinessSaveGame::IconSetChoices`; not `IconSets`, which is SG_Happiness's own variable). The player collection keeps every unlocked set, on or off, so a
saved puzzle always finds its sets (`FindIconSet`). `SelectRandomIconSets` copies only the sets
`UIconSetSubsystem::IsIconSetEnabled` allows, then takes the first PuzzleSize of them, shuffled first when Scramble
Icons is on. The screen lists the default sets, then each unlockable set that is unlocked or not hidden; a locked one
shows its reason ("Locked: Complete Lessons").

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

The game save is the slot `HappinessSave`, written by `SG_Happiness` (parent class `UHappinessSaveGame`). It holds
the lesson and campaign progress, the Free Play settings and pace, and the player's level.

The game on Play before lessons and the campaign (version code 8) saved only the classic puzzle and the level. Its save starts the game over, as for a new player (`UHappinessSaveGame::ResetOldSave`, before anything reads the save): it is replaced by a fresh one keeping `Level` and `Experience`, and the puzzle slots are deleted (`Settings` is kept). An old save is told apart by the file's property tags: it has no `DataVersion` (written into every save from now on, `CurrentDataVersion`) and no `Campaign` (a property at its default isn't written, so the bytes are checked).

Puzzles in progress have a slot per mode, `HappinessPuzzle_Daily`, `_FreePlay`, `_Lessons` and `_Campaign`
(`UPuzzleSlotSubsystem`, `HappinessClassic/PuzzleSlots.*`): each an `SG_Happiness` holding just that mode's puzzle
(every `SG_Happiness` variable but `Level` and `Experience`) and `FPuzzleSlotInfo` (the lesson, stage and seed of a
lesson puzzle, the date of a daily one). `PC_Happiness`'s `SaveHappiness` still writes the whole game save, on every
move; as it is written its puzzle is copied into the slot of the mode the puzzle was started in
(`UPuzzleSlotSubsystem::NotePuzzleStarted`, called by `UCampaignSubsystem::InitPuzzleForPlay`). Resuming calls `UHappinessModeWidget::ResumePuzzle` (Free Play always goes through `WBP_GameSelect`'s `ShowHappinessConfig`,
which resumes or opens the settings): between `BeginResume` and `EndResume`, loading the game save
(`PC_Happiness`'s `LoadHappiness`) copies that slot's puzzle into it. A slot resumes while its puzzle can still be
played: today's unsolved daily; a lesson puzzle whose stage is still on that seed
(`UCampaignSubsystem::ResumeLessonPuzzle` restores its session). Finishing a puzzle (`HappinessFinished`) saves no
puzzle into its slot. The pause menu's Main Menu saves the puzzle, like Save and Quit, rather than dropping it.
`UHappinessSaveGame::HasActivePuzzle` is now always false, so `WBP_GameSelect` never resumes at startup. A save from
before slots has its puzzle moved into the slot of the mode it was being played in (`MigrateOldSave`). Each lesson's progress includes the time spent on its
puzzles (`FLessonProgress::PlaySeconds`, counted by `UCampaignSubsystem::TickPlayTime` while one is on screen,
unpaused and unsolved), shown on the lesson popup left of its Back / Return to Lessons button.


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
The right page scrolls long descriptions. Once a clue's Master Mode is shown (after its reveal animation), the
Master stage row replaces the Campaign stage row, like the bar and text above it; the popup keeps its size.
Campaign's seven stage buttons are narrower than Lessons' five: their labels go on two lines and shrink to fit. The shared ULessonClueDiagram captures
its lesson when its Slate widget is constructed: recreate it when changing the notebook to another clue.
Learn More broadcasts `OnLearnMoreRequested(ECampaignLesson)`. `UCampaignTreeWidget::HandleLearnMore` opens
WBP_GameRules (`RulesClass`) over the tree and popup (Z 300 in its root canvas), selected at that clue's subject
with the index scrolled to it (`UGameRulesWidget::ShowLesson`); its Close returns to the popup. A clue type with
no rules subject, and Game Basics (VerticalTwo, which teaches the game itself), open at Basics > Objective.
