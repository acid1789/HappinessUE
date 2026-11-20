#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HappinessClassic/ClueHelp.h"
#include "FixReportWidget.generated.h"

class UButton;
class UClue;
class UFont;
class UPuzzle;
class UHint;
class USoundBase;
class UTextBlock;
class UTexture2D;
class UVerticalBox;

/**
 * Fix Puzzle's report (UPuzzle::FixPuzzleWithUndo), in the middle of the puzzle screen once the undos have played
 * out: where the player went wrong (their first mistake: what they did, and the clue that shows it's wrong and why,
 * or that it was a guess) and how many moves were undone. OK closes it. A clear blocker over the whole screen keeps
 * the board from taking taps while it's open. Builds its own widgets (no Blueprint layout).
 */
UCLASS()
class HAPPINESS_API UFixReportWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFixReportWidget(const FObjectInitializer& ObjectInitializer);

	/**
	 * Open the report over Screen (the puzzle screen, WBP_Happiness, whose IconSets draw the icons). Mistake is the
	 * error hint for the player's first mistake (UPuzzle::FindFixMistake), or null when there is none to show.
	 * Message says how many moves were undone. OnClosed runs when OK closes it.
	 */
	static void Open(UUserWidget* Screen, UHint* Mistake, const FText& Message, TFunction<void()> OnClosed);

	UPROPERTY(EditAnywhere, Category = "Fix Report")
	TObjectPtr<USoundBase> ClickSound;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	/** In the designer and widget renders (no game running): a sample report */
	virtual void NativePreConstruct() override;

private:
	void BuildLayout();
	void Fill(UHint* Mistake, const FText& Message, const TArray<UObject*>& IconSets);
	/** A line of text and icons into Content, wrapping at the panel's width */
	UWidget* MakeLine(const FClueHelp& Help, const TArray<UObject*>& IconSets, int32 FontSize, const FLinearColor& Color);
	/** Clue drawn as on the board (WBP_HorizontalClue / WBP_VerticalClue), scaled to fit; null if it can't be */
	UWidget* MakeClue(UClue* Clue, UPuzzle* Puzzle, const TArray<UObject*>& IconSets);

	UFUNCTION()
	void HandleOk();

	/** The lines about the mistake, between the title and the message */
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> Content;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MessageText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> OkButton;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> PanelTexture;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> ButtonTexture;

	UPROPERTY(Transient)
	TObjectPtr<UFont> ReportFont;

	TFunction<void()> OnClosed;
};
