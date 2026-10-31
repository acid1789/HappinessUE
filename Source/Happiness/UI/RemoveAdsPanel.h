#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RemoveAdsPanel.generated.h"

class UButton;
class UTextBlock;
class UFont;
class UTexture2D;

/**
 * Parent of WBP_RemoveAds, the Options screen's "Remove Ads" row: a buy button (with the store's price) and a
 * "Restore Purchases" button, kept up to date from UAdRemovalSubsystem.
 */
UCLASS()
class HAPPINESS_API URemoveAdsPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	URemoveAdsPanel(const FObjectInitializer& ObjectInitializer);
	/** The main-menu purchase window stacks these controls; Options keeps the compact row. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Remove Ads")
	bool bDialogLayout = false;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BuyButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BuyText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RestoreButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RestoreText;

private:
	void StyleControls();
	UPROPERTY() TObjectPtr<UFont> GardenFont;
	UPROPERTY() TObjectPtr<UTexture2D> GardenButtons;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PurchaseStatus;
	bool bDialogBuilt = false;

	UFUNCTION()
	void HandleBuyClicked();

	UFUNCTION()
	void HandleRestoreClicked();

	UFUNCTION()
	void Refresh();

	/** A restore ran and found nothing: say so until the next one */
	bool bRestoreFoundNothing = false;
	bool bWasRestoring = false;
};
