#include "UI/CampaignTreeWidget.h"

#include "HappinessClassic/CampaignProgress.h"
#include "UI/CompleteLessonsPanel.h"
#include "HappinessClassic/CampaignTree.h"
#include "UI/GameRulesWidget.h"
#include "UI/LessonPopupWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Fonts/FontMeasure.h"
#include "Engine/Font.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/SLeafWidget.h"


namespace
{
	class SLessonMedallion : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SLessonMedallion)
		{
		}
		SLATE_ARGUMENT(TWeakObjectPtr<ULessonMedallion>, Owner) SLATE_END_ARGS() void Construct(const FArguments& Args)
		{
			Owner = Args._Owner;
		}
		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(240, 176);
		}
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& AllottedGeometry, const FSlateRect&,
							  FSlateWindowElementList& E, int32 Layer, const FWidgetStyle&, bool) const override
		{
			const FGeometry G = AllottedGeometry.MakeChild(
				FVector2D(240, 176), FSlateLayoutTransform(float(AllottedGeometry.GetLocalSize().X / 240.f)));
			const ULessonMedallion* M = Owner.Get();
			if (!M)
				return Layer;
			FSlateBrush Art;
			Art.SetResourceObject(M->Texture);
			Art.DrawAs = ESlateBrushDrawType::Image;
			Art.SetUVRegion(FBox2f(FVector2f(.09725f, .04513f), FVector2f(.90275f, .94043f)));
			const FLinearColor Tint = M->bLocked ? FLinearColor(.70f, .70f, .64f) : FLinearColor::White;
			// Keep the surface opaque even when its button is disabled; connectors must never show through a node.
			FSlateDrawElement::MakeBox(E, Layer, G.ToPaintGeometry(), &Art, ESlateDrawEffect::None, Tint);
			const FLinearColor Ink = M->bLocked ? FLinearColor(.20f, .15f, .095f) : FLinearColor(.105f, .047f, .018f);
			FSlateFontInfo Font = M->LabelFont;
            if (M->Lesson == ECampaignLesson::ThreeNot) Font.Size=18;
			const auto Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			TArray<FString> Words, Lines;
			UCampaignTree::GetLessonDisplayName(M->Lesson).ToString().ParseIntoArrayWS(Words);
			FString Current;
			for (const FString& Word : Words)
			{
				FString Candidate = Current.IsEmpty() ? Word : Current + TEXT(" ") + Word;
				if (!Current.IsEmpty() && Measure->Measure(Candidate, Font).X > 188.f)
				{
					Lines.Add(Current);
					Current = Word;
				}
				else
					Current = Candidate;
			}
			if (!Current.IsEmpty())
				Lines.Add(Current);
			if (M->Lesson == ECampaignLesson::ThreeNot)
            {
                Lines = {TEXT("Two Together,"), TEXT("One Not")};
            }
            const float LineHeight = 20.f;
            const float FontHeight = Measure->Measure(TEXT("Ag"), Font).Y;
			for (int32 I = 0; I < Lines.Num(); ++I)
			{
				const FVector2D Size = Measure->Measure(Lines[I], Font);
				FSlateDrawElement::MakeText(
					E, Layer + 1,
					G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(
														   (240.f - Size.X) * .5f,
														   143.f - FontHeight * .5f - (Lines.Num()-1) * LineHeight * .5f + I * LineHeight))),
					FText::FromString(Lines[I]), Font, ESlateDrawEffect::None, Ink);
			}
            if (M->bLocked && M->LockTexture)
            {
                FSlateBrush Lock;
                Lock.SetResourceObject(M->LockTexture);
                Lock.DrawAs=ESlateBrushDrawType::Image;
                FSlateDrawElement::MakeBox(E, Layer+1,
                    G.ToPaintGeometry(FVector2f(50,50), FSlateLayoutTransform(FVector2f(95,162))),
                    &Lock, ESlateDrawEffect::None, FLinearColor::White);
            }
			return Layer + 7;
		}

	private:
		TWeakObjectPtr<ULessonMedallion> Owner;
	};


    class SLessonCompletionBadge : public SLeafWidget
    {
    public:
        SLATE_BEGIN_ARGS(SLessonCompletionBadge) {}
        SLATE_ARGUMENT(TWeakObjectPtr<ULessonCompletionBadge>, Owner)
        SLATE_END_ARGS()
        void Construct(const FArguments& Args) { Owner = Args._Owner; }
        virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(52,52); }
        virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& E,
                             int32 Layer,const FWidgetStyle&,bool) const override
        {
            const ULessonCompletionBadge* Badge=Owner.Get();
            const float Shimmer=Badge ? Badge->Shimmer : -1.f;
            const float Glow=Badge ? Badge->Glow : 0.f;
            const FVector2f Center(26,26);
            auto Disc=[&](float Radius,FLinearColor Color,int32 Offset)
            {
                FSlateRoundedBoxBrush Brush(Color,Radius);
                FSlateDrawElement::MakeBox(E,Layer+Offset,
                    G.ToPaintGeometry(FVector2f(Radius*2),FSlateLayoutTransform(Center-FVector2f(Radius))),
                    &Brush,ESlateDrawEffect::None,Color);
            };
            if (Glow>0.f)
            {
                Disc(25.f+12.f*Glow,FLinearColor(1.f,.84f,.40f,.22f*Glow),0);
                Disc(25.f+6.f*Glow,FLinearColor(1.f,.90f,.56f,.32f*Glow),0);
            }
            Disc(25,FLinearColor(.008f,.018f,.035f,.72f),1);
            Disc(23,FLinearColor(.94f,.62f,.15f),2);
            Disc(20,FLinearColor(1.f,.96f,.81f),3);
            Disc(17,FLinearColor(.025f,.075f,.15f),4);
            Disc(14,FLinearColor(.055f,.20f,.38f),5);
            // The shimmer: a soft diagonal band of light, made of chords of the seal, sweeping from top left to bottom right
            const FVector2f Across(.7071f,.7071f), Along(.7071f,-.7071f);
            const float BandAt=FMath::Lerp(-34.f,34.f,Shimmer);
            if (Shimmer>=0.f)
            {
                for (int32 K=-7;K<=7;++K)
                {
                    const float Offset=BandAt+K*1.5f, Radius=22.5f;
                    if (FMath::Abs(Offset)>=Radius)
                        continue;
                    const float Half=FMath::Sqrt(Radius*Radius-Offset*Offset);
                    const FVector2f Mid=Center+Across*Offset;
                    TArray<FVector2f> Chord={Mid-Along*Half,Mid+Along*Half};
                    FSlateDrawElement::MakeLines(E,Layer+6,G.ToPaintGeometry(),Chord,ESlateDrawEffect::None,
                        FLinearColor(1.f,.97f,.86f,.42f*FMath::Exp(-FMath::Square(K/3.5f))),true,1.8f);
                }
            }
            TArray<FVector2f> Check={FVector2f(14,26),FVector2f(21,33),FVector2f(37,16)};
            FSlateDrawElement::MakeLines(E,Layer+7,G.ToPaintGeometry(),Check,ESlateDrawEffect::None,
                FLinearColor(.008f,.025f,.055f),true,5.4f);
            FSlateDrawElement::MakeLines(E,Layer+8,G.ToPaintGeometry(),Check,ESlateDrawEffect::None,
                FLinearColor(1.f,.99f,.88f),true,3.5f);
            if (Shimmer>=0.f)
            {
                // Where the band crosses the check, it shines
                TArray<FVector2f> Points;
                TArray<FLinearColor> Colors;
                for (int32 Segment=0;Segment<2;++Segment)
                {
                    for (int32 I=Segment==0 ? 0 : 1;I<=12;++I)
                    {
                        const FVector2f P=FMath::Lerp(Check[Segment],Check[Segment+1],I/12.f);
                        Points.Add(P);
                        const float Distance=FVector2f::DotProduct(P-Center,Across)-BandAt;
                        Colors.Add(FLinearColor(1.f,1.f,1.f,FMath::Exp(-FMath::Square(Distance/5.f))));
                    }
                }
                FSlateDrawElement::MakeLines(E,Layer+9,G.ToPaintGeometry(),Points,Colors,ESlateDrawEffect::None,
                    FLinearColor::White,true,5.f);
            }
            return Layer+9;
        }
    private:
        TWeakObjectPtr<ULessonCompletionBadge> Owner;
    };

	// Abstract lesson diagrams use the same three entities throughout: circle, square, triangle.
	class SLessonClueDiagram : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SLessonClueDiagram)
		{
		}
		SLATE_ARGUMENT(ECampaignLesson, Lesson) SLATE_END_ARGS() void Construct(const FArguments& Args)
		{
			Lesson = Args._Lesson;
		}
		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(170.f, 94.f);
		}
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& E,
							  int32 Layer, const FWidgetStyle&, bool) const override
		{
			FVector2f Min(22, 12), Max(150, 70);
            switch (Lesson)
            {
            case ECampaignLesson::VerticalTwo: Min=FVector2f(46,2); Max=FVector2f(124,57); break;
            case ECampaignLesson::VerticalThree:
            case ECampaignLesson::ThreeNot:
            case ECampaignLesson::AllApart: Min=FVector2f(46,2); Max=FVector2f(124,84); break;
            case ECampaignLesson::TwoNot: Min=FVector2f(41,12); Max=FVector2f(128,72); break;
            case ECampaignLesson::NextTo:
            case ECampaignLesson::DirectlyLeftOf: Min=FVector2f(46,12); Max=FVector2f(125,70); break;
            case ECampaignLesson::EitherOr: Min=FVector2f(15,0); Max=FVector2f(155,82); break;
            case ECampaignLesson::NextToEitherOr: Min=FVector2f(2,6); Max=FVector2f(168,80); break;
            default: break;
            }
            const FVector2f Extent=Max-Min;
            const float Fit=FMath::Min(G.GetLocalSize().X/Extent.X,G.GetLocalSize().Y/Extent.Y);
            const FVector2f Scale(Fit,Fit);
            const FVector2f Origin=(FVector2f(G.GetLocalSize())-Extent*Fit)*.5f-Min*Fit;
            auto ScaledSize = [&](float X, float Y) { return FVector2f(X*Fit,Y*Fit); };
			const FLinearColor Sage(.10f, .23f, .055f), Ink(.25f, .16f, .07f), Red(.65f, .055f, .015f),
				Ivory(.97f, .87f, .65f);
			auto Point = [&](float X, float Y) { return Origin + FVector2f(X * Scale.X, Y * Scale.Y); };
			auto Line = [&](float X1, float Y1, float X2, float Y2, FLinearColor C, float W = 2.f)
			{
				TArray<FVector2f> P = {Point(X1, Y1), Point(X2, Y2)};
				FSlateDrawElement::MakeLines(E, Layer + 2, G.ToPaintGeometry(), P, ESlateDrawEffect::None, C, true,
											 W * .65f);
			};
			auto Box =
				[&](float X, float Y, float W, float H, FLinearColor C, float Radius = 2.f, int32 LayerOffset = 0)
			{
				FSlateRoundedBoxBrush Brush(C, Radius * FMath::Min(Scale.X, Scale.Y), Ink, 1.f);
				FSlateDrawElement::MakeBox(E, Layer + LayerOffset,
										   G.ToPaintGeometry(ScaledSize(W, H), FSlateLayoutTransform(Point(X, Y))), &Brush,
										   ESlateDrawEffect::None, C);
			};
            auto Text = [&](float X, float Y, const TCHAR* T, int32 Size, FLinearColor C)
            {
                const FSlateFontInfo Font=FCoreStyle::GetDefaultFontStyle("Bold", FMath::RoundToInt(Size*Fit));
                const FVector2f Extent(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(T,Font));
                FSlateDrawElement::MakeText(E,Layer+3,
                    G.ToPaintGeometry(Extent,FSlateLayoutTransform(Point(X,Y)-Extent*.5f)),
                    FText::FromString(T),Font,ESlateDrawEffect::None,C);
            };
			auto Marker = [&](int32 Shape, float X, float Y)
			{
				if (Shape == 0)
					Box(X - 8, Y - 8, 16, 16, Sage, 8.f, 1);
				else if (Shape == 1)
					Box(X - 8, Y - 8, 16, 16, Sage, 2.f, 1);
				else
				{
					// A filled triangle with a consistent physical size, rather than a font-dependent glyph.
					for (int32 I = 0; I < 16; ++I)
					{
						const float Half = I * .55f;
						Line(X - Half, Y - 9 + I, X + Half, Y - 9 + I, Sage, 2.f);
					}
				}
			};
			auto Cross = [&](float X, float Y)
			{
				Line(X - 6, Y - 6, X + 6, Y + 6, Red, 3.f);
				Line(X - 6, Y + 6, X + 6, Y - 6, Red, 3.f);
			};
			auto Arrow = [&](float X1, float X2, float Y, bool Both)
			{
				Line(X1, Y, X2, Y, Sage, 2.5f);
				Line(X2 - 6, Y - 5, X2, Y, Sage, 2.5f);
				Line(X2 - 6, Y + 5, X2, Y, Sage, 2.5f);
				if (Both)
				{
					Line(X1 + 6, Y - 5, X1, Y, Sage, 2.5f);
					Line(X1 + 6, Y + 5, X1, Y, Sage, 2.5f);
				}
			};
			auto Grid = [&](int32 Cols, int32 Rows, float X, float Y, float W, float H)
			{
				for (int32 R = 0; R < Rows; ++R)
					for (int32 C = 0; C < Cols; ++C)
						Box(X + C * W, Y + R * H, W - 2, H - 2, Ivory);
			};
			switch (Lesson)
			{
				case ECampaignLesson::VerticalTwo:
				case ECampaignLesson::VerticalThree:
				{
					const int32 Rows = Lesson == ECampaignLesson::VerticalTwo ? 2 : 3;
					Grid(3, Rows, 48, 4, 25, 26);
					for (int32 R = 0; R < Rows; ++R)
						Marker(R, 84, 16 + 26 * R);
					break;
				}
				case ECampaignLesson::NextTo:
				case ECampaignLesson::DirectlyLeftOf:
					Grid(2, 1, 48, 32, 38, 36);
					Marker(0, 66, 49);
					Marker(2, 104, 49);
					Arrow(49, 121, 20, Lesson == ECampaignLesson::NextTo);
					break;
				case ECampaignLesson::Span:
					Grid(3, 1, 34, 34, 34, 34);
					Marker(0, 50, 50);
					Marker(1, 84, 50);
					Marker(2, 118, 50);
					Arrow(35, 134, 20, true);
					break;
				case ECampaignLesson::Edge:
					for (int32 C = 0; C < 5; ++C)
						Box(24 + C * 25, 34, 23, 32, (C == 0 || C == 4) ? Sage : Ivory);
					break;
				case ECampaignLesson::TwoNot:
					Grid(3, 2, 43, 14, 28, 28);
					Marker(0, 56, 27);
					Marker(2, 112, 55);
					Cross(56, 55);
					break;
				case ECampaignLesson::ThreeNot:
					Grid(3, 3, 48, 5, 25, 26);
					Marker(0, 60, 17);
					Marker(1, 60, 43);
					Marker(2, 110, 69);
					Cross(60, 69);
					break;
				case ECampaignLesson::NotNextTo:
					Grid(3, 1, 34, 34, 34, 34);
					Marker(0, 50, 50);
					Marker(2, 118, 50);
					Line(42, 26, 42, 20, Ink);
					Line(42, 20, 90, 20, Ink);
					Line(90, 20, 90, 26, Ink);
					Cross(66, 19);
					break;
				case ECampaignLesson::SpanNotSide:
					Grid(3, 1, 34, 34, 34, 34);
					Marker(0, 50, 50);
					Marker(1, 84, 50);
					Marker(2, 118, 50);
					Cross(118, 50);
					Line(38, 25, 38, 18, Ink);
					Line(38, 18, 98, 18, Ink);
					Line(98, 18, 98, 25, Ink);
					break;
				case ECampaignLesson::Gap:
				case ECampaignLesson::SpanNotMid:
					Grid(3, 1, 34, 34, 34, 34);
					Marker(0, 50, 50);
					Marker(2, 118, 50);
					Arrow(35, 134, 20, true);
					if (Lesson == ECampaignLesson::SpanNotMid)
					{
						Marker(1, 84, 50);
						Cross(84, 50);
					}
					break;
				case ECampaignLesson::LeftOf:
				case ECampaignLesson::NotLeftOf:
					Grid(5, 1, 24, 34, 25, 32);
					Marker(0, 35, 49);
					Marker(2, 135, 49);
					Arrow(29, 145, 20, false);
					if (Lesson == ECampaignLesson::NotLeftOf)
						Cross(85, 20);
					break;
				case ECampaignLesson::Between:
				case ECampaignLesson::Chain:
					Grid(5, 1, 24, 34, 25, 32);
					Marker(0, 35, 49);
					Marker(1, 85, 49);
					Marker(2, 135, 49);
					Arrow(29, 145, 20, Lesson == ECampaignLesson::Between);
					break;
				case ECampaignLesson::AllApart:
					Grid(3, 3, 48, 5, 25, 26);
					Marker(0, 60, 17);
					Marker(1, 85, 43);
					Marker(2, 110, 69);
					break;
                case ECampaignLesson::EitherOr:
                    // The same focal circle shares a column with one candidate or the other.
                    Grid(1,2,25,9,30,32);
                    Marker(0,39,24);
                    Marker(1,39,56);
                    Grid(1,2,115,9,30,32);
                    Marker(0,129,24);
                    Marker(2,129,56);
                    Box(69,30,32,24,Ivory,8.f);
                    Text(85,42,TEXT("OR"),11,Ink);
                    break;
                case ECampaignLesson::NextToEitherOr:
                    // Two readable neighboring pairs replace the crowded four-column alternatives.
                    Grid(2,1,5,34,29,32);
                    Marker(0,18,49);
                    Marker(1,47,49);
                    Arrow(8,59,22,true);
                    Grid(2,1,107,34,29,32);
                    Marker(0,120,49);
                    Marker(2,149,49);
                    Arrow(110,161,22,true);
                    Box(69,37,32,24,Ivory,8.f);
                    Text(85,49,TEXT("OR"),11,Ink);
                    break;
				default:
					break;
			}
			return Layer + 3;
		}

	private:
		ECampaignLesson Lesson;
	};

	constexpr float LessonColumnSpacing = 320.f;
	constexpr float LessonCanvasHeight = 888.f;

	/** Paints the tree's connectors in its own place in the widget hierarchy */
	class SCampaignTreeLines : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SCampaignTreeLines)
		{
		}
		SLATE_ARGUMENT(TWeakObjectPtr<UCampaignTreeWidget>, Tree)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Tree = InArgs._Tree;
		}

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
							  const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
							  const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			const UCampaignTreeWidget* Owner = Tree.Get();
			return Owner ? Owner->PaintConnectors(AllottedGeometry, OutDrawElements, LayerId) : LayerId;
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D::ZeroVector;
		}

	private:
		TWeakObjectPtr<UCampaignTreeWidget> Tree;
	};
} // namespace

TSharedRef<SWidget> ULessonMedallion::RebuildWidget()
{
	return SNew(SLessonMedallion).Owner(this);
}

TSharedRef<SWidget> ULessonCompletionBadge::RebuildWidget()
{
    return SNew(SLessonCompletionBadge).Owner(this);
}

TSharedRef<SWidget> ULessonClueDiagram::RebuildWidget()
{
	return SNew(SLessonClueDiagram).Lesson(Lesson);
}

void UCampaignTreeNodeClick::HandleHovered()
{
	if (UButton* Node = Button.Get())
		Node->SetRenderScale(FVector2D(1.035f));
}

void UCampaignTreeNodeClick::HandleUnhovered()
{
	if (UButton* Node = Button.Get())
		Node->SetRenderScale(FVector2D(1.f));
}

TSharedRef<SWidget> UCampaignTreeLines::RebuildWidget()
{
	return SNew(SCampaignTreeLines).Tree(Tree);
}

void UCampaignTreeNodeClick::HandleClicked()
{
	if (UCampaignTreeWidget* Owner = Tree.Get())
	{
		Owner->NotifyNodeClicked(Lesson);
	}
}

UCampaignTreeWidget::UCampaignTreeWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	RulesClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/Happiness/UI/WBP_GameRules.WBP_GameRules_C")));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Medallion(
		TEXT("/Game/General/Textures/T_LessonMedallion.T_LessonMedallion"));
	MedallionTexture = Medallion.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> Badge(
		TEXT("/Game/General/Textures/T_LessonsGroupBadge.T_LessonsGroupBadge"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Plaque(
		TEXT("/Game/General/Textures/T_LessonsStatusPlaque.T_LessonsStatusPlaque"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Frame(
		TEXT("/Game/General/Textures/T_VillaPaintedFrame.T_VillaPaintedFrame"));
	GroupBadgeTexture = Badge.Object;
	StatusPlaqueTexture = Plaque.Object;
	ViewportFrameTexture = Frame.Object;
	static ConstructorHelpers::FObjectFinder<USoundBase> Check(TEXT("/Game/General/Audio/Cues/Happiness.Happiness"));
	CheckSound = Check.Object;
	// Retain the public style properties for existing Blueprint references.
	NodeStyle = GetDefault<UButton>()->GetStyle();

	// Small enough that two-line lesson names fit a node
	NodeFont = FCoreStyle::GetDefaultFontStyle("Bold", 13);
}

void UCampaignTreeWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (OverallProgress)
		OverallProgress->SetVisibility(ESlateVisibility::Collapsed);

	// PreConstruct also runs in the UMG designer, so the tree shows up in the preview
	RebuildTree();
}

void UCampaignTreeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BackButton)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &UCampaignTreeWidget::HandleBackClicked);
	}

	if (LessonPopup)
	{
		LessonPopup->OnStageChosen.AddUniqueDynamic(this, &UCampaignTreeWidget::HandlePopupStageChosen);
		LessonPopup->OnLearnMoreRequested.AddUniqueDynamic(this, &UCampaignTreeWidget::HandleLearnMore);
	}

	if (UCampaignSubsystem* Campaign =
			GetGameInstance() ? GetGameInstance()->GetSubsystem<UCampaignSubsystem>() : nullptr)
	{
		Campaign->OnProgressChanged.AddUniqueDynamic(this, &UCampaignTreeWidget::HandleProgressChanged);
	}
	LoadProgress();
}

void UCampaignTreeWidget::HandlePopupStageChosen(ECampaignLesson Lesson, int32 Stage)
{
	OnLessonStageChosen.Broadcast(Lesson, Stage);
}

void UCampaignTreeWidget::HandleLearnMore(ECampaignLesson Lesson)
{
	if (!RulesWidget)
	{
		UClass* Loaded = RulesClass.LoadSynchronous();
		RulesWidget = Loaded ? CreateWidget<UUserWidget>(this, Loaded) : nullptr;
		if (!RulesWidget)
		{
			return;
		}
		// Over the whole screen, above the tree and the popup; closing it returns to the popup
		UCanvasPanel* Root = Cast<UCanvasPanel>(GetRootWidget());
		if (UCanvasPanelSlot* RulesSlot = Root ? Root->AddChildToCanvas(RulesWidget) : nullptr)
		{
			RulesSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			RulesSlot->SetOffsets(FMargin(0.f));
			RulesSlot->SetZOrder(300);
		}
		else
		{
			RulesWidget->AddToViewport(300);
		}
	}
	if (UGameRulesWidget* Rules = Cast<UGameRulesWidget>(RulesWidget))
	{
		Rules->ShowLesson(GetOwningPlayer(), Lesson);
	}
	else
	{
		RulesWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void UCampaignTreeWidget::ShowForLessonSession()
{
	LoadProgress();
	SetVisibility(ESlateVisibility::Visible);

	UCampaignSubsystem* Campaign =
		GetGameInstance() ? GetGameInstance()->GetSubsystem<UCampaignSubsystem>() : nullptr;
	ECampaignLesson Completed;
	if (Campaign && Campaign->ConsumeLessonCompletion(Completed) && IsLessonCompleted(Completed))
	{
		// Back from the lesson's final: the lesson is done, so instead of its popup the tree celebrates it
		if (LessonPopup)
		{
			LessonPopup->SetVisibility(ESlateVisibility::Collapsed);
		}
		StartCelebration(Completed);
		return;
	}
	if (LessonPopup && Campaign && Campaign->IsInLessonSession())
	{
		// Reopened after a puzzle, not by the player: a newly unlocked Master Mode stays hidden
		LessonPopup->ShowLessonAfterPuzzle(Campaign->GetSessionLesson());
	}
}

void UCampaignTreeWidget::NativeDestruct()
{
	if (UCampaignSubsystem* Campaign =
			GetGameInstance() ? GetGameInstance()->GetSubsystem<UCampaignSubsystem>() : nullptr)
	{
		Campaign->OnProgressChanged.RemoveDynamic(this, &UCampaignTreeWidget::HandleProgressChanged);
	}
	Super::NativeDestruct();
}

void UCampaignTreeWidget::HandleProgressChanged()
{
	LoadProgress();
}

void UCampaignTreeWidget::LoadProgress()
{
	if (const UCampaignSubsystem* Campaign =
			GetGameInstance() ? GetGameInstance()->GetSubsystem<UCampaignSubsystem>() : nullptr)
	{
		SetCompletedLessons(Campaign->GetCompletedLessons());
	}
}

void UCampaignTreeWidget::HandleBackClicked()
{
	Close();
}

void UCampaignTreeWidget::SetVisibility(ESlateVisibility InVisibility)
{
	if (InVisibility == ESlateVisibility::Visible && GetVisibility() != InVisibility)
	{
		bCenterCurrentColumn = true;
	}
	Super::SetVisibility(InVisibility);
	// A press from before it was shown or hidden isn't a pan of the tree
	bPanCandidate = false;
	bDraggingTree = false;
	if (CompleteLessons)
	{
		CompleteLessons->Refresh();
	}
	if (InVisibility == ESlateVisibility::Collapsed || InVisibility == ESlateVisibility::Hidden)
	{
		EndCelebration();
	}
}

void UCampaignTreeWidget::Close()
{
	SetVisibility(ESlateVisibility::Collapsed);
	OnClosed.Broadcast();
}

void UCampaignTreeWidget::RebuildTree()
{
	if (!TreeCanvas || !WidgetTree)
		return;
	if (!LessonSerifFont)
		LessonSerifFont =
			LoadObject<UFont>(nullptr, TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	TreeCanvas->ClearChildren();
	NodeButtons.Reset();
	NodeClicks.Reset();
	NodeLessons.Reset();
	Medallions.Reset();
    CompletionBadges.Reset();
	ColumnStatusLabels.Reset();
	ColumnStatusPanels.Reset();
	LastViewportWidth = 0.f;
	LastCompletedColumns = INDEX_NONE;
	bCenterCurrentColumn = true;
	bPanCandidate = false;
	bDraggingTree = false;

	auto FillCanvas = [](UCanvasPanelSlot* CanvasSlot)
	{
		CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		CanvasSlot->SetOffsets(FMargin(0.f));
	};
	UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>();
	Blur->SetBlurStrength(5.f);
	Blur->SetVisibility(ESlateVisibility::HitTestInvisible);
	FillCanvas(TreeCanvas->AddChildToCanvas(Blur));
	UImage* Tint = WidgetTree->ConstructWidget<UImage>();
	Tint->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.82f, .80f, .60f, .13f), 0.f));
	Tint->SetVisibility(ESlateVisibility::HitTestInvisible);
	FillCanvas(TreeCanvas->AddChildToCanvas(Tint));
	// The complete viewport frame stays fixed while the lesson content pans beneath it.
	UImage* Rim = WidgetTree->ConstructWidget<UImage>();
	FSlateBrush ViewportRim;
	ViewportRim.SetResourceObject(ViewportFrameTexture);
	ViewportRim.DrawAs = ESlateBrushDrawType::Box;
	ViewportRim.ImageSize = FVector2D(240, 240);
	ViewportRim.Margin = FMargin(.12f);
	Rim->SetBrush(ViewportRim);
	Rim->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* RimSlot = TreeCanvas->AddChildToCanvas(Rim);
	FillCanvas(RimSlot);
	RimSlot->SetZOrder(3);
    auto AddViewportEdge = [&](float Inset, FLinearColor Color, float Width)
    {
        UImage* Edge=WidgetTree->ConstructWidget<UImage>();
        Edge->SetBrush(FSlateRoundedBoxBrush(FLinearColor::Transparent, 5.f, Color, Width));
        Edge->SetVisibility(ESlateVisibility::HitTestInvisible);
        UCanvasPanelSlot* EdgeSlot=TreeCanvas->AddChildToCanvas(Edge);
        FillCanvas(EdgeSlot);
        EdgeSlot->SetOffsets(FMargin(Inset));
        EdgeSlot->SetZOrder(4);
    };
    AddViewportEdge(1.f,FLinearColor(.95f,.85f,.60f),7.f);
    AddViewportEdge(8.f,FLinearColor(.16f,.25f,.075f),3.f);
    AddViewportEdge(11.f,FLinearColor(.98f,.93f,.77f),2.f);

	TreeScroll = WidgetTree->ConstructWidget<UScrollBox>();
	TreeScroll->SetOrientation(Orient_Horizontal);
	TreeScroll->SetScrollBarVisibility(ESlateVisibility::Collapsed);
	TreeScroll->SetAllowOverscroll(false);
	TreeScroll->SetAllowRightClickDragScrolling(true);
	TreeScroll->SetIsTouchScrollingEnabled(true);
	TreeScroll->SetConsumeMouseWheel(EConsumeMouseWheel::Always);
	TreeScroll->SetClipping(EWidgetClipping::ClipToBounds);
	UCanvasPanelSlot* TreeScrollSlot = TreeCanvas->AddChildToCanvas(TreeScroll);
	FillCanvas(TreeScrollSlot);
	TreeScrollSlot->SetZOrder(1);
	ScrollContent = WidgetTree->ConstructWidget<USizeBox>();
	ScrollContent->SetWidthOverride(UCampaignTree::GetNumColumns() * LessonColumnSpacing);
	ScrollContent->SetHeightOverride(LessonCanvasHeight);
	UScrollBoxSlot* ContentSlot = Cast<UScrollBoxSlot>(TreeScroll->AddChild(ScrollContent));
	ContentSlot->SetVerticalAlignment(VAlign_Center);
	// Fit the complete design canvas, including group badges and completion plaques, to the viewport height.
	// The outer size also shrinks in width so scroll offsets and hit testing use the visible content bounds.
	UScaleBox* FitTree = WidgetTree->ConstructWidget<UScaleBox>();
	FitTree->SetStretch(EStretch::ScaleToFit);
	FitTree->SetStretchDirection(EStretchDirection::DownOnly);
	ScrollContent->AddChild(FitTree);
	USizeBox* DesignSize = WidgetTree->ConstructWidget<USizeBox>();
	DesignSize->SetWidthOverride(UCampaignTree::GetNumColumns() * LessonColumnSpacing);
	DesignSize->SetHeightOverride(LessonCanvasHeight);
	FitTree->AddChild(DesignSize);
	UCanvasPanel* ScrollCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	DesignSize->AddChild(ScrollCanvas);
	NodeCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	const auto& Columns = UCampaignTree::GetColumns();
	UCanvasPanelSlot* NodeCanvasSlot = ScrollCanvas->AddChildToCanvas(NodeCanvas);
	NodeCanvasSlot->SetSize(FVector2D(Columns.Num() * LessonColumnSpacing, LessonCanvasHeight));

	Lines = WidgetTree->ConstructWidget<UCampaignTreeLines>();
	Lines->Tree = this;
	Lines->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* LinesSlot=NodeCanvas->AddChildToCanvas(Lines);
	FillCanvas(LinesSlot);
	LinesSlot->SetZOrder(0);
	const FLinearColor Ink(.105f, .047f, .018f), Ivory(1.f, .94f, .77f);
	// Every node child gets its own ZOrder: with explicit canvas ZOrder (bExplicitCanvasChildZOrder), a canvas only moves
	// to a new layer at the first child of each ZOrder, and skips that when the child is culled. Shared ZOrders put every
	// badge and node on the lines' layer (under the halo and connectors) once column 1 scrolls out of view.
	int32 BadgeZOrder = 1000, NodeZOrder = 2000;
	for (int32 Column = 0; Column < Columns.Num(); ++Column)
	{
		const float X = (Column + .5f) * LessonColumnSpacing;
		const auto& Lessons = Columns[Column];
		const float TopY = 64.f + (3 - Lessons.Num()) * 127.f;
		UBorder* Badge = WidgetTree->ConstructWidget<UBorder>();
		FSlateBrush BadgeArt;
		BadgeArt.SetResourceObject(GroupBadgeTexture);
		BadgeArt.DrawAs = ESlateBrushDrawType::Image;
		BadgeArt.SetUVRegion(FBox2f(FVector2f(.03833f, .14205f), FVector2f(.96223f, .85344f)));
		Badge->SetBrush(BadgeArt);
		Badge->SetPadding(FMargin(0));
		Badge->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* BadgeSlot = NodeCanvas->AddChildToCanvas(Badge);
		BadgeSlot->SetPosition(FVector2D(X - 87, TopY - 66));
		BadgeSlot->SetSize(FVector2D(174, 68));
		BadgeSlot->SetZOrder(++BadgeZOrder);
		UTextBlock* Number = WidgetTree->ConstructWidget<UTextBlock>();
		Number->SetText(FText::AsNumber(Column + 1));
		Number->SetFont(FSlateFontInfo(LessonSerifFont, 36));
		Number->SetColorAndOpacity(FSlateColor(Ivory));
		Number->SetJustification(ETextJustify::Center);
		Badge->AddChild(Number);
		Badge->SetVerticalAlignment(VAlign_Center);
		UBorder* CountPanel = WidgetTree->ConstructWidget<UBorder>();
		FSlateBrush PlaqueArt;
		PlaqueArt.SetResourceObject(StatusPlaqueTexture);
		PlaqueArt.DrawAs = ESlateBrushDrawType::Image;
		PlaqueArt.SetUVRegion(FBox2f(FVector2f(.01957f, .24590f), FVector2f(.98089f, .73634f)));
		CountPanel->SetBrush(PlaqueArt);
		CountPanel->SetPadding(FMargin(0, 2));
		CountPanel->SetVerticalAlignment(VAlign_Center);
		CountPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* CountSlot = NodeCanvas->AddChildToCanvas(CountPanel);
		CountSlot->SetPosition(FVector2D(X - 175, TopY + (Lessons.Num() - 1) * 254.f + 260.f));
		CountSlot->SetSize(FVector2D(350, 54));
		CountSlot->SetZOrder(++BadgeZOrder);
		UTextBlock* CountText = WidgetTree->ConstructWidget<UTextBlock>();
		CountText->SetFont(FSlateFontInfo(LessonSerifFont, 26));
		CountText->SetColorAndOpacity(FSlateColor(Ink));
		CountText->SetJustification(ETextJustify::Center);
		CountPanel->AddChild(CountText);
		ColumnStatusLabels.Add(CountText);
		ColumnStatusPanels.Add(CountPanel);
		for (int32 Row = 0; Row < Lessons.Num(); ++Row)
		{
			const ECampaignLesson Lesson = Lessons[Row];
			const float Y = TopY + Row * 254.f;
			UButton* Button = WidgetTree->ConstructWidget<UButton>();
			FButtonStyle Style;
			Style.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
			Style.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
			Style.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
			Style.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
			Style.NormalPadding = Style.PressedPadding = FMargin(0.f);
			Button->SetStyle(Style);
			Button->SetCursor(EMouseCursor::Hand);
			UCanvasPanel* Face = WidgetTree->ConstructWidget<UCanvasPanel>();
			UButtonSlot* FaceSlot = Cast<UButtonSlot>(Button->AddChild(Face));
			FaceSlot->SetPadding(FMargin(0.f));
			FaceSlot->SetHorizontalAlignment(HAlign_Fill);
			FaceSlot->SetVerticalAlignment(VAlign_Fill);
			ULessonMedallion* Frame = WidgetTree->ConstructWidget<ULessonMedallion>();
			Frame->Texture = MedallionTexture;
            Frame->LockTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/General/Textures/T_LessonsLock.T_LessonsLock"));
			Frame->Lesson = Lesson;
			Frame->LabelFont = FSlateFontInfo(LessonSerifFont, 20);
			Frame->SetVisibility(ESlateVisibility::HitTestInvisible);
			FillCanvas(Face->AddChildToCanvas(Frame));
			Medallions.Add(Frame);
			ULessonClueDiagram* Diagram = WidgetTree->ConstructWidget<ULessonClueDiagram>();
			Diagram->Lesson = Lesson;
			Diagram->SetVisibility(ESlateVisibility::HitTestInvisible);
			UCanvasPanelSlot* DiagramSlot = Face->AddChildToCanvas(Diagram);
			DiagramSlot->SetPosition(FVector2D(56.f, 27.f));
			DiagramSlot->SetSize(FVector2D(180.f, 113.f));
            ULessonCompletionBadge* CompleteBadge=WidgetTree->ConstructWidget<ULessonCompletionBadge>();
            CompleteBadge->SetVisibility(ESlateVisibility::Collapsed);
            UCanvasPanelSlot* CompletionSlot=Face->AddChildToCanvas(CompleteBadge);
            CompletionSlot->SetPosition(FVector2D(236.f,8.f));
            CompletionSlot->SetSize(FVector2D(52.f,52.f));
            CompletionSlot->SetZOrder(2);
            CompletionBadges.Add(CompleteBadge);
			UCanvasPanelSlot* NodeSlot = NodeCanvas->AddChildToCanvas(Button);
			NodeSlot->SetPosition(FVector2D(X - 146, Y));
			NodeSlot->SetSize(FVector2D(292, 214));
			NodeSlot->SetZOrder(++NodeZOrder);
			UCampaignTreeNodeClick* Click = NewObject<UCampaignTreeNodeClick>(this);
			Click->Lesson = Lesson;
			Click->Tree = this;
			Click->Button = Button;
			Button->OnClicked.AddDynamic(Click, &UCampaignTreeNodeClick::HandleClicked);
			Button->OnHovered.AddDynamic(Click, &UCampaignTreeNodeClick::HandleHovered);
			Button->OnUnhovered.AddDynamic(Click, &UCampaignTreeNodeClick::HandleUnhovered);
			NodeButtons.Add(Button);
			NodeClicks.Add(Click);
			NodeLessons.Add(Lesson);
		}
	}
	RefreshNodeStates();
}

int32 UCampaignTreeWidget::GetCurrentColumn() const
{
	const auto& Columns = UCampaignTree::GetColumns();
	for (int32 C = 0; C < Columns.Num(); ++C)
		for (ECampaignLesson L : Columns[C])
			if (!IsLessonCompleted(L))
				return C;
	return FMath::Max(0, Columns.Num() - 1);
}

void UCampaignTreeWidget::RefreshNodeStates()
{
	for (int32 I = 0; I < NodeButtons.Num(); ++I)
	{
		const auto L = NodeLessons[I];
		const bool bCompleted = IsLessonShownCompleted(L);
		const bool bUnlocked = UCampaignTree::GetLessonColumn(L) <= GetShownColumn();
		NodeButtons[I]->SetIsEnabled(bUnlocked);
		NodeButtons[I]->SetBackgroundColor(FLinearColor::White);
		NodeButtons[I]->SetToolTipText(
			FText::FromString(bCompleted  ? TEXT("Completed - play again")
							  : bUnlocked ? TEXT("Choose this lesson")
										  : TEXT("Complete every lesson in earlier groups to unlock")));
		if (Medallions.IsValidIndex(I))
		{
			Medallions[I]->bCompleted = bCompleted;
			Medallions[I]->bLocked = !bUnlocked;
			Medallions[I]->InvalidateLayoutAndVolatility();
		}
        if (CompletionBadges.IsValidIndex(I))
        {
            CompletionBadges[I]->bCompleted=bCompleted;
            CompletionBadges[I]->SetVisibility(bCompleted ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
        }
	}
	const auto& Columns = UCampaignTree::GetColumns();
	for (int32 C = 0; C < Columns.Num() && C < ColumnStatusLabels.Num(); ++C)
	{
		int32 Count = 0;
		for (auto L : Columns[C])
			Count += IsLessonShownCompleted(L) ? 1 : 0;
		const bool bCurrent = C == GetShownColumn();
		ColumnStatusPanels[C]->SetVisibility(bCurrent ? ESlateVisibility::HitTestInvisible
													  : ESlateVisibility::Collapsed);
		ColumnStatusPanels[C]->SetRenderOpacity(1.f);
		const FText CountText = FText::Format(NSLOCTEXT("Lessons", "GroupCompletion", "{0} / {1} complete"),
											  FText::AsNumber(Count), FText::AsNumber(Columns[C].Num()));
		ColumnStatusLabels[C]->SetText(CountText);
	}
	bCenterCurrentColumn = true;
	RefreshOverallStatus();
}

void UCampaignTreeWidget::RefreshOverallStatus()
{
	// Progress, not position: the groups whose every lesson shows completed (a celebrated one once its check appears)
	const TArray<TArray<ECampaignLesson>>& Columns = UCampaignTree::GetColumns();
	int32 Completed = 0;
	for (const TArray<ECampaignLesson>& Column : Columns)
	{
		if (!Column.ContainsByPredicate([this](ECampaignLesson Lesson) { return !IsLessonShownCompleted(Lesson); }))
			Completed++;
	}
	if (Completed == LastCompletedColumns)
		return;
	LastCompletedColumns = Completed;
	if (OverallStatus)
		OverallStatus->SetText(FText::Format(NSLOCTEXT("Lessons", "OverallCompletion", "{0} / {1} complete"),
											 FText::AsNumber(Completed), FText::AsNumber(Columns.Num())));
}

FReply UCampaignTreeWidget::NativeOnPreviewMouseButtonDown(const FGeometry& G, const FPointerEvent& Event)
{
	bDraggingTree = false;
	bPanCandidate = Event.GetEffectingButton() == EKeys::LeftMouseButton && TreeScroll &&
					TreeScroll->GetCachedGeometry().IsUnderLocation(Event.GetScreenSpacePosition()) &&
					(!LessonPopup || !LessonPopup->IsVisible()) && !IsCelebrating();
	if (bPanCandidate)
	{
		PanUserIndex = Event.GetUserIndex();
		PanPointerIndex = Event.GetPointerIndex();
		PanStart = Event.GetScreenSpacePosition();
		PanStartOffset = TreeScroll->GetScrollOffset();
	}
	return Super::NativeOnPreviewMouseButtonDown(G, Event);
}

void UCampaignTreeWidget::NativeTick(const FGeometry& G, float Delta)
{
	Super::NativeTick(G, Delta);
	if (!TreeScroll || !ScrollContent || !NodeCanvas)
		return;
	const FVector2D Viewport = TreeScroll->GetCachedGeometry().GetLocalSize();
	const float Width = Viewport.X;
	if (Width <= 0.f || Viewport.Y <= 32.f)
		return;
	const float Scale = FMath::Min(1.f, float((Viewport.Y - 32.f) / LessonCanvasHeight));
	const float ContentWidth = UCampaignTree::GetNumColumns() * LessonColumnSpacing;
	const float ContentHeight = LessonCanvasHeight * Scale;
	if (!FMath::IsNearlyEqual(Width, LastViewportWidth, 1.f) ||
		!FMath::IsNearlyEqual(ContentHeight, ScrollContent->GetHeightOverride(), .1f))
	{
		LastViewportWidth = Width;
		ScrollContent->SetWidthOverride(ContentWidth);
        ScrollContent->SetHeightOverride(ContentHeight);
        // Preserve the concept's horizontal rhythm while fitting each stack vertically.
        // ScaleBox also scales X, so expand the design spacing by the reciprocal scale.
        if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(NodeCanvas->Slot))
        {
            const float OldSpacing = CanvasSlot->GetSize().X / UCampaignTree::GetNumColumns();
            const float DesignSpacing = LessonColumnSpacing / Scale;
            for (UWidget* Child : NodeCanvas->GetAllChildren())
            {
                if (Child == Lines)
                    continue;
                if (UCanvasPanelSlot* ChildSlot = Cast<UCanvasPanelSlot>(Child->Slot))
                {
                    FVector2D Position = ChildSlot->GetPosition();
                    const float HalfWidth = ChildSlot->GetSize().X * .5f;
                    const int32 Column = FMath::RoundToInt((Position.X + HalfWidth) / OldSpacing - .5f);
                    Position.X = (Column + .5f) * DesignSpacing - HalfWidth;
                    ChildSlot->SetPosition(Position);
                }
            }
            CanvasSlot->SetSize(FVector2D(ContentWidth / Scale, LessonCanvasHeight));
            if (USizeBox* DesignSize = Cast<USizeBox>(NodeCanvas->GetParent()->GetParent()))
                DesignSize->SetWidthOverride(ContentWidth / Scale);
        }
		LastCompletedColumns = INDEX_NONE;
		bCenterCurrentColumn = true;
	}
	if (bCenterCurrentColumn)
	{
		const float End = FMath::Max(0.f, ContentWidth - Width);
		TreeScroll->SetScrollOffset(
			FMath::Clamp((GetShownColumn() + .5f) * LessonColumnSpacing - Width * .5f, 0.f, End));
		bCenterCurrentColumn = false;
	}
	UpdateCelebration(Delta, Width, FMath::Max(0.f, ContentWidth - Width));
	if (bPanCandidate && FSlateApplication::IsInitialized())
	{
		auto& Slate = FSlateApplication::Get();
		const TSharedPtr<FSlateUser> PanUser = Slate.GetUser(PanUserIndex);
		if (PanUser && Slate.GetPressedMouseButtons().Contains(EKeys::LeftMouseButton))
		{
			// Where the pointer that pressed is now: on a phone the cursor stays put while a finger moves
			const FVector2D DragCursor = PanUser->GetPointerPosition(PanPointerIndex);
			if (bDraggingTree || FVector2D::Distance(DragCursor, PanStart) > 10.f)
			{
				bDraggingTree = true;
				TreeScroll->EndInertialScrolling();
				const float D = G.AbsoluteToLocal(DragCursor).X - G.AbsoluteToLocal(PanStart).X;
				TreeScroll->SetScrollOffset(FMath::Clamp(PanStartOffset - D, 0.f, TreeScroll->GetScrollOffsetOfEnd()));
			}
		}
		else
			bPanCandidate = false;
	}
	RefreshOverallStatus();
}

void UCampaignTreeWidget::SetCompletedLessons(const TArray<ECampaignLesson>& Lessons)
{
	CompletedLessons = TSet<ECampaignLesson>(Lessons);
	RefreshNodeStates();
}

void UCampaignTreeWidget::SetLessonCompleted(ECampaignLesson Lesson, bool bCompleted)
{
	if (bCompleted)
	{
		CompletedLessons.Add(Lesson);
	}
	else
	{
		CompletedLessons.Remove(Lesson);
	}
	RefreshNodeStates();
}

bool UCampaignTreeWidget::IsLessonCompleted(ECampaignLesson Lesson) const
{
	return CompletedLessons.Contains(Lesson);
}

bool UCampaignTreeWidget::IsLessonUnlocked(ECampaignLesson Lesson) const
{
	const int32 LessonColumn = UCampaignTree::GetLessonColumn(Lesson);
	if (LessonColumn == INDEX_NONE)
	{
		return false;
	}

	for (int32 Column = 0; Column < LessonColumn; Column++)
	{
		for (ECampaignLesson Earlier : UCampaignTree::GetColumnLessons(Column))
		{
			if (!CompletedLessons.Contains(Earlier))
			{
				return false;
			}
		}
	}
	return true;
}

bool UCampaignTreeWidget::IsLessonShownCompleted(ECampaignLesson Lesson) const
{
	return IsLessonCompleted(Lesson) && !(IsCelebrating() && Lesson == CelebrationLesson && !bCelebrationCheckShown);
}

int32 UCampaignTreeWidget::GetShownColumn() const
{
	return IsCelebrating() ? CelebrationFromColumn : GetCurrentColumn();
}

void UCampaignTreeWidget::StartCelebration(ECampaignLesson Lesson)
{
	EndCelebration();
	const int32 Column = UCampaignTree::GetLessonColumn(Lesson);
	if (Column == INDEX_NONE || !NodeLessons.Contains(Lesson))
	{
		return;
	}
	CelebrationLesson = Lesson;
	CelebrationFromColumn = Column;
	// The group the glow moves on to: the next one, if this lesson completed its group
	CelebrationToColumn = FMath::Max(Column, GetCurrentColumn());
	CelebrationTime = 0.f;
	bCelebrationCheckShown = false;
	bCelebrationMoving = false;
	GlowMove = 0.f;
	// Shown as before the lesson was completed, centered on its group
	RefreshNodeStates();
}

void UCampaignTreeWidget::UpdateCelebration(float DeltaTime, float ViewportWidth, float ScrollEnd)
{
	if (!IsCelebrating())
	{
		return;
	}
	CelebrationTime += DeltaTime;
	const int32 Index = NodeLessons.IndexOfByKey(CelebrationLesson);
	ULessonCompletionBadge* Badge = CompletionBadges.IsValidIndex(Index) ? CompletionBadges[Index].Get() : nullptr;

	// The check pops in: from small it grows past its size, then springs back and settles, a shimmer sweeping across
	const float CheckTime = (CelebrationTime - CheckDelay) / FMath::Max(CheckDuration, .01f);
	if (CheckTime >= 0.f && !bCelebrationCheckShown)
	{
		bCelebrationCheckShown = true;
		RefreshNodeStates();
		if (CheckSound)
		{
			UGameplayStatics::PlaySound2D(this, CheckSound, 1.f, 1.f, 0.f, nullptr, nullptr, true);
		}
	}
	if (Badge && CheckTime >= 0.f)
	{
		const float T = FMath::Min(CheckTime, 1.f);
		constexpr float Grow = .45f;
		float Scale;
		if (T < Grow)
		{
			Scale = FMath::Lerp(CheckStartScale, CheckPeakScale, 1.f - FMath::Pow(1.f - T / Grow, 3.f));
		}
		else
		{
			const float Settle = (T - Grow) / (1.f - Grow);
			Scale = 1.f + (CheckPeakScale - 1.f) * FMath::Cos(1.5f * PI * Settle) * FMath::Exp(-4.f * Settle);
		}
		Badge->SetRenderScale(FVector2D(T < 1.f ? Scale : 1.f));
		Badge->SetRenderOpacity(FMath::Clamp(T / .12f, 0.f, 1.f));
		Badge->Shimmer = T < 1.f && T >= .3f ? (T - .3f) / .7f : -1.f;
		Badge->Glow = T < 1.f ? FMath::Sin(PI * T) : 0.f;
		if (const TSharedPtr<SWidget> Slate = Badge->GetCachedWidget())
		{
			Slate->Invalidate(EInvalidateWidgetReason::Paint);
		}
	}
	if (CheckTime < 1.f)
	{
		return;
	}
	if (CelebrationToColumn == CelebrationFromColumn)
	{
		EndCelebration();
		return;
	}

	// The group's complete: its glow moves on to the next group, and the tree scrolls to center it
	const float MoveTime = (CelebrationTime - CheckDelay - CheckDuration - GlowMoveDelay) / FMath::Max(GlowMoveDuration, .01f);
	if (MoveTime < 0.f || !TreeScroll)
	{
		return;
	}
	if (!bCelebrationMoving)
	{
		bCelebrationMoving = true;
		CelebrationScrollFrom = TreeScroll->GetScrollOffset();
		CelebrationScrollTo =
			FMath::Clamp((CelebrationToColumn + .5f) * LessonColumnSpacing - ViewportWidth * .5f, 0.f, ScrollEnd);
	}
	const float T = FMath::Min(MoveTime, 1.f);
	GlowMove = FMath::InterpEaseInOut(0.f, 1.f, T, 2.f);
	TreeScroll->SetScrollOffset(FMath::Lerp(CelebrationScrollFrom, CelebrationScrollTo, GlowMove));
	// The plaque fades out under the old group and in under the new one
	if (ColumnStatusPanels.IsValidIndex(CelebrationFromColumn) && ColumnStatusPanels.IsValidIndex(CelebrationToColumn))
	{
		ColumnStatusPanels[CelebrationFromColumn]->SetRenderOpacity(FMath::Clamp(1.f - 2.f * T, 0.f, 1.f));
		ColumnStatusPanels[CelebrationToColumn]->SetVisibility(ESlateVisibility::HitTestInvisible);
		ColumnStatusPanels[CelebrationToColumn]->SetRenderOpacity(FMath::Clamp(2.f * T - 1.f, 0.f, 1.f));
	}
	if (Lines)
	{
		if (const TSharedPtr<SWidget> Slate = Lines->GetCachedWidget())
		{
			Slate->Invalidate(EInvalidateWidgetReason::Paint);
		}
	}
	if (MoveTime >= 1.f)
	{
		// The glow's arrived: the new group unlocks
		EndCelebration();
	}
}

void UCampaignTreeWidget::EndCelebration()
{
	if (!IsCelebrating())
	{
		return;
	}
	const int32 Index = NodeLessons.IndexOfByKey(CelebrationLesson);
	if (ULessonCompletionBadge* Badge = CompletionBadges.IsValidIndex(Index) ? CompletionBadges[Index].Get() : nullptr)
	{
		Badge->SetRenderScale(FVector2D(1.f));
		Badge->SetRenderOpacity(1.f);
		Badge->Shimmer = -1.f;
		Badge->Glow = 0.f;
	}
	CelebrationTime = -1.f;
	GlowMove = 0.f;
	RefreshNodeStates();
}

void UCampaignTreeWidget::NotifyNodeClicked(ECampaignLesson Lesson)
{
	if (!bDraggingTree && !IsCelebrating() && IsLessonUnlocked(Lesson))
	{
		if (LessonPopup)
		{
			LessonPopup->ShowLesson(Lesson);
		}
		OnLessonSelected.Broadcast(Lesson);
	}
}

UButton* UCampaignTreeWidget::FindNode(ECampaignLesson Lesson) const
{
	const int32 Index = NodeLessons.IndexOfByKey(Lesson);
	return Index != INDEX_NONE ? NodeButtons[Index].Get() : nullptr;
}

int32 UCampaignTreeWidget::PaintConnectors(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements,
										   int32 LayerId) const
{
	// Connectors run edge to edge between nodes, so they never cross one
	struct FNodeEdges
	{
		FVector2f Left;
		FVector2f Right;
	};

	auto GetEdges = [&](ECampaignLesson Lesson, FNodeEdges& Out) -> bool
	{
		const UButton* Button = FindNode(Lesson);
		if (!Button)
		{
			return false;
		}
		const UCanvasPanelSlot* NodeSlot = Cast<UCanvasPanelSlot>(Button->Slot);
		if (!NodeSlot)
			return false;
		// Slots and the lines share NodeCanvas coordinates; cached absolute geometry lags while panning.
		const FVector2D Position = NodeSlot->GetPosition(), Size = NodeSlot->GetSize();
		Out.Left = FVector2f(Position.X, Position.Y + Size.Y * .5f);
		Out.Right = FVector2f(Position.X + Size.X, Position.Y + Size.Y * .5f);
		return true;
	};

	const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();
    const TArray<TArray<ECampaignLesson>>& Columns = UCampaignTree::GetColumns();
    // The halo's center and height around a group's stack and completion plaque
    auto GroupHalo=[&](int32 Column,FVector2f& OutCenter,float& OutHeight)
    {
        Column=FMath::Clamp(Column,0,Columns.Num()-1);
        FNodeEdges Edges;
        if (Columns.IsEmpty() || Columns[Column].IsEmpty() || !GetEdges(Columns[Column][0],Edges))
            return false;
        const int32 LessonCount=Columns[Column].Num();
        const float FirstNodeY=64.f+(3-LessonCount)*127.f;
        const float GroupTop=FirstNodeY-66.f;
        const float GroupBottom=FirstNodeY+(LessonCount-1)*254.f+314.f;
        OutCenter=FVector2f((Edges.Left.X+Edges.Right.X)*.5f,(GroupTop+GroupBottom)*.5f);
        OutHeight=GroupBottom-GroupTop;
        return true;
    };
    FVector2f Center, ToCenter;
    float GroupHeight, ToHeight;
    const bool bGlowMoving=IsCelebrating() && GlowMove>0.f;
    if (GroupHalo(bGlowMoving ? CelebrationFromColumn : GetShownColumn(),Center,GroupHeight))
    {
        // While the glow moves on to the next group, it slides across and fits itself to that group
        if (bGlowMoving && GroupHalo(CelebrationToColumn,ToCenter,ToHeight))
        {
            Center=FMath::Lerp(Center,ToCenter,GlowMove);
            GroupHeight=FMath::Lerp(GroupHeight,ToHeight,GlowMove);
        }
        // A compact sage halo follows this group's actual lesson stack and completion plaque.
        for(int32 Ring=0;Ring<12;++Ring)
        {
            const float T=float(Ring)/11.f;
            const FVector2f Size(FMath::Lerp(430.f,340.f,T),
                FMath::Lerp(GroupHeight+100.f,GroupHeight+10.f,T));
            const float Alpha=.016f+.12f*T*T;
            const FLinearColor Glow(.40f,.65f,.20f,Alpha);
            FSlateRoundedBoxBrush Halo(FLinearColor::White,Size.X*.5f);
            FSlateDrawElement::MakeBox(OutDrawElements,LayerId,AllottedGeometry.ToPaintGeometry(Size,
                FSlateLayoutTransform(Center-Size*.5f)),&Halo,ESlateDrawEffect::None,Glow);
        }
    }
	auto DrawBranch = [&](const FVector2f& From, const FVector2f& To)
	{
		TArray<FVector2f> Points;
		const float BendX = (From.X + To.X) * .5f;
		const FVector2f C1(BendX, From.Y), C2(BendX, To.Y);
		for (int32 I = 0; I <= 32; ++I)
		{
			const float T = I / 32.f, U = 1.f - T;
			Points.Add(U * U * U * From + 3.f * U * U * T * C1 + 3.f * U * T * T * C2 + T * T * T * To);
		}
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, PaintGeometry, Points, ESlateDrawEffect::None,
									 FLinearColor(.48f, .39f, .19f), true, 8.f);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, PaintGeometry, Points, ESlateDrawEffect::None,
									 FLinearColor(.98f, .90f, .70f), true, 5.f);
	};
	auto Rosette = [&](FVector2f Center)
	{
		FSlateRoundedBoxBrush Rim(FLinearColor(.12f, .23f, .045f), 21.f, FLinearColor(.93f, .80f, .48f), 3.f);
		FSlateDrawElement::MakeBox(
			OutDrawElements, LayerId + 2,
			AllottedGeometry.ToPaintGeometry(FVector2f(42, 42), FSlateLayoutTransform(Center - FVector2f(21, 21))),
			&Rim, ESlateDrawEffect::None, FLinearColor(.12f,.23f,.045f));
		FSlateRoundedBoxBrush Petal(FLinearColor(.98f, .96f, .82f), 5.f);
		for (int32 I = 0; I < 5; ++I)
		{
			const float Angle = I * 2.f * PI / 5.f;
			const FVector2f P = Center + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * 8.f;
			FSlateDrawElement::MakeBox(
				OutDrawElements, LayerId + 3,
				AllottedGeometry.ToPaintGeometry(FVector2f(10, 10), FSlateLayoutTransform(P - FVector2f(5, 5))), &Petal,
				ESlateDrawEffect::None, FLinearColor::White);
		}
		FSlateRoundedBoxBrush Middle(FLinearColor(.87f, .65f, .15f), 3.f);
		FSlateDrawElement::MakeBox(
			OutDrawElements, LayerId + 4,
			AllottedGeometry.ToPaintGeometry(FVector2f(6, 6), FSlateLayoutTransform(Center - FVector2f(3, 3))), &Middle,
			ESlateDrawEffect::None, FLinearColor(.87f,.65f,.15f));
	};

	for (int32 Column = 0; Column + 1 < Columns.Num(); Column++)
	{
		TArray<FNodeEdges> From, To;
		for (ECampaignLesson Lesson : Columns[Column])
		{
			FNodeEdges Edges;
			if (GetEdges(Lesson, Edges))
				From.Add(Edges);
		}
		for (ECampaignLesson Lesson : Columns[Column + 1])
		{
			FNodeEdges Edges;
			if (GetEdges(Lesson, Edges))
				To.Add(Edges);
		}
		if (From.IsEmpty() || To.IsEmpty())
		{
			continue;
		}

		const FVector2f Junction((From[0].Right.X + To[0].Left.X) * .5f, 425.f);
        auto GroupBrace = [&](const TArray<FNodeEdges>& Nodes, bool Right)
        {
            const FVector2f First=Right ? Nodes[0].Right : Nodes[0].Left;
            const FVector2f Last=Right ? Nodes.Last().Right : Nodes.Last().Left;
            if (Nodes.Num()==1) { DrawBranch(First,Junction); return; }
            const float Direction=Right ? 1.f : -1.f;
            const float EdgeX=First.X-Direction*12.f;
            const float RailX=First.X+Direction*10.f;
            const float Top=First.Y-80.f, Bottom=Last.Y+80.f;
            const float Mid=Junction.Y;
            TArray<FVector2f> P;
            auto Curve=[&](FVector2f A,FVector2f B,FVector2f C,FVector2f D)
            {
                for(int32 I=P.IsEmpty()?0:1;I<=16;++I)
                {
                    const float T=I/16.f,U=1.f-T;
                    P.Add(U*U*U*A+3*U*U*T*B+3*U*T*T*C+T*T*T*D);
                }
            };
            // Rounded bracket tips, straight outer rails, and a smooth gathering curve at the center.
            Curve(FVector2f(EdgeX,Top),FVector2f(RailX,Top),FVector2f(RailX,Top+8),FVector2f(RailX,Top+30));
            P.Add(FVector2f(RailX,Mid-65));
            Curve(P.Last(),FVector2f(RailX,Mid-20),FVector2f(Junction.X-Direction*22,Mid),Junction);
            Curve(Junction,FVector2f(Junction.X-Direction*22,Mid),FVector2f(RailX,Mid+20),FVector2f(RailX,Mid+65));
            P.Add(FVector2f(RailX,Bottom-30));
            Curve(P.Last(),FVector2f(RailX,Bottom-8),FVector2f(RailX,Bottom),FVector2f(EdgeX,Bottom));
            FSlateDrawElement::MakeLines(OutDrawElements,LayerId,PaintGeometry,P,ESlateDrawEffect::None,
                FLinearColor(.48f,.39f,.19f),true,8.f);
            FSlateDrawElement::MakeLines(OutDrawElements,LayerId+1,PaintGeometry,P,ESlateDrawEffect::None,
                FLinearColor(.98f,.90f,.70f),true,5.f);
        };
        GroupBrace(From,true);
        GroupBrace(To,false);
		Rosette(Junction);
	}

	return LayerId + 4;
}
