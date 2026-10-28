#include "UI/LessonPopupWidget.h"
#include "HappinessClassic/CampaignProgress.h"
#include "HappinessClassic/CampaignTree.h"
#include "UI/LessonProgressTicks.h"
#include "UI/CampaignTreeWidget.h"
#include "UI/PlayerLevelBadge.h"
#include "UI/FireworksWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Brushes/SlateNoResource.h"
#include "Components/BackgroundBlur.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/SLeafWidget.h"
#include "Components/NativeWidgetHost.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Slate/WidgetRenderer.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<float> GMasterCollapseDuration(
	TEXT("Happiness.MasterCollapseDuration"), .6f,
	TEXT("Duration in seconds of the mastery popup suction; increase to inspect the deformation."), ECVF_Cheat);

/** A captured popup drawn as a deformable sheet, with the nearest parts entering the reward point first. */
class SMasterCollapseMesh : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SMasterCollapseMesh) {} SLATE_END_ARGS()
	void Construct(const FArguments&) { SetVisibility(EVisibility::HitTestInvisible); }
	FSlateBrush Brush;
	FVector2D SourceTopLeft, SourceBottomRight, Target;
	float Progress = 0.f;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1500, 950); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&,
		FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle&, bool) const override
	{
		const FVector2D Origin = G.AbsoluteToLocal(SourceTopLeft);
		const FVector2D Size = G.AbsoluteToLocal(SourceBottomRight) - Origin;
		const FVector2D Hole = G.AbsoluteToLocal(Target);
		float MaxDistance = 1.f;
		for (const FVector2D& Corner : {Origin, Origin + Size, Origin + FVector2D(Size.X, 0), Origin + FVector2D(0, Size.Y)})
			MaxDistance = FMath::Max(MaxDistance, float(FVector2D::Distance(Corner, Hole)));
		constexpr int32 Columns = 64, Rows = 40;
		TArray<FSlateVertex> Vertices;
		TArray<SlateIndex> Indices;
		Vertices.Reserve((Columns + 1) * (Rows + 1));
		for (int32 Y = 0; Y <= Rows; ++Y) for (int32 X = 0; X <= Columns; ++X)
		{
			const FVector2D UV(double(X) / Columns, double(Y) / Rows);
			const FVector2D Point = Origin + UV * Size;
			const FVector2D Delta = Point - Hole;
			const float Delay = .65f * Delta.Size() / MaxDistance;
			const float T = FMath::Clamp((Progress - Delay) / (1.f - Delay), 0.f, 1.f);
			const float Pull = FMath::InterpEaseInOut(0.f, 1.f, T, 2.f);
			// Staggered suction bends the sheet into a funnel while keeping it between its origin and the hole.
			const FVector2D Position = Hole + Delta * (1.f - Pull);
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(G.GetAccumulatedRenderTransform(),
				FVector2f(Position), FVector2f(UV), FColor::White));
		}
		for (int32 Y = 0; Y < Rows; ++Y) for (int32 X = 0; X < Columns; ++X)
		{
			const SlateIndex A = Y * (Columns + 1) + X, B = A + 1, C = A + Columns + 1, D = C + 1;
			Indices.Append({A, B, C, B, D, C});
		}
		const FSlateResourceHandle Resource = FSlateApplication::Get().GetRenderer()->GetResourceHandle(Brush);
		FSlateDrawElement::MakeCustomVerts(Elements, Layer, Resource, Vertices, Indices, nullptr, 0, 0);
		return Layer;
	}
};

#define LOCTEXT_NAMESPACE "LessonPopup"

namespace
{
	const FLinearColor NotebookInk(.17f, .085f, .035f);
	const FLinearColor NotebookIvory(1.f, .95f, .80f);
	const FLinearColor NotebookSage(.16f, .29f, .075f);

	class SLessonPopupGlyph : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SLessonPopupGlyph) {} SLATE_END_ARGS()
		void Construct(const FArguments&, ULessonPopupGlyph* InOwner) { Owner = InOwner; }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(32, 32); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&,
			FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle&, bool) const override
		{
			const ULessonPopupGlyph* Glyph = Owner.Get();
			if (!Glyph) return Layer;
			const float Scale = FMath::Min(G.GetLocalSize().X, G.GetLocalSize().Y) / 32.f;
			const FVector2f Origin = (FVector2f(G.GetLocalSize()) - FVector2f(32.f * Scale)) * .5f;
			auto Line = [&](TArray<FVector2f> Points, float Width = 2.4f)
			{
				for (FVector2f& Point : Points) Point = Origin + Point * Scale;
				FSlateDrawElement::MakeLines(Elements, Layer, G.ToPaintGeometry(), Points,
					ESlateDrawEffect::None, Glyph->Color, true, Width * Scale);
			};
			if (Glyph->Symbol == 0)
			{
				Line({{16,7},{9,4},{2,6},{2,26},{9,24},{16,27},{23,24},{30,26},{30,6},{23,4},{16,7},{16,27}});
			}
			else if (Glyph->Symbol == 1)
			{
				for (int32 Y = 4; Y <= 28; ++Y)
				{
					const float Right = 28.f - FMath::Abs(Y - 16.f) * 20.f / 12.f;
					Line({{8,float(Y)},{Right,float(Y)}}, 1.5f);
				}
			}
			else Line({{23,5},{11,16},{23,27}}, 4.f);
			return Layer;
		}
	private:
		TWeakObjectPtr<ULessonPopupGlyph> Owner;
	};
}

TSharedRef<SWidget> ULessonPopupGlyph::RebuildWidget()
{
	return SNew(SLessonPopupGlyph, this);
}

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
	static ConstructorHelpers::FObjectFinder<UTexture2D> Book(TEXT("/Game/General/Textures/T_LessonNotebook.T_LessonNotebook"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Buttons(TEXT("/Game/General/Textures/T_LessonNotebookButtons.T_LessonNotebookButtons"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Lock(TEXT("/Game/General/Textures/T_LessonsLock.T_LessonsLock"));
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	NotebookTexture = Book.Object;
	NotebookButtonTexture = Buttons.Object;
	NotebookLockTexture = Lock.Object;
	NotebookFont = Font.Object;
}

FButtonStyle ULessonPopupWidget::NotebookButtonStyle(bool bPrimary) const
{
	FButtonStyle Style;
	FSlateBrush Face;
	Face.SetResourceObject(NotebookButtonTexture);
	Face.DrawAs = ESlateBrushDrawType::Image;
	Face.SetUVRegion(FBox2f(FVector2f(.03f, bPrimary ? .54f : .06f), FVector2f(.98f, bPrimary ? .963f : .485f)));
	Style.Normal = Face;
	Style.Hovered = Face;
	Style.Hovered.TintColor = FLinearColor(1.08f, 1.08f, 1.04f);
	Style.Pressed = Face;
	Style.Pressed.TintColor = FLinearColor(.83f, .88f, .78f);
	Style.Disabled = Face;
	Style.Disabled.TintColor = FLinearColor(.77f, .76f, .68f);
	Style.NormalPadding = Style.PressedPadding = FMargin(30.f, 8.f);
	return Style;
}

void ULessonPopupWidget::AddActionContent(UButton* Button, UTextBlock* Label, int32 Symbol, bool bPrimary)
{
	if (!Button || !Label) return;
	Label->RemoveFromParent();
	Button->ClearChildren();
	Button->SetStyle(NotebookButtonStyle(bPrimary));
	Button->SetBackgroundColor(FLinearColor::White);
	Button->SetCursor(EMouseCursor::Hand);
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>();
	IconSize->SetWidthOverride(32.f);
	IconSize->SetHeightOverride(32.f);
	ULessonPopupGlyph* Glyph = WidgetTree->ConstructWidget<ULessonPopupGlyph>();
	Glyph->Symbol = Symbol;
	Glyph->Color = bPrimary ? NotebookIvory : NotebookSage;
	Glyph->SetVisibility(ESlateVisibility::HitTestInvisible);
	IconSize->AddChild(Glyph);
	UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconSize);
	IconSlot->SetVerticalAlignment(VAlign_Center);
	IconSlot->SetPadding(FMargin(0, 0, 14, 0));
	Label->SetFont(FSlateFontInfo(NotebookFont, Symbol == 0 ? 30 : 34));
	Label->SetColorAndOpacity(bPrimary ? NotebookIvory : NotebookInk);
	Label->SetJustification(ETextJustify::Center);
	Row->AddChildToHorizontalBox(Label)->SetVerticalAlignment(VAlign_Center);
	UButtonSlot* ContentSlot = Cast<UButtonSlot>(Button->AddChild(Row));
	ContentSlot->SetHorizontalAlignment(HAlign_Center);
	ContentSlot->SetVerticalAlignment(VAlign_Center);
}

void ULessonPopupWidget::BuildNotebookLayout()
{
	if (NotebookCanvas || !WidgetTree || !Panel || !TitleText || !DescriptionText || !LessonProgress || !StageBox) return;
	UCanvasPanel* Root = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Root) return;
	UWidget* ProgressGroup = LessonProgress->GetParent();
	UWidget* ProgressLine = ProgressText ? ProgressText->GetParent() : nullptr;
	UTextBlock* PlayLabel = Cast<UTextBlock>(GetWidgetFromName(TEXT("PlayLabel")));
	for (UWidget* Widget : {static_cast<UWidget*>(TitleText), static_cast<UWidget*>(DescriptionText),
		ProgressGroup, ProgressLine, static_cast<UWidget*>(StageBox), static_cast<UWidget*>(MasterBox),
		static_cast<UWidget*>(PlayButton), static_cast<UWidget*>(CloseButton)})
	{
		if (Widget) Widget->RemoveFromParent();
	}
	Panel->ClearChildren();
	Panel->SetBrush(FSlateNoResource());
	Panel->SetBrushColor(FLinearColor(.98f, .92f, .78f));
	Panel->SetPadding(FMargin(0));
	Panel->SetHorizontalAlignment(HAlign_Fill);
	Panel->SetVerticalAlignment(VAlign_Fill);
	if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(Panel->Slot))
	{
		PanelSlot->SetAnchors(FAnchors(0,0,1,1));
		PanelSlot->SetOffsets(FMargin(32));
		PanelSlot->SetAlignment(FVector2D::ZeroVector);
		PanelSlot->SetZOrder(2);
	}
	if (UImage* Dim = Cast<UImage>(GetWidgetFromName(TEXT("Dim"))))
	{
		Dim->SetColorAndOpacity(FLinearColor(0,0,0,.30f));
		CastChecked<UCanvasPanelSlot>(Dim->Slot)->SetZOrder(1);
	}
	UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>();
	Blur->SetBlurStrength(4.f);
	Blur->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* BlurSlot = Root->AddChildToCanvas(Blur);
	BlurSlot->SetAnchors(FAnchors(0,0,1,1));
	BlurSlot->SetOffsets(FMargin(0));
	BlurSlot->SetZOrder(0);
	UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>();
	Fit->SetStretch(EStretch::ScaleToFit);
	Panel->AddChild(Fit);
	NotebookSize = WidgetTree->ConstructWidget<USizeBox>();
	NotebookSize->SetWidthOverride(1500);
	NotebookSize->SetHeightOverride(950);
	Fit->AddChild(NotebookSize);
	NotebookCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	NotebookSize->AddChild(NotebookCanvas);
	auto Place = [&](UWidget* Widget, FVector2D Position, FVector2D Size, int32 Z = 1)
	{
		UCanvasPanelSlot* LayoutSlot = NotebookCanvas->AddChildToCanvas(Widget);
		LayoutSlot->SetPosition(Position);
		LayoutSlot->SetSize(Size);
		LayoutSlot->SetZOrder(Z);
	};
	UImage* Surface = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("NotebookSurface"));
	Surface->SetBrushFromTexture(NotebookTexture, true);
	Surface->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* SurfaceSlot = NotebookCanvas->AddChildToCanvas(Surface);
	SurfaceSlot->SetAnchors(FAnchors(0,0,1,1));
	SurfaceSlot->SetOffsets(FMargin(0));
	SurfaceSlot->SetZOrder(0);
	TitleText->SetFont(FSlateFontInfo(NotebookFont, 48));
	TitleText->SetColorAndOpacity(NotebookInk);
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetAutoWrapText(true);
	TitleText->SetWrapTextAt(500);
	Place(TitleText, FVector2D(170,100), FVector2D(500,125));
	NotebookDiagram = WidgetTree->ConstructWidget<ULessonClueDiagram>();
	NotebookDiagram->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(NotebookDiagram, FVector2D(225,260), FVector2D(390,215));
	UScrollBox* Explanation = WidgetTree->ConstructWidget<UScrollBox>();
	Explanation->SetScrollBarVisibility(ESlateVisibility::Visible);
	Explanation->SetScrollbarThickness(FVector2D(6,6));
	FScrollBoxStyle PageScrollStyle = Explanation->GetWidgetStyle();
	PageScrollStyle.TopShadowBrush = FSlateNoResource();
	PageScrollStyle.BottomShadowBrush = FSlateNoResource();
	PageScrollStyle.LeftShadowBrush = FSlateNoResource();
	PageScrollStyle.RightShadowBrush = FSlateNoResource();
	Explanation->SetWidgetStyle(PageScrollStyle);
	FScrollBarStyle PageBarStyle = Explanation->GetWidgetBarStyle();
	PageBarStyle.NormalThumbImage = FSlateRoundedBoxBrush(NotebookSage, 3.f);
	PageBarStyle.HoveredThumbImage = FSlateRoundedBoxBrush(NotebookSage, 3.f);
	PageBarStyle.DraggedThumbImage = FSlateRoundedBoxBrush(NotebookSage, 3.f);
	PageBarStyle.VerticalBackgroundImage = FSlateRoundedBoxBrush(FLinearColor(.82f,.76f,.58f,.4f), 3.f);
	Explanation->SetWidgetBarStyle(PageBarStyle);
	Explanation->SetAllowOverscroll(false);
	Explanation->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
	DescriptionText->SetFont(FSlateFontInfo(NotebookFont, 27));
	DescriptionText->SetColorAndOpacity(NotebookInk);
	DescriptionText->SetAutoWrapText(true);
	DescriptionText->SetWrapTextAt(480);
	DescriptionText->SetJustification(ETextJustify::Left);
	Explanation->AddChild(DescriptionText);
	Place(Explanation, FVector2D(810,120), FVector2D(500,280));
	LearnMoreButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("LearnMoreButton"));
	UTextBlock* LearnLabel = WidgetTree->ConstructWidget<UTextBlock>();
	LearnLabel->SetText(LOCTEXT("LearnMore", "Learn More"));
	AddActionContent(LearnMoreButton, LearnLabel, 0, false);
	LearnMoreButton->OnClicked.AddUniqueDynamic(this, &ULessonPopupWidget::HandleLearnMoreClicked);
	Place(LearnMoreButton, FVector2D(865,425), FVector2D(390,80));
	Place(ProgressGroup, FVector2D(95,620), FVector2D(1310,30));
	FProgressBarStyle ProgressStyle;
	ProgressStyle.BackgroundImage = FSlateRoundedBoxBrush(FLinearColor(.80f,.75f,.61f), 12.f, FLinearColor(.65f,.42f,.12f), 2.f);
	ProgressStyle.FillImage = FSlateRoundedBoxBrush(FLinearColor::White, 9.f);
	LessonProgress->SetWidgetStyle(ProgressStyle);
	LessonProgress->SetFillColorAndOpacity(FLinearColor(.30f,.47f,.12f));
	LessonProgress->SetBorderPadding(FVector2D(5,5));
	if (UOverlaySlot* LayoutSlot = Cast<UOverlaySlot>(LessonProgress->Slot))
	{
		LayoutSlot->SetHorizontalAlignment(HAlign_Fill); LayoutSlot->SetVerticalAlignment(VAlign_Fill); LayoutSlot->SetPadding(FMargin(0));
	}
	if (ProgressTicks)
	{
		ProgressTicks->TickColor = FLinearColor(.40f,.28f,.11f);
		ProgressTicks->TickThickness = 2.f;
		ProgressTicks->SynchronizeProperties();
		if (UOverlaySlot* LayoutSlot = Cast<UOverlaySlot>(ProgressTicks->Slot))
		{
			LayoutSlot->SetHorizontalAlignment(HAlign_Fill); LayoutSlot->SetVerticalAlignment(VAlign_Fill); LayoutSlot->SetPadding(FMargin(5,4));
		}
	}
	if (ProgressLine) Place(ProgressLine, FVector2D(95,660), FVector2D(1310,42));
	if (ProgressText)
	{
		ProgressText->SetFont(FSlateFontInfo(NotebookFont, 28));
		ProgressText->SetColorAndOpacity(NotebookInk);
		ProgressText->SetJustification(ETextJustify::Center);
	}
	Place(StageBox, FVector2D(85,710), FVector2D(1330,104));
	if (MasterBox) Place(MasterBox, FVector2D(450,712), FVector2D(600,100));
	if (PlayButton)
	{
		if (!PlayLabel) PlayLabel = WidgetTree->ConstructWidget<UTextBlock>();
		PlayLabel->SetText(LOCTEXT("Play", "Play"));
		AddActionContent(PlayButton, PlayLabel, 1, true);
		Place(PlayButton, FVector2D(775,830), FVector2D(350,88));
	}
	if (CloseButton)
	{
		if (!CloseLabel) CloseLabel = WidgetTree->ConstructWidget<UTextBlock>();
		AddActionContent(CloseButton, CloseLabel, 2, false);
		Place(CloseButton, FVector2D(375,830), FVector2D(350,88));
	}
	PlayTimeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PlayTimeText"));
	PlayTimeText->SetFont(FSlateFontInfo(NotebookFont, 26));
	PlayTimeText->SetColorAndOpacity(NotebookInk);
	PlayTimeText->SetJustification(ETextJustify::Right);
	PlayTimeText->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(PlayTimeText, FVector2D(105,856), FVector2D(250,36));
}

void ULessonPopupWidget::UpdateNotebookLayout()
{
	if (!NotebookCanvas || !NotebookSize) return;
	// The Master row replaces the stage row (Refresh), so the layout is the same either way
	NotebookSize->SetHeightOverride(950.f);
	auto Move = [](UWidget* Widget, float Y)
	{
		if (Widget)
			if (UCanvasPanelSlot* LayoutSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
				LayoutSlot->SetPosition(FVector2D(LayoutSlot->GetPosition().X,Y));
	};
	Move(LessonProgress ? LessonProgress->GetParent() : nullptr, 620.f);
	Move(ProgressText ? ProgressText->GetParent() : nullptr, 660.f);
	Move(StageBox, 710.f);
	Move(MasterBox, 712.f);
	Move(PlayButton, 830.f);
	Move(CloseButton, 830.f);
	if (CloseButton)
	{
		const bool bOnlyBack = !PlayButton || PlayButton->GetVisibility() == ESlateVisibility::Collapsed;
		if (UCanvasPanelSlot* LayoutSlot = Cast<UCanvasPanelSlot>(CloseButton->Slot))
		{
			LayoutSlot->SetPosition(FVector2D(bOnlyBack ? 500.f : 375.f, 830.f));
			LayoutSlot->SetSize(FVector2D(bOnlyBack ? 500.f : 350.f,88.f));
		}
		// The lesson's time sits just left of the button, centered on its height
		if (UCanvasPanelSlot* TimeSlot = PlayTimeText ? Cast<UCanvasPanelSlot>(PlayTimeText->Slot) : nullptr)
		{
			TimeSlot->SetPosition(FVector2D((bOnlyBack ? 500.f : 375.f) - 20.f - 250.f, 856.f));
		}
	}
}

void ULessonPopupWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	BuildNotebookLayout();
	StageButtonSize = FVector2D(250.f, 74.f);
	StageButtonStyle = NotebookButtonStyle(false);
	StageTextColor = NotebookInk;

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
	StageLabels.Reset();
	StageChecks.Reset();
	StageLocks.Reset();

	// The row is sized for five stages (Lessons). More stages (Campaign) share the same width, with each name on
	// two lines ("5x5" over "Normal") so it still fits its narrower button.
	const int32 NumStages = UCampaignSubsystem::GetNumStages();
	constexpr int32 FittedStages = 5;
	constexpr float Spacing = 12.f;
	const bool bNarrow = NumStages > FittedStages;
	// Here rather than only at construction: the popup is rebuilt when the mode changes
	if (NotebookFont)
	{
		StageFont = FSlateFontInfo(NotebookFont, bNarrow ? 22 : 26);
	}
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
	Size->SetHeightOverride(StageButtonSize.Y + 20.f);
	UCanvasPanel* Face = WidgetTree->ConstructWidget<UCanvasPanel>();
	Size->AddChild(Face);

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->SetStyle(StageButtonStyle);
	Button->SetCursor(EMouseCursor::Hand);
	UCanvasPanelSlot* ButtonSlot = Face->AddChildToCanvas(Button);
	ButtonSlot->SetPosition(FVector2D(0,16));
	ButtonSlot->SetSize(FVector2D(Width,StageButtonSize.Y));

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetText(Name);
	Label->SetJustification(ETextJustify::Center);
	Label->SetColorAndOpacity(StageTextColor);
	if (StageFont.HasValidFont())
	{
		Label->SetFont(StageFont);
	}
	// Shrink a name that doesn't fit inside the button's padding (Campaign's narrower two-line buttons)
	UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>();
	Fit->SetStretch(EStretch::ScaleToFit);
	Fit->SetStretchDirection(EStretchDirection::DownOnly);
	Fit->AddChild(Label);
	if (UButtonSlot* LabelSlot = Cast<UButtonSlot>(Button->AddChild(Fit)))
	{
		LabelSlot->SetHorizontalAlignment(HAlign_Fill);
		LabelSlot->SetVerticalAlignment(VAlign_Fill);
	}
	StageLabels.Add(Label);
	ULessonCompletionBadge* Check = WidgetTree->ConstructWidget<ULessonCompletionBadge>();
	Check->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* CheckSlot = Face->AddChildToCanvas(Check);
	CheckSlot->SetPosition(FVector2D(Width-52.f,0));
	CheckSlot->SetSize(FVector2D(52,52));
	CheckSlot->SetZOrder(2);
	StageChecks.Add(Check);
	UImage* Lock = WidgetTree->ConstructWidget<UImage>();
	Lock->SetBrushFromTexture(NotebookLockTexture);
	Lock->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* LockSlot = Face->AddChildToCanvas(Lock);
	LockSlot->SetPosition(FVector2D(Width-52.f,0));
	LockSlot->SetSize(FVector2D(52,52));
	LockSlot->SetZOrder(2);
	StageLocks.Add(Lock);

	UPanelSlot* StageSlot = Box->AddChild(Size);
	if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(StageSlot))
	{
		BoxSlot->SetPadding(FMargin(Spacing / 2.f, 0.f));
		BoxSlot->SetVerticalAlignment(VAlign_Center);
		BoxSlot->SetHorizontalAlignment(HAlign_Center);
		BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	ULessonStageClick* Click = NewObject<ULessonStageClick>(this);
	Click->Stage = Stage;
	Click->Popup = this;
	Button->OnClicked.AddDynamic(Click, &ULessonStageClick::HandleClicked);
	StageClicks.Add(Click);
	return Button;
}

void ULessonPopupWidget::StyleStage(UButton* Button, int32 Stage, bool bUnlocked, bool bCurrent, bool bCompleted)
{
	Button->SetIsEnabled(bUnlocked);
	Button->SetBackgroundColor(FLinearColor::White);
	Button->SetStyle(NotebookButtonStyle(bCurrent));
	if (StageLabels.IsValidIndex(Stage))
		StageLabels[Stage]->SetColorAndOpacity(bCurrent ? NotebookIvory : NotebookInk);
	if (StageChecks.IsValidIndex(Stage))
		StageChecks[Stage]->SetVisibility(bCompleted ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (StageLocks.IsValidIndex(Stage))
		StageLocks[Stage]->SetVisibility(!bUnlocked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

bool ULessonPopupWidget::IsShowingMaster() const
{
	// Not while the reveal animation runs: until it ends the popup shows the completed campaign
	const UCampaignSubsystem* Campaign = GetCampaign();
	return bPreviewMasterCompletion || (Campaign && Campaign->IsMasterRevealed(Lesson) && RevealPhase == ERevealPhase::None);
}

FText ULessonPopupWidget::GetMasterProgressText() const
{
	const UCampaignSubsystem* Campaign = GetCampaign();
	if (!Campaign)
	{
		return FText::GetEmpty();
	}
	const FText StageText = bPreviewMasterCompletion || Campaign->IsMasterCompleted(Lesson) ? LOCTEXT("MasterComplete", "Mastered")
		: UCampaignSubsystem::GetStageDisplayName(Campaign->GetCurrentMasterStage(Lesson));
	return FText::Format(LOCTEXT("MasterProgressFormat", "Master  {0} / {1}  -  {2}"),
		bPreviewMasterCompletion ? UCampaignSubsystem::GetMasterMaxPoints() : Campaign->GetMasterPoints(Lesson), UCampaignSubsystem::GetMasterMaxPoints(), StageText);
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
	TickMasterCollapse(InDeltaTime);
	TickMasterFireworks(InDeltaTime);
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
	ResetMasterCollapse();
	bPreviewMasterCompletion = false;
	Lesson = InLesson;
	Refresh();
	SetVisibility(ESlateVisibility::Visible);
}

void ULessonPopupWidget::PreviewMasterCompletion(ECampaignLesson InLesson)
{
	RevealPhase = ERevealPhase::None;
	HideEraser();
	ShowLessonAfterPuzzle(InLesson);
	bPreviewMasterCompletion = true;
	Refresh();
}

void ULessonPopupWidget::Refresh()
{
	// Lessons and Campaign have different stages
	if (StageButtons.Num() != UCampaignSubsystem::GetNumStages()
		|| (MasterBox && MasterButtons.Num() != UCampaignSubsystem::GetNumMasterStages()))
	{
		BuildStageButtons();
	}

	const bool bCampaignMode = bPreviewMasterCompletion || UCampaignSubsystem::GetMode() == ECampaignMode::Campaign;
	const UCampaignSubsystem* Campaign = GetCampaign();
	const int32 Points = Campaign ? Campaign->GetLessonPoints(Lesson) : 0;
	const int32 CurrentStage = Campaign ? Campaign->GetCurrentStage(Lesson) : 0;
	const bool bCompleted = bPreviewMasterCompletion || (Campaign && Campaign->IsLessonCompleted(Lesson));

	// Once the final is done (Campaign), the popup follows Master Mode: its bar, text and Play button
	const bool bMaster = IsShowingMaster();
	const bool bMasterDone = bPreviewMasterCompletion || (bMaster && Campaign->IsMasterCompleted(Lesson));
	const int32 MasterPoints = bPreviewMasterCompletion ? UCampaignSubsystem::GetMasterMaxPoints() : (bMaster ? Campaign->GetMasterPoints(Lesson) : 0);
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
	if (NotebookDiagram && NotebookDiagram->Lesson != Lesson)
	{
		// The tree's Slate diagram captures its lesson at construction; create a fresh face when switching clues.
		UCanvasPanelSlot* OldDiagramSlot = CastChecked<UCanvasPanelSlot>(NotebookDiagram->Slot);
		const FVector2D Position = OldDiagramSlot->GetPosition(), Size = OldDiagramSlot->GetSize();
		NotebookDiagram->RemoveFromParent();
		NotebookDiagram = WidgetTree->ConstructWidget<ULessonClueDiagram>();
		NotebookDiagram->Lesson = Lesson;
		NotebookDiagram->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* DiagramSlot = NotebookCanvas->AddChildToCanvas(NotebookDiagram);
		DiagramSlot->SetPosition(Position);
		DiagramSlot->SetSize(Size);
		DiagramSlot->SetZOrder(1);
		if (UScrollBox* Explanation = DescriptionText ? Cast<UScrollBox>(DescriptionText->GetParent()) : nullptr)
			Explanation->ScrollToStart();
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
	if (PlayTimeText)
	{
		const int32 Seconds = Campaign ? FMath::FloorToInt(Campaign->GetLessonPlaySeconds(Lesson)) : 0;
		PlayTimeText->SetText(FText::Format(LOCTEXT("PlayTime", "Time  {0}"),
			FText::FromString(FString::Printf(TEXT("%02d:%02d:%02d"), Seconds / 3600, Seconds / 60 % 60, Seconds % 60))));
	}
	if (CloseLabel)
	{
		CloseLabel->SetText(!bAllDone ? LOCTEXT("Close", "Back")
			: (bCampaignMode ? LOCTEXT("ReturnToCampaign", "Return to Campaign") : LOCTEXT("ReturnToLessons", "Return to Lessons")));
	}
	// Master Mode's stages take the Campaign stages' row, like the bar and text above them
	if (MasterBox)
	{
		MasterBox->SetVisibility(bMaster ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (StageBox)
	{
		StageBox->SetVisibility(bMaster && MasterBox ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
	const int32 FirstMaster = UCampaignSubsystem::GetFirstMasterStage();
	for (int32 i = 0; i < MasterButtons.Num(); i++)
	{
		const int32 Stage = FirstMaster + i;
		const bool bUnlocked = bPreviewMasterCompletion || (Campaign && Campaign->IsStageUnlocked(Lesson, Stage));
		const bool bIsCurrent = Stage == CurrentMaster && !bMasterDone;
		StyleStage(MasterButtons[i], Stage, bUnlocked, bIsCurrent, bUnlocked && !bIsCurrent);
	}

	for (int32 Stage = 0; Stage < StageButtons.Num(); Stage++)
	{
		const bool bUnlocked = Campaign ? Campaign->IsStageUnlocked(Lesson, Stage) : Stage == 0;
		const bool bIsCurrent = Stage == CurrentStage && !(Stage == UCampaignSubsystem::GetFinalStage() && bCompleted);

		StyleStage(StageButtons[Stage], Stage, bUnlocked, bIsCurrent,
			bCompleted || (bUnlocked && Stage < CurrentStage));
	}
	UpdateNotebookLayout();
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
	if (CollapseBadge.IsValid()) CollapseBadge->RevealMasterReward(CollapseColumn);
	RevealPhase = ERevealPhase::None;
	HideEraser();
	SetVisibility(ESlateVisibility::Collapsed);
	ResetMasterCollapse();
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
	if (CollapseElapsed >= 0.f) return;
	const int32 Column = UCampaignTree::GetLessonColumn(Lesson);
	TArray<UUserWidget*> Badges;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Badges, UPlayerLevelBadge::StaticClass(), false);
	for (UUserWidget* Widget : Badges)
	{
		UPlayerLevelBadge* Badge = CastChecked<UPlayerLevelBadge>(Widget);
		FGeometry GemGeometry;
		if (NotebookCanvas && Badge->GetPendingMasterReward(Column, GemGeometry))
		{
			// OnPaint's geometry is window-relative; cached/tick geometry includes the desktop window offset.
			const FGeometry Frame = NotebookCanvas->GetPaintSpaceGeometry();
			const FVector2D Size = Frame.GetLocalSize();
			if (Size.IsNearlyZero()) break;
			CollapseBadge = Badge;
			CollapseColumn = Column;
			CollapseElapsed = 0.f;
			CollapseMesh = SNew(SMasterCollapseMesh);
			CollapseMesh->SourceTopLeft = Frame.LocalToAbsolute(FVector2D::ZeroVector);
			CollapseMesh->SourceBottomRight = Frame.LocalToAbsolute(Size);
			CollapseMesh->Target = GemGeometry.LocalToAbsolute(GemGeometry.GetLocalSize() * .5f);
			FWidgetRenderer* Renderer = new FWidgetRenderer(true);
			CollapseTexture = Renderer->DrawWidget(NotebookCanvas->TakeWidget(), Size);
			BeginCleanup(Renderer);
			CollapseMesh->Brush.SetResourceObject(CollapseTexture);
			CollapseMesh->Brush.DrawAs = ESlateBrushDrawType::Image;
			CollapseMesh->Brush.ImageSize = Size;
			CollapseHost = WidgetTree->ConstructWidget<UNativeWidgetHost>();
			CollapseHost->SetContent(CollapseMesh.ToSharedRef());
			CollapseHost->SetVisibility(ESlateVisibility::HitTestInvisible);
			UCanvasPanelSlot* HostSlot = CastChecked<UCanvasPanel>(WidgetTree->RootWidget)->AddChildToCanvas(CollapseHost);
			HostSlot->SetAnchors(FAnchors(0, 0, 1, 1));
			HostSlot->SetOffsets(FMargin(0));
			HostSlot->SetZOrder(30);
			NotebookCanvas->SetVisibility(ESlateVisibility::Hidden);
			return;
		}
	}
	Close();
}

UPlayerLevelBadge* ULessonPopupWidget::GetPendingRewardBadge() const
{
	if (!RewardBadge.IsValid())
	{
		TArray<UUserWidget*> Badges;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Badges, UPlayerLevelBadge::StaticClass(), false);
		RewardBadge = Badges.Num() > 0 ? Cast<UPlayerLevelBadge>(Badges[0]) : nullptr;
	}
	FGeometry GemGeometry;
	UPlayerLevelBadge* Badge = RewardBadge.Get();
	return Badge && Badge->GetPendingMasterReward(UCampaignTree::GetLessonColumn(Lesson), GemGeometry) ? Badge : nullptr;
}

void ULessonPopupWidget::TickMasterFireworks(float DeltaTime)
{
	// Only before Return to Campaign starts the collapse: then no more go up, and those in the air fade over it
	const bool bWanted = IsVisible() && CollapseElapsed < 0.f && NotebookCanvas && IsShowingMaster() && GetPendingRewardBadge();
	if (!MasterFireworks)
	{
		UCanvasPanel* Root = bWanted ? Cast<UCanvasPanel>(WidgetTree->RootWidget) : nullptr;
		if (!Root)
		{
			return;
		}
		MasterFireworks = WidgetTree->ConstructWidget<UFireworksWidget>(UFireworksWidget::StaticClass(), TEXT("MasterFireworks"));
		MasterFireworks->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* FireworksSlot = Root->AddChildToCanvas(MasterFireworks);
		// Over the whole popup (and the collapse, Z 30); the popup takes the taps
		FireworksSlot->SetAnchors(FAnchors(0, 0, 1, 1));
		FireworksSlot->SetOffsets(FMargin(0));
		FireworksSlot->SetZOrder(40);
		// Laid out next frame
		return;
	}
	if (bWanted && !MasterFireworks->HasBurstsLeft())
	{
		QueueMasterFireworks();
	}
	MasterFireworks->Advance(DeltaTime);
}

bool ULessonPopupWidget::QueueMasterFireworks()
{
	const FGeometry& Area = MasterFireworks->GetCachedGeometry();
	const FGeometry& Target = NotebookCanvas->GetCachedGeometry();
	if (Area.GetLocalSize().X <= 0.f || Target.GetLocalSize().X <= 0.f || Area.Scale <= 0.f)
	{
		return false;
	}
	// Round the notebook, in the fireworks' space; sizes go with the notebook's (1500 wide in its layout). As the
	// lessons' congratulations (UHappinessModeWidget::PlayUnlockFireworks), a round about every 2.6 seconds.
	const FVector2D Center = Area.AbsoluteToLocal(Target.GetAbsolutePositionAtCoordinates(FVector2D(.5f)));
	const FVector2D Size = Target.GetAbsoluteSize() / Area.Scale;
	const float Unit = 1.3f * Size.X / 1500.f;
	// Offsets in notebook sizes, from its center
	const struct { float X, Y, Delay; } Plan[] =
	{
		{ 0.f, -.3f, .2f }, { -.36f, -.14f, .55f }, { .36f, -.2f, .85f }, { -.2f, .16f, 1.2f },
		{ .25f, .1f, 1.5f }, { 0.f, -.04f, 1.8f }, { -.42f, .3f, 2.1f }, { .42f, .28f, 2.35f },
	};
	TArray<UFireworksWidget::FBurst> Bursts;
	for (const auto& Step : Plan)
	{
		UFireworksWidget::FBurst& Burst = Bursts.AddDefaulted_GetRef();
		Burst.Position = Center + FVector2D(Step.X * Size.X, Step.Y * Size.Y);
		Burst.Delay = Step.Delay;
		// Each rocket climbs from the notebook's bottom edge
		Burst.RocketRise = Center.Y + Size.Y * .5f - Burst.Position.Y;
	}
	MasterFireworks->AddBursts(Bursts, Unit);
	return true;
}

void ULessonPopupWidget::ResetMasterCollapse()
{
	if (MasterFireworks) MasterFireworks->Stop();
	if (CollapseHost) CollapseHost->RemoveFromParent();
	CollapseHost = nullptr;
	CollapseMesh.Reset();
	CollapseTexture = nullptr;
	CollapseElapsed = -1.f;
	CollapseBadge.Reset();
	CollapseColumn = INDEX_NONE;
	if (NotebookCanvas)
	{
		NotebookCanvas->SetRenderScale(FVector2D(1.f));
		NotebookCanvas->SetRenderTranslation(FVector2D::ZeroVector);
		NotebookCanvas->SetRenderOpacity(1.f);
		NotebookCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (WidgetTree) WidgetTree->ForEachWidget([](UWidget* Widget)
	{
		if (Widget->GetFName() == TEXT("Dim") || Widget->IsA<UBackgroundBlur>()) Widget->SetRenderOpacity(1.f);
	});
}

void ULessonPopupWidget::TickMasterCollapse(float DeltaTime)
{
	if (CollapseElapsed < 0.f || !NotebookCanvas) return;
	CollapseElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(CollapseElapsed / FMath::Max(.1f, GMasterCollapseDuration.GetValueOnGameThread()), 0.f, 1.f);
	if (CollapseMesh)
	{
		FGeometry GemGeometry;
		if (CollapseBadge.IsValid() && CollapseBadge->GetPendingMasterReward(CollapseColumn, GemGeometry))
			CollapseMesh->Target = GemGeometry.LocalToAbsolute(GemGeometry.GetLocalSize() * .5f);
		CollapseMesh->Progress = Alpha;
		CollapseMesh->Invalidate(EInvalidateWidgetReason::Paint);
	}
	if (WidgetTree) WidgetTree->ForEachWidget([Alpha](UWidget* Widget)
	{
		if (Widget->GetFName() == TEXT("Dim") || Widget->IsA<UBackgroundBlur>())
			Widget->SetRenderOpacity(1.f - FMath::Clamp(Alpha * 3.f, 0.f, 1.f));
	});
	if (Alpha >= 1.f) Close();
}

void ULessonPopupWidget::HandleLearnMoreClicked()
{
	OnLearnMoreRequested.Broadcast(Lesson);
}

void ULessonPopupWidget::HandleProgressChanged()
{
	Refresh();
}

#undef LOCTEXT_NAMESPACE
