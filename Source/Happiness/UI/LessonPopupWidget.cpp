#include "UI/LessonPopupWidget.h"
#include "HappinessClassic/CampaignProgress.h"
#include "HappinessClassic/CampaignTree.h"
#include "UI/LessonProgressTicks.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Border.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/OverlaySlot.h"
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
	HideEraser();
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
	MasterButtons.Reset();
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
		const FText StageName = UCampaignSubsystem::GetStageDisplayName(Stage);
		StageButtons.Add(AddStageButton(StageBox, Stage, bNarrow ? FText::FromString(StageName.ToString().Replace(TEXT(" "), TEXT("\n"))) : StageName,
			ButtonWidth, Spacing));
	}

	// Master Mode's stages in their own row, full width, "Master" over "8x8 Normal"
	if (MasterBox)
	{
		MasterBox->ClearChildren();
		const int32 FirstMaster = UCampaignSubsystem::GetFirstMasterStage();
		for (int32 Stage = FirstMaster; Stage < FirstMaster + UCampaignSubsystem::GetNumMasterStages(); Stage++)
		{
			FString Name = UCampaignSubsystem::GetStageDisplayName(Stage).ToString();
			int32 Space = INDEX_NONE;
			if (Name.FindChar(TEXT(' '), Space))
			{
				Name[Space] = TEXT('\n');
			}
			MasterButtons.Add(AddStageButton(MasterBox, Stage, FText::FromString(Name), StageButtonSize.X, Spacing));
		}
	}
}

UButton* ULessonPopupWidget::AddStageButton(UPanelWidget* Box, int32 Stage, const FText& Name, float Width, float Spacing)
{
	// A size box per button keeps them a fixed size whatever panel the row is
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Size->SetWidthOverride(Width);
	Size->SetHeightOverride(StageButtonSize.Y);

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->SetStyle(StageButtonStyle);
	Size->AddChild(Button);

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetText(Name);
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

	UPanelSlot* StageSlot = Box->AddChild(Size);
	if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(StageSlot))
	{
		BoxSlot->SetPadding(FMargin(Spacing / 2.f, 0.f));
		BoxSlot->SetVerticalAlignment(VAlign_Center);
	}

	ULessonStageClick* Click = NewObject<ULessonStageClick>(this);
	Click->Stage = Stage;
	Click->Popup = this;
	Button->OnClicked.AddDynamic(Click, &ULessonStageClick::HandleClicked);
	StageClicks.Add(Click);
	return Button;
}

bool ULessonPopupWidget::IsShowingMaster() const
{
	// Not while the reveal animation runs: until it ends the popup shows the completed campaign
	const UCampaignSubsystem* Campaign = GetCampaign();
	return Campaign && Campaign->IsMasterRevealed(Lesson) && RevealPhase == ERevealPhase::None;
}

FText ULessonPopupWidget::GetMasterProgressText() const
{
	const UCampaignSubsystem* Campaign = GetCampaign();
	if (!Campaign)
	{
		return FText::GetEmpty();
	}
	const FText StageText = Campaign->IsMasterCompleted(Lesson) ? LOCTEXT("MasterComplete", "Mastered")
		: UCampaignSubsystem::GetStageDisplayName(Campaign->GetCurrentMasterStage(Lesson));
	return FText::Format(LOCTEXT("MasterProgressFormat", "Master  {0} / {1}  -  {2}"),
		Campaign->GetMasterPoints(Lesson), UCampaignSubsystem::GetMasterMaxPoints(), StageText);
}

void ULessonPopupWidget::ShowLesson(ECampaignLesson InLesson)
{
	// The player opening a cleared clue is what first shows its Master Mode, with the reveal animation. It's
	// saved as shown right away, so closing the popup mid-animation just shows Master Mode next time.
	bool bReveal = false;
	if (UCampaignSubsystem* Campaign = GetCampaign())
	{
		bReveal = Campaign->IsMasterUnlocked(InLesson) && !Campaign->IsMasterRevealed(InLesson);
		Campaign->RevealMaster(InLesson);
	}
	RevealPhase = bReveal ? ERevealPhase::Drain : ERevealPhase::None;
	RevealElapsed = 0.f;
	HideEraser();
	ShowLessonAfterPuzzle(InLesson);
}

void ULessonPopupWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (RevealPhase != ERevealPhase::None)
	{
		TickReveal(InDeltaTime);
	}
}

void ULessonPopupWidget::TickReveal(float DeltaTime)
{
	RevealElapsed += DeltaTime;
	switch (RevealPhase)
	{
		case ERevealPhase::Drain:
		{
			const float Alpha = RevealDrainTime > 0.f ? FMath::Min(RevealElapsed / RevealDrainTime, 1.f) : 1.f;
			if (LessonProgress)
			{
				LessonProgress->SetPercent(1.f - Alpha);
			}
			if (Alpha >= 1.f)
			{
				RevealPhase = ERevealPhase::Erase;
				RevealElapsed = 0.f;
				if (EraseCover)
				{
					// Painted in the panel's color, so covering the text erases it
					FLinearColor Cover = Panel ? Panel->GetBrushColor() : FLinearColor::White;
					Cover.A = 1.f;
					EraseCover->SetColorAndOpacity(Cover);
					EraseCover->SetVisibility(ESlateVisibility::HitTestInvisible);
				}
				if (Eraser)
				{
					Eraser->SetColorAndOpacity(EraserColor);
					Eraser->SetVisibility(ESlateVisibility::HitTestInvisible);
				}
				SetEraseProgress(0.f);
			}
			break;
		}

		case ERevealPhase::Erase:
		{
			const float Alpha = RevealEraseTime > 0.f ? FMath::Min(RevealElapsed / RevealEraseTime, 1.f) : 1.f;
			SetEraseProgress(Alpha);
			if (Alpha >= 1.f)
			{
				// The line is gone: write the Master line in its place
				RevealPhase = ERevealPhase::Text;
				RevealElapsed = 0.f;
				HideEraser();
				if (ProgressText)
				{
					ProgressText->SetText(GetMasterProgressText());
				}
			}
			break;
		}

		case ERevealPhase::Text:
			if (RevealElapsed >= RevealTextHold)
			{
				// Then the Master buttons and the Master bar
				RevealPhase = ERevealPhase::None;
				Refresh();
			}
			break;

		default:
			break;
	}
}

void ULessonPopupWidget::SetEraseProgress(float Alpha)
{
	// The eraser travels from the line's right edge to its left; the cover fills in behind it (to its right)
	const UWidget* Line = EraseCover ? EraseCover->GetParent() : nullptr;
	const FVector2D Size = Line ? Line->GetCachedGeometry().GetLocalSize() : FVector2D::ZeroVector;
	const float Travel = FMath::Max(Size.X - EraserWidth, 0.f) * Alpha;
	if (EraseCover)
	{
		EraseCover->SetDesiredSizeOverride(FVector2D(Alpha >= 1.f ? Size.X : Travel + EraserWidth * 0.5f, Size.Y));
	}
	if (Eraser)
	{
		Eraser->SetDesiredSizeOverride(FVector2D(EraserWidth, Size.Y));
		if (UOverlaySlot* EraserSlot = Cast<UOverlaySlot>(Eraser->Slot))
		{
			EraserSlot->SetPadding(FMargin(0.f, 0.f, Travel, 0.f));
		}
	}
}

void ULessonPopupWidget::HideEraser()
{
	if (EraseCover)
	{
		EraseCover->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Eraser)
	{
		Eraser->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ULessonPopupWidget::ShowLessonAfterPuzzle(ECampaignLesson InLesson)
{
	Lesson = InLesson;
	Refresh();
	SetVisibility(ESlateVisibility::Visible);
}

void ULessonPopupWidget::Refresh()
{
	// Lessons and Campaign have different stages
	if (StageButtons.Num() != UCampaignSubsystem::GetNumStages()
		|| (MasterBox && MasterButtons.Num() != UCampaignSubsystem::GetNumMasterStages()))
	{
		BuildStageButtons();
	}

	const bool bCampaignMode = UCampaignSubsystem::GetMode() == ECampaignMode::Campaign;
	const UCampaignSubsystem* Campaign = GetCampaign();
	const int32 Points = Campaign ? Campaign->GetLessonPoints(Lesson) : 0;
	const int32 CurrentStage = Campaign ? Campaign->GetCurrentStage(Lesson) : 0;
	const bool bCompleted = Campaign && Campaign->IsLessonCompleted(Lesson);

	// Once the final is done (Campaign), the popup follows Master Mode: its bar, text and Play button
	const bool bMaster = IsShowingMaster();
	const bool bMasterDone = bMaster && Campaign->IsMasterCompleted(Lesson);
	const int32 MasterPoints = bMaster ? Campaign->GetMasterPoints(Lesson) : 0;
	const int32 MasterMax = UCampaignSubsystem::GetMasterMaxPoints();
	const int32 CurrentMaster = bMaster ? Campaign->GetCurrentMasterStage(Lesson) : INDEX_NONE;
	const bool bAllDone = bCompleted && (!bMaster || bMasterDone);

	if (TitleText)
	{
		TitleText->SetText(UCampaignTree::GetLessonDisplayName(Lesson));
	}
	if (DescriptionText)
	{
		DescriptionText->SetText(UCampaignTree::GetLessonDescription(Lesson));
	}
	// While the reveal erases the line and shows the Master line, the animation owns the bar and the line
	const bool bRevealOwnsLine = RevealPhase == ERevealPhase::Erase || RevealPhase == ERevealPhase::Text;
	if (LessonProgress && !bRevealOwnsLine)
	{
		LessonProgress->SetPercent(bMaster ? float(MasterPoints) / MasterMax : float(Points) / UCampaignSubsystem::GetMaxPoints());
	}
	if (ProgressTicks)
	{
		if (bMaster)
		{
			TArray<float> Ticks;
			const int32 FirstMaster = UCampaignSubsystem::GetFirstMasterStage();
			for (int32 Stage = FirstMaster + 1; Stage < FirstMaster + UCampaignSubsystem::GetNumMasterStages(); Stage++)
			{
				Ticks.Add(float(UCampaignSubsystem::GetMasterStagePointsRequired(Stage)) / MasterMax);
			}
			ProgressTicks->SetTickFractions(Ticks);
		}
		else
		{
			ProgressTicks->ClearTickFractions();
		}
	}
	if (ProgressText && !bRevealOwnsLine && bMaster)
	{
		ProgressText->SetText(GetMasterProgressText());
	}
	else if (ProgressText && !bRevealOwnsLine)
	{
		const FText StageText = !bCompleted ? UCampaignSubsystem::GetStageDisplayName(CurrentStage)
			: (bCampaignMode ? LOCTEXT("CampaignComplete", "Complete") : LOCTEXT("LessonComplete", "Lesson complete"));
		ProgressText->SetText(FText::Format(bCampaignMode
				? LOCTEXT("CampaignProgressFormat", "{0} / {1} puzzles  -  {2}")
				: LOCTEXT("ProgressFormat", "{0} / {1} points  -  {2}"),
			Points, UCampaignSubsystem::GetMaxPoints(), StageText));
	}

	// Once everything is done (the final, and Master Mode in Campaign) there is nothing left to progress: a single
	// button back to the tree (every stage can still be replayed from the stage buttons)
	if (PlayButton)
	{
		PlayButton->SetVisibility(bAllDone ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (CloseLabel)
	{
		CloseLabel->SetText(!bAllDone ? LOCTEXT("Close", "Close")
			: (bCampaignMode ? LOCTEXT("ReturnToCampaign", "Return to Campaign") : LOCTEXT("ReturnToLessons", "Return to Lessons")));
	}
	if (MasterBox)
	{
		MasterBox->SetVisibility(bMaster ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	const int32 FirstMaster = UCampaignSubsystem::GetFirstMasterStage();
	for (int32 i = 0; i < MasterButtons.Num(); i++)
	{
		const int32 Stage = FirstMaster + i;
		const bool bUnlocked = Campaign && Campaign->IsStageUnlocked(Lesson, Stage);
		const bool bIsCurrent = Stage == CurrentMaster && !bMasterDone;
		MasterButtons[i]->SetIsEnabled(bUnlocked);
		MasterButtons[i]->SetBackgroundColor(!bUnlocked ? LockedTint : (bIsCurrent ? CurrentTint : PassedTint));
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
	RevealPhase = ERevealPhase::None;
	HideEraser();
	SetVisibility(ESlateVisibility::Collapsed);
	OnClosed.Broadcast();
}

void ULessonPopupWidget::HandlePlayClicked()
{
	if (const UCampaignSubsystem* Campaign = GetCampaign())
	{
		ChooseStage(IsShowingMaster() ? Campaign->GetCurrentMasterStage(Lesson) : Campaign->GetCurrentStage(Lesson));
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
