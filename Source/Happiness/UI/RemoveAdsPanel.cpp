#include "UI/RemoveAdsPanel.h"

#include "Ads/AdRemoval.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "RemoveAds"

void URemoveAdsPanel::NativeConstruct()
{
	Super::NativeConstruct();

	BuyButton->OnClicked.AddUniqueDynamic(this, &URemoveAdsPanel::HandleBuyClicked);
	RestoreButton->OnClicked.AddUniqueDynamic(this, &URemoveAdsPanel::HandleRestoreClicked);
	if (UAdRemovalSubsystem* AdRemoval = UAdRemovalSubsystem::Get(this))
	{
		AdRemoval->OnChanged.AddUniqueDynamic(this, &URemoveAdsPanel::Refresh);
	}
	bRestoreFoundNothing = false;
	Refresh();
}

void URemoveAdsPanel::NativeDestruct()
{
	if (UAdRemovalSubsystem* AdRemoval = UAdRemovalSubsystem::Get(this))
	{
		AdRemoval->OnChanged.RemoveDynamic(this, &URemoveAdsPanel::Refresh);
	}
	Super::NativeDestruct();
}

void URemoveAdsPanel::HandleBuyClicked()
{
	if (UAdRemovalSubsystem* AdRemoval = UAdRemovalSubsystem::Get(this))
	{
		AdRemoval->BuyRemoveAds();
	}
}

void URemoveAdsPanel::HandleRestoreClicked()
{
	if (UAdRemovalSubsystem* AdRemoval = UAdRemovalSubsystem::Get(this))
	{
		bRestoreFoundNothing = false;
		AdRemoval->RestorePurchases();
	}
}

void URemoveAdsPanel::Refresh()
{
	const UAdRemovalSubsystem* AdRemoval = UAdRemovalSubsystem::Get(this);
	const ERemoveAdsState State = AdRemoval ? AdRemoval->GetState() : ERemoveAdsState::Unavailable;
	const bool bRestoring = AdRemoval && AdRemoval->IsRestoring();

	// A restore that ended without the purchase found nothing
	if (bWasRestoring && !bRestoring && State != ERemoveAdsState::Purchased)
	{
		bRestoreFoundNothing = true;
	}
	bWasRestoring = bRestoring;

	FText Buy;
	switch (State)
	{
		case ERemoveAdsState::Purchased:
			Buy = LOCTEXT("Purchased", "Ads removed - thank you!");
			break;
		case ERemoveAdsState::Purchasing:
			Buy = LOCTEXT("Purchasing", "Purchasing...");
			break;
		case ERemoveAdsState::Pending:
			Buy = LOCTEXT("Pending", "Purchase pending");
			break;
		case ERemoveAdsState::Available:
			Buy = AdRemoval->GetPriceText().IsEmpty() ? LOCTEXT("Buy", "Remove Ads")
				: FText::Format(LOCTEXT("BuyPrice", "Remove Ads  {0}"), AdRemoval->GetPriceText());
			break;
		default:
			Buy = LOCTEXT("Offline", "Remove Ads (store unavailable)");
			break;
	}
	BuyText->SetText(Buy);
	BuyButton->SetIsEnabled(State == ERemoveAdsState::Available);

	RestoreText->SetText(bRestoring ? LOCTEXT("Restoring", "Checking...")
		: bRestoreFoundNothing ? LOCTEXT("NothingFound", "No purchase found") : LOCTEXT("Restore", "Restore Purchases"));
	RestoreButton->SetIsEnabled(!bRestoring && State != ERemoveAdsState::Purchasing);
	RestoreButton->SetVisibility(State == ERemoveAdsState::Purchased ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

#undef LOCTEXT_NAMESPACE
