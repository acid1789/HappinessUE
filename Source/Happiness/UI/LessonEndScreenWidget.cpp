#include "UI/LessonEndScreenWidget.h"
#include "HappinessClassic/CampaignTree.h"
#include "HappinessClassic/Puzzle.h"

#include "Ads/AdsSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Brushes/SlateNoResource.h"
#include "UI/LessonProgressTicks.h"
#include "Components/BackgroundBlur.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ButtonSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "LessonEndScreen"

namespace
{
	const FLinearColor LessonInk(.17f,.085f,.035f), LessonSage(.16f,.29f,.075f), LessonIvory(1.f,.95f,.8f);
	const FLinearColor PenaltyInk(.55f,.075f,.025f);
	class SLessonResultGlyph : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SLessonResultGlyph) {} SLATE_END_ARGS()
		void Construct(const FArguments&, ULessonResultGlyph* InOwner) { Owner=InOwner; }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(32,32); }
		virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,
			FSlateWindowElementList& E,int32 Layer,const FWidgetStyle&,bool) const override
		{
			const ULessonResultGlyph* Glyph=Owner.Get();
			if (!Glyph) return Layer;
			const float Scale=FMath::Min(G.GetLocalSize().X,G.GetLocalSize().Y)/32.f;
			const FVector2f Origin=(FVector2f(G.GetLocalSize())-FVector2f(32*Scale))*.5f;
			auto Line=[&](TArray<FVector2f> P,float Width,FLinearColor C,int32 Z=0)
			{
				for (FVector2f& V:P) V=Origin+V*Scale;
				FSlateDrawElement::MakeLines(E,Layer+Z,G.ToPaintGeometry(),P,ESlateDrawEffect::None,C,true,Width*Scale);
			};
			if (Glyph->Symbol>=3)
			{
				auto Disc=[&](float R,FLinearColor C,int32 Z)
				{
					FSlateRoundedBoxBrush Brush(C,R*Scale);
					FSlateDrawElement::MakeBox(E,Layer+Z,
						G.ToPaintGeometry(FVector2f(R*2*Scale),FSlateLayoutTransform(Origin+FVector2f((16-R)*Scale))),
						&Brush,ESlateDrawEffect::None,C);
				};
				Disc(15.8f,FLinearColor(.12f,.065f,.025f,.3f),0);
				Disc(15.3f,FLinearColor(.88f,.48f,.065f),1);
				Disc(14.6f,LessonIvory,2);
				Disc(13.7f,FLinearColor(.62f,.32f,.035f),3);
				Disc(13.2f,Glyph->Symbol==3?FLinearColor(.018f,.065f,.13f):FLinearColor(.35f,.065f,.025f),4);
				Disc(12.2f,Glyph->Symbol==3?FLinearColor(.032f,.105f,.20f):FLinearColor(.48f,.11f,.045f),5);
				if (Glyph->Symbol==3)
				{
					// One filled silhouette: joined line segments leave a notch at this scale.
					const TArray<FVector2f> Check={{6.8f,15.5f},{9,13.3f},{13,17.3f},{23,7.3f},{25.2f,9.5f},{13,21.7f}};
					const TArray<SlateIndex> Indices={2,3,4,2,4,5,2,5,0,2,0,1};
					const FSlateResourceHandle Resource=FSlateApplication::Get().GetRenderer()->GetResourceHandle(
						*FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")));
					auto FillCheck=[&](FVector2f Offset,FLinearColor Color,int32 Z)
					{
						TArray<FSlateVertex> Vertices;
						for (const FVector2f& Point:Check)
							Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(G.GetAccumulatedRenderTransform(),
								Origin+(Point+Offset)*Scale,FVector2f(.5f,.5f),Color.ToFColor(true)));
						FSlateDrawElement::MakeCustomVerts(E,Layer+Z,Resource,Vertices,Indices,nullptr,0,0);
					};
					FillCheck({0,.55f},FLinearColor(.006f,.018f,.04f),6);
					FillCheck({0,0},LessonIvory,7);
				}
				else
				{
					Line({{11,11},{21,21}},2.3f,LessonIvory,7);
					Line({{21,11},{11,21}},2.3f,LessonIvory,7);
				}
			}
			else if (Glyph->Symbol==0)
			{
				TArray<FVector2f> Arc;
				for(int32 I=0;I<=28;++I)
				{
					const float A=FMath::DegreesToRadians(-135.f+I*300.f/28.f);
					Arc.Add(FVector2f(16+11*FMath::Cos(A),17+11*FMath::Sin(A)));
				}
				Line(Arc,2.8f,Glyph->Color);
				Line({{8,3},{8,11},{16,11}},2.8f,Glyph->Color);
			}
			else if (Glyph->Symbol==1)
			{
				for(int32 Y=4;Y<=28;++Y)
					Line({{8,float(Y)},{28.f-FMath::Abs(Y-16.f)*20.f/12.f,float(Y)}},1.5f,Glyph->Color);
			}
			else Line({{23,5},{11,16},{23,27}},3.5f,Glyph->Color);
			return Layer+7;
		}
	private:
		TWeakObjectPtr<ULessonResultGlyph> Owner;
	};
}

TSharedRef<SWidget> ULessonResultGlyph::RebuildWidget()
{
	return SNew(SLessonResultGlyph,this);
}


ULessonEndScreenWidget::ULessonEndScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> Book(TEXT("/Game/General/Textures/T_LessonResultsNotebook.T_LessonResultsNotebook"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Buttons(TEXT("/Game/General/Textures/T_LessonNotebookButtons.T_LessonNotebookButtons"));
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	NotebookTexture=Book.Object;
	ButtonTexture=Buttons.Object;
	NotebookFont=Font.Object;
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


void ULessonEndScreenWidget::BuildNotebookLayout()
{
	if (NotebookCanvas || !WidgetTree || !ResultText || !LessonProgress) return;
	UCanvasPanel* Root=Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Root) return;
	for (UWidget* W : {static_cast<UWidget*>(LessonText),static_cast<UWidget*>(ResultText),
		static_cast<UWidget*>(TimeText),static_cast<UWidget*>(CompletionText),static_cast<UWidget*>(HintPenaltyText),
		static_cast<UWidget*>(LessonHintPenaltyText),static_cast<UWidget*>(TotalText),static_cast<UWidget*>(NoteText),
		static_cast<UWidget*>(LessonProgress),static_cast<UWidget*>(ProgressText),static_cast<UWidget*>(UnlockText),
		static_cast<UWidget*>(NextPuzzleText),static_cast<UWidget*>(RestartButton),static_cast<UWidget*>(ReturnButton),
		static_cast<UWidget*>(NextPuzzleButton)})
		if (W) W->RemoveFromParent();
	Root->ClearChildren();
	UBackgroundBlur* Blur=WidgetTree->ConstructWidget<UBackgroundBlur>();
	Blur->SetBlurStrength(4.f);
	Blur->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* BlurSlot=Root->AddChildToCanvas(Blur);
	BlurSlot->SetAnchors(FAnchors(0,0,1,1)); BlurSlot->SetOffsets(FMargin(0)); BlurSlot->SetZOrder(0);
	UImage* Dim=WidgetTree->ConstructWidget<UImage>();
	Dim->SetColorAndOpacity(FLinearColor(0,0,0,.32f));
	Dim->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* DimSlot=Root->AddChildToCanvas(Dim);
	DimSlot->SetAnchors(FAnchors(0,0,1,1)); DimSlot->SetOffsets(FMargin(0)); DimSlot->SetZOrder(1);
	UScaleBox* Fit=WidgetTree->ConstructWidget<UScaleBox>();
	Fit->SetStretch(EStretch::ScaleToFit);
	UCanvasPanelSlot* FitSlot=Root->AddChildToCanvas(Fit);
	FitSlot->SetAnchors(FAnchors(0,0,1,1)); FitSlot->SetOffsets(FMargin(40)); FitSlot->SetZOrder(2);
	USizeBox* Size=WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(1600); Size->SetHeightOverride(1000);
	Fit->AddChild(Size);
	NotebookCanvas=WidgetTree->ConstructWidget<UCanvasPanel>(); Size->AddChild(NotebookCanvas);
	auto Place=[&](UWidget* W,FVector2D P,FVector2D S,int32 Z=1)
	{
		if (!W) return;
		UCanvasPanelSlot* LayoutSlot=NotebookCanvas->AddChildToCanvas(W);
		LayoutSlot->SetPosition(P); LayoutSlot->SetSize(S); LayoutSlot->SetZOrder(Z);
	};
	auto Face=[&](bool Primary)
	{
		FSlateBrush Brush; Brush.SetResourceObject(ButtonTexture); Brush.DrawAs=ESlateBrushDrawType::Image;
		Brush.SetUVRegion(FBox2f(FVector2f(.03f,Primary?.54f:.06f),FVector2f(.98f,Primary?.963f:.485f)));
		return Brush;
	};
	auto Image=[&](FSlateBrush Brush,FVector2D P,FVector2D S)
	{
		UImage* W=WidgetTree->ConstructWidget<UImage>(); W->SetBrush(Brush);
		W->SetVisibility(ESlateVisibility::HitTestInvisible); Place(W,P,S); return W;
	};
	auto StyleText=[&](UTextBlock* W,int32 Points,FLinearColor Color,ETextJustify::Type Align=ETextJustify::Left)
	{
		if (!W) return;
		W->SetFont(FSlateFontInfo(NotebookFont,Points)); W->SetColorAndOpacity(Color);
		W->SetJustification(Align); W->SetShadowOffset(FVector2D::ZeroVector);
	};
	auto Label=[&](const FText& Text,FVector2D P,FVector2D S,int32 Points=32,FLinearColor Color=LessonInk,ETextJustify::Type Align=ETextJustify::Left)
	{
		UTextBlock* W=WidgetTree->ConstructWidget<UTextBlock>(); W->SetText(Text);
		StyleText(W,Points,Color,Align); Place(W,P,S); return W;
	};
	UImage* Surface=WidgetTree->ConstructWidget<UImage>(); Surface->SetBrushFromTexture(NotebookTexture);
	Surface->SetVisibility(ESlateVisibility::HitTestInvisible); Place(Surface,{0,0},{1600,1000},0);
	Image(Face(false),{500,5},{600,110});
	StyleText(ResultText,56,LessonInk,ETextJustify::Center); Place(ResultText,{550,18},{500,80},2);
	StyleText(LessonText,34,LessonInk,ETextJustify::Center); Place(LessonText,{850,130},{540,75});
	LessonText->SetAutoWrapText(true);
	const FText Labels[]={LOCTEXT("TimeLabel","Time"),LOCTEXT("CompletionLabel","Completion"),
		LOCTEXT("HintsLabel","Hints"),LOCTEXT("LessonHintLabel","Lesson Hint"),LOCTEXT("TotalLabel","Total")};
	UTextBlock* Values[]={TimeText,CompletionText,HintPenaltyText,LessonHintPenaltyText,TotalText};
	for (int32 I=0;I<5;++I)
	{
		const float Y=245.f+I*72.f+(I==4?20.f:0.f);
		Label(Labels[I],{280,Y},{330,65},I==4?42:34);
		StyleText(Values[I],I==4?46:38,I==1||I==4?LessonSage:LessonInk,ETextJustify::Right);
		Place(Values[I],{620,Y},{125,65});
		if (I<4) Image(FSlateRoundedBoxBrush(FLinearColor(.44f,.28f,.10f,I==3?.6f:.18f),0.f),{275,Y+66},{480,I==3?2.f:1.f});
	}
	if (UCanvasPanelSlot* TimeSlot=Cast<UCanvasPanelSlot>(TimeText->Slot))
	{
		TimeSlot->SetPosition({535,245}); TimeSlot->SetSize({210,65});
	}
	if (NoteText)
	{
		StyleText(NoteText,22,LessonInk); NoteText->SetAutoWrapText(true); NoteText->SetWrapTextAt(410);
		Place(NoteText,{335,625},{410,85});
	}
	ResultSeal=WidgetTree->ConstructWidget<ULessonResultGlyph>(); ResultSeal->Symbol=3;
	ResultSeal->SetVisibility(ESlateVisibility::HitTestInvisible); Place(ResultSeal,{1015,210},{210,210});
	Label(LOCTEXT("ProgressLabel","Lesson Progress"),{880,435},{480,55},36,LessonInk,ETextJustify::Center);
	// Keep the frame separate: ProgressBar border padding clips its fill instead of insetting it.
	Image(FSlateRoundedBoxBrush(FLinearColor(.80f,.75f,.61f),17.f,FLinearColor(.65f,.42f,.12f),2.f),
		{890,500},{460,34});
	FProgressBarStyle ProgressStyle;
	ProgressStyle.BackgroundImage=FSlateNoResource();
	ProgressStyle.FillImage=FSlateRoundedBoxBrush(FLinearColor::White,12.f);
	LessonProgress->SetWidgetStyle(ProgressStyle);
	LessonProgress->SetFillColorAndOpacity(FLinearColor(.30f,.47f,.12f));
	LessonProgress->SetBarFillStyle(EProgressBarFillStyle::Scale);
	LessonProgress->SetBorderPadding(FVector2D::ZeroVector);
	Place(LessonProgress,{895,505},{450,24},2);
	ULessonProgressTicks* StageTicks=WidgetTree->ConstructWidget<ULessonProgressTicks>();
	StageTicks->TickColor=FLinearColor(.40f,.28f,.11f,.4f);
	StageTicks->TickThickness=1.2f;
	StageTicks->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(StageTicks,{895,509},{450,16},3);
	StageTicks->SynchronizeProperties();
	StyleText(ProgressText,30,LessonInk,ETextJustify::Center); Place(ProgressText,{890,545},{460,50});
	UnlockFrame=Image(Face(false),{870,620},{500,90});
	StyleText(UnlockText,29,LessonSage,ETextJustify::Center); Place(UnlockText,{920,638},{400,60},2);
	if (UnlockText) { UnlockText->SetAutoWrapText(true); UnlockText->SetWrapTextAt(400); }
	auto Action=[&](UButton* Button,const FText& Text,int32 Symbol,bool Primary,FVector2D Position)
	{
		if (!Button) return;
		Button->ClearChildren(); FButtonStyle Style;
		Style.Normal=Style.Hovered=Style.Pressed=Style.Disabled=Face(Primary);
		Style.Hovered.TintColor=FLinearColor(1.08f,1.08f,1.04f);
		Style.Pressed.TintColor=FLinearColor(.83f,.88f,.78f); Style.Disabled.TintColor=FLinearColor(.77f,.76f,.68f);
		Style.NormalPadding=Style.PressedPadding=FMargin(30,8);
		Button->SetStyle(Style); Button->SetBackgroundColor(FLinearColor::White); Button->SetCursor(EMouseCursor::Hand);
		UHorizontalBox* Row=WidgetTree->ConstructWidget<UHorizontalBox>();
		USizeBox* IconSize=WidgetTree->ConstructWidget<USizeBox>(); IconSize->SetWidthOverride(30); IconSize->SetHeightOverride(30);
		ULessonResultGlyph* Glyph=WidgetTree->ConstructWidget<ULessonResultGlyph>();
		Glyph->Symbol=Symbol; Glyph->Color=Primary?LessonIvory:LessonSage; Glyph->SetVisibility(ESlateVisibility::HitTestInvisible);
		IconSize->AddChild(Glyph);
		UHorizontalBoxSlot* IconSlot=Row->AddChildToHorizontalBox(IconSize);
		IconSlot->SetVerticalAlignment(VAlign_Center); IconSlot->SetPadding(FMargin(0,0,12,0));
		UTextBlock* TextWidget=WidgetTree->ConstructWidget<UTextBlock>(); TextWidget->SetText(Text);
		StyleText(TextWidget,Symbol==0?29:32,Primary?LessonIvory:LessonInk,ETextJustify::Center);
		Row->AddChildToHorizontalBox(TextWidget)->SetVerticalAlignment(VAlign_Center);
		UButtonSlot* ContentSlot=CastChecked<UButtonSlot>(Button->AddChild(Row));
		ContentSlot->SetHorizontalAlignment(HAlign_Center); ContentSlot->SetVerticalAlignment(VAlign_Center);
		Place(Button,Position,{380,95});
	};
	Action(RestartButton,LOCTEXT("RestartAction","Restart Puzzle"),0,false,{210,845});
	Action(ReturnButton,LOCTEXT("LessonsAction","Lessons"),2,false,{610,845});
	Action(NextPuzzleButton,LOCTEXT("NextAction","Next Puzzle"),1,true,{1010,845});
	StyleText(NextPuzzleText,27,LessonInk,ETextJustify::Center); Place(NextPuzzleText,{1010,795},{380,45});
}

void ULessonEndScreenWidget::UpdateNotebookActions()
{
	if (!NotebookCanvas) return;
	const bool bNext=NextPuzzleButton->GetVisibility()!=ESlateVisibility::Collapsed;
	if (UCanvasPanelSlot* LayoutSlot=Cast<UCanvasPanelSlot>(RestartButton->Slot)) LayoutSlot->SetPosition({bNext?210.f:410.f,845});
	if (UCanvasPanelSlot* LayoutSlot=Cast<UCanvasPanelSlot>(ReturnButton->Slot)) LayoutSlot->SetPosition({bNext?610.f:810.f,845});
}

void ULessonEndScreenWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	BuildNotebookLayout();
	// Render previews don't have a campaign; sample values never record progress.
	if (!GetGameInstance() && NotebookCanvas)
	{
		ResultText->SetText(LOCTEXT("PreviewSuccess","Success!"));
		LessonText->SetText(LOCTEXT("PreviewStage","Span - 3x3 Normal"));
		TimeText->SetText(FText::FromString(TEXT("2:14")));
		CompletionText->SetText(FText::FromString(TEXT("+3")));
		HintPenaltyText->SetText(FText::FromString(TEXT("-1"))); HintPenaltyText->SetColorAndOpacity(PenaltyInk);
		LessonHintPenaltyText->SetText(FText::FromString(TEXT("-0")));
		TotalText->SetText(FText::FromString(TEXT("+2")));
		if (NoteText) NoteText->SetText(FText::GetEmpty());
		SetShownPoints(18);
		if (UnlockText) UnlockText->SetText(LOCTEXT("PreviewUnlock","4x4 Easy unlocked!"));
		if (NextPuzzleText) NextPuzzleText->SetText(LOCTEXT("PreviewNext","4x4 Easy"));
		for (UWidget* W:GetRevealedWidgets()) W->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UnlockText) UnlockText->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (NextPuzzleText) NextPuzzleText->SetVisibility(ESlateVisibility::HitTestInvisible);
		NextPuzzleButton->SetVisibility(ESlateVisibility::Visible);
		SetVisibility(ESlateVisibility::Visible);
	}
	UpdateNotebookActions();
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

	ResultText->SetText(Result.bSolved ? LOCTEXT("Solved", "Success!") : LOCTEXT("Incorrect", "Incorrect"));
	ResultText->SetColorAndOpacity(Result.bSolved ? LessonInk : PenaltyInk);
	if (ResultSeal) ResultSeal->Symbol=Result.bSolved?3:4;
	if (UnlockFrame) UnlockFrame->SetVisibility(ESlateVisibility::Hidden);

	// The score lines add up to the total: completion 3, -1 for using hints, another -1 for a lesson-clue hint
	const int32 Completion = Result.bSolved ? UCampaignSubsystem::GetMaxPuzzleScore() : 0;
	const int32 HintPenalty = Result.bUsedHints ? -1 : 0;
	const int32 LessonHintPenalty = Result.bUsedLessonHint ? -1 : 0;
	TimeText->SetText(FText::FromString(Puzzle->FormatTimeString(PuzzleSeconds)));
	CompletionText->SetText(Completion>0 ? FText::Format(LOCTEXT("PositiveScore", "+{0}"),Completion) : FText::AsNumber(Completion));
	// Penalties always show their minus sign, so -0 says no cost was paid
	const auto PenaltyText = [](int32 Penalty) { return FText::Format(LOCTEXT("Penalty", "-{0}"), FText::AsNumber(FMath::Abs(Penalty))); };
	HintPenaltyText->SetText(PenaltyText(HintPenalty));
	LessonHintPenaltyText->SetText(PenaltyText(LessonHintPenalty));
	const int32 Total=Result.bSolved?Result.Score:0;
	TotalText->SetText(Total>0 ? FText::Format(LOCTEXT("PositiveScore", "+{0}"),Total) : FText::AsNumber(Total));
	HintPenaltyText->SetColorAndOpacity(HintPenalty<0?PenaltyInk:LessonInk);
	LessonHintPenaltyText->SetColorAndOpacity(LessonHintPenalty<0?PenaltyInk:LessonInk);
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
	UpdateNotebookActions();
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
	if (UnlockFrame) UnlockFrame->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UnlockText)
	{
		UnlockText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	PlaySound(UnlockSound);
}

void ULessonEndScreenWidget::SetShownPoints(float Points)
{
	LessonProgress->SetPercent(Points / UCampaignSubsystem::GetMaxPoints());
	if (ProgressText)
	{
		ProgressText->SetText(FText::Format(LOCTEXT("Progress", "{0} / {1} points"), FMath::FloorToInt(Points), UCampaignSubsystem::GetMaxPoints()));
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
	if (Result.Stage == UCampaignSubsystem::GetFinalStage())
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

void ULessonEndScreenWidget::PlayAdThen(TFunction<void()> Action)
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

void ULessonEndScreenWidget::HandleRestartClicked()
{
	PlaySound(ButtonSound);
	Hide();
	PlayAdThen([this]() { OnRestartPuzzle.Broadcast(); });
}

void ULessonEndScreenWidget::HandleReturnClicked()
{
	PlaySound(ButtonSound);
	Hide();
	PlayAdThen([this]() { OnReturnToLessons.Broadcast(); });
}

void ULessonEndScreenWidget::HandleNextClicked()
{
	PlaySound(ButtonSound);
	Hide();
	PlayAdThen([this]() { OnNextPuzzle.Broadcast(); });
}

#undef LOCTEXT_NAMESPACE
