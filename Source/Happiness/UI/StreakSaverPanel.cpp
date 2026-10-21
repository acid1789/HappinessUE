#include "UI/StreakSaverPanel.h"

#include "HappinessClassic/DailyPuzzle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
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

	FSlateBrush RoundedBrush(const FLinearColor& Fill, float Radius, const FLinearColor& Outline, float OutlineWidth)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(Radius, Radius, Radius, Radius), FSlateColor(Outline), OutlineWidth);
		return Brush;
	}
}

UStreakSaverPanel::UStreakSaverPanel(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Accept(TEXT("/Game/General/Audio/Cues/MenuAccept.MenuAccept"));
	TitleFont = FSlateFontInfo(Font.Object, 44);
	BodyFont = FSlateFontInfo(Font.Object, 24);
	TierFont = FSlateFontInfo(Font.Object, 32);
	ClickSound = Accept.Object;

	const FLinearColor Gold(0.78f, 0.6f, 0.25f, 1.f);
	ButtonStyle.SetNormal(RoundedBrush(FLinearColor(1.f, 0.98f, 0.93f), 16.f, Gold, 2.f))
		.SetHovered(RoundedBrush(FLinearColor(0.97f, 0.93f, 0.8f), 16.f, Gold, 3.f))
		.SetPressed(RoundedBrush(FLinearColor(0.92f, 0.86f, 0.68f), 16.f, Gold, 3.f))
		.SetDisabled(RoundedBrush(FLinearColor(0.9f, 0.88f, 0.84f, 0.7f), 16.f, FLinearColor(0.7f, 0.66f, 0.6f, 0.7f), 2.f));
	ButtonStyle.SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f));

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
	return Block;
}

void UStreakSaverPanel::BuildLayout()
{
	// The dimmer takes the clicks meant for the mode screen
	UBorder* Dimmer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dimmer"));
	Dimmer->SetBrushColor(DimColor);
	Dimmer->SetHorizontalAlignment(HAlign_Center);
	Dimmer->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Dimmer;

	UBorder* Window = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Window"));
	Window->SetBrush(RoundedBrush(WindowColor, 28.f, FLinearColor(0.78f, 0.6f, 0.25f, 1.f), 3.f));
	Window->SetPadding(FMargin(48.f, 30.f, 48.f, 30.f));
	Dimmer->SetContent(Window);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Window->SetContent(Column);
	auto AddRow = [Column](UWidget* Widget, float Bottom)
	{
		if (UVerticalBoxSlot* RowSlot = Column->AddChildToVerticalBox(Widget))
		{
			RowSlot->SetHorizontalAlignment(HAlign_Center);
			RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, Bottom));
		}
	};

	AddRow(MakeText(LOCTEXT("Title", "Streak Saver"), TitleFont), 8.f);
	StatusText = MakeText(FText::GetEmpty(), BodyFont);
	StatusText->SetAutoWrapText(true);
	StatusText->SetWrapTextAt(760.f);
	AddRow(StatusText, 22.f);

	UHorizontalBox* Tiers = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddRow(Tiers, 14.f);
	TierButtons.Reset();
	PriceTexts.Reset();
	for (EStreakSaverTier Tier : PanelTiers)
	{
		const int32 Days = UStreakSaverStoreSubsystem::GetTierDays(Tier);
		UButton* Button = WidgetTree->ConstructWidget<UButton>();
		Button->SetStyle(ButtonStyle);
		UVerticalBox* Label = WidgetTree->ConstructWidget<UVerticalBox>();
		UTextBlock* DaysText = MakeText(FText::Format(LOCTEXT("TierDays", "{0} {0}|plural(one=Day,other=Days)"), Days), TierFont);
		UTextBlock* Price = MakeText(FText::GetEmpty(), BodyFont);
		for (UTextBlock* Line : { DaysText, Price })
		{
			if (UVerticalBoxSlot* LineSlot = Label->AddChildToVerticalBox(Line))
			{
				LineSlot->SetHorizontalAlignment(HAlign_Center);
			}
		}
		Button->SetContent(Label);
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(TierSize.X);
		Size->SetHeightOverride(TierSize.Y);
		Size->SetContent(Button);
		if (UHorizontalBoxSlot* TierSlot = Tiers->AddChildToHorizontalBox(Size))
		{
			TierSlot->SetPadding(FMargin(12.f, 0.f));
		}
		TierButtons.Add(Button);
		PriceTexts.Add(Price);
	}
	TierButtons[0]->OnClicked.AddUniqueDynamic(this, &UStreakSaverPanel::HandleOneDay);
	TierButtons[1]->OnClicked.AddUniqueDynamic(this, &UStreakSaverPanel::HandleSevenDays);
	TierButtons[2]->OnClicked.AddUniqueDynamic(this, &UStreakSaverPanel::HandleThirtyDays);

	UTextBlock* Rule = MakeText(LOCTEXT("Rule", "Each saver day covers one missed day. Days you missed are covered first: your streak comes back, without counting them."), BodyFont);
	Rule->SetAutoWrapText(true);
	Rule->SetWrapTextAt(760.f);
	Rule->SetColorAndOpacity(FSlateColor(InkColor * FLinearColor(1.f, 1.f, 1.f, 0.75f)));
	AddRow(Rule, 6.f);
	StoreText = MakeText(FText::GetEmpty(), BodyFont);
	AddRow(StoreText, 12.f);

	CloseButton = WidgetTree->ConstructWidget<UButton>();
	CloseButton->SetStyle(ButtonStyle);
	UTextBlock* CloseText = MakeText(LOCTEXT("Close", "Close"), BodyFont);
	CloseButton->SetContent(CloseText);
	USizeBox* CloseSize = WidgetTree->ConstructWidget<USizeBox>();
	CloseSize->SetWidthOverride(200.f);
	CloseSize->SetHeightOverride(56.f);
	CloseSize->SetContent(CloseButton);
	AddRow(CloseSize, 0.f);
	CloseButton->OnClicked.AddUniqueDynamic(this, &UStreakSaverPanel::HandleClose);
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
	const UDailySubsystem* Daily = UDailySubsystem::Get(this);
	const UStreakSaverStoreSubsystem* Store = UStreakSaverStoreSubsystem::Get(this);
	const int32 Streak = Daily ? Daily->GetStreak() : 0;
	const int32 SaverDays = Daily ? Daily->GetSaverDays() : 0;
	int32 Broken = 0, Missed = 0, Needed = 0;
	const bool bBroken = Daily && Daily->GetBrokenStreak(Broken, Missed, Needed);

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
		PriceTexts[Index]->SetText(Price.IsEmpty() ? LOCTEXT("NoPrice", "-") : Price);
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

#undef LOCTEXT_NAMESPACE
