#include "UI/CampaignEndScreenWidget.h"
#include "HappinessClassic/CampaignTree.h"
#include "HappinessClassic/Puzzle.h"
#include "UI/LessonProgressTicks.h"
#include "UI/LessonEndScreenWidget.h"

#include "Ads/AdsSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Brushes/SlateNoResource.h"
#include "Components/BackgroundBlur.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "CampaignEndScreen"

namespace
{
	const FLinearColor CampaignInk(.17f,.085f,.035f);
	const FLinearColor CampaignGreen(.16f,.29f,.075f);
	const FLinearColor CampaignIvory(.98f,.96f,.87f);
}

UCampaignEndScreenWidget::UCampaignEndScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> Book(TEXT("/Game/General/Textures/T_LessonResultsNotebook.T_LessonResultsNotebook"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Buttons(TEXT("/Game/General/Textures/T_LessonNotebookButtons.T_LessonNotebookButtons"));
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	NotebookTexture = Book.Object;
	ButtonTexture = Buttons.Object;
	NotebookFont = Font.Object;
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

void UCampaignEndScreenWidget::BuildNotebookLayout()
{
	if (NotebookCanvas || !WidgetTree || !ResultText || !StageProgress) return;
	UCanvasPanel* Root = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Root || !NotebookTexture || !ButtonTexture || !NotebookFont) return;
	for (UWidget* W : {static_cast<UWidget*>(LessonText),static_cast<UWidget*>(ResultText),
		static_cast<UWidget*>(TimeText),static_cast<UWidget*>(ParTimeText),static_cast<UWidget*>(CompletionExpText),
		static_cast<UWidget*>(BonusExpText),static_cast<UWidget*>(TotalText),static_cast<UWidget*>(NoteText),
		static_cast<UWidget*>(ExpBar),static_cast<UWidget*>(LevelText),static_cast<UWidget*>(ExpText),
		static_cast<UWidget*>(StageProgress),static_cast<UWidget*>(StageLabel),static_cast<UWidget*>(ProgressTicks),
		static_cast<UWidget*>(ProgressText),static_cast<UWidget*>(UnlockText),static_cast<UWidget*>(NextPuzzleText),
		static_cast<UWidget*>(RestartButton),static_cast<UWidget*>(ReturnButton),static_cast<UWidget*>(NextPuzzleButton)})
		if (W) W->RemoveFromParent();
	Root->ClearChildren();
	UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>();
	Blur->SetBlurStrength(4.f); Blur->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* BlurSlot = Root->AddChildToCanvas(Blur);
	BlurSlot->SetAnchors(FAnchors(0,0,1,1)); BlurSlot->SetOffsets(FMargin(0)); BlurSlot->SetZOrder(0);
	UImage* Dim = WidgetTree->ConstructWidget<UImage>();
	Dim->SetColorAndOpacity(FLinearColor(0,0,0,.32f)); Dim->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim);
	DimSlot->SetAnchors(FAnchors(0,0,1,1)); DimSlot->SetOffsets(FMargin(0)); DimSlot->SetZOrder(1);
	UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>(); Fit->SetStretch(EStretch::ScaleToFit);
	UCanvasPanelSlot* FitSlot = Root->AddChildToCanvas(Fit);
	FitSlot->SetAnchors(FAnchors(0,0,1,1)); FitSlot->SetOffsets(FMargin(40)); FitSlot->SetZOrder(2);
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(1600); Size->SetHeightOverride(1000); Fit->AddChild(Size);
	NotebookCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(); Size->AddChild(NotebookCanvas);
	auto Place = [&](UWidget* W,FVector2D P,FVector2D S,int32 Z=1)
	{
		if (!W) return;
		UCanvasPanelSlot* Slot = NotebookCanvas->AddChildToCanvas(W);
		Slot->SetPosition(P); Slot->SetSize(S); Slot->SetZOrder(Z);
	};
	auto Face = [&](bool Primary)
	{
		FSlateBrush Brush; Brush.SetResourceObject(ButtonTexture); Brush.DrawAs=ESlateBrushDrawType::Image;
		Brush.SetUVRegion(FBox2f(FVector2f(.03f,Primary?.50f:.06f),FVector2f(.98f,Primary?.985f:.485f)));
		return Brush;
	};
	auto Image = [&](FSlateBrush Brush,FVector2D P,FVector2D S,int32 Z=1)
	{
		UImage* W=WidgetTree->ConstructWidget<UImage>(); W->SetBrush(Brush);
		W->SetVisibility(ESlateVisibility::HitTestInvisible); Place(W,P,S,Z); return W;
	};
	auto StyleText = [&](UTextBlock* W,int32 Points,FLinearColor Color,ETextJustify::Type Align=ETextJustify::Left)
	{
		if (!W) return;
		W->SetFont(FSlateFontInfo(NotebookFont,Points)); W->SetColorAndOpacity(Color);
		W->SetJustification(Align); W->SetShadowOffset(FVector2D::ZeroVector);
	};
	auto Label = [&](const FText& Text,FVector2D P,FVector2D S,int32 Points=34,ETextJustify::Type Align=ETextJustify::Left)
	{
		UTextBlock* W=WidgetTree->ConstructWidget<UTextBlock>(); W->SetText(Text);
		StyleText(W,Points,CampaignInk,Align); Place(W,P,S); return W;
	};
	UImage* Surface=WidgetTree->ConstructWidget<UImage>(); Surface->SetBrushFromTexture(NotebookTexture);
	Surface->SetVisibility(ESlateVisibility::HitTestInvisible); Place(Surface,{0,0},{1600,1000},0);
	Image(Face(false),{500,5},{600,110});
	StyleText(ResultText,56,CampaignInk,ETextJustify::Center); Place(ResultText,{550,18},{500,80},2);
	StyleText(LessonText,32,CampaignInk,ETextJustify::Center); LessonText->SetAutoWrapText(true);
	Place(LessonText,{900,120},{460,105});
	const FText Labels[]={LOCTEXT("PuzzleTime","Puzzle Time"),LOCTEXT("ParTime","Par Time"),
		LOCTEXT("CompletionExp","Completion EXP"),LOCTEXT("TimeBonus","Time Bonus"),LOCTEXT("TotalExp","Total EXP")};
	UTextBlock* Values[]={TimeText,ParTimeText,CompletionExpText,BonusExpText,TotalText};
	for (int32 I=0;I<5;++I)
	{
		const float Y=260.f+I*72.f+(I==4?24.f:0.f);
		Label(Labels[I],{255,Y},{350,65},I==4?42:34);
		StyleText(Values[I],I==4?46:38,I>=2?CampaignGreen:CampaignInk,ETextJustify::Right);
		Place(Values[I],{610,Y},{135,65});
		if (I<4) Image(FSlateRoundedBoxBrush(FLinearColor(.44f,.28f,.10f,I==3?.6f:.18f),0.f),{250,Y+66},{500,I==3?2.f:1.f});
	}
	if (NoteText)
	{
		StyleText(NoteText,23,CampaignInk); NoteText->SetAutoWrapText(true); NoteText->SetWrapTextAt(480);
		Place(NoteText,{270,658},{480,75});
	}
	ResultSeal=WidgetTree->ConstructWidget<ULessonResultGlyph>(); ResultSeal->Symbol=3;
	ResultSeal->SetVisibility(ESlateVisibility::HitTestInvisible); Place(ResultSeal,{1070,235},{120,120});
	Label(LOCTEXT("LevelLabel","Level"),{1030,370},{125,50},32,ETextJustify::Right);
	StyleText(LevelText,32,CampaignInk); Place(LevelText,{1170,370},{80,50});
	Image(FSlateRoundedBoxBrush(FLinearColor(.80f,.75f,.61f),17.f,FLinearColor(.65f,.42f,.12f),2.f),{905,420},{460,34});
	FProgressBarStyle BarStyle;
	BarStyle.BackgroundImage=FSlateNoResource(); BarStyle.FillImage=FSlateRoundedBoxBrush(FLinearColor::White,12.f);
	for (UProgressBar* Bar : {ExpBar,StageProgress})
	{
		Bar->SetWidgetStyle(BarStyle); Bar->SetFillColorAndOpacity(CampaignGreen);
		Bar->SetBarFillStyle(EProgressBarFillStyle::Scale); Bar->SetBorderPadding(FVector2D::ZeroVector);
	}
	Place(ExpBar,{910,425},{450,24},2);
	StyleText(ExpText,28,CampaignInk,ETextJustify::Center); Place(ExpText,{905,458},{460,42});
	Image(FSlateRoundedBoxBrush(FLinearColor(.80f,.75f,.61f),17.f,FLinearColor(.65f,.42f,.12f),2.f),{905,570},{460,34});
	StyleText(StageLabel,31,CampaignInk,ETextJustify::Center); Place(StageLabel,{905,512},{460,46});
	Place(StageProgress,{910,575},{450,24},2);
	if (ProgressTicks) { ProgressTicks->SetVisibility(ESlateVisibility::HitTestInvisible); Place(ProgressTicks,{910,575},{450,24},3); }
	StyleText(ProgressText,28,CampaignInk,ETextJustify::Center); Place(ProgressText,{905,607},{460,42});
	UnlockFrame=Image(Face(false),{875,650},{520,74});
	// One line, shrunk to fit the frame if it's ever too long
	if (UnlockText)
	{
		StyleText(UnlockText,28,CampaignGreen,ETextJustify::Center); UnlockText->SetAutoWrapText(false);
		UScaleBox* UnlockFit=WidgetTree->ConstructWidget<UScaleBox>(); UnlockFit->SetStretch(EStretch::ScaleToFit);
		UnlockFit->SetStretchDirection(EStretchDirection::DownOnly); UnlockFit->SetVisibility(ESlateVisibility::HitTestInvisible);
		UnlockFit->AddChild(UnlockText); Place(UnlockFit,{955,663},{360,50},2);	// between the frame's leaf ends
	}
	auto Action = [&](UButton* Button,const FText& Text,int32 Symbol,bool Primary,FVector2D P)
	{
		Button->ClearChildren(); FButtonStyle Style;
		Style.Normal=Style.Hovered=Style.Pressed=Style.Disabled=Face(Primary);
		Style.Hovered.TintColor=FLinearColor(1.08f,1.08f,1.04f);
		Style.Pressed.TintColor=FLinearColor(.83f,.88f,.78f); Style.Disabled.TintColor=FLinearColor(.77f,.76f,.68f);
		Style.NormalPadding=Style.PressedPadding=FMargin(24,8);
		Button->SetStyle(Style); Button->SetBackgroundColor(FLinearColor::White); Button->SetCursor(EMouseCursor::Hand);
		UHorizontalBox* Row=WidgetTree->ConstructWidget<UHorizontalBox>();
		USizeBox* IconSize=WidgetTree->ConstructWidget<USizeBox>(); IconSize->SetWidthOverride(30); IconSize->SetHeightOverride(30);
		ULessonResultGlyph* Glyph=WidgetTree->ConstructWidget<ULessonResultGlyph>();
		Glyph->Symbol=Symbol; Glyph->Color=Primary?CampaignIvory:CampaignGreen;
		Glyph->SetVisibility(ESlateVisibility::HitTestInvisible); IconSize->AddChild(Glyph);
		UHorizontalBoxSlot* IconSlot=Row->AddChildToHorizontalBox(IconSize);
		IconSlot->SetVerticalAlignment(VAlign_Center); IconSlot->SetPadding(FMargin(0,0,12,0));
		UTextBlock* TextWidget=WidgetTree->ConstructWidget<UTextBlock>(); TextWidget->SetText(Text);
		StyleText(TextWidget,Symbol==2?22:31,Primary?CampaignIvory:CampaignInk,ETextJustify::Center);
		Row->AddChildToHorizontalBox(TextWidget)->SetVerticalAlignment(VAlign_Center);
		UButtonSlot* ContentSlot=CastChecked<UButtonSlot>(Button->AddChild(Row));
		ContentSlot->SetHorizontalAlignment(HAlign_Center); ContentSlot->SetVerticalAlignment(VAlign_Center);
		Place(Button,P,{380,95});
	};
	Action(RestartButton,LOCTEXT("RestartAction","Restart Puzzle"),0,false,{210,845});
	Action(ReturnButton,LOCTEXT("CampaignAction","Return to Campaign"),2,false,{610,845});
	Action(NextPuzzleButton,LOCTEXT("NextAction","Next Puzzle"),1,true,{1010,845});
	StyleText(NextPuzzleText,26,CampaignInk,ETextJustify::Center); Place(NextPuzzleText,{1010,805},{380,40});
}

void UCampaignEndScreenWidget::UpdateNotebookActions()
{
	if (!NotebookCanvas) return;
	const bool bNext=NextPuzzleButton->GetVisibility()!=ESlateVisibility::Collapsed;
	if (UCanvasPanelSlot* LayoutSlot=Cast<UCanvasPanelSlot>(RestartButton->Slot)) LayoutSlot->SetPosition({bNext?210.f:410.f,845.f});
	if (UCanvasPanelSlot* LayoutSlot=Cast<UCanvasPanelSlot>(ReturnButton->Slot)) LayoutSlot->SetPosition({bNext?610.f:810.f,845.f});
	if (UnlockFrame) UnlockFrame->SetVisibility(UnlockText && !UnlockText->GetText().IsEmpty() && UnlockText->GetVisibility()!=ESlateVisibility::Hidden
		? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}

void UCampaignEndScreenWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	BuildNotebookLayout();
	if (!GetGameInstance() && NotebookCanvas)
	{
		ResultText->SetText(LOCTEXT("PreviewSuccess","Success!"));
		LessonText->SetText(LOCTEXT("PreviewStage","Game Basics\n5x5 Normal"));
		TimeText->SetText(FText::FromString(TEXT("01:23")));
		ParTimeText->SetText(FText::FromString(TEXT("02:10")));
		CompletionExpText->SetText(FText::FromString(TEXT("+96")));
		BonusExpText->SetText(FText::FromString(TEXT("+12")));
		TotalText->SetText(FText::FromString(TEXT("+108")));
		LevelText->SetText(LOCTEXT("PreviewLevel","17"));
		ExpBar->SetPercent(.27f); ExpText->SetText(FText::FromString(TEXT("123 / 456 EXP")));
		StageLabel->SetText(LOCTEXT("PreviewCampaign","Campaign"));
		StageProgress->SetPercent(.47f); ProgressText->SetText(FText::FromString(TEXT("7 / 15")));
		UnlockText->SetText(LOCTEXT("PreviewUnlock","5x5 Hard unlocked!"));
		NextPuzzleText->SetText(LOCTEXT("PreviewNext","5x5 Hard"));
		if (NoteText) NoteText->SetText(FText::GetEmpty());
		NextPuzzleButton->SetVisibility(ESlateVisibility::Visible);
		SetVisibility(ESlateVisibility::Visible);
	}
	UpdateNotebookActions();
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

	LessonText->SetText(FText::Format(LOCTEXT("LessonStage", "{0}\n{1}"),
		UCampaignTree::GetLessonDisplayName(Result.Lesson), UCampaignSubsystem::GetStageDisplayName(Result.Stage)));

	ResultText->SetText(Result.bSolved ? LOCTEXT("Solved", "Success!") : LOCTEXT("Incorrect", "Incorrect"));
	ResultText->SetColorAndOpacity(Result.bSolved ? CampaignInk : FLinearColor(.58f,.16f,.10f));
	if (ResultSeal) { ResultSeal->Symbol=Result.bSolved?3:4; ResultSeal->InvalidateLayoutAndVolatility(); }

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
	UpdateNotebookActions();
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
	// Never an empty frame
	if (UnlockFrame) UnlockFrame->SetVisibility(UnlockText && !UnlockText->GetText().IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
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
		return LOCTEXT("ClueMastered", "Mastered!");
	}
	// Clearing the final. Master Mode isn't mentioned: the player finds it by reopening the clue from the tree
	if (Result.bLessonCompleted)
	{
		return LOCTEXT("ClueCompleted", "Completed!");
	}
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
