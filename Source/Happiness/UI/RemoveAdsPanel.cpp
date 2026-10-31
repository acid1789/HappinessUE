#include "UI/RemoveAdsPanel.h"

#include "Ads/AdRemoval.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "Components/ButtonSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "RemoveAds"

URemoveAdsPanel::URemoveAdsPanel(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Buttons(TEXT("/Game/General/Textures/T_LessonNotebookButtons.T_LessonNotebookButtons"));
	GardenFont = Font.Object;
	GardenButtons = Buttons.Object;
}

void URemoveAdsPanel::NativePreConstruct()
{
	Super::NativePreConstruct();
	StyleControls();
}

void URemoveAdsPanel::StyleControls()
{
	if (!BuyButton || !RestoreButton || !BuyText || !RestoreText) return;
	if (bDialogLayout && !bDialogBuilt)
	{
		if (UHorizontalBox* Row = Cast<UHorizontalBox>(GetWidgetFromName(TEXT("Row"))))
		{
			BuyButton->RemoveFromParent();
			RestoreButton->RemoveFromParent();
			Row->ClearChildren();
			USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
			Size->SetWidthOverride(560.f);
			Size->SetHeightOverride(190.f);
			UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
			Size->AddChild(Canvas);
			Row->AddChildToHorizontalBox(Size);
			auto Place = [Canvas](UWidget* Widget, FVector2D Position, FVector2D Extent)
			{
				UCanvasPanelSlot* ControlSlot = Canvas->AddChildToCanvas(Widget);
				ControlSlot->SetPosition(Position);
				ControlSlot->SetSize(Extent);
			};
			Place(BuyButton, FVector2D::ZeroVector, FVector2D(560.f, 84.f));
			Place(RestoreButton, FVector2D(85.f, 95.f), FVector2D(390.f, 60.f));
			PurchaseStatus = WidgetTree->ConstructWidget<UTextBlock>();
			PurchaseStatus->SetFont(FSlateFontInfo(GardenFont, 19));
			PurchaseStatus->SetColorAndOpacity(FLinearColor(.17f, .085f, .035f));
			PurchaseStatus->SetJustification(ETextJustify::Center);
			PurchaseStatus->SetVisibility(ESlateVisibility::HitTestInvisible);
			PurchaseStatus->SetText(LOCTEXT("OneTime", "One-time purchase"));
			Place(PurchaseStatus, FVector2D(0.f, 160.f), FVector2D(560.f, 30.f));
			bDialogBuilt = true;
		}
	}
	auto Style = [&](UButton* Button, UTextBlock* Text, bool bPrimary)
	{
		FSlateBrush Face;
		Face.SetResourceObject(GardenButtons);
		Face.DrawAs = ESlateBrushDrawType::Image;
		Face.SetUVRegion(FBox2f(FVector2f(.03f, bPrimary ? .54f : .06f), FVector2f(.98f, bPrimary ? .963f : .485f)));
		FButtonStyle ButtonStyle;
		ButtonStyle.Normal = ButtonStyle.Hovered = ButtonStyle.Pressed = ButtonStyle.Disabled = Face;
		ButtonStyle.Hovered.TintColor = FLinearColor(1.08f, 1.08f, 1.04f);
		ButtonStyle.Pressed.TintColor = FLinearColor(.85f, .9f, .8f);
		ButtonStyle.Disabled.TintColor = FLinearColor(.8f, .8f, .74f);
		ButtonStyle.NormalPadding = ButtonStyle.PressedPadding = FMargin(bDialogLayout ? 44.f : 22.f, 6.f);
		Button->SetStyle(ButtonStyle);
		Text->SetFont(FSlateFontInfo(GardenFont, bDialogLayout ? (bPrimary ? 34 : 26) : 22));
		Text->SetColorAndOpacity(bPrimary ? FLinearColor(1.f, .95f, .8f) : FLinearColor(.17f, .085f, .035f));
		Text->SetJustification(ETextJustify::Center);
		Text->SetShadowOffset(FVector2D::ZeroVector);
		Text->SetShadowColorAndOpacity(FLinearColor::Transparent);
		if (!Cast<UScaleBox>(Button->GetChildAt(0)))
		{
			Text->RemoveFromParent();
			UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>();
			Fit->SetStretch(EStretch::ScaleToFit);
			Fit->SetStretchDirection(EStretchDirection::DownOnly);
			Fit->AddChild(Text);
			UButtonSlot* ContentSlot = CastChecked<UButtonSlot>(Button->AddChild(Fit));
			ContentSlot->SetHorizontalAlignment(HAlign_Fill);
			ContentSlot->SetVerticalAlignment(VAlign_Fill);
		}
	};
	Style(BuyButton, BuyText, true);
	Style(RestoreButton, RestoreText, false);
}

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
			Buy = bDialogLayout ? LOCTEXT("Buy", "Remove Ads") : LOCTEXT("Offline", "Remove Ads (store unavailable)");
			break;
	}
	BuyText->SetText(Buy);
	if (PurchaseStatus)
	{
		PurchaseStatus->SetText(State == ERemoveAdsState::Unavailable ? LOCTEXT("StoreUnavailable", "Store unavailable - please try again later") : LOCTEXT("OneTime", "One-time purchase"));
	}
	BuyButton->SetIsEnabled(State == ERemoveAdsState::Available);

	RestoreText->SetText(bRestoring ? LOCTEXT("Restoring", "Checking...")
		: bRestoreFoundNothing ? LOCTEXT("NothingFound", "No purchase found") : LOCTEXT("Restore", "Restore Purchases"));
	RestoreButton->SetIsEnabled(!bRestoring && State != ERemoveAdsState::Purchasing);
	RestoreButton->SetVisibility(State == ERemoveAdsState::Purchased ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

#undef LOCTEXT_NAMESPACE
