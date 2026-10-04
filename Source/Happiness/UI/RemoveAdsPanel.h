#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RemoveAdsPanel.generated.h"

class UButton;
class UTextBlock;

/**
 * Parent of WBP_RemoveAds, the Options screen's "Remove Ads" row: a buy button (with the store's price) and a
 * "Restore Purchases" button, kept up to date from UAdRemovalSubsystem.
 */
UCLASS()
class HAPPINESS_API URemoveAdsPanel : public UUserWidget
{
	GENERATED_BODY()

protected:
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
