#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IconSetsPanel.generated.h"

class UButton;
class UCanvasPanel;
class UFont;
class UHorizontalBox;
class UScrollBox;
class UTextBlock;
class UTexture2D;
class USoundBase;
class UIconSetsPanel;
struct FIconSetEntry;

/** Routes one card's click to the panel (UButton::OnClicked has no parameters) */
UCLASS()
class UIconSetCardClick : public UObject
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<UIconSetsPanel> Panel;
	int32 Index = INDEX_NONE;

	UFUNCTION()
	void HandleClicked();
};

/**
 * The icon sets screen, opened from the player info's Icon Sets button (over the whole menu). A left/right scrolling
 * row of cards, one per icon set the player knows of (UIconSetSubsystem::GetEntries: hidden locked sets are left
 * out): its name, its icons, and whether it is on. Tapping an unlocked set turns it on or off, down to
 * UIconSetSubsystem::MinEnabledSets; a locked one shows what unlocks it. Built in code, in the pause menu's garden style.
 */
UCLASS()
class HAPPINESS_API UIconSetsPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	UIconSetsPanel(const FObjectInitializer& ObjectInitializer);

	/** Reads the sets again and shows the panel */
	void Show();

	void Hide();

	/** Called when a card is tapped */
	void ToggleSet(int32 Index);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildLayout();
	/** Remake the cards and the count line from the sets */
	void Refresh();
	void UpdateStatus();
	UWidget* MakeCard(int32 Index, const FIconSetEntry& Entry);
	void ScrollBy(int32 Cards);
	void PlaySound(USoundBase* Sound) const;

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleLeftClicked();

	UFUNCTION()
	void HandleRightClicked();

	UPROPERTY() TObjectPtr<UTexture2D> PanelTexture;
	UPROPERTY() TObjectPtr<UTexture2D> ButtonTexture;
	UPROPERTY() TObjectPtr<UFont> PanelFont;
	UPROPERTY() TObjectPtr<USoundBase> AcceptSound;
	UPROPERTY() TObjectPtr<USoundBase> CancelSound;

	UPROPERTY(Transient) TObjectPtr<UScrollBox> Scroll;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> CardRow;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UButton> LeftButton;
	UPROPERTY(Transient) TObjectPtr<UButton> RightButton;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> Cards;
	UPROPERTY(Transient) TArray<TObjectPtr<UIconSetCardClick>> CardClicks;

	/** The sets on the cards, in order (UIconSetSubsystem::GetEntries) */
	UPROPERTY(Transient) TArray<TObjectPtr<UObject>> CardSets;

	/** While above zero, the count line shows why a set couldn't be turned off (or what unlocks one) */
	float MessageSeconds = 0.f;
};
