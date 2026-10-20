#include "UI/FreePlayEndScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Brushes/SlateNoResource.h"
#include "Components/BackgroundBlur.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ButtonSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"


#include "UI/LessonEndScreenWidget.h"
#include "HappinessClassic/Puzzle.h"
#include "HappinessClassic/FreePlaySettings.h"
#include "GameFramework/PlayerController.h"
#include "Components/Widget.h"

#define LOCTEXT_NAMESPACE "FreePlayResults"

namespace
{
	const FLinearColor Ink(.17f,.085f,.035f), Sage(.16f,.29f,.075f), Ivory(1.f,.95f,.8f);
}

UFreePlayEndScreenWidget::UFreePlayEndScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> Book(TEXT("/Game/General/Textures/T_LessonResultsNotebook.T_LessonResultsNotebook"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Buttons(TEXT("/Game/General/Textures/T_LessonNotebookButtons.T_LessonNotebookButtons"));
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	NotebookTexture=Book.Object; ButtonTexture=Buttons.Object; NotebookFont=Font.Object;
	// The end screens' fill and reward sounds
	static ConstructorHelpers::FObjectFinder<USoundBase> Count(TEXT("/Game/General/Audio/Cues/GameAction5.GameAction5"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Bonus(TEXT("/Game/General/Audio/Cues/Happiness.Happiness"));
	CountSound = Count.Object;
	BonusSound = Bonus.Object;
}

void UFreePlayEndScreenWidget::BuildNotebookLayout()
{
	if (NotebookCanvas) return;
	auto Text=[&](const TCHAR* Name) { return Cast<UTextBlock>(GetWidgetFromName(Name)); };
	UTextBlock* StageText=Text(TEXT("TextBlock_41"));
	UTextBlock* ResultText=Text(TEXT("TextBlock_76"));
	UTextBlock* PuzzleTimeText=Text(TEXT("PuzzleTime"));
	UTextBlock* ParTimeText=Text(TEXT("ParTime"));
	UTextBlock* CompletionExpText=Text(TEXT("CompletionExp"));
	UTextBlock* TimeBonusText=Text(TEXT("BonusExp"));
	UTextBlock* TotalText=Text(TEXT("TotalExp"));
	UTextBlock* ProgressText=Text(TEXT("TextBlock_10"));
	UTextBlock* UnlockText=Text(TEXT("UnlockedText"));
	UProgressBar* ExpProgress=Cast<UProgressBar>(GetWidgetFromName(TEXT("ExpBar")));
	UButton* RestartButton=Cast<UButton>(GetWidgetFromName(TEXT("Button_RestartPuzzle")));
	UButton* MenuButton=Cast<UButton>(GetWidgetFromName(TEXT("Button_MainMenu")));
	UButton* NextPuzzleButton=Cast<UButton>(GetWidgetFromName(TEXT("Button_NextPuzzle")));
	UTextBlock* LevelValue=Text(TEXT("TextBlock_8"));
	UTextBlock* DailyBonus=Text(TEXT("DailyBonusText"));
	UTextBlock* DailyStreak=Text(TEXT("DailyStreakText"));
	for(UWidget* W:{static_cast<UWidget*>(LevelValue),static_cast<UWidget*>(DailyBonus),static_cast<UWidget*>(DailyStreak)})
		if(W) W->RemoveFromParent();
	if (!WidgetTree || !ResultText || !ExpProgress) return;
	UCanvasPanel* Root=Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Root) return;
	for (UWidget* W : {static_cast<UWidget*>(StageText),static_cast<UWidget*>(ResultText),
		static_cast<UWidget*>(PuzzleTimeText),static_cast<UWidget*>(ParTimeText),static_cast<UWidget*>(CompletionExpText),
		static_cast<UWidget*>(TimeBonusText),static_cast<UWidget*>(TotalText),
		static_cast<UWidget*>(ExpProgress),static_cast<UWidget*>(ProgressText),static_cast<UWidget*>(UnlockText),
		static_cast<UWidget*>(RestartButton),static_cast<UWidget*>(MenuButton),
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
		W->SetFont(FSlateFontInfo(NotebookFont,Points));
		W->ColorAndOpacityDelegate.Unbind(); W->SetColorAndOpacity(Color);
		W->SetJustification(Align); W->SetShadowOffset(FVector2D::ZeroVector);
	};
	auto Label=[&](const FText& Text,FVector2D P,FVector2D S,int32 Points=32,FLinearColor Color=Ink,ETextJustify::Type Align=ETextJustify::Left)
	{
		UTextBlock* W=WidgetTree->ConstructWidget<UTextBlock>(); W->SetText(Text);
		StyleText(W,Points,Color,Align); Place(W,P,S); return W;
	};
	UImage* Surface=WidgetTree->ConstructWidget<UImage>(); Surface->SetBrushFromTexture(NotebookTexture);
	Surface->SetVisibility(ESlateVisibility::HitTestInvisible); Place(Surface,{0,0},{1600,1000},0);
	Image(Face(false),{500,5},{600,110});
	StyleText(ResultText,56,Ink,ETextJustify::Center); Place(ResultText,{550,18},{500,80},2);
	StyleText(StageText,34,Ink,ETextJustify::Center); Place(StageText,{850,130},{540,75});
	StageText->SetAutoWrapText(true);
	StageText->SetText(LOCTEXT("FreePlay","Free Play"));
	const FText Labels[]={LOCTEXT("TimeLabel","Puzzle Time"),LOCTEXT("CompletionLabel","Par Time"),
		LOCTEXT("HintsLabel","Completion EXP"),LOCTEXT("LessonHintLabel","Time Bonus"),LOCTEXT("TotalLabel","Total EXP")};
	UTextBlock* Values[]={PuzzleTimeText,ParTimeText,CompletionExpText,TimeBonusText,TotalText};
	for (int32 I=0;I<5;++I)
	{
		const float Y=245.f+I*72.f+(I==4?20.f:0.f);
		Label(Labels[I],{280,Y},{300,65},I==4?42:32);
		StyleText(Values[I],I==4?46:38,I>=2?Sage:Ink,ETextJustify::Right);
		Place(Values[I],{585,Y},{160,65});
		if (I<4) Image(FSlateRoundedBoxBrush(FLinearColor(.44f,.28f,.10f,I==3?.6f:.18f),0.f),{275,Y+66},{480,I==3?2.f:1.f});
	}
	if (UCanvasPanelSlot* TimeSlot=Cast<UCanvasPanelSlot>(PuzzleTimeText->Slot))
	{
		TimeSlot->SetPosition({535,245}); TimeSlot->SetSize({210,65});
	}
	if (UCanvasPanelSlot* ParSlot=Cast<UCanvasPanelSlot>(ParTimeText->Slot))
	{ ParSlot->SetPosition({535,317}); ParSlot->SetSize({210,65}); }
	ResultSeal=WidgetTree->ConstructWidget<ULessonResultGlyph>(); ResultSeal->Symbol=3;
	ResultSeal->SetVisibility(ESlateVisibility::HitTestInvisible); Place(ResultSeal,{1015,210},{210,210});
	UHorizontalBox* LevelRow=WidgetTree->ConstructWidget<UHorizontalBox>();
	UTextBlock* LevelCaption=WidgetTree->ConstructWidget<UTextBlock>();
	LevelCaption->SetText(LOCTEXT("LevelLabel","Level")); StyleText(LevelCaption,36,Ink);
	StyleText(LevelValue,36,Ink);
	LevelRow->AddChildToHorizontalBox(LevelCaption)->SetPadding(FMargin(0,0,16,0));
	LevelRow->AddChildToHorizontalBox(LevelValue);
	UOverlay* LevelGroup=WidgetTree->ConstructWidget<UOverlay>();
	LevelGroup->AddChildToOverlay(LevelRow)->SetHorizontalAlignment(HAlign_Center);
	Place(LevelGroup,{880,435},{480,55});
	// Keep the frame separate: ProgressBar border padding clips its fill instead of insetting it.
	Image(FSlateRoundedBoxBrush(FLinearColor(.80f,.75f,.61f),17.f,FLinearColor(.65f,.42f,.12f),2.f),
		{890,500},{460,34});
	FProgressBarStyle ProgressStyle;
	ProgressStyle.BackgroundImage=FSlateNoResource();
	ProgressStyle.FillImage=FSlateRoundedBoxBrush(FLinearColor::White,12.f);
	ExpProgress->SetWidgetStyle(ProgressStyle);
	ExpProgress->SetFillColorAndOpacity(FLinearColor(.30f,.47f,.12f));
	ExpProgress->SetBarFillStyle(EProgressBarFillStyle::Scale);
	ExpProgress->SetBorderPadding(FVector2D::ZeroVector);
	Place(ExpProgress,{895,505},{450,24},2);
	StyleText(ProgressText,30,Ink,ETextJustify::Center); Place(ProgressText,{890,545},{460,50});
	UnlockFrame=Image(Face(false),{870,620},{500,90});
	StyleText(UnlockText,29,Sage,ETextJustify::Center); Place(UnlockText,{920,638},{400,60},2);
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
		Glyph->Symbol=Symbol; Glyph->Color=Primary?Ivory:Sage; Glyph->SetVisibility(ESlateVisibility::HitTestInvisible);
		IconSize->AddChild(Glyph);
		UHorizontalBoxSlot* IconSlot=Row->AddChildToHorizontalBox(IconSize);
		IconSlot->SetVerticalAlignment(VAlign_Center); IconSlot->SetPadding(FMargin(0,0,12,0));
		UTextBlock* TextWidget=WidgetTree->ConstructWidget<UTextBlock>(); TextWidget->SetText(Text);
		StyleText(TextWidget,Symbol==0?29:32,Primary?Ivory:Ink,ETextJustify::Center);
		Row->AddChildToHorizontalBox(TextWidget)->SetVerticalAlignment(VAlign_Center);
		UButtonSlot* ContentSlot=CastChecked<UButtonSlot>(Button->AddChild(Row));
		ContentSlot->SetHorizontalAlignment(HAlign_Center); ContentSlot->SetVerticalAlignment(VAlign_Center);
		Place(Button,Position,{380,95});
	};
	Action(RestartButton,LOCTEXT("RestartAction","Restart Puzzle"),0,false,{210,845});
	Action(MenuButton,LOCTEXT("MenuAction","Main Menu"),2,false,{610,845});
	Action(NextPuzzleButton,LOCTEXT("NextAction","Next Puzzle"),1,true,{1010,845});
	DailyFrame=Image(Face(false),{285,635},{465,95});
	StyleText(DailyBonus,27,Sage,ETextJustify::Center); Place(DailyBonus,{325,644},{385,45},2);
	StyleText(DailyStreak,22,Ink,ETextJustify::Center); Place(DailyStreak,{325,685},{385,40},2);
}



void UFreePlayEndScreenWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	BuildNotebookLayout();
	if (!GetGameInstance() && NotebookCanvas)
	{
		const TPair<const TCHAR*,const TCHAR*> Samples[]={
			{TEXT("TextBlock_76"),TEXT("Success!")},{TEXT("TextBlock_41"),TEXT("5x5 Normal")},
			{TEXT("PuzzleTime"),TEXT("04:12")},{TEXT("ParTime"),TEXT("06:00")},
			{TEXT("CompletionExp"),TEXT("+100")},{TEXT("BonusExp"),TEXT("+30")},
			{TEXT("TotalExp"),TEXT("+260")},{TEXT("TextBlock_8"),TEXT("12")},
			{TEXT("TextBlock_10"),TEXT("540 / 600 EXP")},{TEXT("UnlockedText"),TEXT("New icon set unlocked!")},
			{TEXT("DailyBonusText"),TEXT("x2 Daily Bonus!")},{TEXT("DailyStreakText"),TEXT("Daily streak: 7 days")}};
		for(const auto& Sample:Samples)
			if(UTextBlock* Text=Cast<UTextBlock>(GetWidgetFromName(Sample.Key)))
			{
				Text->SetText(FText::FromString(Sample.Value));
				Text->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		if(UProgressBar* Bar=Cast<UProgressBar>(GetWidgetFromName(TEXT("ExpBar")))) Bar->SetPercent(.9f);
		SetVisibility(ESlateVisibility::Visible);
	}
	UpdateNotebookResult();
}

void UFreePlayEndScreenWidget::UpdateNotebookResult()
{
	if (!NotebookCanvas) return;
	auto Text=[&](const TCHAR* Name) { return Cast<UTextBlock>(GetWidgetFromName(Name)); };
	UTextBlock* Unlock=Text(TEXT("UnlockedText"));
	if(UnlockFrame) UnlockFrame->SetVisibility(Unlock && Unlock->IsVisible() && !Unlock->GetText().IsEmpty()
		? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	UTextBlock* Bonus=Text(TEXT("DailyBonusText"));
	UTextBlock* Streak=Text(TEXT("DailyStreakText"));
	if(DailyFrame) DailyFrame->SetVisibility((Bonus && Bonus->IsVisible()) || (Streak && Streak->IsVisible())
		? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	if(UTextBlock* Result=Text(TEXT("TextBlock_76")))
	{
		const bool bIncorrect=GetGameInstance() && !UFreePlaySubsystem::GetLastFreePlayScore(this).bSolved;
		if(ResultSeal) ResultSeal->Symbol=bIncorrect?4:3;
		Result->SetColorAndOpacity(bIncorrect?FLinearColor(.55f,.075f,.025f):Ink);
	}
	// The existing Blueprint owns the scoring and reveals; only the subtitle comes from the active board.
	APlayerController* PC=UGameplayStatics::GetPlayerController(this,0);
	auto ObjectProperty=[](UObject* Owner,const TCHAR* Name)->UObject*
	{
		const FObjectPropertyBase* Property=Owner?FindFProperty<FObjectPropertyBase>(Owner->GetClass(),Name):nullptr;
		return Property?Property->GetObjectPropertyValue_InContainer(Owner):nullptr;
	};
	if(UPuzzle* Puzzle=Cast<UPuzzle>(ObjectProperty(ObjectProperty(PC,TEXT("HappinessWidget")),TEXT("ThePuzzle"))))
	{
		const FText Difficulties[]={LOCTEXT("Easy","Easy"),LOCTEXT("Normal","Normal"),LOCTEXT("Hard","Hard")};
		if(UTextBlock* Stage=Text(TEXT("TextBlock_41")))
			Stage->SetText(FText::Format(LOCTEXT("PuzzleStage","{0}x{0} {1}"),Puzzle->m_iSize,
				Difficulties[FMath::Clamp(Puzzle->m_iDifficulty,0,2)]));
	}
}

void UFreePlayEndScreenWidget::StartDailyBonus(int32 InMultiplier)
{
	Multiplier = InMultiplier;
	State = Multiplier > 1 ? EBonusState::WaitingForTotal : EBonusState::None;
	Elapsed = 0.f;
}

void UFreePlayEndScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateNotebookResult();

	switch (State)
	{
		case EBonusState::WaitingForTotal:
		{
			// The Blueprint reveals Total EXP as its last line
			const UWidget* Total = GetWidgetFromName(TEXT("TotalExp"));
			if (Total && Total->IsVisible())
			{
				State = EBonusState::Delay;
				Elapsed = 0.f;
			}
			break;
		}

		case EBonusState::Delay:
			Elapsed += InDeltaTime;
			if (Elapsed >= BonusDelay)
			{
				BaseTotal = GetTotalExp();
				State = EBonusState::Counting;
				Elapsed = 0.f;
			}
			break;

		case EBonusState::Counting:
		{
			Elapsed += InDeltaTime;
			const float Alpha = BonusCountTime > 0.f ? FMath::Min(Elapsed / BonusCountTime, 1.f) : 1.f;
			const int32 Doubled = BaseTotal * Multiplier;
			SetTotalExp(FMath::RoundToInt(FMath::Lerp(float(BaseTotal), float(Doubled), Alpha)), false);
			PlaySound(CountSound);
			if (Alpha >= 1.f)
			{
				// The level bar takes the doubled amount
				SetTotalExp(Doubled, true);
				if (UWidget* Bonus = GetWidgetFromName(TEXT("DailyBonusText")))
				{
					Bonus->SetVisibility(ESlateVisibility::HitTestInvisible);
				}
				PlaySound(BonusSound);
				State = EBonusState::None;
			}
			break;
		}

		default:
			break;
	}
}

int32 UFreePlayEndScreenWidget::GetTotalExp() const
{
	if (const FIntProperty* Gained = FindFProperty<FIntProperty>(GetClass(), TEXT("TotalExpGained")))
	{
		return Gained->GetPropertyValue_InContainer(this);
	}
	if (const FNumericProperty* Gained = FindFProperty<FNumericProperty>(GetClass(), TEXT("TotalExpGained")))
	{
		return int32(Gained->GetFloatingPointPropertyValue(Gained->ContainerPtrToValuePtr<void>(this)));
	}
	return 0;
}

void UFreePlayEndScreenWidget::SetTotalExp(int32 Shown, bool bAlsoGained)
{
	if (const FStrProperty* Text = FindFProperty<FStrProperty>(GetClass(), TEXT("TotalExpString")))
	{
		Text->SetPropertyValue_InContainer(this, FString::FromInt(Shown));
	}
	if (!bAlsoGained)
	{
		return;
	}
	if (const FIntProperty* Gained = FindFProperty<FIntProperty>(GetClass(), TEXT("TotalExpGained")))
	{
		Gained->SetPropertyValue_InContainer(this, Shown);
	}
	else if (const FNumericProperty* Number = FindFProperty<FNumericProperty>(GetClass(), TEXT("TotalExpGained")))
	{
		Number->SetFloatingPointPropertyValue(Number->ContainerPtrToValuePtr<void>(this), double(Shown));
	}
}

void UFreePlayEndScreenWidget::PlaySound(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound, 1.f, 1.f, 0.f, nullptr, nullptr, true);
	}
}

#undef LOCTEXT_NAMESPACE
