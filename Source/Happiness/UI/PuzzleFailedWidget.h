#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "PuzzleFailedWidget.generated.h"

class UButton;
class USoundBase;
class UTexture2D;
class UFont;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPuzzleFailedAction);

/**
 * Shown instead of an end screen when the board is completed with mistakes (PuzzleFailed in WBP_Happiness).
 * A slim banner sits over the top edge of the grid so the marked icons stay visible. The clear root blocks board
 * input while the player chooses Undo or Restart.
 * Also shown when a move leaves the board impossible (UPuzzle::IsImpossible, ShowImpossible): "Impossible Puzzle
 * State", with Fix (WBP_Happiness's Fix Puzzle, as confirmed from the pause menu) in Undo's place.
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

	/** The board can't be solved from here (UPuzzle::RecordPlayerMove calls it): Fix or Restart */
	UFUNCTION(BlueprintCallable, Category = "Puzzle Failed")
	void ShowImpossible();

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
	UButton* MakeButton(const FText& Label, bool bRestart, UTextBlock** OutCaption = nullptr);
	/** The texts for mistakes (Undo) or an impossible board (Fix) */
	void SetImpossible(bool bInImpossible);
	/** WBP_Happiness's Fix Puzzle: its ConfirmConfirm with the confirm type FixPuzzle */
	void RunFix();

	UFUNCTION()
	void HandleUndo();

	UFUNCTION()
	void HandleRestart();

	UPROPERTY(Transient)
	TObjectPtr<UButton> UndoButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RestartButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MessageText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> UndoCaption;

	bool bImpossible = false;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> FrameTexture;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> ButtonTexture;

	UPROPERTY(Transient)
	TObjectPtr<UFont> GardenFont;
};
