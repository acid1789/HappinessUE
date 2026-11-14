# Game structure

## Modes

The game always starts on the mode screen (`WBP_HappinessMode`). Each mode (Daily, Free Play, Lessons, Campaign)
keeps its own puzzle in progress: choosing a mode with one asks whether to resume or abandon it (a `WBP_ConfirmDialog`
the mode screen makes, `UHappinessModeWidget::AskResumeOrAbandon`), otherwise it goes into that mode's menus (see
[Save data](#save-data)). Abandoning drops the puzzle from its slot (`UPuzzleSlotSubsystem::AbandonPuzzle`), then
goes on as if there were none: Daily starts over, Free Play opens its settings, Lessons and Campaign open the tree.

| Mode | What it is | Key code |
|---|---|---|
| **Daily** | One puzzle per date, its size, difficulty, seed and clue types chosen from the date. Only clue types whose lessons the player has unlocked (`GetLockedClueMask`); the rest are left out without changing the size, difficulty or seed, and the mask is kept for that date once its puzzle starts (`FDailySaveData::ActiveLockedClues`). Test with `Tools/puzzle.sh -daily=YYYYMMDD -dailycolumns=N`. | `UDailySubsystem` (`HappinessClassic/DailyPuzzle.*`) |
| **Free Play** | Any size 3–8 and difficulty, with a choice of clue types. Scored with EXP against a par time; EXP fills the player's level. | `UFreePlaySubsystem` (`HappinessClassic/FreePlaySettings.*`) |
| **Lessons** | A tree of clue-type lessons (`WBP_CampaignTree`). Each lesson: 3x3 Easy, 3x3 Normal, 4x4 Easy, 4x4 Normal, then a 4x4 Hard final. Points per puzzle (3, minus hints) unlock stages. | `UCampaignSubsystem` (`HappinessClassic/CampaignProgress.*`), tree in `CampaignTree.cpp` |
| **Campaign** | Unlocked by completing every lesson. The same tree on bigger boards: per clue 4×5x5 Normal, 5x5 Hard, 4×6x6 Normal, 6x6 Hard, 4×7x7 Normal, 7x7 Hard, then an 8x8 Easy final. One step per solved puzzle; scored with EXP like Free Play. | Same subsystem, `ECampaignMode::Campaign` |
| **Master Mode** | Per clue, once its Campaign final is cleared: 4×8x8 Normal, then 4×8x8 Hard. Stages 7 and 8 of the clue's track (`GetFirstMasterStage()`), with their own count (`FLessonProgress::MasterPoints`) that doesn't affect Campaign progression. Each Master puzzle is the best of `UCampaignSubsystem::MasterCandidates` (3) generated from its seed: `UPuzzle::InitCampaign` keeps the one whose solve takes the most deductions (`ComputeRating().Steps`), as plain generation can give Master puzzles far easier than their size suggests. No hints or Fix Puzzle (`UCampaignSubsystem::AreAssistsAllowed`): the button panel (`UI/ButtonPanelWidget`) disables the hint and fix buttons and stops the hint button's wiggle, and the pause menu fades and disables Fix Puzzle. | Same subsystem (`IsMasterStage`, `GetCurrentMasterStage`, ...) |

`UCampaignSubsystem::GetMode()` says which track the tree, popup and end screens are showing; the stage
functions (`GetNumStages`, `GetStageSize`, `GetMaxPoints`, ...) follow it.

## Screens

| Widget (`Content/...`) | C++ parent (`Source/Happiness/...`) | Role |
|---|---|---|
| `General/UI/WBP_GameSelect` | — | Main menu; hosts the mode screen, campaign tree and Free Play config. A new player (no `HappinessSave` at startup, `UCampaignSubsystem::ConsumeNewPlayerStart`) starts on the Lessons tree with Game Basics' popup open (`UHappinessModeWidget::StartNewPlayer`, on the mode screen's first tick); everyone else starts on the mode screen. Until the tutorial is completed (`FCampaignSaveData::bTutorialCompleted`, set when the tutorial reaches its solved step or the player answers No), the mode screen's first ticks ask "Would you like to learn how to play?" with its `ConfirmDialog` (`UHappinessModeWidget::OfferTutorial`, Yes/No): Yes plays the tutorial (`UCampaignTreeWidget::PlayTutorial`, as the Lessons screen's Tutorial button), No closes it and sets the flag. Asking replaces the new player's jump to the Lessons tree (`ConsumeNewPlayerStart`); the tutorial ends on Game Basics anyway. |
| `Happiness/UI/WBP_HappinessMode` | `UI/HappinessModeWidget` | Mode tiles in a 2×2 grid (`ModeGrid`): Lessons and Campaign on top, Daily and Free Play (`ClassicButton`) below, the Streak Saver button under Daily (outside the grid: `ModeGridSize`'s height is fixed at the two rows, 504, so the Remove Ads card beside it matches them); Back ; top right, the player level badge (`UI/PlayerLevelBadge`, made in code): the level on `T_PlayerLevelSun`, ringed by twelve shaped rewards from `T_MasterRewardGems` (4×3 atlas). Each reward appears once every clue in that tree column has completed Campaign Master Mode (`UCampaignSubsystem::IsMasterColumnCompleted`). `SunTexture` and `IconTextures` remain art override hooks. The mode screen moves the badge into WBP_GameSelect's MainCanvas at Z 0 (`MoveBadgeToMenu`, after construction), so it stays up behind every menu screen (mode Z 5, tree Z 10, Free Play settings added later); it goes transparent while the puzzle screen (`WBP_Happiness`, in the same canvas) is visible (`UPlayerLevelBadge::SetCoveredBy`) and refreshes when it comes back. Tapping the sun opens the player info (`UI/PlayerInfoPanel`, made in code): the mode screen puts a clear `SunButton` over the sun, in the menu just above the badge (so the menu's screens still cover it; hidden with the badge, `SetTapTarget`). The panel goes over the whole menu (Z 50) with the badge raised above its dim (Z 55, `SetBadgeRaised`), so the sun stays up; its window keeps clear of the sun on both sides. It shows Mastered Clues (the badge's twelve rewards in a row, at `LockedRewardOpacity` (25%) until earned) and Options (`WBP_Options`), Game Rules (`WBP_GameRules`) and Icon Sets (`UI/IconSetsPanel`, see Icon sets). The three open over the whole menu (Z 300, above the raised sun) and close back to the panel; its close button (top right, `UPauseMenuGlyph` symbol 9), tapping outside the window or the sun closes it. |
| `Happiness/UI/WBP_HappinessConfig` | — | Free Play setup: size, difficulty, clue selection (`WBP_ClueSelect`, `UI/ClueSelectWidget`) |
| `Happiness/UI/WBP_CampaignTree` | `UI/CampaignTreeWidget` | The lesson tree (Lessons and Campaign). Bottom left in Lessons mode until every lesson is completed: `WBP_CompleteLessons` (`UI/CompleteLessonsPanel`), which asks with the screen's own `ConfirmDialog` (`UConfirmDialogWidget::Ask`), then completes every lesson, unlocking Campaign. When every lesson has just been completed (the last final, or that button), the tree closes to the mode screen, which opens the congratulations (`UI/LessonsCompletePopup`, made in code: the Garden Visitors and Campaign unlocks) with fireworks over them (`UI/FireworksWidget`, code-drawn). `UCampaignSubsystem::ConsumeLessonsCompletion` says when; like the lesson completion, it isn't saved. Completing the campaign (every lesson's Campaign final) does the same with the popup's campaign variant (`bCampaignComplete`: the Mythical Creatures unlock, "Look for Master Challenges!"), its fireworks going a round at a time until Continue (`ConsumeCampaignCompletion`). Test: `Happiness.Campaign.CompleteAll` (repeatable; on the mode screen the congratulations open at once, on the Campaign tree it first celebrates the last lesson) In Lessons mode, `WBP_Tutorial` (`UI/TutorialPanel`, styled like `WBP_CompleteLessons`): `RebuildTree` moves it into the scrolling content (top left, above the first column), so it scrolls and scales with the tree, and a press that pans the tree doesn't press it. Pressed, it broadcasts `OnTutorialRequested` and the tree plays the tutorial puzzle (`UCampaignSubsystem::StartTutorial`, then `OnLessonStageChosen` for Lessons' 3x3 first stage, which `WBP_GameSelect` plays like a lesson's): `InitPuzzleForPlay` sets up `Init(TutorialSeed, 3, Easy)` with only Same Column, Not Same Column (TwoNot) and Span clues (plus starting clues; seed 129: two Same Column, one Not Same Column, one Span, one given icon, solved last by an elimination), and its icons are always Master Reward Gems, Mythical Creatures and Garden Visitors (from Icons/Basic, unlocked or not), put in PC_Happiness's `HappinessIconSets`/`HappinessIcons`, which `SelectRandomIconSets` uses when set. It's kept in no puzzle slot and doesn't advance the Free Play puzzle number; `RequestLessonPuzzle` and `StopPlayingLesson` drop its icons (`ForgetTutorialIcons`) so they don't carry into the next puzzle. No banner ad while it's on (`UAdBannerSlot` treats it as ads off). Its end is still Free Play's (end screen with EXP, then the mode screen). |
| `Happiness/UI/WBP_LessonPopup` | `UI/LessonPopupWidget` | A lesson's description, progress bar with stage ticks, stage buttons. When a clue's Master Mode has just been completed, the popup (Mastered) holds its reward back from the sun's badge (`UPlayerLevelBadge::GetPendingMasterReward`) and plays fireworks over itself, round after round (`UFireworksWidget::AddBursts`), until Return to Campaign collapses it into the reward's place on the sun (`RevealMasterReward`). Preview: `Happiness.PreviewMasterReward`. The first time Master Mode opens, an eraser clears the Campaign controls and the Master ones are drawn in, with sounds for each (`EraseSound` / `DrawSound`: `/Game/Happiness/Audio/SFX_MasterErase`, `SFX_MasterDraw`, synthesized to the animation's timing, fitted to `RevealEraseTime`). Preview: `Happiness.PreviewMasterTransition [column]` |
| `Happiness/UI/WBP_Happiness` | — | The puzzle screen: board, clue panels, buttons, overlays |
| `Happiness/UI/WBP_GamePanel` | `UI/GamePanelWidget` | Centers the board and sizes columns to the height of its two candidate-icon rows; the Blueprint still builds and updates the grid |
| `Happiness/UI/WBP_Cell`, `WBP_Icon` | — | Board cells and candidate icons |
| `Happiness/UI/WBP_VerticalCluePanel` | — | The **bottom** clue strip (vertical clues) |
| `Happiness/UI/WBP_HorizontalCluePanel` | — | The **right** clue column (horizontal clues) |
| `Happiness/UI/WBP_ButtonPanel` | `UI/ButtonPanelWidget` | Side buttons; the hint button wiggles after 5 s idle (not in the tutorial puzzle, `UCampaignSubsystem::IsPlayingTutorial`). With the phone's notch on the left (over the buttons), the panel moves right by the left safe-area inset and `NotchMakeRoom` (GamePanel, HelpPanel) give up that width; with it on the right nothing moves. iOS reports the same inset on both sides in landscape, so there only `FIOSApplication::CachedOrientation == UIInterfaceOrientationLandscapeRight` (notch left) counts; Android reports the cutout's own side |
| `Happiness/UI/WBP_HintInfo` | `UI/HintInfoWidget` | Hint explanation panel over the bottom clue strip: the Hint badge in its top left corner and the hint's explanation after it, wrapping underneath it (the clue itself isn't repeated: the help line at the top describes it) |
| `Happiness/UI/WBP_Icon` | `UI/HintSparkleIconWidget` | One icon of a cell or clue. While its `ShowingHint` is set, sparkles orbit it (a clue's orbit its whole clue, run by its first icon, the first child of the clue's box). When a hint starts, its sparkles stream in to lead the player's eye: out of the hint button (`UButtonPanelWidget::Button_H`) to the clue, then, `CellStreamDelay` later, off the clue's ring to the board icon (`ShownClue`). The cell dialog shows the hint too when it opens on the hinted cell (`WBP_CellDialog.ShowHint`): its icon gets the sparkles (drawn just above the overlay the icon is in, so over the dialog; no stream, the ring is there at once, `PrefillRing`, as the dialog is the board's cell up close) and, for an elimination (`UHint::IsElimination`), the board's red tint. Each sparkle flies into its place in the orbit (`StreamSeconds`, `StreamSpawns`, `TravelSeconds`). A clue's sparkles run from a core ticker, since the clue panels' scroll boxes stop ticking clues scrolled out of view: the hint scrolls its clue into view, the ring follows the clue as the panel scrolls (its place comes from arranging the scroll box), and sparkles outside the panel are hidden. A board icon's sparkles stop once the icon stops ticking (`TickBoardWatch`): placing a hinted icon ends the hint and collapses the cell's candidates in the same frame, before the icon could see it. |
| `Happiness/UI/WBP_EndScreen` | — | Free Play end screen: times, EXP, level bar |
| `Happiness/UI/WBP_LessonEndScreen` | `UI/LessonEndScreenWidget` | Lessons end screen: points breakdown, lesson progress |
| `Happiness/UI/WBP_CampaignEndScreen` | `UI/CampaignEndScreenWidget` | Campaign end screen: Free Play scoring plus campaign progress |
| (`PuzzleFailed` in `WBP_Happiness`) | `UI/PuzzleFailedWidget` | Completed with mistakes: Undo / Restart over the board |
| `Happiness/UI/WBP_PauseMenu`, `WBP_ConfirmDialog`, `WBP_CellDialog`, `WBP_HelpPanel` | — | Overlays in the puzzle screen |
| (made in code) | `UI/FocusOverlay` | The focus effect: the screen dimmed (75% black) except any number of clear circles, fixed (`AddFocusCircle`), following a widget (`AddFocusOnWidget`, a circle through its box's corners), or moving from round one set of widgets to round another (`AddFocusMove`: center and radius over a Duration, both ends following their widgets; `bTight` sizes a circle to half the box's longer side instead of its corners). Each can animate in: from a StartRadius to its radius over a Duration (linear). Drawn as a Slate mesh of pixel rows with soft edges, no render target; takes no input unless `SetBlocksInput(true)`, which makes it swallow every tap and click (circles included), or `SetInputWindow(Widgets)`: an input preprocessor drops taps, clicks and scrolls anywhere but on those widgets (while shown), whatever is drawn there, while the overlay is on screen (drawn in the last 0.25 s). Used by the tutorial puzzle: as it starts, `UGamePanelWidget::UpdateTutorialFocus` puts one over the puzzle screen (Z `TutorialFocusZOrder` 8: over the board, clues, buttons and banner, under the hint panel, dialogs, end screens and pause menu) with a circle round the grid closing in from the screen's diagonal over `TutorialFocusSeconds` (1 s); once the circle is round the grid, a `UI/TutorialPopup` (made in code: cream panel, gold rim, lessons serif, fades in; its message is rich text, `<icon id="Name"/>` showing an icon given to `SetIcon` inline, `UTutorialIconDecorator`; `ShowTutorialPopup` gives it the tutorial icons by name, e.g. Triangle, Dragon) explains the grid, centered in the space right of the grid (`PlaceTutorialPopup`, up to 620 wide, Z 9). Its Next button appears 3 s after it (`NextDelay`). Next (`HandleTutorialNext`, `TutorialStep` 1) closes it; the grid's circle stays and a second popup (placed the same) says "The puzzle has one category per row. Each icon in a category belongs in one of the columns.", while each row gets its own color behind its icons (`UpdateTutorialRowBoxes`: its cells' `Button_Cell` backgrounds tinted, put back when the tutorial ends, and a box of it behind the grid in the grid's canvas for the gaps between cells), row 1's first, then each next one 0.5 s later. Its Next (`TutorialStep` 2) brings a third (placed the same): "Your goal is to figure out which column each icon belongs in.", and the columns get the colors instead of the rows, the same way (column 1's first, `bTutorialColorColumns`); they stay for the rest of the tutorial. Its Next (`TutorialStep` 3) closes it and moves the grid's circle (`UFocusOverlay::AddFocusMove`) over `TutorialFocusSeconds` to the vertical clues (bottom left: the clues the panel shows, `GetShownClues`, its `UClueWidget`s), input still blocked; then a wider popup (up to 1180, centered across the screen over the grid, a little above the middle) explains vertical clues. Its Next (step 4) moves the circle on to the horizontal clues (top right) with their popup, placed the same way; that Next (step 5) moves it to tight round the first vertical clue's two icons (`AddFocusMove`'s `bTight`: half the box's longer side). The rest follows the tutorial puzzle's solve (seed 129; columns and rows counted from 1 here, from 0 in code; icons: row 1 Heart, Triangle, Star; row 2 Dragon, Unicorn, Phoenix; row 3 Butterfly, Bee, Ladybug; solution: column 1 Heart, Phoenix, Butterfly; column 2 Triangle, Dragon, Bee; column 3 Star, Unicorn, Ladybug; the unicorn is given). Each "cells step" (6, 11, 17, 23, 27; `GetTutorialCellsStep`) has its circle's cells (`GetTutorialStepCells`), the cell its icon goes in (`GetTutorialStepCell`) and that icon (`GetTutorialStepIcon`); only that cell takes input (`UFocusOverlay::SetInputWindow`), plus the cell window on touch. Its popup's last line depends on `PC_Happiness.InputMode` (touch: tap the cell; mouse: click the icon in it). Touch: the cell window opening on that cell (`FindCellDialogIcon`, by `WBP_Icon.TheIcon`'s texture) starts three window steps (cells step + 1 to + 3): the focus goes above the window (its Z order + 1) and a single tight circle moves to the window's icon ("Tap on the [icon] icon to select it."), then, once the window's `SelectedIcon` is it, to Confirm (`SizeBox_Confirm`; "Tap the Confirm button to set the [icon] in this cell."), then, once Confirm clears `SelectedIcon`, to Commit (`Button_Done`; "Tap the Commit button to apply this change to the puzzle."); each alone takes input, and the popup sits over the window's Reset All and Eliminate buttons, right of Confirm. Mouse: clicking the icon in doesn't move on; input is blocked and the popup shows its Next at once (`bTutorialPlacedNext`, `UTutorialPopup::ShowNextNow`; it kept the button's room from the start, `bReserveNext`, so it doesn't resize). Touch moves on once the cell has its icon (`IsTutorialCellPlaced`, `WBP_Happiness.ThePuzzle`) and the window is closed. Step 5's popup (right of the vertical clues, below the grid's second row): "This clue says [star] and [unicorn] are in the same column." Step 6 (its Next): that circle stays and a second moves to tight round column 3's rows 1-2; the star's cell (row 1, column 3); "Since we know where the [unicorn] is, the [star] must be in this column too." then "Tap the cell with the [star] above the [unicorn]." (touch) or "Left Click the [star] icon to set it in this spot in the grid." (mouse). Step 10 (the star placed): the focus goes back to its Z order, input blocked but for Next, the circle moves from the star's cell to the horizontal clue (the Span); popup with Next, as wide as the screen allows, just below the grid's second row: "This clue says the [triangle] is in a column next to the [star]." Step 11 (its Next): the clue's circle stays and a second moves (tight) to row 1's cells in columns 2 and 3; the triangle's cell (row 1, column 2); popup (placed as step 10's) "There is only one place that [triangle] can go to comply with the clue." then "Tap the middle cell containing the [triangle]." or "Left click the [triangle] in the middle cell to set it here." Step 15 (the triangle placed): the circle moves to round the heart's cell (row 1, column 1, which the game set as its only option left); popup with Next: "Notice the [heart] is now set here as the only option left for this cell." Step 16 (its Next): back to the horizontal clue: "This clue also says the [phoenix] is in a column next to the [triangle]" (no full stop). Step 17 (its Next): the clue's circle stays and a second moves to round rows 1-2 of columns 1-2 (through the four cells' outer corners); the phoenix's cell (row 2, column 1); popup "The [star] is in the right column. The [triangle] is in the middle column. The clue says the next column should have the [phoenix], so the [phoenix] belongs on the left column.", then on a new line "Tap the far left cell containing the [phoenix]" (no full stop) or "Left click the [phoenix] to set it here." (the game then sets the dragon). Step 21 (the phoenix placed): the clue's circle goes and one moves to the Hint button (the button panel's `Button_H`, `GetHintButton`), which alone takes input, with the confirm window it opens (the screen's `ConfirmDialog`); popup (no Next) "If you don't know what to do next, press the Hint button. Hints wont hurt you, they can only help you to improve. Press the Hint button now." Cancel doesn't move on. Step 22 (its Confirm: the screen's `CurrentHint` is set; the hint is the second vertical clue, dragon over bee): two circles move from the Hint button, to tight round the second vertical clue and to tight round column 2's rows 2-3; input blocked but for Next; popup at the top of the screen, above row 2: "This clue can help you right now. The highlighted icon is the one it's talking about, and the description tells you what to do with it." Step 23 (its Next): the same circles; the bee's cell (row 3, column 2); popup (placed as step 22's) "Follow the hint description and set the [bee] in the middle column with the [dragon]" (no full stop). Step 27 (the bee placed), the elimination: the clue's circle moves to tight round the third vertical clue (Not Same Column: the phoenix and the ladybug), the grid's to tight round column 1's rows 2-3; the cell is row 3, column 1 and the icon the ladybug (`IsTutorialEliminateStep`); popup (right of the vertical clues, just below the grid) "This clue says the [ladybug] is not in the same column as the [phoenix]." then "Tap the cell below the [phoenix] to eliminate the [ladybug] from it." or "Right click the [ladybug] below the [phoenix] to eliminate it from this cell." Eliminating it leaves the butterfly as that cell's only option, so the game sets it (and the ladybug in column 3): the cell counts as placed, as in the other cells steps. Touch: its window steps are the ladybug (28), the window's Eliminate button (29, `SizeBox_Eliminate`, once `SelectedIcon` is the ladybug; "Tap the Eliminate button to remove the [ladybug] from this cell.", popup over Confirm and Reset All, left of Eliminate) and Commit (30, once the window's `EliminatedButtonText` reads Restore). That completes the puzzle: in the tutorial `PlaySolvedFireworks` returns true without fireworks and never fires `OnFireworksFinished`, so `RefreshPuzzle` shows no end screen and the board stays completed. Step 31, the last: the clue's circle goes and column 1's grows to round the whole grid; input is blocked but for the popup's buttons; the popup (across the bottom of the screen) says "This is what a fully solved puzzle looks like. ... check out the Game Rules." with three buttons (`UTutorialPopup::ThirdLabel`/`OnThird`, `SecondLabel`/`OnSecond` and `NextLabel`): Restart Tutorial (`HandleTutorialRestart`) closes the puzzle screen as Proceed to Lessons does, hides the mode screen and plays the tutorial again (`UCampaignTreeWidget::PlayTutorial`); Learn More opens WBP_GameRules over the puzzle screen (Z 300 in its canvas, `HandleTutorialLearnMore`), closing back to the popup; Proceed to Lessons (`ProceedFromTutorial`) fires WBP_Happiness's `Close` dispatcher (WBP_GameSelect's CloseHappiness, then the mode screen) and then `UHappinessModeWidget::ShowLessonsAt(VerticalTwo)`: the Lessons tree with Game Basics' popup open. The clue-panel circles go round the clue widgets (`GetShownClues`); the close-up and its popup area use their icon boxes (`bIconBoxes`), as a vertical clue's widget is taller than its icons. From the focus's start until Next, the focus takes every tap and click (`UFocusOverlay::SetBlocksInput`), so the Next button is the only thing that can be pressed. A new start of the tutorial (`UCampaignSubsystem::GetTutorialStarts`) plays it all again; any other puzzle removes both. Dev: `Happiness.Focus x,y,r[,StartRadius,Duration] ...` (fractions of the screen; alone, removes it), `Happiness.FocusWidget <Name> [Padding] [StartRadius] [Duration]`. |
| `Happiness/UI/WBP_Options` | `UI/OptionsMenuWidget` | Garden Options A: Sound Effect Volume, Music Volume, Prevent Sleep on Idle, Animate Hint Button. Native layout reuses the original Blueprint controls and delegates; Show/Apply/Cancel and sound previews stay in the Blueprint. Prevent Sleep on Idle is displayed only on Android/iOS; PC omits its entire row. Hidden legacy controls retain their loaded values. SettingsScroll holds the settings, with Apply/Cancel fixed below. Add future WBP_CheckBox children to the designer's ExtraSettings vertical box: native layout applies the same row/check styling and includes them in scrolling; wire their settings in Show/Apply as usual. Opened from pause and Player Info. |

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
`Undo` then `RefreshPuzzle`, so with nothing to undo it simply shows again; Restart (`OnRestart`) resets like the pause menu's Reset Puzzle. A **reset** (`UPuzzle::ResetBoard`) keeps the clock and the hints used; only an end screen's Restart (`RestartPuzzle`) starts over as if the puzzle had just been entered: no hint or selected
clue, `UPuzzle::Reset` (no hints used), the clock at 0:00, then `SetPuzzle` again (clue panels rebuilt, undo list
emptied; `RestoreUndo` only acts on the first `SetPuzzle` after a load) and one save through `RefreshPuzzle`.
The widget builds its own layout in C++ (`RebuildWidget`) with the existing hint-reading frame and notebook button art.

The player controller `General/Blueprints/PC_Happiness` starts, loads and saves puzzles and holds the player's
level and EXP (`HappinessLevel`, `HappinessExp`). Its `InputMode` (mouse, or touch on Android and iOS) must be set
before any puzzle screen exists, so BeginPlay sets it first: `WBP_GamePanel` wires each cell for the mode when it
builds the grid (touch: `Button_Cell` opens the cell dialog; mouse: the icons take clicks). Set after that, a grid
takes no taps.

## Puzzle code (`Source/Happiness/HappinessClassic/`)

- `Puzzle.*`: generation, solving, rating (`ComputeRating`/`GetRating`), hint selection (`GenerateHint`).
- `Clue.*`: every clue type, its analysis and its help text.
- `Hint.*`: one hint (`UHint`): the action and its plain-English explanation (`GetExplanation`). With mistakes on the board (an icon placed
  where it doesn't go, or the right icon ruled out) a hint points at one instead (`UPuzzle::FindErrorHint`,
  `UHint::bError`): its reasons come from the board with the mistakes corrected (`FixPuzzle` on a copy): a clue that
  shows it straight away (its own explanation), else one that would leave an icon nowhere to go were the mistake
  right, else the clue that settles that cell later ("No clue puts it there yet"). Wrong placements first. The cell's
  placed image then gets the hint's sparkles and red tint (`UGameCellWidget::UpdatePlacedHint`); once it's cleared, the
  hint goes on as the elimination. Clearing a placed icon (dialog Reset All, a middle click: `RestoreIocn` /
  `BreakFinalIcon`, which call `UPuzzle::ClearPlacement`) makes the icon possible again in the row's other open cells
  (placed cells are left alone: the analyzers expect a placed cell's candidates to be only its icon, and a cleared
  cell's candidates are rebuilt from the row anyway). The cell dialog's Reset brings back only the icons not placed
  elsewhere in the row (`UGameCellWidget::IsPlacedElsewhere`, the puzzle screen's `UPuzzle::IsPlacedElsewhere`): committing an icon placed elsewhere restores it here, which
  breaks that other placement (`RestoreIocn`). Ruling a placed icon out (right click)
  clears the placement first (`UnplaceIcon`), so the icon can't be left with nowhere to go. Test: `Tools/puzzle.sh -mistakes`. Rule from the
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
completed; `IS_MythicalCreatures` once every lesson's Campaign final is (`UCampaignSubsystem::IsCampaignCompleted`,
Master Mode not needed). `IS_MasterRewardGems` is hidden until every lesson's Master Mode is completed
(`IsAllMasterCompleted`; test with `Happiness.Campaign.MasterAll`). The 31 sets in `Icons/Extra` are listed
with `Purchase`: on PC unlocked from the start but off (`FIconSetSaveData::EnabledSets` holds the ones turned on); on
phones each is a one-time store product, `iconset_<set id in lower case>` (`iconset_arcticanimals`), bought from its
card on the icon sets screen ("Buy <price>") and on once bought. `Ads/IconSetStore` (`UIconSetStoreSubsystem`) keeps
the purchases in `iconsets.owned` in the app's private storage, so they hold offline, and checks them against the
store at startup and on each return to the foreground: a paid purchase missing from the file is added (reinstall,
new phone); one the store no longer lists is removed only once the store's servers are confirmed reachable (as Remove
Ads, `Ads/AdRemoval.h`). The Remove Ads Restore Purchases also restores the sets. Test on PC:
`Happiness.IconSets.SimulateStore 1` (act as a phone; buying grants at once), `Happiness.IconSets.Owned <SetId|all> <1|0>`.
`ComingSoon` (shown locked, never unlocked) is unused. To add an unlockable set, add it to `DA_IconSetUnlocks` (new conditions go in `EIconSetUnlock` /
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
store: buying grants the days at once. The mode screen makes the Streak Saver button (just below the Daily tile, never over a tile) and the
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
`bRestoreNotificationsOnReboot` brings Android's back after a restart.
Tapping one starts or resumes GameActivity without SplashActivity; `Happiness_UPL.xml` keeps Back/Home/Recents hidden
then: it passes the full-screen settings on at a cold start, and hides the bars again whenever the game's window gets
focus back (`onWindowFocusChanged`), as the notification shade closes after the engine's own `onResume` hide.
Tapping one while the game is running in the background goes to the main menu (`UDailyReminderSubsystem::ReturnToMainMenu`
opens the game level again, as at a start): Android catches the tap in `onNewIntent` (`Happiness_UPL.xml`,
`AndroidThunkJava_Happiness_TakeNotificationTap`, read as the game comes back), iOS through the engine's
`ApplicationReceivedLocalNotificationDelegate`; only once the game has been in the background, so a cold start is left alone. On Android the small icon is the
`ic_notification` drawable if there is one, else the app icon. Test on a phone with `Happiness.Reminder.Test [seconds]`.
Android and iOS only; it does nothing in the editor.

## Leaving the app mid-puzzle

When the app stops being active (the phone locked, Home, another app or a call on top: `FCoreDelegates::ApplicationWillDeactivateDelegate`, Android and iOS), `UAppPauseSubsystem` (`Source/Happiness/AppPause.*`) pauses a puzzle in progress, so the player comes back to the pause menu with the clock stopped (`PausePuzzle`: WBP_Happiness's `Pause`, unless the pause menu or an end screen is already showing). An interstitial covers the game too, and Android may hand the game that deactivation only after the ad has closed, so deactivations while one plays or within 2 s of its end are ignored (`UAdsSubsystem::IsInterstitialRecent`). `Happiness.SimulateAppDeactivate` does the same in PIE (dev builds).

## Android back button

`UBackButtonSubsystem` (`Source/Happiness/BackButton.*`, Android only) catches the back button before the UI does (a
Slate input preprocessor) and sends the app to the background with `moveTaskToBack` (Java side
`AndroidThunkJava_Happiness_MoveToBack` in `Happiness_UPL.xml`), like Home: the game keeps its state. Mid-puzzle it
first pauses the puzzle (`UAppPauseSubsystem::PausePuzzle`, above). No screen handles back itself.

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
The undo list goes with the puzzle: as a game save is written, `UHappinessSaveGame` takes the puzzle screen's
`ActionList` (`HappinessAction` entries, as property text, `UndoActions`) and the puzzle's solution (`UndoSolution`);
`WBP_Happiness`'s `SetPuzzle` empties the list, then `UHappinessSaveGame::RestoreUndo` brings the saved one back if
the screen shows that puzzle on the board it was saved with (a resume; a new puzzle never matches). Hidden clues go
with it too (`HiddenClues`: each clue's place in the puzzle's horizontal or vertical clues, in the order they were
hidden, with its undo step; `UPuzzle::GetHiddenClues` / `RestoreHiddenClues`, which takes them off the rebuilt clue
panels in that order). `Undo` removes the action before `RefreshPuzzle` saves. Clue changes aren't board moves, so
`HideSelectedClue`, `Undo` (when it brings a clue back) and `UnhideClues` call `SaveHappiness` themselves.
A save of a solved board (the solving move, the end screen's saves) clears `ActivePuzzle`, so leaving the game on the
end screen doesn't resume a finished puzzle (`UHappinessSaveGame::IsBoardSolved`; `CanResume` also refuses such a slot).
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

### Icon Sets catalogue UI

`UI/IconSetsPanel` is a code-built book: a vertical selectable list on the left, a large square icon grid and state button on the right. Row status buttons and the large button enable/disable the set; row backgrounds select the preview. `UIconSetSubsystem::GetEntries` filters hidden sets and supplies achievement locks; `SetEnabled` keeps at least eight enabled. Purchase presentation follows `FIconSetEntry::bForSale` and `UIconSetStoreSubsystem`: localized prices, purchasing/pending states, and `Buy(Entry.Id)`. Store change notifications refresh both the list and the selected preview while preserving selection and scroll position.

### Remove Ads garden purchase window

`WBP_HappinessMode.AdsPurchaseDialog` uses `T_RemoveAdsGarden` with live benefit/title text and a blurred, dimmed backdrop. Its existing `PurchaseControls` (`WBP_RemoveAds`, native parent `UI/RemoveAdsPanel`) sets `bDialogLayout=true` to stack the live buy/restore buttons. The same widget in Options keeps its compact row. Native preconstruct applies the shared garden font/button faces; the ad-removal subsystem still supplies prices, pending/purchased/offline states and restore results. The background texture is directly referenced by the Blueprint for cooking.

### Main-menu Streak Saver button

`UHappinessModeWidget::MakeStreakSaverButton` creates a 300x72 calendar/shield button below the Daily tile, aligned with its left edge, with a 10px gap. It is a sibling of the Daily button, so neither tap rectangle overlaps and the shop stays usable after today's Daily is completed. The title is constant; the second line shows `UDailySubsystem::GetSaverDays()` with singular/plural days. `T_StreakSaverButton` is tightly cropped and included in `PAL_LoadedByCode`. The existing shop callback and purchase behavior are unchanged.

### Streak Saver purchase window

`UI/StreakSaverPanel` is a code-built garden panel using `T_StreakSaverShop` (listed in `PAL_LoadedByCode`). Its live status strip separates current/restorable streak length from available saver days. A wrapping recovery message sits above the three 1/7/30-day purchase cards, whose actions retain `UStreakSaverStoreSubsystem::Buy` and localized prices. Pending and offline messages sit in the footer; the round close button replaces the old bottom Close row. Preview-only data never changes transactions, inventory or saves.

Campaign/Lessons puzzle slots keep the board's original `FCampaignSaveData::OpenSeed`, not the stage's advanced `NextSeed`. Once completion advances the stage seed and closes the puzzle, its end-screen snapshot is inactive even if undo/solution capture is unavailable; leaving an unfinished session remains resumable. Resume also rejects older finished-session slots mislabeled with `OpenSeed + 1`. Regression check: `Tools/puzzle.sh -saveresume` (in-memory serialization; no player save writes).

The shared Campaign tree heading is mode-aware (`UCampaignTreeWidget::RefreshHeading`): Lessons keeps its designer brush; Campaign uses `T_CampaignHeading` in the same title slot. The latter is explicitly cooked via `PAL_LoadedByCode`. Render either mode with `RenderWidget`'s `PreviewTreeMode` (0 Lessons, 1 Campaign), without changing live progression.
