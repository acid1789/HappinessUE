#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HappinessClassic/CampaignTree.h"
#include "Styling/SlateTypes.h"
#include "HappinessClassic/PuzzleCell.h"
#include "GameRulesWidget.generated.h"

class APlayerController;
class UButton;
class UBorder;
class UPanelWidget;
class UPuzzle;
class UClue;
class UScaleBox;
class UTextBlock;
class UTexture2D;
struct FClueExample;

/** One step of a clue page's block of board cells (FRulesCard), on a piece of the clue's example (FClueExample) */
enum class ERulesBlockOp : uint8
{
	Set,	// the piece's icon placed in Column (ruled out of the rest of its row)
	Other,	// another icon of the piece's row placed in Column, so the piece's icon can't be there
	Out,	// the piece's icon ruled out of Column
	Red,	// the piece's icon in Column shown ruled out as a hint shows it (red): what the clue rules out
	Anim,	// the piece's icon in Column placed and taken back in a loop: what the clue places
};

struct FRulesBlockOp
{
	ERulesBlockOp Op = ERulesBlockOp::Set;
	int32 Piece = 0;
	int32 Column = 0;
};

/**
 * A clue page's card: a block of board cells (the clue's rows, Columns columns) showing one thing the clue tells you,
 * with a line saying it. bRuleOut: what it shows is ruled out (red card), else placed (green card).
 */
struct FRulesCard
{
	bool bRuleOut = false;
	int32 Columns = 2;
	FText Note;
	TArray<FRulesBlockOp> Ops;
	/** If any: a second mock beside the first, separate from it (its own cells, set apart) */
	TArray<FRulesBlockOp> SecondOps;
};

/** One entry of the rules' subject index */
USTRUCT()
struct FRulesSubject
{
	GENERATED_BODY()

	/** Section header to start before this subject (empty: none) */
	FText Section;
	FText Title;
	/** One or two lines: the picture does the explaining */
	FText Description;
	/** Additional guidance below the diagrams, scrollable independently of the examples */
	FText Details;
	/** Controls: Description and Details for touch input (the cell dialog); the plain ones are for the mouse */
	FText TouchDescription;
	FText TouchDetails;
	bool bControls = false;
	/** Basics: what the picture should show, until there is artwork (Pictures) */
	FText PictureNote;
	/** Which Basics illustration to draw when there is no supplied picture */
	int32 BasicIndex = -1;
	/** Game Layout: a diagram of the puzzle screen's areas instead of a picture */
	bool bLayoutDiagram = false;
	/** Clues: a mock clue panel, three horizontal clues with the middle one selected, instead of a picture */
	bool bClueSelect = false;
	/** Hints: the hint window, the hinted clue with its sparkles and the cell it rules an icon out of, instead of a picture */
	bool bHintMock = false;
	/** A clue subject: the text beside its example clue (Description is the page's main text) */
	FText ExampleCaption;
	/** A clue subject: its example is a generated clue of this type */
	bool bClue = false;
	ECampaignLesson Lesson = ECampaignLesson::Given;
	/** The Edge lesson's inner-edges page: its example is a NotEdge clue (the outer-edges page's an Edge one) */
	bool bNotEdge = false;
};

/**
 * Parent of WBP_GameRules, the game rules screen (from the pause menu's Game Rules). A scrolling subject index on
 * the left (Basics, Vertical Clues, Horizontal Clues); on the right the selected subject's title, its pictures and a
 * line or two of text.
 *
 * Clue subjects: the clue itself (a real WBP_VerticalClue / WBP_HorizontalClue filled from a small generated Lessons
 * puzzle of that type, with the game's icon sets), then two small boards of the clue's rows: "Fits the clue" with
 * the icons where the solution has them, and "Breaks the clue" with one moved where the clue forbids (FClueExample).
 * Basics subjects: a picture (Pictures, by title), or a placeholder saying what the picture should show.
 */
UCLASS()
class HAPPINESS_API UGameRulesWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGameRulesWidget(const FObjectInitializer& ObjectInitializer);

	/** Show the rules, open at the first subject, Objective (PlayerController: for the game's icon sets) */
	UFUNCTION(BlueprintCallable, Category = "Game Rules")
	void Show(APlayerController* PlayerController);

	/** Show the rules open at a clue type's subject (the lessons' Learn More); Game Basics (VerticalTwo), or a clue with none, opens at Objective */
	UFUNCTION(BlueprintCallable, Category = "Game Rules")
	void ShowLesson(APlayerController* PlayerController, ECampaignLesson Lesson);

	/** Look of the index entries */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	FButtonStyle EntryStyle;

	UPROPERTY(EditAnywhere, Category = "Game Rules")
	FSlateFontInfo EntryFont;

	UPROPERTY(EditAnywhere, Category = "Game Rules")
	FSlateFontInfo SectionFont;

	UPROPERTY(EditAnywhere, Category = "Game Rules")
	FLinearColor EntryColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Game Rules")
	FLinearColor SelectedColor = FLinearColor(1.f, 0.85f, 0.2f);

	UPROPERTY(EditAnywhere, Category = "Game Rules")
	FLinearColor SectionColor = FLinearColor(1.f, 1.f, 0.f);

	/** Height of the button copies in the Game Layout legend */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	float LegendButtonSize = 52.f;

	/** The puzzle screen's button panel: the Game Layout legend shows its buttons (soft, like the clue widgets) */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	TSoftClassPtr<UUserWidget> ButtonPanelClass;

	/** The game's clue widgets (soft: loaded on first use, see UPauseMenuWidget::OptionsClass) */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	TSoftClassPtr<UUserWidget> VerticalClueClass;

	UPROPERTY(EditAnywhere, Category = "Game Rules")
	TSoftClassPtr<UUserWidget> HorizontalClueClass;

	/** The puzzle screen's cell dialog: the touch Controls page shows one, filled with a sample cell */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	TSoftClassPtr<UUserWidget> CellDialogClass;

	/** The board's cell (WBP_Cell): the mouse Controls page shows one */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	TSoftClassPtr<UUserWidget> GridCellClass;

	/** The cell dialog's panel size in WBP_CellDialog (SizeBox_15, centered in its canvas) */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	FVector2D CellDialogSize = FVector2D(1180.f, 620.f);

	/** The mouse Controls page's grid cell (WBP_Cell), at its own size before it is scaled to the picture */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	FVector2D GridCellSize = FVector2D(240.f, 160.f);

	/** The puzzle screen's horizontal clue panel width (WBP_Happiness's HorizontalCluePanel): the Clues page's clues are laid out at it */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	float HorizontalCluePanelWidth = 200.f;

	/** How wide the Clues page shows its clue stack */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	float ClueSelectMockWidth = 420.f;

	/** The clue info's height on the puzzle screen (WBP_Happiness's HelpPanel) */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	float HelpPanelHeight = 40.f;

	/** The widest the Clues page shows the clue info */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	float ClueInfoMockMaxWidth = 1150.f;

	/** The hint window's box on the puzzle screen (WBP_Happiness's HintInfo, the part beside the clue), and how wide the Hints page shows it */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	FVector2D HintInfoMockSize = FVector2D(760.f, 230.f);

	UPROPERTY(EditAnywhere, Category = "Game Rules")
	float HintInfoMockWidth = 1000.f;

	/** Same Column's Works board: each of its board cells, as shown */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	FVector2D SameColumnCellSize = FVector2D(150.f, 100.f);

	/** The Hints page's board cells, as shown */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	FVector2D HintCellMockSize = FVector2D(225.f, 150.f);

	/** The Hints page's space between the cells and the clue (a quarter of it between the two cells) */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	float HintMockGap = 32.f;

	/** Basics pictures by subject title (Objective, Game Layout, ...); a placeholder box until set */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	TMap<FString, TObjectPtr<UTexture2D>> Pictures;

	/** Board cells in the example boards */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	float BoardCellSize = 54.f;

	UPROPERTY(EditAnywhere, Category = "Game Rules")
	FLinearColor FitsColor = FLinearColor(0.25f, 0.85f, 0.35f);

	UPROPERTY(EditAnywhere, Category = "Game Rules")
	FLinearColor BreaksColor = FLinearColor(0.95f, 0.3f, 0.25f);

	/** The subject shown in the designer and in widget renders (the game always starts at the first) */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	int32 PreviewSubject = 0;

	/** Size of the puzzles the examples come from */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	int32 ExamplePuzzleSize = 4;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** The index entries go in here (inside a ScrollBox) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> SubjectList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Backdrop;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> IndexPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> DetailPaper;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DescriptionText;

	/** Short rule summary above the illustrations */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SummaryText;

	/** Holds the example clue (scaled to fit) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScaleBox> ExampleBox;

	/** The boards (clues) or the picture (basics) go in here */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> IllustrationBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

private:
	friend class URulesEntryHandler;

	void BuildIndex();
	void Select(int32 Index);
	/** Move the example and picture areas into the details' scroll box, under the fixed title and summary */
	void ScrollBelowSummary();
	/** The player's input is touch (PC_Happiness.InputMode), so the grid's cells open the cell dialog */
	bool IsTouchInput() const;
	/** The clue subject of this type, or INDEX_NONE */
	int32 FindLessonSubject(ECampaignLesson Lesson) const;
	/** A clue widget showing a clue of this type, or null */
	UWidget* MakeExample(ECampaignLesson Lesson, bool bNotEdge = false);
	/** A cell dialog showing a sample cell (two icons eliminated, one selected), for looking at only; null if none */
	UWidget* MakeCellDialogMock();
	/** A grid cell showing Cell in IconSet (the Balls if none), for looking at only; null if none */
	UUserWidget* MakeGridCellMock(const FPuzzleCell& Cell, UObject* IconSet = nullptr);
	/** The mouse Controls picture: the same ball eliminated (right-click) and placed (left-click), side by side; null if none */
	UWidget* MakeMouseControlsMock();
	/** The game's clue widget (vertical or horizontal) showing Clue, or null */
	UUserWidget* MakeClueWidget(UClue& Clue, UPuzzle& Puzzle, const TArray<UObject*>& IconSets);
	/** Clues: "Tap a clue to select it." over three horizontal clues, the middle one selected, then the clue info it fills */
	void AddClueSelectMock();
	/** Hints: the hint window over the hinted clue (sparkling) and the board cell with its icon ruled out (red, sparkling) */
	void AddHintMock();
	/** A generated puzzle with a clue of this type in it; for the Edge lesson, an Edge clue or (bNotEdge) a NotEdge one */
	bool FindExampleClue(ECampaignLesson Lesson, UPuzzle*& OutPuzzle, UClue*& OutClue, bool bNotEdge = false);
	/** The game's icon sets (PC_Happiness's HappinessIconSets), or empty */
	TArray<UObject*> GetIconSets() const;
	/** The two example boards for a clue */
	void AddBoards(const UPuzzle& Puzzle, const UClue& Clue, const TArray<UObject*>& IconSets);
	/**
	 * One board: the pieces' rows across every column, each piece in its column; Highlight: a piece shown in BreaksColor.
	 * GridOverride (GridHeight tall) goes in the board's frame instead of those rows.
	 */
	UWidget* MakeBoard(const FClueExample& Example, const TArray<int32>& Columns, int32 Highlight, const TArray<UObject*>& IconSets,
		const FText& Caption, const FLinearColor& Color, UWidget* GridOverride = nullptr, float GridHeight = 0.f,
		const FText& NoteOverride = FText::GetEmpty());
	/** The cards of a clue page other than Same Column's, for the clue's example; empty if it has none */
	static TArray<FRulesCard> MakeRulesCards(const UClue& Clue, const FClueExample& Example);
	/** A card's block of board cells (OutHeight tall), or null if it can't be made */
	UWidget* MakeRulesBlock(const FClueExample& Example, const FRulesCard& Card, const TArray<UObject*>& IconSets, float& OutHeight);
	/**
	 * Same Column's boards: the clue's column, two rows of board cells. The clue's first icon set at the top with
	 * the second still a candidate below it. bBreaks: another icon set at the top instead, the second ruled out (red)
	 * below it. Null if it can't be made.
	 */
	UWidget* MakeSameColumnBlock(const FClueExample& Example, const TArray<UObject*>& IconSets, float& OutHeight, bool bBreaks);
	/** A grid cell's icon shown ruled out, as a hint shows it: the hint effect, the icon red */
	void MarkCellRuledOut(UUserWidget* Cell, int32 Icon);
	/** The blocks' animated cells: Cell shows From, then To, in a loop (StartBlockAnimation) */
	void AddBlockAnimation(UUserWidget* Cell, const FPuzzleCell& From, const FPuzzleCell& To);
	void StartBlockAnimation();
	/** Each animated cell to its next state */
	void StepBlockAnimation();
	void StopBlockAnimation();
	/** Basics: the picture, or a placeholder box with the note */
	void AddPicture(const FRulesSubject& Subject);
	/** Game Layout: the puzzle screen's areas as labelled boxes, where they are on the real screen */
	void AddLayoutDiagram();
	/** Game Layout: each button of the button panel as it looks, with what it does */
	void AddButtonLegend();

	UFUNCTION()
	void HandleClose();

	/** Show the rules open at Subject, its index entry in view */
	void Open(APlayerController* PlayerController, int32 Subject);

	/** Next, bottom right: on to the following subject (hidden on the last) */
	void BuildNextButton(const FButtonStyle& Style);
	UFUNCTION()
	void HandleNext();

	UPROPERTY(Transient)
	TObjectPtr<UButton> NextButton;

	static TArray<FRulesSubject> MakeSubjects();

	TArray<FRulesSubject> Subjects;

	/** Per subject: its index button */
	UPROPERTY()
	TArray<TObjectPtr<UButton>> EntryButtons;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> EntryTexts;

	UPROPERTY()
	TArray<TObjectPtr<URulesEntryHandler>> EntryHandlers;

	/** Example puzzles and clues by clue type (made once), keyed by lesson * 2 (+1: the inner-edges example) */
	UPROPERTY()
	TMap<int32, TObjectPtr<UPuzzle>> ExamplePuzzles;

	UPROPERTY()
	TMap<int32, TObjectPtr<UClue>> ExampleClues;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> FieldGuideTexture;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CloseButtonTexture;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> TopicPillTexture;


	UPROPERTY()
	TObjectPtr<APlayerController> PC;

	/** The diagram's size (the phone's 20:9 screen) */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	FVector2D LayoutDiagramSize = FVector2D(600.f, 270.f);

	int32 Selected = INDEX_NONE;

	/** The clue pages' blocks: seconds between an animated cell placed and back to its candidates */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	float BlockAnimInterval = 2.f;

	/** The blocks' animated cells, two states for each (From, To), and which they show */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UUserWidget>> BlockAnimCells;

	TArray<FPuzzleCell> BlockAnimStates;
	int32 BlockAnimState = 0;
	FTimerHandle BlockAnimTimer;
};

/** Forwards one index button's click to its subject */
UCLASS()
class URulesEntryHandler : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UGameRulesWidget> Owner;

	int32 Index = 0;

	UFUNCTION()
	void HandleClicked();
};
