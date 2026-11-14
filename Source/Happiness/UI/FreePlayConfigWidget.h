#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FreePlayConfigWidget.generated.h"

class UButton;

/**
 * The Free Play setup screen (WBP_HappinessConfig, whose Blueprint does the rest: Show, Start, Cancel, the size and
 * difficulty locks). Adds a Random button at the top right, made in code in the dialog's canvas and styled like its
 * Cancel button: it picks a random puzzle number, a size and a difficulty the player can play (PC_Happiness's
 * CanPlayHappinessSize / CanPlayHappinessDifficulty), and a random set of clue types (UFreePlaySubsystem, so the
 * clue selection shows it and it is saved).
 */
UCLASS()
class HAPPINESS_API UFreePlayConfigWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Random puzzle numbers are from 1 to this */
	UPROPERTY(EditAnywhere, Category = "Random")
	int32 MaxRandomPuzzleNumber = 99999;

protected:
	virtual void NativeConstruct() override;

private:
	void MakeRandomButton();

	UFUNCTION()
	void HandleRandomClicked();

	/** Pick option Index of a WBP_FreePlaySelection (its Select function, which updates the text and the locks) */
	static void SelectOption(UUserWidget* Selection, int32 Index);
	/** How many options a WBP_FreePlaySelection has (its OptionStrings) */
	static int32 GetOptionCount(const UUserWidget* Selection);
	/** Calls a PC_Happiness check (CanPlayHappinessSize, CanPlayHappinessDifficulty) with one int; true without one */
	bool CanPlay(FName Check, int32 Value) const;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RandomButton;
};
