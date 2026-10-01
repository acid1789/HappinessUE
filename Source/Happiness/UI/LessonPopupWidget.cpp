#include "UI/LessonPopupWidget.h"
#include "HappinessClassic/CampaignProgress.h"
#include "HappinessClassic/CampaignTree.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "LessonPopup"

void ULessonStageClick::HandleClicked()
{
	if (ULessonPopupWidget* Owner = Popup.Get())
	{
		Owner->ChooseStage(Stage);
	}
}

ULessonPopupWidget::ULessonPopupWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	StageButtonStyle = GetDefault<UButton>()->GetStyle();
	StageFont = FCoreStyle::GetDefaultFontStyle("Bold", 16);
}

void ULessonPopupWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// Also runs in the designer, where there is no campaign progress: shows an empty lesson
	BuildStageButtons();
	Refresh();
}

void ULessonPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PlayButton)
	{
		PlayButton->OnClicked.AddUniqueDynamic(this, &ULessonPopupWidget::HandlePlayClicked);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &ULessonPopupWidget::HandleCloseClicked);
	}
	if (UCampaignSubsystem* Campaign = GetCampaign())
	{
		Campaign->OnProgressChanged.AddUniqueDynamic(this, &ULessonPopupWidget::HandleProgressChanged);
	}
}

void ULessonPopupWidget::NativeDestruct()
{
	if (UCampaignSubsystem* Campaign = GetCampaign())
	{
		Campaign->OnProgressChanged.RemoveDynamic(this, &ULessonPopupWidget::HandleProgressChanged);
	}
	Super::NativeDestruct();
}

UCampaignSubsystem* ULessonPopupWidget::GetCampaign() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UCampaignSubsystem>() : nullptr;
}

void ULessonPopupWidget::BuildStageButtons()
{
	if (!StageBox || !WidgetTree)
	{
		return;
	}

	StageBox->ClearChildren();
	StageButtons.Reset();
	StageClicks.Reset();

	// The row is sized for five stages (Lessons). More stages (Campaign) share the same width, with each name on
	// two lines ("5x5" over "Normal") so it still fits its narrower button.
	const int32 NumStages = UCampaignSubsystem::GetNumStages();
	constexpr int32 FittedStages = 5;
	constexpr float Spacing = 12.f;
	const bool bNarrow = NumStages > FittedStages;
	const float ButtonWidth = bNarrow ? FittedStages * (StageButtonSize.X + Spacing) / NumStages - Spacing : StageButtonSize.X;

	for (int32 Stage = 0; Stage < NumStages; Stage++)
	{
		// A size box per button keeps them a fixed size whatever panel StageBox is
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Size->SetWidthOverride(ButtonWidth);
		Size->SetHeightOverride(StageButtonSize.Y);

		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Button->SetStyle(StageButtonStyle);
		Size->AddChild(Button);

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		const FText StageName = UCampaignSubsystem::GetStageDisplayName(Stage);
		Label->SetText(bNarrow ? FText::FromString(StageName.ToString().Replace(TEXT(" "), TEXT("\n"))) : StageName);
		Label->SetJustification(ETextJustify::Center);
		Label->SetColorAndOpacity(StageTextColor);
		if (StageFont.HasValidFont())
		{
			Label->SetFont(StageFont);
		}
		if (UButtonSlot* LabelSlot = Cast<UButtonSlot>(Button->AddChild(Label)))
		{
			LabelSlot->SetHorizontalAlignment(HAlign_Center);
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}

		UPanelSlot* StageSlot = StageBox->AddChild(Size);
		if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(StageSlot))
		{
			BoxSlot->SetPadding(FMargin(Spacing / 2.f, 0.f));
			BoxSlot->SetVerticalAlignment(VAlign_Center);
		}

		ULessonStageClick* Click = NewObject<ULessonStageClick>(this);
		Click->Stage = Stage;
		Click->Popup = this;
		Button->OnClicked.AddDynamic(Click, &ULessonStageClick::HandleClicked);

		StageButtons.Add(Button);
		StageClicks.Add(Click);
	}
}

void ULessonPopupWidget::ShowLesson(ECampaignLesson InLesson)
{
	Lesson = InLesson;
	Refresh();
	SetVisibility(ESlateVisibility::Visible);
}

void ULessonPopupWidget::Refresh()
{
	// Lessons and Campaign have different stages
	if (StageButtons.Num() != UCampaignSubsystem::GetNumStages())
	{
		BuildStageButtons();
	}

	const bool bCampaignMode = UCampaignSubsystem::GetMode() == ECampaignMode::Campaign;
	const UCampaignSubsystem* Campaign = GetCampaign();
	const int32 Points = Campaign ? Campaign->GetLessonPoints(Lesson) : 0;
	const int32 CurrentStage = Campaign ? Campaign->GetCurrentStage(Lesson) : 0;
	const bool bCompleted = Campaign && Campaign->IsLessonCompleted(Lesson);

	if (TitleText)
	{
		TitleText->SetText(UCampaignTree::GetLessonDisplayName(Lesson));
	}
	if (DescriptionText)
	{
		DescriptionText->SetText(UCampaignTree::GetLessonDescription(Lesson));
	}
	if (LessonProgress)
	{
		LessonProgress->SetPercent(float(Points) / UCampaignSubsystem::GetMaxPoints());
	}
	if (ProgressText)
	{
		const FText StageText = !bCompleted ? UCampaignSubsystem::GetStageDisplayName(CurrentStage)
			: (bCampaignMode ? LOCTEXT("CampaignComplete", "Complete") : LOCTEXT("LessonComplete", "Lesson complete"));
		ProgressText->SetText(FText::Format(bCampaignMode
				? LOCTEXT("CampaignProgressFormat", "{0} / {1} puzzles  -  {2}")
				: LOCTEXT("ProgressFormat", "{0} / {1} points  -  {2}"),
			Points, UCampaignSubsystem::GetMaxPoints(), StageText));
	}

	// Once the final is done there is nothing left to progress: a single button back to the tree
	// (stages, including the final, can still be replayed from the stage buttons)
	if (PlayButton)
	{
		PlayButton->SetVisibility(bCompleted ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (CloseLabel)
	{
		CloseLabel->SetText(!bCompleted ? LOCTEXT("Close", "Close")
			: (bCampaignMode ? LOCTEXT("ReturnToCampaign", "Return to Campaign") : LOCTEXT("ReturnToLessons", "Return to Lessons")));
	}

	for (int32 Stage = 0; Stage < StageButtons.Num(); Stage++)
	{
		const bool bUnlocked = Campaign ? Campaign->IsStageUnlocked(Lesson, Stage) : Stage == 0;
		const bool bIsCurrent = Stage == CurrentStage && !(Stage == UCampaignSubsystem::GetFinalStage() && bCompleted);

		StageButtons[Stage]->SetIsEnabled(bUnlocked);
		StageButtons[Stage]->SetBackgroundColor(!bUnlocked ? LockedTint : (bIsCurrent ? CurrentTint : PassedTint));
	}
}

void ULessonPopupWidget::ChooseStage(int32 Stage)
{
	UCampaignSubsystem* Campaign = GetCampaign();
	if (!Campaign || !Campaign->IsStageUnlocked(Lesson, Stage))
	{
		return;
	}

	Campaign->RequestLessonPuzzle(Lesson, Stage);
	OnStageChosen.Broadcast(Lesson, Stage);
}

void ULessonPopupWidget::Close()
{
	SetVisibility(ESlateVisibility::Collapsed);
	OnClosed.Broadcast();
}

void ULessonPopupWidget::HandlePlayClicked()
{
	if (const UCampaignSubsystem* Campaign = GetCampaign())
	{
		ChooseStage(Campaign->GetCurrentStage(Lesson));
	}
}

void ULessonPopupWidget::HandleCloseClicked()
{
	Close();
}

void ULessonPopupWidget::HandleProgressChanged()
{
	Refresh();
}

#undef LOCTEXT_NAMESPACE
