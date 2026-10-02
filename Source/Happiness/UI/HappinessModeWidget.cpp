#include "UI/HappinessModeWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "HappinessClassic/DailyPuzzle.h"

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
	RefreshCampaignLock();
	RefreshDaily();
	if (BackButton)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &UHappinessModeWidget::HandleBackClicked);
	}
}

void UHappinessModeWidget::Show()
{
	RefreshCampaignLock();
	RefreshDaily();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UHappinessModeWidget::RefreshCampaignLock()
{
	if (CampaignButton)
	{
		const UCampaignSubsystem* Campaign = UCampaignSubsystem::Get(this);
		CampaignButton->SetIsEnabled(Campaign && Campaign->IsCampaignModeUnlocked());
	}
}

void UHappinessModeWidget::Hide()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UHappinessModeWidget::RefreshStreak()
{
	if (!StreakText)
	{
		return;
	}
	const UDailySubsystem* Daily = UDailySubsystem::Get(this);
	const int32 Streak = Daily ? Daily->GetStreak() : 0;
	const bool bDone = Daily && Daily->IsTodaySolved();
	FText Text = LOCTEXT("StartStreak", "Start a streak today!");
	if (Streak > 0)
	{
		Text = FText::Format(bDone ? LOCTEXT("StreakDone", "Streak: {0} {0}|plural(one=day,other=days)  -  done today")
			: LOCTEXT("StreakOpen", "Streak: {0} {0}|plural(one=day,other=days)"), Streak);
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
