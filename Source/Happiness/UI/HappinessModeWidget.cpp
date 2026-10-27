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
#include "HappinessClassic/PuzzleSlots.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UI/ConfirmDialogWidget.h"
#include "UI/FireworksWidget.h"
#include "UI/LessonsCompletePopup.h"
#include "UI/PlayerLevelBadge.h"
#include "UI/StreakSaverPanel.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "HappinessMode"

UHappinessModeWidget::UHappinessModeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<USoundBase> Unlock(TEXT("/Game/General/Audio/Cues/PuzzleComplete.PuzzleComplete"));
	UnlockSound = Unlock.Object;
	ConfirmDialogClass = TSoftClassPtr<UConfirmDialogWidget>(FSoftObjectPath(TEXT("/Game/Happiness/UI/WBP_ConfirmDialog.WBP_ConfirmDialog_C")));
}

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
	MakePlayerLevelBadge();
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
	UCampaignSubsystem* Campaign = UCampaignSubsystem::Get(this);
	if (Campaign && Campaign->ConsumeLessonsCompletion())
	{
		StartUnlockCelebration();
	}
	RefreshCampaignLock();
	RefreshDaily();
	if (PlayerLevelBadge)
	{
		// The level changes in games
		PlayerLevelBadge->Refresh();
	}
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

void UHappinessModeWidget::MakePlayerLevelBadge()
{
	UCanvasPanel* Root = Cast<UCanvasPanel>(GetRootWidget());
	if (PlayerLevelBadge || !Root)
	{
		return;
	}
	PlayerLevelBadge = CreateWidget<UPlayerLevelBadge>(this, UPlayerLevelBadge::StaticClass());
	if (UCanvasPanelSlot* BadgeSlot = PlayerLevelBadge ? Root->AddChildToCanvas(PlayerLevelBadge) : nullptr)
	{
		// Top right, right of the logo and above the tiles; under the Remove Ads dialog (Z 30)
		BadgeSlot->SetAnchors(FAnchors(1.f, 0.f));
		BadgeSlot->SetAlignment(FVector2D(1.f, 0.f));
		BadgeSlot->SetPosition(FVector2D(-16.f, 0.f));
		BadgeSlot->SetSize(FVector2D(UPlayerLevelBadge::BadgeSize));
		BadgeSlot->SetZOrder(2);
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
	EndUnlockCelebration();
	if (LessonsCompletePopup)
	{
		LessonsCompletePopup->Hide();
	}
	HandleClosePurchaseClicked();
	if (StreakSaverPanel)
	{
		StreakSaverPanel->Hide();
	}
	SetVisibility(ESlateVisibility::Collapsed);
}

void UHappinessModeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateUnlockCelebration(InDeltaTime);
}

void UHappinessModeWidget::StartUnlockCelebration()
{
	UCanvasPanel* Root = Cast<UCanvasPanel>(GetRootWidget());
	if (!Root)
	{
		return;
	}
	if (!Fireworks)
	{
		Fireworks = WidgetTree->ConstructWidget<UFireworksWidget>(UFireworksWidget::StaticClass(), TEXT("UnlockFireworks"));
		if (UCanvasPanelSlot* FireworksSlot = Root->AddChildToCanvas(Fireworks))
		{
			// Over the whole screen, above the congratulations (Z 40); the congratulations take the taps
			FireworksSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			FireworksSlot->SetOffsets(FMargin(0.f));
			FireworksSlot->SetZOrder(45);
		}
	}
	Fireworks->Stop();
	Fireworks->SetVisibility(ESlateVisibility::HitTestInvisible);
	UnlockTime = 0.f;
	bUnlockFireworksStarted = false;
	FirstBurstAt = -1.f;
	ShowLessonsCompletePopup();
}

bool UHappinessModeWidget::PlayUnlockFireworks()
{
	const UWidget* Window = LessonsCompletePopup ? LessonsCompletePopup->GetWindow() : nullptr;
	if (!Window)
	{
		return false;
	}
	const FGeometry& Area = Fireworks->GetCachedGeometry();
	const FGeometry& Target = Window->GetCachedGeometry();
	if (Area.GetLocalSize().X <= 0.f || Target.GetLocalSize().X <= 0.f || Area.Scale <= 0.f)
	{
		return false;
	}
	// Over the congratulations' window, in the fireworks' space; sizes go with the window's (1400 wide in its layout)
	const FVector2D Center = Area.AbsoluteToLocal(Target.GetAbsolutePositionAtCoordinates(FVector2D(.5f)));
	const FVector2D Size = Target.GetAbsoluteSize() / Area.Scale;
	const float Unit = 1.6f * Size.X / 1400.f;
	// Offsets in window sizes, from its center
	const struct { float X, Y, Delay; } Plan[] =
	{
		{ 0.f, -.3f, .45f }, { -.36f, -.14f, .8f }, { .36f, -.2f, 1.1f }, { -.2f, .16f, 1.45f },
		{ .25f, .1f, 1.75f }, { 0.f, -.04f, 2.05f }, { -.42f, .3f, 2.35f }, { .42f, .28f, 2.6f },
	};
	TArray<UFireworksWidget::FBurst> Bursts;
	for (const auto& Step : Plan)
	{
		UFireworksWidget::FBurst& Burst = Bursts.AddDefaulted_GetRef();
		Burst.Position = Center + FVector2D(Step.X * Size.X, Step.Y * Size.Y);
		Burst.Delay = Step.Delay;
		// Each rocket climbs from just below the window
		Burst.RocketRise = Center.Y + Size.Y * .5f + 40.f * Unit - Burst.Position.Y;
	}
	Fireworks->Play(Bursts, Unit);
	return true;
}

void UHappinessModeWidget::UpdateUnlockCelebration(float DeltaTime)
{
	if (!IsCelebratingUnlock())
	{
		return;
	}
	UnlockTime += DeltaTime;
	// Once the congratulations have opened (and been laid out)
	if (!bUnlockFireworksStarted && UnlockTime >= UnlockFireworksDelay && PlayUnlockFireworks())
	{
		bUnlockFireworksStarted = true;
		FirstBurstAt = UnlockTime + Fireworks->GetFirstBurstTime();
	}
	Fireworks->Advance(DeltaTime);
	if (FirstBurstAt >= 0.f && UnlockTime >= FirstBurstAt)
	{
		FirstBurstAt = -1.f;
		if (UnlockSound)
		{
			UGameplayStatics::PlaySound2D(this, UnlockSound, 1.f, 1.f, 0.f, nullptr, nullptr, true);
		}
	}
	if (bUnlockFireworksStarted ? !Fireworks->IsPlaying() : UnlockTime >= UnlockFireworksDelay + 2.f)
	{
		EndUnlockCelebration();
	}
}

void UHappinessModeWidget::EndUnlockCelebration()
{
	if (!IsCelebratingUnlock())
	{
		return;
	}
	UnlockTime = -1.f;
	FirstBurstAt = -1.f;
	if (Fireworks)
	{
		Fireworks->Stop();
		Fireworks->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UHappinessModeWidget::ShowLessonsCompletePopup()
{
	if (!LessonsCompletePopup)
	{
		LessonsCompletePopup = CreateWidget<ULessonsCompletePopup>(this, ULessonsCompletePopup::StaticClass());
		UCanvasPanel* Root = Cast<UCanvasPanel>(GetRootWidget());
		if (UCanvasPanelSlot* PopupSlot = Root && LessonsCompletePopup ? Root->AddChildToCanvas(LessonsCompletePopup) : nullptr)
		{
			// Over the whole screen, under the fireworks
			PopupSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			PopupSlot->SetOffsets(FMargin(0.f));
			PopupSlot->SetZOrder(40);
		}
		else if (LessonsCompletePopup)
		{
			LessonsCompletePopup->AddToViewport(40);
		}
	}
	if (LessonsCompletePopup)
	{
		// The Campaign tile's picture, on the popup's Campaign card
		const UImage* Art = Cast<UImage>(GetWidgetFromName(TEXT("CampaignButtonArt")));
		LessonsCompletePopup->Show(Art ? &Art->GetBrush() : nullptr);
	}
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
	if (!AskResumeOrAbandon(EPuzzleSlot::Daily, [this]() { PlayDaily(true); }, [this]() { PlayDaily(false); }))
	{
		PlayDaily(false);
	}
}

void UHappinessModeWidget::PlayDaily(bool bResume)
{
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
	// Today's puzzle already started: carry on with it
	if (bResume && ResumePuzzle(EPuzzleSlot::Daily))
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
	// Abandoned, the slot has no puzzle in progress, so Free Play opens its settings
	if (!AskResumeOrAbandon(EPuzzleSlot::FreePlay, [this]() { PlayFreePlay(); }, [this]() { PlayFreePlay(); }))
	{
		PlayFreePlay();
	}
}

void UHappinessModeWidget::PlayFreePlay()
{
	UDailySubsystem::StopDaily(this);
	Hide();
	// The game select screen goes on (WBP_GameSelect's ShowHappinessConfig): it loads the game save, which then holds
	// Free Play's puzzle in progress, and resumes it, or with none opens the Free Play settings
	UPuzzleSlotSubsystem* Slots = UPuzzleSlotSubsystem::Get(this);
	if (Slots)
	{
		Slots->BeginResume(EPuzzleSlot::FreePlay);
	}
	OnClassicChosen.Broadcast();
	if (Slots)
	{
		Slots->EndResume();
	}
}

bool UHappinessModeWidget::AskResumeOrAbandon(EPuzzleSlot PuzzleSlot, TFunction<void()> OnResume, TFunction<void()> OnAbandon)
{
	UPuzzleSlotSubsystem* Slots = UPuzzleSlotSubsystem::Get(this);
	if (!Slots || !Slots->CanResume(PuzzleSlot))
	{
		return false;
	}
	UConfirmDialogWidget* Dialog = GetConfirmDialog();
	if (!Dialog)
	{
		// No dialog to ask with: carry on with the puzzle, as nothing is lost that way
		OnResume();
		return true;
	}

	FText Description;
	switch (PuzzleSlot)
	{
	case EPuzzleSlot::Daily:
		Description = LOCTEXT("DailyInProgress", "Today's daily puzzle is in progress. Abandoning it starts it again from the beginning.");
		break;
	case EPuzzleSlot::FreePlay:
		Description = LOCTEXT("FreePlayInProgress", "You have a Free Play puzzle in progress. Abandoning it lets you choose a new one.");
		break;
	case EPuzzleSlot::Lessons:
		Description = LOCTEXT("LessonInProgress", "You have a lesson puzzle in progress. Abandoning it takes you to the lessons.");
		break;
	default:
		Description = LOCTEXT("CampaignInProgress", "You have a campaign puzzle in progress. Abandoning it takes you to the campaign.");
		break;
	}
	TWeakObjectPtr<UHappinessModeWidget> WeakThis(this);
	Dialog->Ask(LOCTEXT("ResumeQuestion", "Resume your puzzle?"), Description.ToString(),
		[WeakThis, OnResume]()
		{
			if (WeakThis.IsValid())
			{
				OnResume();
			}
		},
		[WeakThis, PuzzleSlot, OnAbandon]()
		{
			UPuzzleSlotSubsystem* AbandonSlots = WeakThis.IsValid() ? UPuzzleSlotSubsystem::Get(WeakThis.Get()) : nullptr;
			if (AbandonSlots)
			{
				AbandonSlots->AbandonPuzzle(PuzzleSlot);
				OnAbandon();
			}
		},
		LOCTEXT("Resume", "Resume"), LOCTEXT("Abandon", "Abandon"));
	return true;
}

UConfirmDialogWidget* UHappinessModeWidget::GetConfirmDialog()
{
	if (!ConfirmDialog)
	{
		UClass* DialogClass = ConfirmDialogClass.LoadSynchronous();
		UCanvasPanel* Root = Cast<UCanvasPanel>(GetRootWidget());
		ConfirmDialog = DialogClass && Root ? CreateWidget<UConfirmDialogWidget>(this, DialogClass) : nullptr;
		if (UCanvasPanelSlot* DialogSlot = ConfirmDialog ? Root->AddChildToCanvas(ConfirmDialog) : nullptr)
		{
			// Over the whole screen, above everything else on it
			DialogSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			DialogSlot->SetOffsets(FMargin(0.f));
			DialogSlot->SetZOrder(60);
		}
		if (ConfirmDialog)
		{
			ConfirmDialog->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	return ConfirmDialog;
}

bool UHappinessModeWidget::ResumePuzzle(EPuzzleSlot PuzzleSlot)
{
	UPuzzleSlotSubsystem* Slots = UPuzzleSlotSubsystem::Get(this);
	FPuzzleSlotInfo Info;
	if (!Slots || !Slots->CanResume(PuzzleSlot, &Info))
	{
		return false;
	}
	if (PuzzleSlot == EPuzzleSlot::Lessons || PuzzleSlot == EPuzzleSlot::Campaign)
	{
		// Back into its lesson session, so the puzzle set up is that one again
		UCampaignSubsystem* Campaign = UCampaignSubsystem::Get(this);
		if (!Campaign || !Campaign->ResumeLessonPuzzle(PuzzleSlot == EPuzzleSlot::Campaign ? ECampaignMode::Campaign : ECampaignMode::Lessons,
			Info.Lesson, Info.Stage, Info.Seed))
		{
			return false;
		}
	}

	// The game select screen this sits in (WBP_GameSelect) resumes the puzzle its load of the game save finds
	UUserWidget* Screen = GetTypedOuter<UUserWidget>();
	UFunction* Resume = Screen ? Screen->FindFunction(TEXT("ShowHappinessConfig")) : nullptr;
	if (!Resume || Resume->ParmsSize > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Resuming a puzzle: the screen holding the mode screen has no ShowHappinessConfig"));
		return false;
	}
	Hide();
	Slots->BeginResume(PuzzleSlot);
	Screen->ProcessEvent(Resume, nullptr);
	Slots->EndResume();
	return true;
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
	// A lesson puzzle in progress in this track: back into it, or abandon it for the tree
	const EPuzzleSlot PuzzleSlot = Mode == ECampaignMode::Campaign ? EPuzzleSlot::Campaign : EPuzzleSlot::Lessons;
	if (!AskResumeOrAbandon(PuzzleSlot,
		[this, Mode, PuzzleSlot]()
		{
			UDailySubsystem::StopDaily(this);
			if (!ResumePuzzle(PuzzleSlot))
			{
				ShowTree(Mode);
			}
		},
		[this, Mode]() { ShowTree(Mode); }))
	{
		ShowTree(Mode);
	}
}

void UHappinessModeWidget::ShowTree(ECampaignMode Mode)
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
