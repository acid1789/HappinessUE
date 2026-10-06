#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HappinessClassic/CampaignTree.h"
#include "Styling/SlateTypes.h"
#include "GameRulesWidget.generated.h"

class APlayerController;
class UButton;
class UPanelWidget;
class UPuzzle;
class UClue;
class UScaleBox;
class UTextBlock;
class UTexture2D;
struct FClueExample;

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
	/** Basics: what the picture should show, until there is artwork (Pictures) */
	FText PictureNote;
	/** Game Layout: a diagram of the puzzle screen's areas instead of a picture */
	bool bLayoutDiagram = false;
	/** A clue subject: its example is a generated clue of this type */
	bool bClue = false;
	ECampaignLesson Lesson = ECampaignLesson::Given;
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

	/** Show the rules (PlayerController: for the game's icon sets) */
	UFUNCTION(BlueprintCallable, Category = "Game Rules")
	void Show(APlayerController* PlayerController);

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

	/** The index entries go in here (inside a ScrollBox) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> SubjectList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DescriptionText;

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
	/** A clue widget showing a clue of this type, or null */
	UUserWidget* MakeExample(ECampaignLesson Lesson);
	/** A generated puzzle with a clue of this type in it */
	bool FindExampleClue(ECampaignLesson Lesson, UPuzzle*& OutPuzzle, UClue*& OutClue);
	/** The game's icon sets (PC_Happiness's HappinessIconSets), or empty */
	TArray<UObject*> GetIconSets() const;
	/** The two example boards for a clue */
	void AddBoards(const UPuzzle& Puzzle, const UClue& Clue, const TArray<UObject*>& IconSets);
	/** One board: the pieces' rows across every column, each piece in its column; Highlight: a piece shown in BreaksColor */
	UWidget* MakeBoard(const FClueExample& Example, const TArray<int32>& Columns, int32 Highlight, const TArray<UObject*>& IconSets,
		const FText& Caption, const FLinearColor& Color);
	/** Basics: the picture, or a placeholder box with the note */
	void AddPicture(const FRulesSubject& Subject);
	/** Game Layout: the puzzle screen's areas as labelled boxes, where they are on the real screen */
	void AddLayoutDiagram();
	/** Game Layout: each button of the button panel as it looks, with what it does */
	void AddButtonLegend();

	UFUNCTION()
	void HandleClose();

	static TArray<FRulesSubject> MakeSubjects();

	TArray<FRulesSubject> Subjects;

	/** Per subject: its index button */
	UPROPERTY()
	TArray<TObjectPtr<UButton>> EntryButtons;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> EntryTexts;

	UPROPERTY()
	TArray<TObjectPtr<URulesEntryHandler>> EntryHandlers;

	/** Example puzzles and clues by clue type (made once) */
	UPROPERTY()
	TMap<ECampaignLesson, TObjectPtr<UPuzzle>> ExamplePuzzles;

	UPROPERTY()
	TMap<ECampaignLesson, TObjectPtr<UClue>> ExampleClues;

	UPROPERTY()
	TObjectPtr<APlayerController> PC;

	/** The diagram's size (the phone's 20:9 screen) */
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	FVector2D LayoutDiagramSize = FVector2D(600.f, 270.f);

	int32 Selected = INDEX_NONE;
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
