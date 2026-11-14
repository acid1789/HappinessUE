#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "GamePanelWidget.generated.h"

class UCanvasPanel;
class UFocusOverlay;
class UGameCellWidget;
class UGridPanel;
class UPuzzle;
class UTutorialPopup;
class UImage;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFireworksFinished);

/**
 * Keeps the puzzle's columns sized to its two rows of candidate icons, and celebrates a solved puzzle before its end
 * screen: every icon bursts into confetti (UGameCellWidget::PlayFirework) in a random order, spread evenly over
 * FireworksDuration. EndScreenDelay after the first burst the screen is told to show the end screen
 * (OnFireworksFinished); the rest go off behind it. The icons stay hidden until the puzzle is no longer complete
 * (restarted, or another puzzle started).
 *
 * In the tutorial puzzle it also focuses the board: a UFocusOverlay over the puzzle screen (TutorialFocusZOrder, under
 * its hint panel, dialogs and menus) dims everything but circles round what each step is about, with a UTutorialPopup
 * explaining it. The steps follow the tutorial puzzle's solve (seed 129, UCampaignSubsystem::TutorialSeed): the grid,
 * the clues, a Same Column clue and the star it places, the Span clue and the triangle and phoenix it places (the heart
 * set by the game as its cell's only option), a hint (the bee), and the Not Same Column clue eliminating the ladybug,
 * which completes the puzzle. Docs/Agents/GameStructure.md has them step by step (TutorialStep).
 */
UCLASS(Abstract)
class HAPPINESS_API UGamePanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Seconds every icon bursts within */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fireworks")
	float FireworksDuration = 1.f;

	/** Seconds from the first burst to the end screen */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fireworks")
	float EndScreenDelay = 0.5f;

	/**
	 * Call when the puzzle is completed, before showing its end screen. Returns true if the end screen has to wait:
	 * the fireworks have started (or are still playing) and OnFireworksFinished follows. False to show it now: the
	 * board has mistakes, or the fireworks for this puzzle have just finished. The tutorial puzzle, solved: true, with no
	 * fireworks and no OnFireworksFinished (no end screen; the board stays completed).
	 */
	UFUNCTION(BlueprintCallable, Category = "Fireworks")
	bool PlaySolvedFireworks(UPuzzle* Puzzle);

	/** The fireworks are over: show the end screen (call RefreshPuzzle again, which won't wait this time) */
	UPROPERTY(BlueprintAssignable, Category = "Fireworks")
	FOnFireworksFinished OnFireworksFinished;

	/** Seconds the tutorial's focus circle takes to close in on the grid */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	float TutorialFocusSeconds = 1.f;

	/** Where the tutorial's focus overlay goes in the puzzle screen's canvas: over the board, clues and buttons (0)
	 *  and the banner (5), under the hint panel (10), the cell window and end screens (20) and the pause menu (100) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	int32 TutorialFocusZOrder = 8;

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCanvasPanel> Border_44;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void UpdateFireworks(float DeltaTime);
	/** Shows the icons of the cells that have burst again */
	void EndFireworks();
	/** The puzzle the fireworks were for has every icon set */
	bool IsFireworksPuzzleComplete() const;

	/** The tutorial's focus on the grid: added once each time the tutorial puzzle starts, gone with any other puzzle */
	void UpdateTutorialFocus(UGridPanel* Grid);
	TWeakObjectPtr<UFocusOverlay> TutorialFocus;
	TWeakObjectPtr<UTutorialPopup> TutorialPopup;
	/** UCampaignSubsystem::GetTutorialStarts when TutorialFocus was added */
	int32 TutorialFocusStart = -1;
	/** Seconds since TutorialFocus was added */
	float TutorialFocusTime = 0.f;
	/** The tutorial's step: 0 the grid, 1 the categories (the grid still; the rows' colors show), 2 the columns (the
	 *  grid still; the columns' colors show instead), 3 the vertical clues (after the grid popup's Next), 4 the
	 *  horizontal clues, 5 the first vertical clue up close (star over unicorn), 6 its cells in the grid (the star's), 7
	 *  (touch) the star in the cell window, 8 (touch) its Confirm button, 9 (touch) its Commit button, 10 the horizontal
	 *  clue (the Span), 11 the triangle's cells, 12 to 14 (touch) as 7 to 9 for the triangle, 15 the heart's cell (set by
	 *  the game), 16 the horizontal clue again, 17 rows 1-2 of columns 1-2 (the phoenix's cell, row 2 column 1), 18 to 20
	 *  (touch) as 7 to 9 for the phoenix, 21 the Hint button, 22 the hint shown (the second vertical clue and column 2's
	 *  rows 2-3), 23 the bee's cell (the same circles), 24 to 26 (touch) as 7 to 9 for the bee, 27 the third vertical
	 *  clue (Not Same Column) and column 1's rows 2-3 (eliminating the ladybug from row 3), 28 (touch) the ladybug in the
	 *  cell window, 29 (touch) its Eliminate button, 30 (touch) its Commit button, 31 the solved grid (the last: Learn
	 *  More opens the game rules over it, Proceed to Lessons closes the puzzle onto Game Basics' lesson popup).
	 *  TutorialFocusTime counts from the step's start. */
	int32 TutorialStep = 0;
	/** Mouse, a cells step (6, 11, 17, 23, 27): its icon was clicked into its cell (27: eliminated from it, and the game
	 *  set the cell). Input is blocked, and the step's popup shows Next, which moves on (instead of the click itself) */
	bool bTutorialPlacedNext = false;
	/** Shows the step's popup with Message, in the screen's canvas (placed by PlaceTutorialPopup) */
	void ShowTutorialPopup(UUserWidget* Screen, UCanvasPanel* ScreenCanvas, const FText& Message);
	/** The grid the tutorial's focus is on */
	TWeakObjectPtr<UGridPanel> TutorialGrid;
	/** The clues shown in the puzzle screen's clue panel Name (VerticalCluePanel, HorizontalCluePanel); bIconBoxes:
	 *  each one's icon box instead (a vertical clue's widget is taller than its icons) */
	TArray<UWidget*> GetShownClues(FName PanelName, bool bIconBoxes = false) const;
	/** The grid's cells in Column (from 0) and rows FirstRow to LastRow */
	TArray<UWidget*> GetGridCells(int32 Column, int32 FirstRow, int32 LastRow) const;
	/** The player's input is touch (PC_Happiness.InputMode): cells open the cell window */
	bool IsTutorialTouch() const;
	/** The open cell window's icon showing Texture (a WBP_Icon), if it's shown */
	UWidget* FindCellDialogIcon(const UTexture* Texture) const;
	/** The cells step the current step belongs to: 6 (the star's cells and its cell window steps, to 9), 11 (the
	 *  triangle's, to 14), 17 (the phoenix's, to 20), 23 (the bee's, to 26), 27 (the ladybug's elimination, to 30), or
	 *  -1 */
	int32 GetTutorialCellsStep() const;
	/** The cells a cells step's circle goes round (GetTutorialCellsStep), the cell its icon goes in (or, step 27, is
	 *  eliminated from; row and column from 0), and that icon */
	TArray<UWidget*> GetTutorialStepCells() const;
	void GetTutorialStepCell(int32& Row, int32& Column) const;
	const TCHAR* GetTutorialStepIcon() const;
	/** The cells step is the elimination (27): the cell window's Eliminate button instead of Confirm */
	bool IsTutorialEliminateStep() const;
	/** Step 7 or 12: the cell window opened on the step's cell; the focus goes above it, round its Icon */
	void StartTutorialDialogStep(UWidget* Icon);
	/** The cell window (WBP_CellDialog), if the puzzle screen has one */
	UUserWidget* GetCellDialog() const;
	/** A cells step + 2: the window's Icon is selected; the focus moves from it to the window's Confirm button (or, the
	 *  elimination, its Eliminate button) */
	void StartTutorialConfirmStep(UWidget* Icon);
	/** A cells step + 3: Confirm (or Eliminate) was pressed; the focus moves from it to the window's Commit button */
	void StartTutorialCommitStep();
	/** The open puzzle's cell at Row, Column (from 0) has an icon placed */
	bool IsTutorialCellPlaced(int32 Row, int32 Column) const;
	/** Step 10: the star is placed; the focus moves from its cell to the horizontal clue */
	void StartTutorialHorizontalStep();
	/** Step 15: the triangle is placed; the focus moves from its cells to the heart's cell (the game set it) */
	void StartTutorialOnlyOptionStep();
	/** Step 21: the phoenix is placed; the focus moves from its cells to the Hint button (the clue's circle goes) */
	void StartTutorialHintStep();
	/** The puzzle screen's Hint button (its button panel's Button_H), if it has one */
	UWidget* GetHintButton() const;
	/** Step 22: the hint's Confirm was pressed (the screen has a CurrentHint); two circles move from the Hint button */
	void StartTutorialHintShownStep();
	/** Step 27: the bee is placed; the circles move to the third vertical clue (Not Same Column) and column 1's rows 2-3 */
	void StartTutorialEliminateStep();
	/** Step 31: the ladybug is eliminated (the game sets the rest: the puzzle solved); the circle goes round the grid */
	void StartTutorialSolvedStep();
	/** The solved step's Learn More: the game rules (WBP_GameRules) over the puzzle screen; closing them comes back */
	UFUNCTION()
	void HandleTutorialLearnMore();
	/** The solved step's Proceed to Lessons: the puzzle screen closes onto the Lessons tree, Game Basics' popup open */
	void ProceedFromTutorial();
	/** The solved step's Restart Tutorial: the puzzle screen closes and the tutorial plays again from its start (as the
	 *  Lessons screen's Tutorial button) */
	UFUNCTION()
	void HandleTutorialRestart();
	/** Hides the Learn More rules and closes the puzzle screen (WBP_Happiness's Close dispatcher); the menu canvas the
	 *  screen was in, or null */
	UCanvasPanel* CloseTutorialScreen();
	/** HandleTutorialLearnMore's rules screen, made the first time (it hides itself when closed) */
	UPROPERTY()
	TObjectPtr<UUserWidget> TutorialRulesWidget;
	UFUNCTION()
	void HandleTutorialNext();
	/** From the categories step (1) on: each of the grid's rows gets its own color behind its icons (its cells' buttons'
	 *  backgrounds, and a box behind the grid in the grid's canvas for the gaps between them), row 1's first, then each
	 *  next one half a second later. From the columns step (2), the columns get them instead (bTutorialColorColumns), the
	 *  same way; they stay for the rest of the tutorial */
	void UpdateTutorialRowBoxes(UGridPanel* Grid, float DeltaTime);
	TArray<TWeakObjectPtr<UImage>> TutorialRowBoxes;
	/** The cells' buttons UpdateTutorialRowBoxes tinted, with their own styles (put back when the tutorial ends) */
	TMap<TWeakObjectPtr<UButton>, FButtonStyle> TutorialRowButtonStyles;
	/** Seconds since the categories step started, or -1 before it */
	float TutorialRowTime = -1.f;
	bool bTutorialColorColumns = false;
	/** Places TutorialPopup in the space right of the grid, in the screen's canvas */
	void PlaceTutorialPopup(UGridPanel* Grid, UCanvasPanel* ScreenCanvas);

	/** Playing until the end screen is due (bursts may go on behind it: bBursting), then Finished until it shows */
	enum class EFireworks : uint8 { None, Playing, Finished };
	bool bBursting = false;
	EFireworks Fireworks = EFireworks::None;
	TWeakObjectPtr<UPuzzle> FireworksPuzzle;
	/** Seconds since the first burst, and between bursts (FireworksDuration over the number of cells) */
	float FireworksTime = 0.f;
	float FireworksInterval = 0.f;
	int32 NumFired = 0;
	/** The first update after starting only waits a frame, for the cells' effect widgets (PrepareFirework) */
	bool bFireworksPrepared = false;
	/** Cells still to burst, in order; and those that have, to show their icons again once the puzzle changes */
	TArray<TWeakObjectPtr<UGameCellWidget>> FireworkCells;
	TArray<TWeakObjectPtr<UGameCellWidget>> FiredCells;

	bool bSavedHintLayout = false;
	float NormalPanelTop = 0.f;
	ESlateVisibility NormalHintVisibility = ESlateVisibility::SelfHitTestInvisible;
};
