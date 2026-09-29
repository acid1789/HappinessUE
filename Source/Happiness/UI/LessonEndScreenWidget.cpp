#include "UI/LessonEndScreenWidget.h"
#include "HappinessClassic/CampaignTree.h"
#include "HappinessClassic/Puzzle.h"

#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "LessonEndScreen"

ULessonEndScreenWidget::ULessonEndScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// The classic end screen's sounds
	static ConstructorHelpers::FObjectFinder<USoundBase> Step(TEXT("/Game/General/Audio/Cues/GameLoad.GameLoad"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Fill(TEXT("/Game/General/Audio/Cues/GameAction5.GameAction5"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Unlock(TEXT("/Game/General/Audio/Cues/Happiness.Happiness"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Button(TEXT("/Game/General/Audio/Cues/MenuAccept.MenuAccept"));
	StepSound = Step.Object;
	FillSound = Fill.Object;
	UnlockSound = Unlock.Object;
	ButtonSound = Button.Object;
}

void ULessonEndScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RestartButton)
	{
		RestartButton->OnClicked.AddUniqueDynamic(this, &ULessonEndScreenWidget::HandleRestartClicked);
	}
	if (ReturnButton)
	{
		ReturnButton->OnClicked.AddUniqueDynamic(this, &ULessonEndScreenWidget::HandleReturnClicked);
	}
	if (NextPuzzleButton)
	{
		NextPuzzleButton->OnClicked.AddUniqueDynamic(this, &ULessonEndScreenWidget::HandleNextClicked);
	}
}

void ULessonEndScreenWidget::Show(UPuzzle* Puzzle, float PuzzleSeconds)
{
	UCampaignSubsystem* Campaign = UCampaignSubsystem::Get(this);
	if (!Campaign || !Puzzle)
	{
		return;
	}

	Result = Campaign->FinishLessonPuzzle(Puzzle);

	// The stage Next Puzzle plays: the one that currently earns points (see UCampaignSubsystem::PrepareNextLessonPuzzle)
	NextStage = Campaign->GetCurrentStage(Result.Lesson);

	LessonText->SetText(FText::Format(LOCTEXT("LessonStage", "{0} - {1}"),
		UCampaignTree::GetLessonDisplayName(Result.Lesson), UCampaignSubsystem::GetStageDisplayName(Result.Stage)));

	ResultText->SetText(Result.bSolved ? LOCTEXT("Solved", "SUCCESS!") : LOCTEXT("Incorrect", "INCORRECT"));
	ResultText->SetColorAndOpacity(Result.bSolved ? SolvedColor : IncorrectColor);

	// The score lines add up to the total: completion 3, -1 for using hints, another -1 for a lesson-clue hint
	const int32 Completion = Result.bSolved ? UCampaignSubsystem::MaxPuzzleScore : 0;
	const int32 HintPenalty = Result.bUsedHints ? -1 : 0;
	const int32 LessonHintPenalty = Result.bUsedLessonHint ? -1 : 0;
	TimeText->SetText(FText::FromString(Puzzle->FormatTimeString(PuzzleSeconds)));
	CompletionText->SetText(FText::AsNumber(Completion));
	HintPenaltyText->SetText(FText::AsNumber(HintPenalty));
	LessonHintPenaltyText->SetText(FText::AsNumber(LessonHintPenalty));
	TotalText->SetText(FText::AsNumber(Result.bSolved ? Result.Score : 0));
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

	NextPuzzleButton->SetVisibility(Result.bSolved && !Result.bLessonCompleted ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	SetButtonsEnabled(false);

	SetShownPoints(Result.PreviousPoints);
	AnimState = EAnimState::Revealing;
	AnimTimeRemaining = StartDelay;
	RevealedCount = 0;
	FillElapsed = 0.f;
	bUnlockShown = false;

	SetVisibility(ESlateVisibility::Visible);
}

void ULessonEndScreenWidget::Hide()
{
	AnimState = EAnimState::Idle;
	SetVisibility(ESlateVisibility::Collapsed);
}

TArray<UWidget*> ULessonEndScreenWidget::GetRevealedWidgets() const
{
	// In order of appearance; the note comes with the total
	TArray<UWidget*> Widgets = { TimeText, CompletionText, HintPenaltyText, LessonHintPenaltyText, TotalText };
	if (NoteText)
	{
		Widgets.Add(NoteText);
	}
	return Widgets;
}

void ULessonEndScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
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

			// One line per step: time, completion, hint penalty, lesson hint penalty, total (with the note)
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
				// After the last line's pause: fill the bar, if the puzzle added any points
				AnimState = Result.PointsEarned > 0 ? EAnimState::Filling : EAnimState::Done;
				if (AnimState == EAnimState::Done)
				{
					Finish();
				}
			}
			break;
		}

		case EAnimState::Filling:
		{
			FillElapsed = FMath::Min(FillElapsed + InDeltaTime, FillTime);
			const float Alpha = FillTime > 0.f ? FillElapsed / FillTime : 1.f;
			const float Points = FMath::Lerp(float(Result.PreviousPoints), float(Result.TotalPoints), Alpha);
			SetShownPoints(Points);
			PlaySound(FillSound);

			// The unlock appears when the bar reaches the stage
			if (Result.bStageUnlocked && Points >= float(UCampaignSubsystem::GetStagePointsRequired(NextStage)))
			{
				ShowUnlock();
			}
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

void ULessonEndScreenWidget::Finish()
{
	SetShownPoints(Result.TotalPoints);
	if (Result.bStageUnlocked || Result.bLessonCompleted)
	{
		ShowUnlock();
	}
	if (NextPuzzleText && NextPuzzleButton->GetVisibility() == ESlateVisibility::Visible)
	{
		NextPuzzleText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	SetButtonsEnabled(true);
}

void ULessonEndScreenWidget::ShowUnlock()
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

void ULessonEndScreenWidget::SetShownPoints(float Points)
{
	LessonProgress->SetPercent(Points / UCampaignSubsystem::MaxPoints);
	if (ProgressText)
	{
		ProgressText->SetText(FText::Format(LOCTEXT("Progress", "{0} / {1}"), FMath::FloorToInt(Points), UCampaignSubsystem::MaxPoints));
	}
}

void ULessonEndScreenWidget::SetButtonsEnabled(bool bEnabled)
{
	RestartButton->SetIsEnabled(bEnabled);
	ReturnButton->SetIsEnabled(bEnabled);
	NextPuzzleButton->SetIsEnabled(bEnabled);
}

void ULessonEndScreenWidget::PlaySound(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound, 1.f, 1.f, 0.f, nullptr, nullptr, true);
	}
}

FText ULessonEndScreenWidget::GetNoteText() const
{
	if (!Result.bSolved)
	{
		return LOCTEXT("NoPointsIncorrect", "Some icons are wrong. Restart to try again.");
	}
	if (!Result.bFirstFinish)
	{
		return LOCTEXT("AlreadyScored", "Already scored: no points added");
	}
	if (Result.Stage == UCampaignSubsystem::FinalStage)
	{
		return LOCTEXT("FinalSolved", "Final solved!");
	}
	if (Result.PointsEarned <= 0 && Result.Score > 0)
	{
		return LOCTEXT("Practice", "Practice: this stage no longer earns points");
	}
	return FText::GetEmpty();
}

FText ULessonEndScreenWidget::GetUnlockText() const
{
	if (Result.bLessonCompleted)
	{
		return LOCTEXT("LessonComplete", "Lesson complete!");
	}
	if (Result.bStageUnlocked)
	{
		return FText::Format(LOCTEXT("StageUnlocked", "{0} unlocked!"), UCampaignSubsystem::GetStageDisplayName(NextStage));
	}
	return FText::GetEmpty();
}

void ULessonEndScreenWidget::HandleRestartClicked()
{
	PlaySound(ButtonSound);
	Hide();
	OnRestartPuzzle.Broadcast();
}

void ULessonEndScreenWidget::HandleReturnClicked()
{
	PlaySound(ButtonSound);
	Hide();
	OnReturnToLessons.Broadcast();
}

void ULessonEndScreenWidget::HandleNextClicked()
{
	PlaySound(ButtonSound);
	Hide();
	OnNextPuzzle.Broadcast();
}

#undef LOCTEXT_NAMESPACE
