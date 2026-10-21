#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "PuzzleFailedWidget.generated.h"

class UButton;
class USoundBase;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPuzzleFailedAction);

/**
 * Shown instead of an end screen when the board is completed with mistakes (PuzzleFailed in WBP_Happiness). Minimal
 * and see-through so the icons marked wrong stay visible: a small translucent panel over the bottom clue strip with
 * a line of text and Undo / Restart. The rest is clear but takes the clicks, so the board can't be played meanwhile.
 * Builds its own widgets (no Blueprint layout); either button hides it and fires its event.
 */
UCLASS()
class HAPPINESS_API UPuzzleFailedWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPuzzleFailedWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Puzzle Failed")
	void Show();

	UFUNCTION(BlueprintCallable, Category = "Puzzle Failed")
	void Hide();

	/** Undo the last move and play on (WBP_Happiness: Undo, then RefreshPuzzle) */
	UPROPERTY(BlueprintAssignable, Category = "Puzzle Failed")
	FOnPuzzleFailedAction OnUndo;

	/** Start the puzzle over (WBP_Happiness: RestartPuzzle) */
	UPROPERTY(BlueprintAssignable, Category = "Puzzle Failed")
	FOnPuzzleFailedAction OnRestart;

	/** Where the panel sits: the bottom clue strip (WBP_Happiness's VerticalCluePanel area) */
	UPROPERTY(EditAnywhere, Category = "Puzzle Failed")
	FMargin PanelMargin = FMargin(10.f, 0.f, 205.f, 30.f);

	UPROPERTY(EditAnywhere, Category = "Puzzle Failed")
	FLinearColor PanelColor = FLinearColor(0.f, 0.f, 0.f, 0.55f);

	UPROPERTY(EditAnywhere, Category = "Puzzle Failed")
	FSlateFontInfo MessageFont;

	UPROPERTY(EditAnywhere, Category = "Puzzle Failed")
	FSlateFontInfo ButtonFont;

	UPROPERTY(EditAnywhere, Category = "Puzzle Failed")
	FButtonStyle ButtonStyle;

	UPROPERTY(EditAnywhere, Category = "Puzzle Failed")
	FVector2D ButtonSize = FVector2D(200.f, 64.f);

	UPROPERTY(EditAnywhere, Category = "Puzzle Failed")
	TObjectPtr<USoundBase> ClickSound;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildLayout();
	UButton* MakeButton(const FText& Label);

	UFUNCTION()
	void HandleUndo();

	UFUNCTION()
	void HandleRestart();

	UPROPERTY(Transient)
	TObjectPtr<UButton> UndoButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RestartButton;
};
