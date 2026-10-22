#pragma once

#include "CoreMinimal.h"
#include "Ads/StreakSaverStore.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "StreakSaverPanel.generated.h"

class UButton;
class USoundBase;
class UTextBlock;

/**
 * The streak saver shop, opened from the mode screen's Streak Saver button (UHappinessModeWidget makes both): a
 * dimmed screen with a window holding what the savers would do for the streak now (bring a broken one back, or
 * protect it), the three tiers with their store prices, and Close. Builds its own widgets (no Blueprint layout) and
 * stays current with UDailySubsystem and UStreakSaverStoreSubsystem.
 */
UCLASS()
class HAPPINESS_API UStreakSaverPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	UStreakSaverPanel(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Streak Saver")
	void Show();

	UFUNCTION(BlueprintCallable, Category = "Streak Saver")
	void Hide();

	UPROPERTY(EditAnywhere, Category = "Streak Saver")
	FLinearColor DimColor = FLinearColor(0.f, 0.f, 0.f, 0.6f);

	UPROPERTY(EditAnywhere, Category = "Streak Saver")
	FLinearColor WindowColor = FLinearColor(0.99f, 0.96f, 0.89f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Streak Saver")
	FLinearColor InkColor = FLinearColor(0.32f, 0.22f, 0.13f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Streak Saver")
	FSlateFontInfo TitleFont;

	UPROPERTY(EditAnywhere, Category = "Streak Saver")
	FSlateFontInfo BodyFont;

	UPROPERTY(EditAnywhere, Category = "Streak Saver")
	FSlateFontInfo TierFont;

	UPROPERTY(EditAnywhere, Category = "Streak Saver")
	FButtonStyle ButtonStyle;

	UPROPERTY(EditAnywhere, Category = "Streak Saver")
	FVector2D TierSize = FVector2D(230.f, 130.f);

	UPROPERTY(EditAnywhere, Category = "Streak Saver")
	TObjectPtr<USoundBase> ClickSound;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildLayout();
	UTextBlock* MakeText(const FText& Text, const FSlateFontInfo& Font);

	UFUNCTION()
	void Refresh();

	UFUNCTION()
	void HandleOneDay();

	UFUNCTION()
	void HandleSevenDays();

	UFUNCTION()
	void HandleThirtyDays();

	UFUNCTION()
	void HandleClose();

	void BuyTier(EStreakSaverTier Tier);

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StoreText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> TierButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> PriceTexts;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CloseButton;
};
