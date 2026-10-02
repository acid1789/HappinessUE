#include "UI/LessonProgressTicks.h"
#include "HappinessClassic/CampaignProgress.h"

#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"

#define LOCTEXT_NAMESPACE "LessonProgressTicks"

class SLessonProgressTicks : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SLessonProgressTicks) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
	}

	FLinearColor Color = FLinearColor::Black;
	float Thickness = 2.f;

	// Explicit tick positions (fractions of the width), when set
	bool bCustom = false;
	TArray<float> Fractions;

	TArray<float> GetFractions() const
	{
		if (bCustom)
		{
			return Fractions;
		}
		// One tick per stage after the first, at the points that unlock it; the final's is the bar's end
		TArray<float> Track;
		for (int32 Stage = 1; Stage < UCampaignSubsystem::GetFinalStage(); Stage++)
		{
			Track.Add(float(UCampaignSubsystem::GetStagePointsRequired(Stage)) / UCampaignSubsystem::GetMaxPoints());
		}
		return Track;
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const FVector2D Size = AllottedGeometry.GetLocalSize();
		const FLinearColor TintedColor = Color * InWidgetStyle.GetColorAndOpacityTint();

		for (const float Fraction : GetFractions())
		{
			const float X = Size.X * Fraction;
			const TArray<FVector2D> Points = { FVector2D(X, 0.f), FVector2D(X, Size.Y) };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Points,
				ESlateDrawEffect::None, TintedColor, true, Thickness);
		}
		return LayerId;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(100.f, 20.f);
	}
};

TSharedRef<SWidget> ULessonProgressTicks::RebuildWidget()
{
	Ticks = SNew(SLessonProgressTicks);
	return Ticks.ToSharedRef();
}

void ULessonProgressTicks::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (Ticks)
	{
		Ticks->Color = TickColor;
		Ticks->Thickness = TickThickness;
		Ticks->bCustom = bCustomTicks;
		Ticks->Fractions = CustomFractions;
	}
}

void ULessonProgressTicks::SetTickFractions(const TArray<float>& Fractions)
{
	bCustomTicks = true;
	CustomFractions = Fractions;
	SynchronizeProperties();
	if (Ticks)
	{
		Ticks->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void ULessonProgressTicks::ClearTickFractions()
{
	bCustomTicks = false;
	CustomFractions.Reset();
	SynchronizeProperties();
	if (Ticks)
	{
		Ticks->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void ULessonProgressTicks::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Ticks.Reset();
}

#if WITH_EDITOR
const FText ULessonProgressTicks::GetPaletteCategory()
{
	return LOCTEXT("Category", "Happiness");
}
#endif

#undef LOCTEXT_NAMESPACE
