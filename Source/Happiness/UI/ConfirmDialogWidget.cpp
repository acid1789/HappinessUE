#include "UI/ConfirmDialogWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/SLeafWidget.h"

#define LOCTEXT_NAMESPACE "ConfirmGardenPlaque"

namespace
{
	const FLinearColor ConfirmInk(.17f,.085f,.035f), ConfirmIvory(1.f,.95f,.8f), ConfirmSage(.16f,.29f,.075f);
	class SConfirmDialogGlyph : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SConfirmDialogGlyph) {} SLATE_END_ARGS()
		void Construct(const FArguments&,UConfirmDialogGlyph* InOwner) { Owner=InOwner; }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(32,32); }
		virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,
			FSlateWindowElementList& E,int32 Layer,const FWidgetStyle&,bool) const override
		{
			const UConfirmDialogGlyph* Glyph=Owner.Get();
			if(!Glyph) return Layer;
			const float Scale=FMath::Min(G.GetLocalSize().X,G.GetLocalSize().Y)/32.f;
			const FVector2f Origin=(FVector2f(G.GetLocalSize())-FVector2f(32*Scale))*.5f;
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
			if(Glyph->Symbol==0)
				Fill({{23,4},{27,8},{16,16},{27,24},{23,28},{9,16}},{5,0,1,5,1,2,5,2,3,3,4,5});
			else if(Glyph->Symbol==1)
				Fill({{6.8f,15.5f},{9,13.3f},{13,17.3f},{23,7.3f},{25.2f,9.5f},{13,21.7f}},{2,3,4,2,4,5,2,5,0,2,0,1});
			else
			{
				auto Leaf=[&](FVector2f Start,FVector2f End,float Width)
				{
					const FVector2f Direction=End-Start;
					const FVector2f Normal=FVector2f(-Direction.Y,Direction.X).GetSafeNormal();
					TArray<FVector2f> Points;
					for(int32 I=0;I<=8;++I)
					{ const float T=I/8.f; Points.Add(Start+Direction*T-Normal*FMath::Sin(PI*T)*Width); }
					for(int32 I=7;I>0;--I)
					{ const float T=I/8.f; Points.Add(Start+Direction*T+Normal*FMath::Sin(PI*T)*Width); }
					TArray<SlateIndex> Triangles;
					for(int32 I=1;I<Points.Num()-1;++I) Triangles.Append({0,SlateIndex(I),SlateIndex(I+1)});
					Fill(Points,Triangles);
				};
				Leaf({14,18},{3,10},3.f); Leaf({13,19},{26,21},3.f); Leaf({21,12},{29,4},3.f);
				TArray<FVector2f> Stem={Origin+FVector2f(4,24)*Scale,Origin+FVector2f(17,16)*Scale,Origin+FVector2f(29,8)*Scale};
				FSlateDrawElement::MakeLines(E,Layer,G.ToPaintGeometry(),Stem,ESlateDrawEffect::None,Glyph->Color,true,1.3f*Scale);
			}
			return Layer;
		}
	private:
		TWeakObjectPtr<UConfirmDialogGlyph> Owner;
	};
}

TSharedRef<SWidget> UConfirmDialogGlyph::RebuildWidget()
{
	return SNew(SConfirmDialogGlyph,this);
}

UConfirmDialogWidget::UConfirmDialogWidget(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> Plaque(TEXT("/Game/General/Textures/T_ConfirmGardenPlaque.T_ConfirmGardenPlaque"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Buttons(TEXT("/Game/General/Textures/T_LessonNotebookButtons.T_LessonNotebookButtons"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Confirm(TEXT("/Game/General/Textures/T_PauseQuitButton.T_PauseQuitButton"));
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	PlaqueTexture=Plaque.Object; ButtonTexture=Buttons.Object; ConfirmTexture=Confirm.Object; PlaqueFont=Font.Object;
}


void UConfirmDialogWidget::BuildPlaqueLayout()
{
	if(PlaqueRoot || !WidgetTree) return;
	UCanvasPanel* Root=Cast<UCanvasPanel>(WidgetTree->RootWidget);
	UTextBlock* Question=Cast<UTextBlock>(GetWidgetFromName(TEXT("TextBlock_142")));
	UButton* Cancel=Cast<UButton>(GetWidgetFromName(TEXT("Button_Cancel")));
	UButton* Confirm=Cast<UButton>(GetWidgetFromName(TEXT("Button_Confirm")));
	if(!Root || !Question || !Cancel || !Confirm) return;
	for(UWidget* W:{static_cast<UWidget*>(Question),static_cast<UWidget*>(DescriptionText),static_cast<UWidget*>(Cancel),static_cast<UWidget*>(Confirm)})
		if(W) W->RemoveFromParent();
	Root->ClearChildren();
	UBackgroundBlur* Blur=WidgetTree->ConstructWidget<UBackgroundBlur>(); Blur->SetBlurStrength(4.f);
	Blur->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* BlurSlot=Root->AddChildToCanvas(Blur);
	BlurSlot->SetAnchors(FAnchors(0,0,1,1)); BlurSlot->SetOffsets(FMargin(0)); BlurSlot->SetZOrder(0);
	UBorder* Dim=WidgetTree->ConstructWidget<UBorder>(); Dim->SetBrush(FSlateRoundedBoxBrush(FLinearColor::White,0.f));
	Dim->SetBrushColor(FLinearColor(0,0,0,.28f));
	UCanvasPanelSlot* DimSlot=Root->AddChildToCanvas(Dim);
	DimSlot->SetAnchors(FAnchors(0,0,1,1)); DimSlot->SetOffsets(FMargin(0)); DimSlot->SetZOrder(1);
	UScaleBox* Fit=WidgetTree->ConstructWidget<UScaleBox>(); Fit->SetStretch(EStretch::ScaleToFit);
	UCanvasPanelSlot* FitSlot=Root->AddChildToCanvas(Fit);
	FitSlot->SetAnchors(FAnchors(.15f,.17f,.85f,.83f)); FitSlot->SetOffsets(FMargin(0)); FitSlot->SetZOrder(2);
	USizeBox* Size=WidgetTree->ConstructWidget<USizeBox>(); Size->SetWidthOverride(1200); Size->SetMinDesiredHeight(600);
	Fit->AddChild(Size);
	UOverlay* Layers=WidgetTree->ConstructWidget<UOverlay>(); Size->AddChild(Layers);
	PlaqueRoot=WidgetTree->ConstructWidget<UCanvasPanel>();
	UOverlaySlot* SurfaceLayer=Layers->AddChildToOverlay(PlaqueRoot);
	SurfaceLayer->SetHorizontalAlignment(HAlign_Fill); SurfaceLayer->SetVerticalAlignment(VAlign_Fill);
	UImage* Surface=WidgetTree->ConstructWidget<UImage>(); Surface->SetBrushFromTexture(PlaqueTexture);
	Surface->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* SurfaceSlot=PlaqueRoot->AddChildToCanvas(Surface);
	SurfaceSlot->SetAnchors(FAnchors(0,0,1,1)); SurfaceSlot->SetOffsets(FMargin(0));
	UVerticalBox* Content=WidgetTree->ConstructWidget<UVerticalBox>();
	UOverlaySlot* ContentSlot=Layers->AddChildToOverlay(Content);
	ContentSlot->SetHorizontalAlignment(HAlign_Fill); ContentSlot->SetVerticalAlignment(VAlign_Center);
	ContentSlot->SetPadding(FMargin(100,65,100,45));
	auto TextStyle=[&](UTextBlock* Text,int32 Points,FLinearColor Color)
	{
		Text->SetFont(FSlateFontInfo(PlaqueFont,Points)); Text->ColorAndOpacityDelegate.Unbind();
		Text->SetColorAndOpacity(Color); Text->SetShadowOffset(FVector2D::ZeroVector); Text->SetJustification(ETextJustify::Center);
	};
	TextStyle(Question,44,ConfirmInk); Question->SetAutoWrapText(true); Question->SetWrapTextAt(800);
	Content->AddChildToVerticalBox(Question)->SetPadding(FMargin(100,0,100,0));
	USizeBox* DividerSize=WidgetTree->ConstructWidget<USizeBox>(); DividerSize->SetWidthOverride(600); DividerSize->SetHeightOverride(55);
	UCanvasPanel* Divider=WidgetTree->ConstructWidget<UCanvasPanel>(); DividerSize->AddChild(Divider);
	for(float X:{0.f,360.f})
	{
		UImage* Line=WidgetTree->ConstructWidget<UImage>(); Line->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.44f,.28f,.10f,.45f),0.f));
		UCanvasPanelSlot* LineSlot=Divider->AddChildToCanvas(Line); LineSlot->SetPosition({X,27}); LineSlot->SetSize({240,1});
	}
	UConfirmDialogGlyph* Leaf=WidgetTree->ConstructWidget<UConfirmDialogGlyph>(); Leaf->Symbol=2; Leaf->Color=ConfirmSage;
	Leaf->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* LeafSlot=Divider->AddChildToCanvas(Leaf); LeafSlot->SetPosition({270,0}); LeafSlot->SetSize({60,55});
	Content->AddChildToVerticalBox(DividerSize)->SetHorizontalAlignment(HAlign_Center);
	if(DescriptionText)
	{
		TextStyle(DescriptionText,29,ConfirmInk); DescriptionText->SetAutoWrapText(true); DescriptionText->SetWrapTextAt(800);
		Content->AddChildToVerticalBox(DescriptionText)->SetPadding(FMargin(100,0,100,32));
	}
	UHorizontalBox* Actions=WidgetTree->ConstructWidget<UHorizontalBox>();
	auto Action=[&](UButton* Button,const FText& Caption,bool bConfirm)
	{
		Button->ClearChildren();
		FSlateBrush Face; Face.DrawAs=ESlateBrushDrawType::Image; Face.SetResourceObject(bConfirm?ConfirmTexture:ButtonTexture);
		Face.SetUVRegion(bConfirm?FBox2f(FVector2f(.024f,.19f),FVector2f(.976f,.782f))
			:FBox2f(FVector2f(.03f,.06f),FVector2f(.98f,.485f)));
		FButtonStyle Style; Style.Normal=Style.Hovered=Style.Pressed=Style.Disabled=Face;
		Style.Hovered.TintColor=FLinearColor(1.08f,1.08f,1.04f); Style.Pressed.TintColor=FLinearColor(.83f,.88f,.78f);
		Style.Disabled.TintColor=FLinearColor(.77f,.76f,.68f); Style.NormalPadding=Style.PressedPadding=FMargin(35,8);
		Button->SetStyle(Style); Button->SetBackgroundColor(FLinearColor::White); Button->SetCursor(EMouseCursor::Hand);
		UHorizontalBox* Row=WidgetTree->ConstructWidget<UHorizontalBox>();
		USizeBox* IconSize=WidgetTree->ConstructWidget<USizeBox>(); IconSize->SetWidthOverride(64); IconSize->SetHeightOverride(64);
		UConfirmDialogGlyph* Icon=WidgetTree->ConstructWidget<UConfirmDialogGlyph>(); Icon->Symbol=bConfirm?1:0;
		Icon->Color=bConfirm?ConfirmIvory:ConfirmInk; Icon->SetVisibility(ESlateVisibility::HitTestInvisible); IconSize->AddChild(Icon);
		UHorizontalBoxSlot* IconSlot=Row->AddChildToHorizontalBox(IconSize); IconSlot->SetPadding(FMargin(0,0,16,0)); IconSlot->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Label=WidgetTree->ConstructWidget<UTextBlock>(); Label->SetText(Caption); TextStyle(Label,36,Icon->Color);
		Row->AddChildToHorizontalBox(Label)->SetVerticalAlignment(VAlign_Center);
		UButtonSlot* ButtonSlot=CastChecked<UButtonSlot>(Button->AddChild(Row));
		ButtonSlot->SetHorizontalAlignment(HAlign_Center); ButtonSlot->SetVerticalAlignment(VAlign_Center);
		USizeBox* ButtonSize=WidgetTree->ConstructWidget<USizeBox>(); ButtonSize->SetWidthOverride(470); ButtonSize->SetHeightOverride(130); ButtonSize->AddChild(Button);
		Actions->AddChildToHorizontalBox(ButtonSize)->SetPadding(FMargin(bConfirm?40:0,0,0,0));
	};
	Action(Cancel,LOCTEXT("Cancel","Cancel"),false); Action(Confirm,LOCTEXT("Confirm","Confirm"),true);
	Content->AddChildToVerticalBox(Actions)->SetHorizontalAlignment(HAlign_Center);
}

void UConfirmDialogWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	BuildPlaqueLayout();
	if(!GetGameInstance())
	{
		if(UTextBlock* Question=Cast<UTextBlock>(GetWidgetFromName(TEXT("TextBlock_142"))))
			Question->SetText(LOCTEXT("PreviewQuestion","Reset this puzzle?"));
		SetDescription(TEXT("Your current choices will be cleared."));
		SetVisibility(ESlateVisibility::Visible);
	}
}

void UConfirmDialogWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetDescription(GetGameInstance()?FString():TEXT("Your current choices will be cleared."));
	// Pressed, like the Blueprint's Confirm and Cancel: the dialog hides then, so a click would never finish
	if(UButton* Confirm=Cast<UButton>(GetWidgetFromName(TEXT("Button_Confirm"))))
	{
		Confirm->OnPressed.AddUniqueDynamic(this,&UConfirmDialogWidget::HandleConfirmPressed);
	}
	if(UButton* Cancel=Cast<UButton>(GetWidgetFromName(TEXT("Button_Cancel"))))
	{
		Cancel->OnPressed.AddUniqueDynamic(this,&UConfirmDialogWidget::HandleCancelPressed);
	}
}

void UConfirmDialogWidget::Ask(const FText& Question, const FString& Description, TFunction<void()> OnConfirm)
{
	// The Blueprint's SetMessage sets its Message variable, which may be bound to the question's text
	if(const FTextProperty* Message=FindFProperty<FTextProperty>(GetClass(),TEXT("Message")))
		Message->SetPropertyValue_InContainer(this,Question);
	else if(const FStrProperty* MessageString=FindFProperty<FStrProperty>(GetClass(),TEXT("Message")))
		MessageString->SetPropertyValue_InContainer(this,Question.ToString());
	if(UTextBlock* QuestionText=Cast<UTextBlock>(GetWidgetFromName(TEXT("TextBlock_142"))))
		QuestionText->SetText(Question);
	SetVisibility(ESlateVisibility::Visible);
	SetDescription(Description);
	AskConfirmed=MoveTemp(OnConfirm);
	bAsking=true;
}

void UConfirmDialogWidget::HandleConfirmPressed()
{
	if(!bAsking) return;
	TFunction<void()> Confirmed=MoveTemp(AskConfirmed);
	SetVisibility(ESlateVisibility::Collapsed);
	if(Confirmed) Confirmed();
}

void UConfirmDialogWidget::HandleCancelPressed()
{
	if(bAsking) SetVisibility(ESlateVisibility::Collapsed);
}

void UConfirmDialogWidget::SetDescription(FString Description)
{
	if (DescriptionText)
	{
		DescriptionText->SetText(FText::FromString(Description));
		DescriptionText->SetVisibility(Description.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UConfirmDialogWidget::SetVisibility(ESlateVisibility InVisibility)
{
	Super::SetVisibility(InVisibility);
	// Closed: the next dialog starts without a description
	if (InVisibility == ESlateVisibility::Hidden || InVisibility == ESlateVisibility::Collapsed)
	{
		SetDescription(FString());
		AskConfirmed.Reset();
		bAsking=false;
	}
}

#undef LOCTEXT_NAMESPACE
