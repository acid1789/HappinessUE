#include "UI/CampaignEndScreenWidget.h"
#include "HappinessClassic/CampaignTree.h"
#include "HappinessClassic/Puzzle.h"
#include "UI/LessonProgressTicks.h"

#include "Ads/AdsSubsystem.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "CampaignEndScreen"

UCampaignEndScreenWidget::UCampaignEndScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// The other end screens' sounds
	static ConstructorHelpers::FObjectFinder<USoundBase> Step(TEXT("/Game/General/Audio/Cues/GameLoad.GameLoad"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Fill(TEXT("/Game/General/Audio/Cues/GameAction5.GameAction5"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Unlock(TEXT("/Game/General/Audio/Cues/Happiness.Happiness"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Button(TEXT("/Game/General/Audio/Cues/MenuAccept.MenuAccept"));
	StepSound = Step.Object;
	FillSound = Fill.Object;
	UnlockSound = Unlock.Object;
	ButtonSound = Button.Object;
}

void UCampaignEndScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RestartButton)
	{
		RestartButton->OnClicked.AddUniqueDynamic(this, &UCampaignEndScreenWidget::HandleRestartClicked);
	}
	if (ReturnButton)
	{
		ReturnButton->OnClicked.AddUniqueDynamic(this, &UCampaignEndScreenWidget::HandleReturnClicked);
	}
	if (NextPuzzleButton)
	{
		NextPuzzleButton->OnClicked.AddUniqueDynamic(this, &UCampaignEndScreenWidget::HandleNextClicked);
	}
}

void UCampaignEndScreenWidget::Show(UPuzzle* Puzzle, float PuzzleSeconds)
{
	UCampaignSubsystem* Campaign = UCampaignSubsystem::Get(this);
	if (!Campaign || !Puzzle)
	{
		return;
	}

	Result = Campaign->FinishLessonPuzzle(Puzzle);

	// Scored the same as Free Play: par from the rating and the player's pace, experience and a time bonus
	Score = UFreePlaySubsystem::FinishFreePlayPuzzle(this, Puzzle, PuzzleSeconds);
	const int32 TotalExp = Score.BaseExp + Score.BonusExp;
	AddPlayerExp(TotalExp);
	ExpToAdd = float(TotalExp);
	ExpRate = ExpFillTime > 0.f ? ExpToAdd / ExpFillTime : ExpToAdd;

	// The stage Next Puzzle plays (see UCampaignSubsystem::PrepareNextLessonPuzzle)
	// (Master Mode's current stage after a Master Mode puzzle; the final never leads into Master Mode)
	const bool bNextMaster = Campaign->IsMasterUnlocked(Result.Lesson) && Result.bMaster;
	NextStage = bNextMaster ? Campaign->GetCurrentMasterStage(Result.Lesson) : Campaign->GetCurrentStage(Result.Lesson);

	// The bar: the clue's campaign track, or Master Mode's for a Master Mode puzzle
	if (StageLabel)
	{
		StageLabel->SetText(Result.bMaster ? LOCTEXT("MasterLabel", "Master:") : LOCTEXT("CampaignLabel", "Campaign:"));
	}
	if (ProgressTicks)
	{
		if (Result.bMaster)
		{
			TArray<float> Ticks;
			const int32 FirstMaster = UCampaignSubsystem::GetFirstMasterStage();
			for (int32 Stage = FirstMaster + 1; Stage < FirstMaster + UCampaignSubsystem::GetNumMasterStages(); Stage++)
			{
				Ticks.Add(float(UCampaignSubsystem::GetMasterStagePointsRequired(Stage)) / UCampaignSubsystem::GetMasterMaxPoints());
			}
			ProgressTicks->SetTickFractions(Ticks);
		}
		else
		{
			ProgressTicks->ClearTickFractions();
		}
	}

	LessonText->SetText(FText::Format(LOCTEXT("LessonStage", "{0} - {1}"),
		UCampaignTree::GetLessonDisplayName(Result.Lesson), UCampaignSubsystem::GetStageDisplayName(Result.Stage)));

	ResultText->SetText(Result.bSolved ? LOCTEXT("Solved", "SUCCESS!") : LOCTEXT("Incorrect", "INCORRECT"));
	ResultText->SetColorAndOpacity(Result.bSolved ? SolvedColor : IncorrectColor);

	TimeText->SetText(FText::FromString(Puzzle->FormatTimeString(PuzzleSeconds)));
	ParTimeText->SetText(FText::FromString(Puzzle->FormatTimeString(Score.ParSeconds)));
	CompletionExpText->SetText(FText::AsNumber(Score.BaseExp));
	BonusExpText->SetText(FText::AsNumber(Score.BonusExp));
	TotalText->SetText(FText::AsNumber(TotalExp));
	if (NoteText)
	{
		NoteText->SetText(GetNoteText());
	}
	if (UnlockText)
	{
		UnlockText->SetText(GetUnlockText());
		UnlockText->SetVisibility(ESlateVisibility::Hidden);
	}
	if (NextPuzzleText)
	{
		NextPuzzleText->SetText(UCampaignSubsystem::GetStageDisplayName(NextStage));
		NextPuzzleText->SetVisibility(ESlateVisibility::Hidden);
	}

	for (UWidget* Widget : GetRevealedWidgets())
	{
		Widget->SetVisibility(ESlateVisibility::Hidden);
	}

	// Hidden when the clue was just completed (Master Mode opens from the campaign tree) or just mastered
	NextPuzzleButton->SetVisibility(Result.bSolved && !Result.bLessonCompleted && !Result.bMasterCompleted
		? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	SetButtonsEnabled(false);

	SetShownProgress(Result.PreviousPoints);
	SetShownExp();
	AnimState = EAnimState::Revealing;
	AnimTimeRemaining = StartDelay;
	RevealedCount = 0;
	FillElapsed = 0.f;
	bUnlockShown = false;

	SetVisibility(ESlateVisibility::Visible);
}

void UCampaignEndScreenWidget::Hide()
{
	AnimState = EAnimState::Idle;
	SetVisibility(ESlateVisibility::Collapsed);
}

int32 UCampaignEndScreenWidget::GetExpForNextLevel(int32 Level)
{
	return FMath::Max(1, FMath::TruncToInt(FMath::Loge(float(Level + 3)) * 1000.f));
}

void UCampaignEndScreenWidget::AddPlayerExp(int32 Exp)
{
	// The player's experience and level live on the player controller (PC_Happiness), as the Free Play end screen
	// keeps them; the game saves them right after this screen is shown. The bar animates up from the old values.
	ShownLevel = 0;
	ShownExp = 0.f;
	APlayerController* PC = GetOwningPlayer();
	FIntProperty* ExpProperty = PC ? FindFProperty<FIntProperty>(PC->GetClass(), TEXT("HappinessExp")) : nullptr;
	FIntProperty* LevelProperty = PC ? FindFProperty<FIntProperty>(PC->GetClass(), TEXT("HappinessLevel")) : nullptr;
	if (!ExpProperty || !LevelProperty)
	{
		return;
	}

	int32 PlayerExp = ExpProperty->GetPropertyValue_InContainer(PC);
	int32 Level = LevelProperty->GetPropertyValue_InContainer(PC);
	ShownLevel = Level;
	ShownExp = float(PlayerExp);

	PlayerExp += FMath::Max(Exp, 0);
	while (PlayerExp >= GetExpForNextLevel(Level))
	{
		PlayerExp -= GetExpForNextLevel(Level);
		Level++;
	}
	ExpProperty->SetPropertyValue_InContainer(PC, PlayerExp);
	LevelProperty->SetPropertyValue_InContainer(PC, Level);
}

void UCampaignEndScreenWidget::SetShownExp()
{
	const int32 Needed = GetExpForNextLevel(ShownLevel);
	ExpBar->SetPercent(ShownExp / Needed);
	if (LevelText)
	{
		LevelText->SetText(FText::AsNumber(ShownLevel));
	}
	if (ExpText)
	{
		ExpText->SetText(FText::Format(LOCTEXT("Exp", "{0} / {1}"), FMath::FloorToInt(ShownExp), Needed));
	}
}

TArray<UWidget*> UCampaignEndScreenWidget::GetRevealedWidgets() const
{
	// In order of appearance; the note comes with the total
	TArray<UWidget*> Widgets = { TimeText, ParTimeText, CompletionExpText, BonusExpText, TotalText };
	if (NoteText)
	{
		Widgets.Add(NoteText);
	}
	return Widgets;
}

void UCampaignEndScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	switch (AnimState)
	{
		case EAnimState::Revealing:
		{
			AnimTimeRemaining -= InDeltaTime;
			if (AnimTimeRemaining > 0.f)
			{
				break;
			}
			AnimTimeRemaining = StepTime;

			// One line per step: time, par, completion, bonus, total (with the note); the level bar after that
			const TArray<UWidget*> Widgets = GetRevealedWidgets();
			const int32 LineCount = 5;
			if (RevealedCount < LineCount)
			{
				Widgets[RevealedCount]->SetVisibility(ESlateVisibility::HitTestInvisible);
				if (RevealedCount == LineCount - 1 && NoteText)
				{
					NoteText->SetVisibility(ESlateVisibility::HitTestInvisible);
				}
				RevealedCount++;
				PlaySound(StepSound);
			}
			else
			{
				// After the last line's pause
				AnimState = EAnimState::FillingExp;
			}
			break;
		}

		case EAnimState::FillingExp:
		{
			// The experience goes in at a steady rate; each full bar is a level up
			const float Add = FMath::Min(ExpToAdd, ExpRate * InDeltaTime);
			ExpToAdd -= Add;
			ShownExp += Add;
			while (ShownExp >= GetExpForNextLevel(ShownLevel))
			{
				ShownExp -= GetExpForNextLevel(ShownLevel);
				ShownLevel++;
				PlaySound(UnlockSound);
			}
			SetShownExp();
			if (Add > 0.f)
			{
				PlaySound(FillSound);
			}

			if (ExpToAdd <= 0.f)
			{
				// Then the campaign bar, if the puzzle was a step
				AnimState = Result.TotalPoints > Result.PreviousPoints ? EAnimState::FillingStage : EAnimState::Done;
				if (AnimState == EAnimState::Done)
				{
					Finish();
				}
			}
			break;
		}

		case EAnimState::FillingStage:
		{
			FillElapsed = FMath::Min(FillElapsed + InDeltaTime, FillTime);
			const float Alpha = FillTime > 0.f ? FillElapsed / FillTime : 1.f;
			SetShownProgress(FMath::Lerp(float(Result.PreviousPoints), float(Result.TotalPoints), Alpha));
			PlaySound(FillSound);
			if (FillElapsed >= FillTime)
			{
				AnimState = EAnimState::Done;
				Finish();
			}
			break;
		}

		default:
			break;
	}
}

void UCampaignEndScreenWidget::Finish()
{
	SetShownProgress(Result.TotalPoints);
	if (Result.bStageUnlocked || Result.bLessonCompleted || Result.bMasterCompleted)
	{
		ShowUnlock();
	}
	if (NextPuzzleText && NextPuzzleButton->GetVisibility() == ESlateVisibility::Visible)
	{
		NextPuzzleText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	SetButtonsEnabled(true);
}

void UCampaignEndScreenWidget::ShowUnlock()
{
	if (bUnlockShown)
	{
		return;
	}
	bUnlockShown = true;
	if (UnlockText)
	{
		UnlockText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	PlaySound(UnlockSound);
}

void UCampaignEndScreenWidget::SetShownProgress(float Points)
{
	const int32 MaxPoints = Result.bMaster ? UCampaignSubsystem::GetMasterMaxPoints() : UCampaignSubsystem::GetMaxPoints();
	StageProgress->SetPercent(Points / MaxPoints);
	if (ProgressText)
	{
		ProgressText->SetText(FText::Format(LOCTEXT("Progress", "{0} / {1}"), FMath::FloorToInt(Points), MaxPoints));
	}
}

void UCampaignEndScreenWidget::SetButtonsEnabled(bool bEnabled)
{
	RestartButton->SetIsEnabled(bEnabled);
	ReturnButton->SetIsEnabled(bEnabled);
	NextPuzzleButton->SetIsEnabled(bEnabled);
}

void UCampaignEndScreenWidget::PlaySound(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound, 1.f, 1.f, 0.f, nullptr, nullptr, true);
	}
}

FText UCampaignEndScreenWidget::GetNoteText() const
{
	if (!Result.bSolved)
	{
		return LOCTEXT("IncorrectNote", "Some icons are wrong. Restart to try again.");
	}
	if (!Result.bFirstFinish)
	{
		return LOCTEXT("AlreadyCounted", "Already counted for this stage");
	}
	if (Result.Stage != UCampaignSubsystem::GetFinalStage() && Result.PointsEarned <= 0)
	{
		return LOCTEXT("Practice", "Practice: this stage is already done");
	}
	return FText::GetEmpty();
}

FText UCampaignEndScreenWidget::GetUnlockText() const
{
	if (Result.bMasterCompleted)
	{
		return FText::Format(LOCTEXT("ClueMastered", "{0} mastered!"), UCampaignTree::GetLessonDisplayName(Result.Lesson));
	}
	// Clearing the final shows no unlock text: Master Mode is found by reopening the clue from the tree
	if (Result.bStageUnlocked)
	{
		return FText::Format(LOCTEXT("StageUnlocked", "{0} unlocked!"), UCampaignSubsystem::GetStageDisplayName(NextStage));
	}
	return FText::GetEmpty();
}

void UCampaignEndScreenWidget::PlayAdThen(TFunction<void()> Action)
{
	// Every button here plays a full-screen ad first (when one is ready), then does its thing
	if (UAdsSubsystem* Ads = UAdsSubsystem::Get(this))
	{
		Ads->ShowInterstitialThen(FSimpleDelegate::CreateWeakLambda(this, MoveTemp(Action)));
	}
	else
	{
		Action();
	}
}

void UCampaignEndScreenWidget::HandleRestartClicked()
{
	PlaySound(ButtonSound);
	Hide();
	PlayAdThen([this]() { OnRestartPuzzle.Broadcast(); });
}

void UCampaignEndScreenWidget::HandleReturnClicked()
{
	PlaySound(ButtonSound);
	Hide();
	PlayAdThen([this]() { OnReturnToLessons.Broadcast(); });
}

void UCampaignEndScreenWidget::HandleNextClicked()
{
	PlaySound(ButtonSound);
	Hide();
	PlayAdThen([this]() { OnNextPuzzle.Broadcast(); });
}

#undef LOCTEXT_NAMESPACE
