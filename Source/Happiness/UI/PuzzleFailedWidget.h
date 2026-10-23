#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "PuzzleFailedWidget.generated.h"

class UButton;
class USoundBase;
class UTexture2D;
class UFont;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPuzzleFailedAction);

/**
 * Shown instead of an end screen when the board is completed with mistakes (PuzzleFailed in WBP_Happiness).
 * A slim banner sits over the top edge of the grid so the marked icons stay visible. The clear root blocks board
 * input while the player chooses Undo or Restart.
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

	UPROPERTY(EditAnywhere, Category = "Puzzle Failed")
	TObjectPtr<USoundBase> ClickSound;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildLayout();
	UButton* MakeButton(const FText& Label, bool bRestart);

	UFUNCTION()
	void HandleUndo();

	UFUNCTION()
	void HandleRestart();

	UPROPERTY(Transient)
	TObjectPtr<UButton> UndoButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RestartButton;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> FrameTexture;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> ButtonTexture;

	UPROPERTY(Transient)
	TObjectPtr<UFont> GardenFont;
};
