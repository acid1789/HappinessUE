#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FreePlayConfigWidget.generated.h"

class UButton;
class UCanvasPanel;
class UFont;
class UTextBlock;
class UTexture2D;
class UFreePlayConfigWidget;

/** One of the setup's size/difficulty segments; the original Blueprint selector owns the choice. */
UCLASS()
class UFreePlayOptionToggle : public UObject
{
    GENERATED_BODY()
public:
    TWeakObjectPtr<UFreePlayConfigWidget> Owner;
    bool bDifficulty = false;
    int32 Index = 0;
    UFUNCTION() void HandleClicked();
};

/** Garden layout for WBP_HappinessConfig; its Blueprint still owns Show, Start, Cancel and unlock checks. */
UCLASS()
class HAPPINESS_API UFreePlayConfigWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFreePlayConfigWidget(const FObjectInitializer& ObjectInitializer);

	/** Random puzzle numbers are from 1 to this */
	UPROPERTY(EditAnywhere, Category = "Random")
	int32 MaxRandomPuzzleNumber = 99999;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	friend class UFreePlayOptionToggle;
	void BuildGardenLayout();
	void FitToMenuCanvas();
	void RefreshSegments();
	void HandleOptionSelected(bool bDifficulty, int32 Index);

	UFUNCTION()
	void HandleRandomClicked();

	/** Pick option Index of a WBP_FreePlaySelection (its Select function, which updates the text and the locks) */
	static void SelectOption(UUserWidget* Selection, int32 Index);
	/** How many options a WBP_FreePlaySelection has (its OptionStrings) */
	static int32 GetOptionCount(const UUserWidget* Selection);
	/** Calls a PC_Happiness check (CanPlayHappinessSize, CanPlayHappinessDifficulty) with one int; true without one */
	bool CanPlay(FName Check, int32 Value) const;

	UPROPERTY(Transient) TObjectPtr<UButton> RandomButton;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> SizeSelection;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> DifficultySelection;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> SegmentButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> SegmentLabels;
	UPROPERTY(Transient) TArray<TObjectPtr<UFreePlayOptionToggle>> SegmentHandlers;
	UPROPERTY() TObjectPtr<UTexture2D> GardenTexture;
	UPROPERTY() TObjectPtr<UTexture2D> ActionTexture;
	UPROPERTY() TObjectPtr<UTexture2D> FieldTexture;
	UPROPERTY() TObjectPtr<UTexture2D> SelectedTexture;
	UPROPERTY() TObjectPtr<UFont> GardenFont;
	int32 LastSizeSelection = INDEX_NONE;
	int32 LastDifficultySelection = INDEX_NONE;
};
