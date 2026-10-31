#include "UI/StreakSaverPanel.h"

#include "HappinessClassic/DailyPuzzle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BackgroundBlur.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ButtonSlot.h"
#include "Components/ScaleBox.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Styling/SlateBrush.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "StreakSaver"

namespace
{
	const EStreakSaverTier PanelTiers[] = { EStreakSaverTier::OneDay, EStreakSaverTier::SevenDays, EStreakSaverTier::ThirtyDays };


}

UStreakSaverPanel::UStreakSaverPanel(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Accept(TEXT("/Game/General/Audio/Cues/MenuAccept.MenuAccept"));
	TitleFont = FSlateFontInfo(Font.Object, 44);
	BodyFont = FSlateFontInfo(Font.Object, 22);
	TierFont = FSlateFontInfo(Font.Object, 38);
	ClickSound = Accept.Object;

	static ConstructorHelpers::FObjectFinder<UTexture2D> Art(TEXT("/Game/General/Textures/T_StreakSaverShop.T_StreakSaverShop"));
	ShopTexture = Art.Object;
	ButtonStyle.Normal = FSlateRoundedBoxBrush(FLinearColor::Transparent, 18.f);
	ButtonStyle.Hovered = FSlateRoundedBoxBrush(FLinearColor(1.f, 1.f, .9f, .08f), 18.f);
	ButtonStyle.Pressed = FSlateRoundedBoxBrush(FLinearColor(.12f, .18f, .06f, .15f), 18.f);
	ButtonStyle.Disabled = FSlateRoundedBoxBrush(FLinearColor(.12f, .10f, .06f, .14f), 18.f);
	ButtonStyle.NormalPadding = ButtonStyle.PressedPadding = FMargin(0.f);

	SetVisibility(ESlateVisibility::Collapsed);
}

TSharedRef<SWidget> UStreakSaverPanel::RebuildWidget()
{
	// No Blueprint layout: the widgets are made here, also for the designer preview
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

UTextBlock* UStreakSaverPanel::MakeText(const FText& Text, const FSlateFontInfo& Font)
{
	UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>();
	Block->SetText(Text);
	Block->SetFont(Font);
	Block->SetColorAndOpacity(FSlateColor(InkColor));
	Block->SetJustification(ETextJustify::Center);
	Block->SetVisibility(ESlateVisibility::HitTestInvisible);
	return Block;
}

void UStreakSaverPanel::BuildLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Root;
	auto Fill = [Root](UWidget* Widget, int32 Layer)
	{
		UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Widget);
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		PanelSlot->SetOffsets(FMargin(0.f));
		PanelSlot->SetZOrder(Layer);
		return PanelSlot;
	};
	UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>();
	Blur->SetBlurStrength(7.f);
	Blur->SetVisibility(ESlateVisibility::HitTestInvisible);
	Fill(Blur, 0);
	UBorder* Dimmer = WidgetTree->ConstructWidget<UBorder>();
	Dimmer->SetBrush(FSlateRoundedBoxBrush(FLinearColor::White, 0.f));
	Dimmer->SetBrushColor(DimColor);
	Fill(Dimmer, 1);
	UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>();
	Fit->SetStretch(EStretch::ScaleToFit);
	Fit->SetStretchDirection(EStretchDirection::DownOnly);
	Fit->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Fill(Fit, 2)->SetOffsets(FMargin(36.f));
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(1400.f);
	Size->SetHeightOverride(600.f);
	Fit->AddChild(Size);
	UCanvasPanel* Window = WidgetTree->ConstructWidget<UCanvasPanel>();
	Size->AddChild(Window);
	auto Place = [](UCanvasPanel* Canvas, UWidget* Widget, FVector2D Position, FVector2D Extent, int32 Layer)
	{
		UCanvasPanelSlot* ContentSlot = Canvas->AddChildToCanvas(Widget);
		ContentSlot->SetPosition(Position);
		ContentSlot->SetSize(Extent);
		ContentSlot->SetZOrder(Layer);
	};
	auto TextFit = [&](UTextBlock* Text, FVector2D Position, FVector2D Extent)
	{
		UScaleBox* TextScale = WidgetTree->ConstructWidget<UScaleBox>();
		TextScale->SetStretch(EStretch::ScaleToFit);
		TextScale->SetStretchDirection(EStretchDirection::DownOnly);
		TextScale->SetVisibility(ESlateVisibility::HitTestInvisible);
		TextScale->AddChild(Text);
		Place(Window, TextScale, Position, Extent, 2);
	};
	UImage* Surface = WidgetTree->ConstructWidget<UImage>();
	Surface->SetBrushFromTexture(ShopTexture);
	Place(Window, Surface, FVector2D::ZeroVector, FVector2D(1400.f, 600.f), 0);
	TextFit(MakeText(LOCTEXT("Title", "Streak Saver"), TitleFont), FVector2D(606.f, 30.f), FVector2D(420.f, 78.f));
	FSlateFontInfo CountFont = BodyFont;
	CountFont.Size = 27;
	StreakCountText = MakeText(FText::GetEmpty(), CountFont);
	SaverCountText = MakeText(FText::GetEmpty(), CountFont);
	TextFit(StreakCountText, FVector2D(230.f, 147.f), FVector2D(402.f, 48.f));
	TextFit(SaverCountText, FVector2D(815.f, 147.f), FVector2D(375.f, 48.f));
	StatusText = MakeText(FText::GetEmpty(), BodyFont);
	StatusText->SetWrapTextAt(990.f);
	TextFit(StatusText, FVector2D(205.f, 215.f), FVector2D(990.f, 65.f));

	TierButtons.Reset();
	PriceTexts.Reset();
	constexpr float CardLefts[] = {28.f, 482.f, 926.f};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(PanelTiers); ++Index)
	{
		const int32 Days = UStreakSaverStoreSubsystem::GetTierDays(PanelTiers[Index]);
		UButton* Button = WidgetTree->ConstructWidget<UButton>();
		Button->SetStyle(ButtonStyle);
		Button->SetCursor(EMouseCursor::Hand);
		Place(Window, Button, FVector2D(CardLefts[Index], 282.f), FVector2D(435.f, 228.f), 3);
		UCanvasPanel* Content = WidgetTree->ConstructWidget<UCanvasPanel>();
		UButtonSlot* TierContentSlot = CastChecked<UButtonSlot>(Button->AddChild(Content));
		TierContentSlot->SetHorizontalAlignment(HAlign_Fill);
		TierContentSlot->SetVerticalAlignment(VAlign_Fill);
		auto TierLabel = [&](UTextBlock* Text, float Y, float Height)
		{
			UScaleBox* LabelFit = WidgetTree->ConstructWidget<UScaleBox>();
			LabelFit->SetStretch(EStretch::ScaleToFit);
			LabelFit->SetStretchDirection(EStretchDirection::DownOnly);
			LabelFit->SetVisibility(ESlateVisibility::HitTestInvisible);
			LabelFit->AddChild(Text);
			Place(Content, LabelFit, FVector2D(227.f, Y), FVector2D(190.f, Height), 0);
		};
		UTextBlock* DaysText = MakeText(FText::Format(LOCTEXT("TierDays", "{0} {0}|plural(one=Day,other=Days)"), Days), TierFont);
		TierLabel(DaysText, 32.f, 76.f);
		FSlateFontInfo PriceFont = BodyFont;
		PriceFont.Size = 29;
		UTextBlock* Price = MakeText(LOCTEXT("Buy", "Buy"), PriceFont);
		Price->SetColorAndOpacity(FLinearColor(1.f, .95f, .8f));
		TierLabel(Price, 139.f, 63.f);
		TierButtons.Add(Button);
		PriceTexts.Add(Price);
	}
	TierButtons[0]->OnClicked.AddUniqueDynamic(this, &UStreakSaverPanel::HandleOneDay);
	TierButtons[1]->OnClicked.AddUniqueDynamic(this, &UStreakSaverPanel::HandleSevenDays);
	TierButtons[2]->OnClicked.AddUniqueDynamic(this, &UStreakSaverPanel::HandleThirtyDays);
	FSlateFontInfo RuleFont = BodyFont;
	RuleFont.Size = 20;
	UTextBlock* Rule = MakeText(LOCTEXT("Rule", "Each saver day covers one missed day. Missed days don't increase your streak."), RuleFont);
	Rule->SetWrapTextAt(1030.f);
	TextFit(Rule, FVector2D(185.f, 513.f), FVector2D(1030.f, 43.f));
	FSlateFontInfo StoreFont = BodyFont;
	StoreFont.Size = 18;
	StoreText = MakeText(FText::GetEmpty(), StoreFont);
	TextFit(StoreText, FVector2D(210.f, 558.f), FVector2D(980.f, 27.f));
	CloseButton = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle CloseStyle;
	CloseStyle.Normal = CloseStyle.Disabled = FSlateRoundedBoxBrush(FLinearColor::Transparent, 40.f);
	CloseStyle.Hovered = FSlateRoundedBoxBrush(FLinearColor(1.f, 1.f, .9f, .1f), 40.f);
	CloseStyle.Pressed = FSlateRoundedBoxBrush(FLinearColor(.12f, .18f, .06f, .15f), 40.f);
	CloseStyle.NormalPadding = CloseStyle.PressedPadding = FMargin(0.f);
	CloseButton->SetStyle(CloseStyle);
	CloseButton->SetCursor(EMouseCursor::Hand);
	CloseButton->OnClicked.AddUniqueDynamic(this, &UStreakSaverPanel::HandleClose);
	Place(Window, CloseButton, FVector2D(1277.f, 68.f), FVector2D(84.f), 4);
	Refresh();
}

void UStreakSaverPanel::NativeConstruct()
{
	Super::NativeConstruct();
	if (UDailySubsystem* Daily = UDailySubsystem::Get(this))
	{
		Daily->OnChanged.AddUniqueDynamic(this, &UStreakSaverPanel::Refresh);
	}
	if (UStreakSaverStoreSubsystem* Store = UStreakSaverStoreSubsystem::Get(this))
	{
		Store->OnChanged.AddUniqueDynamic(this, &UStreakSaverPanel::Refresh);
	}
	Refresh();
}

void UStreakSaverPanel::NativeDestruct()
{
	if (UDailySubsystem* Daily = UDailySubsystem::Get(this))
	{
		Daily->OnChanged.RemoveDynamic(this, &UStreakSaverPanel::Refresh);
	}
	if (UStreakSaverStoreSubsystem* Store = UStreakSaverStoreSubsystem::Get(this))
	{
		Store->OnChanged.RemoveDynamic(this, &UStreakSaverPanel::Refresh);
	}
	Super::NativeDestruct();
}

void UStreakSaverPanel::Show()
{
	Refresh();
	SetVisibility(ESlateVisibility::Visible);
}

void UStreakSaverPanel::Hide()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UStreakSaverPanel::Refresh()
{
	if (!StatusText)
	{
		return;
	}
#if WITH_EDITOR
	if (PreviewStreakSaverState >= 0) { ApplyPreview(); return; }
#endif
	const UDailySubsystem* Daily = UDailySubsystem::Get(this);
	const UStreakSaverStoreSubsystem* Store = UStreakSaverStoreSubsystem::Get(this);
	const int32 Streak = Daily ? Daily->GetStreak() : 0;
	const int32 SaverDays = Daily ? Daily->GetSaverDays() : 0;
	SaverCountText->SetText(FText::Format(LOCTEXT("SaverCount", "{0} saver {0}|plural(one=day,other=days) available"), SaverDays));
	int32 Broken = 0, Missed = 0, Needed = 0;
	const bool bBroken = Daily && Daily->GetBrokenStreak(Broken, Missed, Needed);
	StreakCountText->SetText(bBroken ? FText::Format(LOCTEXT("StreakToRestore", "{0}-day streak to restore"), Broken)
		: Streak > 0 ? FText::Format(LOCTEXT("StreakCount", "{0}-day streak"), Streak) : LOCTEXT("NoCurrentStreak", "No current streak"));

	FText Status;
	if (bBroken)
	{
		Status = FText::Format(LOCTEXT("Broken",
			"Your {0}-day streak ended: you missed {1} {1}|plural(one=day,other=days). {2} more saver {2}|plural(one=day brings,other=days bring) it back."),
			Broken, Missed, Needed);
	}
	else if (Streak > 0 && SaverDays > 0)
	{
		Status = FText::Format(LOCTEXT("Protected", "Your {0}-day streak is protected for {1} missed {1}|plural(one=day,other=days)."), Streak, SaverDays);
	}
	else if (Streak > 0)
	{
		Status = FText::Format(LOCTEXT("Unprotected", "Keep your {0}-day streak safe on the days you can't play."), Streak);
	}
	else if (SaverDays > 0)
	{
		Status = FText::Format(LOCTEXT("Banked", "You have {0} saver {0}|plural(one=day,other=days) ready for your next streak."), SaverDays);
	}
	else
	{
		Status = LOCTEXT("NoStreak", "Keep your daily streak safe on the days you can't play.");
	}
	StatusText->SetText(Status);

	const bool bCanBuy = Store && Store->CanBuy();
	for (int32 Index = 0; Index < TierButtons.Num(); Index++)
	{
		const FText Price = Store ? Store->GetPriceText(PanelTiers[Index]) : FText::GetEmpty();
		PriceTexts[Index]->SetText(Price.IsEmpty() ? LOCTEXT("Buy", "Buy") : FText::Format(LOCTEXT("BuyPrice", "Buy  {0}"), Price));
		TierButtons[Index]->SetIsEnabled(bCanBuy);
	}

	FText StoreStatus;
	if (!Store || (!bCanBuy && !Store->IsPurchasing()))
	{
		StoreStatus = LOCTEXT("StoreUnavailable", "The store can't be reached right now.");
	}
	if (Store && Store->IsPurchasing())
	{
		StoreStatus = LOCTEXT("Purchasing", "Waiting for the store...");
	}
	else if (Store && Store->IsPending())
	{
		StoreStatus = LOCTEXT("Pending", "A purchase is waiting for its payment to clear.");
	}
	StoreText->SetText(StoreStatus);
	StoreText->SetVisibility(StoreStatus.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void UStreakSaverPanel::BuyTier(EStreakSaverTier Tier)
{
	UGameplayStatics::PlaySound2D(this, ClickSound);
	if (UStreakSaverStoreSubsystem* Store = UStreakSaverStoreSubsystem::Get(this))
	{
		Store->Buy(Tier);
	}
	Refresh();
}

void UStreakSaverPanel::HandleOneDay()
{
	BuyTier(EStreakSaverTier::OneDay);
}

void UStreakSaverPanel::HandleSevenDays()
{
	BuyTier(EStreakSaverTier::SevenDays);
}

void UStreakSaverPanel::HandleThirtyDays()
{
	BuyTier(EStreakSaverTier::ThirtyDays);
}

void UStreakSaverPanel::HandleClose()
{
	UGameplayStatics::PlaySound2D(this, ClickSound);
	Hide();
}

#if WITH_EDITOR
void UStreakSaverPanel::ApplyPreview()
{
	const bool bBroken = PreviewStreakSaverState == 1;
	StreakCountText->SetText(bBroken ? LOCTEXT("PreviewRestore", "12-day streak to restore") : LOCTEXT("PreviewStreak", "12-day streak"));
	SaverCountText->SetText(bBroken ? LOCTEXT("PreviewNoSavers", "0 saver days available") : LOCTEXT("PreviewSavers", "3 saver days available"));
	StatusText->SetText(bBroken ? LOCTEXT("PreviewBroken", "Your 12-day streak ended: you missed 2 days. 2 more saver days bring it back.")
		: LOCTEXT("PreviewProtected", "Your 12-day streak is protected for 3 missed days."));
	for (int32 Index = 0; Index < TierButtons.Num(); ++Index)
	{
		TierButtons[Index]->SetIsEnabled(PreviewStreakSaverState < 2);
		PriceTexts[Index]->SetText(LOCTEXT("Buy", "Buy"));
	}
	StoreText->SetText(PreviewStreakSaverState == 2 ? LOCTEXT("Pending", "A purchase is waiting for its payment to clear.")
		: PreviewStreakSaverState == 3 ? LOCTEXT("StoreUnavailable", "The store can't be reached right now.") : FText::GetEmpty());
	StoreText->SetVisibility(StoreText->GetText().IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}
#endif

#undef LOCTEXT_NAMESPACE
