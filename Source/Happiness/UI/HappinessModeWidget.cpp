#include "UI/HappinessModeWidget.h"

#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Ads/AdRemoval.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Font.h"
#include "HappinessClassic/DailyPuzzle.h"
#include "UI/StreakSaverPanel.h"

#define LOCTEXT_NAMESPACE "HappinessMode"

void UHappinessModeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ClassicButton)
	{
		ClassicButton->OnClicked.AddUniqueDynamic(this, &UHappinessModeWidget::HandleClassicClicked);
	}
	if (LessonsButton)
	{
		LessonsButton->OnClicked.AddUniqueDynamic(this, &UHappinessModeWidget::HandleLessonsClicked);
	}
	if (CampaignButton)
	{
		CampaignButton->OnClicked.AddUniqueDynamic(this, &UHappinessModeWidget::HandleCampaignClicked);
	}
	if (DailyButton)
	{
		DailyButton->OnClicked.AddUniqueDynamic(this, &UHappinessModeWidget::HandleDailyClicked);
	}
	if (RemoveAdsButton)
	{
		RemoveAdsButton->OnClicked.AddUniqueDynamic(this, &UHappinessModeWidget::HandleRemoveAdsClicked);
	}
	if (ClosePurchaseButton)
	{
		ClosePurchaseButton->OnClicked.AddUniqueDynamic(this, &UHappinessModeWidget::HandleClosePurchaseClicked);
	}
	if (UAdRemovalSubsystem* AdRemoval = UAdRemovalSubsystem::Get(this))
	{
		AdRemoval->OnChanged.AddUniqueDynamic(this, &UHappinessModeWidget::RefreshAdRemoval);
	}
	MakeStreakSaverButton();
	if (StreakSaverButton)
	{
		StreakSaverButton->OnClicked.AddUniqueDynamic(this, &UHappinessModeWidget::HandleStreakSaverClicked);
	}
	if (UDailySubsystem* Daily = UDailySubsystem::Get(this))
	{
		Daily->OnChanged.AddUniqueDynamic(this, &UHappinessModeWidget::RefreshStreak);
	}
	RefreshAdRemoval();
	RefreshCampaignLock();
	RefreshDaily();
	if (BackButton)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &UHappinessModeWidget::HandleBackClicked);
	}
}

void UHappinessModeWidget::NativeDestruct()
{
	if (UAdRemovalSubsystem* AdRemoval = UAdRemovalSubsystem::Get(this))
	{
		AdRemoval->OnChanged.RemoveDynamic(this, &UHappinessModeWidget::RefreshAdRemoval);
	}
	if (UDailySubsystem* Daily = UDailySubsystem::Get(this))
	{
		Daily->OnChanged.RemoveDynamic(this, &UHappinessModeWidget::RefreshStreak);
	}
	Super::NativeDestruct();
}

void UHappinessModeWidget::RefreshAdRemoval()
{
	const UAdRemovalSubsystem* AdRemoval = UAdRemovalSubsystem::Get(this);
	const bool bOwned = AdRemoval && AdRemoval->AreAdsRemoved();
	if (RemoveAdsCard)
	{
		// The centered horizontal layout closes the gap when the offer is removed.
		RemoveAdsCard->SetVisibility(bOwned ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
	if (bOwned)
	{
		HandleClosePurchaseClicked();
	}
}

void UHappinessModeWidget::HandleRemoveAdsClicked()
{
	const UAdRemovalSubsystem* AdRemoval = UAdRemovalSubsystem::Get(this);
	if (AdsPurchaseDialog && (!AdRemoval || !AdRemoval->AreAdsRemoved()))
	{
		AdsPurchaseDialog->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UHappinessModeWidget::HandleClosePurchaseClicked()
{
	if (AdsPurchaseDialog)
	{
		AdsPurchaseDialog->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UHappinessModeWidget::Show()
{
	HandleClosePurchaseClicked();
	RefreshAdRemoval();
	RefreshCampaignLock();
	RefreshDaily();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UHappinessModeWidget::RefreshCampaignLock()
{
	if (CampaignButton)
	{
		const UCampaignSubsystem* Campaign = UCampaignSubsystem::Get(this);
		const bool bUnlocked = Campaign && Campaign->IsCampaignModeUnlocked();
		CampaignButton->SetIsEnabled(bUnlocked);
		if (UWidget* Note = GetWidgetFromName(TEXT("CampaignNote")))
		{
			Note->SetVisibility(bUnlocked ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		}
	}
}

void UHappinessModeWidget::MakeStreakSaverButton()
{
	UCanvasPanel* Tiles = DailyButton ? Cast<UCanvasPanel>(DailyButton->GetParent()) : nullptr;
	if (StreakSaverButton || !Tiles)
	{
		return;
	}
	const FLinearColor Ink(0.32f, 0.22f, 0.13f, 1.f);
	const FLinearColor Gold(0.78f, 0.6f, 0.25f, 1.f);
	auto Pill = [Gold](const FLinearColor& Fill)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(20.f, 20.f, 20.f, 20.f), FSlateColor(Gold), 2.f);
		return Brush;
	};
	FButtonStyle Style;
	Style.SetNormal(Pill(FLinearColor(1.f, 0.97f, 0.88f))).SetHovered(Pill(FLinearColor(0.98f, 0.92f, 0.76f)))
		.SetPressed(Pill(FLinearColor(0.93f, 0.85f, 0.64f)));
	Style.SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f));

	StreakSaverButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StreakSaverButton"));
	StreakSaverButton->SetStyle(Style);
	StreakSaverLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StreakSaverLabel"));
	if (const UFont* Font = LoadObject<UFont>(nullptr, TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font")))
	{
		StreakSaverLabel->SetFont(FSlateFontInfo(Font, 20));
	}
	StreakSaverLabel->SetColorAndOpacity(FSlateColor(Ink));
	StreakSaverButton->SetContent(StreakSaverLabel);

	// Just above the Daily tile, at its right edge: never over a tile, so taps go where they're meant
	const UCanvasPanelSlot* DailySlot = Cast<UCanvasPanelSlot>(DailyButton->Slot);
	const FVector2D TileOrigin = DailySlot ? DailySlot->GetPosition() : FVector2D::ZeroVector;
	const FVector2D TileSize = DailySlot ? DailySlot->GetSize() : FVector2D(510.f, 240.f);
	if (UCanvasPanelSlot* ButtonSlot = Tiles->AddChildToCanvas(StreakSaverButton))
	{
		const FVector2D Size(190.f, 40.f);
		ButtonSlot->SetPosition(TileOrigin + FVector2D(TileSize.X - Size.X, -Size.Y - 16.f));
		ButtonSlot->SetSize(Size);
		ButtonSlot->SetZOrder(DailySlot ? DailySlot->GetZOrder() + 1 : 1);
	}
}

void UHappinessModeWidget::HandleStreakSaverClicked()
{
	if (!StreakSaverPanel)
	{
		StreakSaverPanel = CreateWidget<UStreakSaverPanel>(this, UStreakSaverPanel::StaticClass());
		UCanvasPanel* Root = Cast<UCanvasPanel>(GetRootWidget());
		if (UCanvasPanelSlot* PanelSlot = Root && StreakSaverPanel ? Root->AddChildToCanvas(StreakSaverPanel) : nullptr)
		{
			// Over the whole screen, above the Remove Ads dialog
			PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			PanelSlot->SetOffsets(FMargin(0.f));
			PanelSlot->SetZOrder(40);
		}
		else if (StreakSaverPanel)
		{
			StreakSaverPanel->AddToViewport(40);
		}
	}
	if (StreakSaverPanel)
	{
		StreakSaverPanel->Show();
	}
}

void UHappinessModeWidget::Hide()
{
	HandleClosePurchaseClicked();
	if (StreakSaverPanel)
	{
		StreakSaverPanel->Hide();
	}
	SetVisibility(ESlateVisibility::Collapsed);
}

void UHappinessModeWidget::RefreshStreak()
{
	const UDailySubsystem* Daily = UDailySubsystem::Get(this);
	if (StreakSaverLabel)
	{
		const int32 SaverDays = Daily ? Daily->GetSaverDays() : 0;
		StreakSaverLabel->SetText(SaverDays > 0
			? FText::Format(LOCTEXT("SaverDays", "Saver: {0} {0}|plural(one=day,other=days)"), SaverDays)
			: LOCTEXT("StreakSaver", "Streak Saver"));
	}
	if (!StreakText)
	{
		return;
	}
	const int32 Streak = Daily ? Daily->GetStreak() : 0;
	const bool bDone = Daily && Daily->IsTodaySolved();
	int32 Broken = 0, Missed = 0, Needed = 0;
	FText Text = LOCTEXT("StartStreak", "Start a streak today!");
	if (Streak > 0)
	{
		Text = FText::Format(bDone ? LOCTEXT("StreakDone", "Streak: {0} {0}|plural(one=day,other=days)  -  done today")
			: LOCTEXT("StreakOpen", "Streak: {0} {0}|plural(one=day,other=days)"), Streak);
	}
	else if (Daily && Daily->GetBrokenStreak(Broken, Missed, Needed))
	{
		// Until a new streak starts, a streak saver can still bring it back
		Text = FText::Format(LOCTEXT("StreakBroken", "Your {0}-day streak can be saved!"), Broken);
	}
	StreakText->SetText(Text);
}

void UHappinessModeWidget::RefreshDaily()
{
	// One daily puzzle a day: once it's solved, the button waits for tomorrow
	if (DailyButton)
	{
		const UDailySubsystem* Daily = UDailySubsystem::Get(this);
		DailyButton->SetIsEnabled(!Daily || !Daily->IsTodaySolved());
	}
	RefreshStreak();
}

void UHappinessModeWidget::HandleDailyClicked()
{
	// One a day (the button is disabled then too)
	const UDailySubsystem* Daily = UDailySubsystem::Get(this);
	if (Daily && Daily->IsTodaySolved())
	{
		return;
	}

	// Lessons and the daily puzzle don't mix
	UCampaignSubsystem::StopPlayingLesson(this);
	Hide();
	OnDailyChosen.Broadcast();

	// Play it the way Free Play does: the game select screen this sits in has PlayHappiness(Number, Size, Difficulty)
	int32 Number = 0, Size = 0, Difficulty = 0;
	if (!UDailySubsystem::StartDaily(this, Number, Size, Difficulty))
	{
		return;
	}
	UUserWidget* Screen = GetTypedOuter<UUserWidget>();
	UFunction* Play = Screen ? Screen->FindFunction(TEXT("PlayHappiness")) : nullptr;
	if (!Play)
	{
		UE_LOG(LogTemp, Warning, TEXT("Daily puzzle: the screen holding the mode screen has no PlayHappiness"));
		return;
	}

	uint8* Params = static_cast<uint8*>(FMemory_Alloca(Play->ParmsSize));
	FMemory::Memzero(Params, Play->ParmsSize);
	Play->InitializeStruct(Params);
	for (const TPair<const TCHAR*, int32>& Arg : { TPair<const TCHAR*, int32>(TEXT("Number"), Number),
		TPair<const TCHAR*, int32>(TEXT("Size"), Size), TPair<const TCHAR*, int32>(TEXT("Difficulty"), Difficulty) })
	{
		if (const FIntProperty* Param = FindFProperty<FIntProperty>(Play, Arg.Key))
		{
			Param->SetPropertyValue_InContainer(Params, Arg.Value);
		}
	}
	Screen->ProcessEvent(Play, Params);
	Play->DestroyStruct(Params);
}

void UHappinessModeWidget::HandleClassicClicked()
{
	UDailySubsystem::StopDaily(this);
	Hide();
	OnClassicChosen.Broadcast();
}

void UHappinessModeWidget::HandleLessonsClicked()
{
	OpenTree(ECampaignMode::Lessons);
}

void UHappinessModeWidget::HandleCampaignClicked()
{
	OpenTree(ECampaignMode::Campaign);
}

void UHappinessModeWidget::OpenTree(ECampaignMode Mode)
{
	UDailySubsystem::StopDaily(this);
	if (UCampaignSubsystem* Campaign = UCampaignSubsystem::Get(this))
	{
		Campaign->SetMode(Mode);
	}
	Hide();
	OnLessonsChosen.Broadcast();
}

void UHappinessModeWidget::HandleBackClicked()
{
	Hide();
	OnClosed.Broadcast();
}

#undef LOCTEXT_NAMESPACE
