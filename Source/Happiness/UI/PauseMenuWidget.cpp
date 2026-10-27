#include "UI/PauseMenuWidget.h"
#include "HappinessClassic/CampaignProgress.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"
#include "UI/LessonEndScreenWidget.h"
#include "UI/LessonPopupWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "HappinessClassic/Puzzle.h"
#include "UI/GameRulesWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "PauseGarden"

namespace
{
	const FLinearColor GardenInk(.17f,.085f,.035f), GardenSage(.16f,.29f,.075f), GardenIvory(1.f,.95f,.8f);
	class SPauseMenuGlyph : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPauseMenuGlyph) {} SLATE_END_ARGS()
		void Construct(const FArguments&,UPauseMenuGlyph* InOwner) { Owner=InOwner; }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(32,32); }
		virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,
			FSlateWindowElementList& E,int32 Layer,const FWidgetStyle&,bool) const override
		{
			const UPauseMenuGlyph* Glyph=Owner.Get();
			if(!Glyph) return Layer;
			const float Scale=FMath::Min(G.GetLocalSize().X,G.GetLocalSize().Y)/32.f;
			const FVector2f Origin=(FVector2f(G.GetLocalSize())-FVector2f(32*Scale))*.5f;
			auto Line=[&](TArray<FVector2f> Points,float Width=2.4f)
			{
				for(FVector2f& Point:Points) Point=Origin+Point*Scale;
				FSlateDrawElement::MakeLines(E,Layer,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,Glyph->Color,true,Width*Scale);
			};
			auto Circle=[&](float X,float Y,float R,float Width=2.4f)
			{
				TArray<FVector2f> Points;
				for(int32 I=0;I<=40;++I)
				{ const float A=I*2.f*PI/40.f; Points.Add({X+R*FMath::Cos(A),Y+R*FMath::Sin(A)}); }
				Line(Points,Width);
			};
			// Filled silhouettes with a transparent edge fringe stay smooth at phone scale.
			auto Fill=[&](const TArray<FVector2f>& Points,const TArray<SlateIndex>& Triangles)
			{
				TArray<FSlateVertex> Vertices;
				TArray<SlateIndex> Indices=Triangles;
				const int32 Count=Points.Num();
				const FColor Solid=Glyph->Color.ToFColor(true);
				FColor Clear=Solid; Clear.A=0;
				const float Feather=.8f/FMath::Max(.1f,G.GetAccumulatedLayoutTransform().GetScale());
				for(const FVector2f& Point:Points)
					Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(G.GetAccumulatedRenderTransform(),
						Origin+Point*Scale,FVector2f(.5f,.5f),Solid));
				for(int32 I=0;I<Count;++I)
				{
					const FVector2f Before=(Points[I]-Points[(I+Count-1)%Count]).GetSafeNormal();
					const FVector2f After=(Points[(I+1)%Count]-Points[I]).GetSafeNormal();
					const FVector2f N1(Before.Y,-Before.X),N2(After.Y,-After.X);
					const FVector2f Normal=(N1+N2).GetSafeNormal();
					const FVector2f Outer=Origin+Points[I]*Scale+Normal*Feather/FMath::Max(.3f,FVector2f::DotProduct(Normal,N2));
					Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(G.GetAccumulatedRenderTransform(),Outer,FVector2f(.5f,.5f),Clear));
					const SlateIndex A=I,B=(I+1)%Count;
					Indices.Append({A,B,SlateIndex(B+Count),A,SlateIndex(B+Count),SlateIndex(A+Count)});
				}
				const FSlateResourceHandle Resource=FSlateApplication::Get().GetRenderer()->GetResourceHandle(*FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")));
				FSlateDrawElement::MakeCustomVerts(E,Layer,Resource,Vertices,Indices,nullptr,0,0);
			};
			switch(Glyph->Symbol)
			{
			case 0: Circle(16,16,12); Line({{16,7},{16,16},{22,19}},2.5f); break;
			case 1:
				Line({{7,26},{21,12}},5.f); Circle(7,26,2,1.5f);
				Line({{19,4},{16,8},{18,14},{24,16},{29,12},{25,12},{22,9},{22,4}},3.f); break;
			case 2:
				Line({{3,16},{8,10},{16,7},{24,10},{29,16},{24,22},{16,25},{8,22},{3,16}});
				Circle(16,16,4,4.f); break;
			case 3:
				Circle(16,16,8,4.f);
				for(int32 I=0;I<8;++I)
				{
					const float A=I*PI/4.f;
					Line({{16+8*FMath::Cos(A),16+8*FMath::Sin(A)},
						{16+13*FMath::Cos(A),16+13*FMath::Sin(A)}},5.f);
				} break;
			case 4:
				Fill({{3,14},{16,3},{29,14},{24,14},{24,28},{19,28},{19,20},{13,20},{13,28},{8,28},{8,14}},{10,0,1,1,2,3,10,1,3,3,4,5,3,5,6,10,3,6,10,6,7,10,7,8,8,9,10}); break;
			case 5:
				Fill({{4,4},{17,4},{17,7},{7,7},{7,25},{17,25},{17,28},{4,28}},{0,1,2,0,2,3,7,0,3,7,3,4,7,4,5,5,6,7});
				Fill({{13,14},{24,14},{24,10},{31,16},{24,22},{24,18},{13,18}},{6,0,1,1,2,3,6,1,3,3,4,5,3,5,6}); break;
			case 8:
				Fill({{8,4},{28,16},{8,28}},{0,1,2}); break;
			}
			return Layer;
		}
	private:
		TWeakObjectPtr<UPauseMenuGlyph> Owner;
	};
}

TSharedRef<SWidget> UPauseMenuGlyph::RebuildWidget()
{
	return SNew(SPauseMenuGlyph,this);
}

UPauseMenuWidget::UPauseMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> Panel(TEXT("/Game/General/Textures/T_PauseGardenPanel.T_PauseGardenPanel"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Buttons(TEXT("/Game/General/Textures/T_LessonNotebookButtons.T_LessonNotebookButtons"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Quit(TEXT("/Game/General/Textures/T_PauseQuitButton.T_PauseQuitButton"));
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	GardenTexture=Panel.Object; ButtonTexture=Buttons.Object; QuitTexture=Quit.Object; GardenFont=Font.Object;
	// The old pause menu's sounds and Options screen
	static ConstructorHelpers::FObjectFinder<USoundBase> Accept(TEXT("/Game/General/Audio/Cues/MenuAccept.MenuAccept"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Cancel(TEXT("/Game/General/Audio/Cues/MenuCancel.MenuCancel"));
	AcceptSound = Accept.Object;
	CancelSound = Cancel.Object;
	OptionsClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/Happiness/UI/WBP_Options.WBP_Options_C")));
	RulesClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/Happiness/UI/WBP_GameRules.WBP_GameRules_C")));
}


void UPauseMenuWidget::BuildGardenLayout()
{
	if(GardenCanvas || !WidgetTree || !ResumeButton) return;
	UCanvasPanel* Root=Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if(!Root) return;
	UTextBlock* Title=Cast<UTextBlock>(GetWidgetFromName(TEXT("Title")));
	UBorder* Blocker=Cast<UBorder>(GetWidgetFromName(TEXT("InputBlocker")));
	for(UWidget* W:{static_cast<UWidget*>(Title),static_cast<UWidget*>(TimeText),static_cast<UWidget*>(Blocker),
		static_cast<UWidget*>(ResumeButton),static_cast<UWidget*>(ResetButton),static_cast<UWidget*>(FixButton),
		static_cast<UWidget*>(UnhideButton),static_cast<UWidget*>(RulesButton),static_cast<UWidget*>(OptionsButton),
		static_cast<UWidget*>(MainMenuButton),static_cast<UWidget*>(QuitAppButton)})
		if(W) W->RemoveFromParent();
	Root->ClearChildren();
	UBackgroundBlur* Blur=WidgetTree->ConstructWidget<UBackgroundBlur>();
	Blur->SetBlurStrength(4.f); Blur->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* BlurSlot=Root->AddChildToCanvas(Blur);
	BlurSlot->SetAnchors(FAnchors(0,0,1,1)); BlurSlot->SetOffsets(FMargin(0)); BlurSlot->SetZOrder(0);
	if(!Blocker) Blocker=WidgetTree->ConstructWidget<UBorder>();
	Blocker->SetBrush(FSlateRoundedBoxBrush(FLinearColor::White,0.f));
	Blocker->SetBrushColor(FLinearColor(0,0,0,.24f)); Blocker->SetVisibility(ESlateVisibility::Visible);
	UCanvasPanelSlot* BlockSlot=Root->AddChildToCanvas(Blocker);
	BlockSlot->SetAnchors(FAnchors(0,0,1,1)); BlockSlot->SetOffsets(FMargin(0)); BlockSlot->SetZOrder(1);
	UScaleBox* Fit=WidgetTree->ConstructWidget<UScaleBox>(); Fit->SetStretch(EStretch::ScaleToFit);
	UCanvasPanelSlot* FitSlot=Root->AddChildToCanvas(Fit);
	FitSlot->SetAnchors(FAnchors(0,0,1,1)); FitSlot->SetOffsets(FMargin(32)); FitSlot->SetZOrder(2);
	USizeBox* Size=WidgetTree->ConstructWidget<USizeBox>(); Size->SetWidthOverride(1500); Size->SetHeightOverride(1000);
	Fit->AddChild(Size);
	GardenCanvas=WidgetTree->ConstructWidget<UCanvasPanel>(); Size->AddChild(GardenCanvas);
	auto Place=[&](UWidget* W,FVector2D Position,FVector2D Extent,int32 Z=1)
	{
		if(!W) return;
		UCanvasPanelSlot* LayoutSlot=GardenCanvas->AddChildToCanvas(W);
		LayoutSlot->SetPosition(Position); LayoutSlot->SetSize(Extent); LayoutSlot->SetZOrder(Z);
	};
	auto Face=[&](int32 Style)
	{
		FSlateBrush Brush; Brush.DrawAs=ESlateBrushDrawType::Image;
		Brush.SetResourceObject(Style==2?QuitTexture:ButtonTexture);
		if(Style==2) Brush.SetUVRegion(FBox2f(FVector2f(.024f,.19f),FVector2f(.976f,.782f)));
		else Brush.SetUVRegion(FBox2f(FVector2f(.03f,Style==1?.50f:.06f),FVector2f(.98f,Style==1?.985f:.485f)));
		return Brush;
	};
	auto Image=[&](FSlateBrush Brush,FVector2D Position,FVector2D Extent,int32 Z=1)
	{
		UImage* W=WidgetTree->ConstructWidget<UImage>(); W->SetBrush(Brush);
		W->SetVisibility(ESlateVisibility::HitTestInvisible); Place(W,Position,Extent,Z); return W;
	};
	auto TextStyle=[&](UTextBlock* Text,int32 Points,FLinearColor Color)
	{
		Text->SetFont(FSlateFontInfo(GardenFont,Points)); Text->ColorAndOpacityDelegate.Unbind();
		Text->SetColorAndOpacity(Color); Text->SetShadowOffset(FVector2D::ZeroVector);
		Text->SetJustification(ETextJustify::Center);
	};
	FSlateBrush Surface; Surface.SetResourceObject(GardenTexture); Surface.DrawAs=ESlateBrushDrawType::Image;
	Surface.SetUVRegion(FBox2f(FVector2f(.008f,.05f),FVector2f(.992f,.93f)));
	Image(Surface,{0,0},{1500,1000},0);
	Image(Face(0),{500,65},{500,110});
	if(!Title) Title=WidgetTree->ConstructWidget<UTextBlock>();
	Title->SetText(LOCTEXT("Paused","Paused")); TextStyle(Title,60,GardenInk); Place(Title,{550,76},{400,90},2);
	if(TimeText)
	{
		TextStyle(TimeText,32,GardenInk);
		UHorizontalBox* Row=WidgetTree->ConstructWidget<UHorizontalBox>();
		USizeBox* IconSize=WidgetTree->ConstructWidget<USizeBox>(); IconSize->SetWidthOverride(32); IconSize->SetHeightOverride(32);
		UPauseMenuGlyph* Clock=WidgetTree->ConstructWidget<UPauseMenuGlyph>(); Clock->Color=GardenInk;
		Clock->SetVisibility(ESlateVisibility::HitTestInvisible); IconSize->AddChild(Clock);
		UHorizontalBoxSlot* ClockSlot=Row->AddChildToHorizontalBox(IconSize); ClockSlot->SetPadding(FMargin(0,0,12,0));
		ClockSlot->SetVerticalAlignment(VAlign_Center); Row->AddChildToHorizontalBox(TimeText)->SetVerticalAlignment(VAlign_Center);
		UOverlay* Center=WidgetTree->ConstructWidget<UOverlay>(); Center->AddChildToOverlay(Row)->SetHorizontalAlignment(HAlign_Center);
		Place(Center,{500,185},{500,55});
	}
	auto Action=[&](UButton* Button,const FText& Label,int32 Symbol,int32 Style,FVector2D Position,FVector2D Extent)
	{
		if(!Button) return;
		Button->ClearChildren(); FButtonStyle ButtonStyle;
		ButtonStyle.Normal=ButtonStyle.Hovered=ButtonStyle.Pressed=ButtonStyle.Disabled=Face(Style);
		ButtonStyle.Hovered.TintColor=FLinearColor(1.08f,1.08f,1.04f);
		ButtonStyle.Pressed.TintColor=FLinearColor(.83f,.88f,.78f);
		ButtonStyle.Disabled.TintColor=FLinearColor(.77f,.76f,.68f);
		ButtonStyle.NormalPadding=ButtonStyle.PressedPadding=FMargin(35,8);
		Button->SetStyle(ButtonStyle); Button->SetBackgroundColor(FLinearColor::White); Button->SetCursor(EMouseCursor::Hand);
		const FLinearColor Color=Style?GardenIvory:GardenInk;
		UHorizontalBox* Row=WidgetTree->ConstructWidget<UHorizontalBox>();
		const float IconExtent=Style==1?80.f:48.f;
		USizeBox* IconSize=WidgetTree->ConstructWidget<USizeBox>(); IconSize->SetWidthOverride(IconExtent); IconSize->SetHeightOverride(IconExtent);
		UWidget* Icon=nullptr;
		if(Symbol==6)
		{ ULessonResultGlyph* Glyph=WidgetTree->ConstructWidget<ULessonResultGlyph>(); Glyph->Symbol=0; Glyph->Color=Color; Icon=Glyph; }
		else if(Symbol==7)
		{ ULessonPopupGlyph* Glyph=WidgetTree->ConstructWidget<ULessonPopupGlyph>(); Glyph->Symbol=0; Glyph->Color=Color; Icon=Glyph; }
		else
		{ UPauseMenuGlyph* Glyph=WidgetTree->ConstructWidget<UPauseMenuGlyph>(); Glyph->Symbol=Symbol; Glyph->Color=Color; Icon=Glyph; }
		Icon->SetVisibility(ESlateVisibility::HitTestInvisible); IconSize->AddChild(Icon);
		UHorizontalBoxSlot* IconSlot=Row->AddChildToHorizontalBox(IconSize); IconSlot->SetPadding(FMargin(0,0,18,0)); IconSlot->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Caption=WidgetTree->ConstructWidget<UTextBlock>(); Caption->SetText(Label); TextStyle(Caption,Style==1?50:34,Color);
		Row->AddChildToHorizontalBox(Caption)->SetVerticalAlignment(VAlign_Center);
		UButtonSlot* ContentSlot=CastChecked<UButtonSlot>(Button->AddChild(Row));
		ContentSlot->SetHorizontalAlignment(HAlign_Center); ContentSlot->SetVerticalAlignment(VAlign_Center);
		Place(Button,Position,Extent);
	};
	Action(ResumeButton,LOCTEXT("Resume","Resume"),8,1,{220,250},{1060,140});
	Action(ResetButton,LOCTEXT("Reset","Reset Puzzle"),6,0,{220,415},{510,110});
	Action(FixButton,LOCTEXT("Fix","Fix Puzzle"),1,0,{770,415},{510,110});
	Action(UnhideButton,LOCTEXT("Unhide","Unhide Clues"),2,0,{220,545},{510,110});
	Action(RulesButton,LOCTEXT("Rules","Game Rules"),7,0,{770,545},{510,110});
	Action(OptionsButton,LOCTEXT("Options","Options"),3,0,{370,675},{760,105});
	Image(FSlateRoundedBoxBrush(FLinearColor(.44f,.28f,.1f,.35f),0.f),{220,800},{1060,1});
	Action(MainMenuButton,LOCTEXT("MainMenu","Main Menu"),4,0,{220,825},{510,110});
	Action(QuitAppButton,LOCTEXT("QuitApp","Quit App"),5,2,{770,825},{510,110});
}

void UPauseMenuWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	BuildGardenLayout();
	if(!GetGameInstance())
	{
		if(TimeText) TimeText->SetText(FText::FromString(TEXT("00:04:12")));
		SetVisibility(ESlateVisibility::Visible);
	}
}

void UPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ResumeButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleResume);
	ResetButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleReset);
	FixButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleFix);
	UnhideButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleUnhide);
	RulesButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleRules);
	OptionsButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleOptions);
	MainMenuButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleMainMenu);
	QuitAppButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleQuitApp);

#if PLATFORM_IOS
	// iOS apps don't quit themselves
	QuitAppButton->SetVisibility(ESlateVisibility::Collapsed);
	if (GardenCanvas)
	{
		if(UCanvasPanelSlot* MenuSlot=Cast<UCanvasPanelSlot>(MainMenuButton->Slot)) MenuSlot->SetPosition({495,825});
	}
	else if (UWidget* QuitParent = QuitAppButton->GetParent())
	{
		QuitParent->SetVisibility(ESlateVisibility::Collapsed);
	}
#endif
}

void UPauseMenuWidget::Show(APlayerController* PlayerController)
{
	PC = PlayerController;
	UpdateTime();
	// No fixing on Master Mode stages: Fix Puzzle shows faded and takes no taps
	if (FixButton)
	{
		const bool bFixAllowed = UCampaignSubsystem::AreAssistsAllowed(this);
		FixButton->SetIsEnabled(bFixAllowed);
		FixButton->SetRenderOpacity(bFixAllowed ? 1.f : .45f);
	}
	SetVisibility(ESlateVisibility::Visible);
}

void UPauseMenuWidget::UpdateTime()
{
	// The puzzle screen (WBP_Happiness) this menu is part of: its puzzle and the time it saved when pausing
	const UUserWidget* Screen = GetTypedOuter<UUserWidget>();
	if (!TimeText || !Screen)
	{
		return;
	}
	const FObjectPropertyBase* PuzzleProperty = FindFProperty<FObjectPropertyBase>(Screen->GetClass(), TEXT("ThePuzzle"));
	const FNumericProperty* TimeProperty = FindFProperty<FNumericProperty>(Screen->GetClass(), TEXT("PuzzleTime"));
	const UPuzzle* Puzzle = PuzzleProperty ? Cast<UPuzzle>(PuzzleProperty->GetObjectPropertyValue_InContainer(Screen)) : nullptr;
	if (!Puzzle || !TimeProperty)
	{
		TimeText->SetText(FText::GetEmpty());
		return;
	}
	const void* Time = TimeProperty->ContainerPtrToValuePtr<void>(Screen);
	const double Seconds = TimeProperty->IsFloatingPoint() ? TimeProperty->GetFloatingPointPropertyValue(Time)
		: static_cast<double>(TimeProperty->GetSignedIntPropertyValue(Time));
	TimeText->SetText(FText::FromString(Puzzle->FormatTimeString(static_cast<float>(Seconds))));
}

void UPauseMenuWidget::SetVisibility(ESlateVisibility InVisibility)
{
	Super::SetVisibility(InVisibility);
	HideScreen(InVisibility != ESlateVisibility::Hidden && InVisibility != ESlateVisibility::Collapsed);
}

void UPauseMenuWidget::HideScreen(bool bHide)
{
	if (!bHide)
	{
		for (const TPair<TWeakObjectPtr<UWidget>, ESlateVisibility>& Hidden : HiddenScreen)
		{
			if (UWidget* Widget = Hidden.Key.Get())
			{
				Widget->SetVisibility(Hidden.Value);
			}
		}
		HiddenScreen.Reset();
		return;
	}

	// Already hidden (shown again while showing): keep the first visibilities
	const UPanelWidget* Screen = GetParent();
	if (!Screen || HiddenScreen.Num() > 0)
	{
		return;
	}
	for (UWidget* Sibling : Screen->GetAllChildren())
	{
		if (Sibling && Sibling != this && Sibling->IsVisible())
		{
			HiddenScreen.Add(Sibling, Sibling->GetVisibility());
			Sibling->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UPauseMenuWidget::HandleResume()
{
	PlaySound(CancelSound);
	OnResumeGame.Broadcast();
}

void UPauseMenuWidget::HandleReset()
{
	PlaySound(AcceptSound);
	OnResetPuzzle.Broadcast();
}

void UPauseMenuWidget::HandleFix()
{
	PlaySound(AcceptSound);
	OnFixPuzzle.Broadcast();
}

void UPauseMenuWidget::HandleUnhide()
{
	PlaySound(AcceptSound);
	OnUnhideClues.Broadcast();
}

void UPauseMenuWidget::HandleRules()
{
	PlaySound(AcceptSound);
	bool bNew = false;
	if (UGameRulesWidget* Rules = Cast<UGameRulesWidget>(OpenOverMenu(RulesWidget, RulesClass, bNew)))
	{
		Rules->Show(PC ? PC.Get() : GetOwningPlayer());
	}
	OnGameRules.Broadcast();
}

UUserWidget* UPauseMenuWidget::OpenOverMenu(TObjectPtr<UUserWidget>& Screen, TSoftClassPtr<UUserWidget>& Class, bool& bNew)
{
	UCanvasPanel* Canvas = Cast<UCanvasPanel>(GetRootWidget());
	if (!Canvas)
	{
		return nullptr;
	}
	bNew = !Screen;
	if (bNew)
	{
		if (UClass* Loaded = Class.LoadSynchronous())
		{
			Screen = CreateWidget<UUserWidget>(this, Loaded);
		}
		// Over the whole screen, above the menu
		if (UCanvasPanelSlot* ScreenSlot = Screen ? Canvas->AddChildToCanvas(Screen) : nullptr)
		{
			ScreenSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			ScreenSlot->SetOffsets(FMargin(0.f));
			ScreenSlot->SetZOrder(300);
		}
	}
	if (Screen)
	{
		Screen->SetVisibility(ESlateVisibility::Visible);
	}
	return Screen;
}

void UPauseMenuWidget::HandleOptions()
{
	PlaySound(AcceptSound);
	bool bNew = false;
	UUserWidget* Options = OpenOverMenu(OptionsWidget, OptionsClass, bNew);
	if (!Options)
	{
		return;
	}

	// WBP_Options.Show(PC) loads the current settings into it
	if (UFunction* ShowFunction = Options->FindFunction(TEXT("Show")))
	{
		uint8* Params = static_cast<uint8*>(FMemory_Alloca(ShowFunction->ParmsSize));
		FMemory::Memzero(Params, ShowFunction->ParmsSize);
		if (const FObjectPropertyBase* PCParam = CastField<FObjectPropertyBase>(ShowFunction->ChildProperties))
		{
			PCParam->SetObjectPropertyValue_InContainer(Params, PC ? PC.Get() : GetOwningPlayer());
		}
		Options->ProcessEvent(ShowFunction, Params);
	}
}

void UPauseMenuWidget::HandleMainMenu()
{
	PlaySound(CancelSound);
	OnMainMenu.Broadcast();
}

void UPauseMenuWidget::HandleQuitApp()
{
	PlaySound(CancelSound);
	OnQuitApp.Broadcast();
}

void UPauseMenuWidget::PlaySound(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound, 1.f, 1.f, 0.f, nullptr, nullptr, true);
	}
}

#undef LOCTEXT_NAMESPACE
