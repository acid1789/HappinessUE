#include "UI/FireworksWidget.h"

#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"

namespace
{
	/** Seconds a rocket takes to rise to its burst */
	constexpr float RocketRiseTime = 0.45f;
	/** Speeds, sizes and pulls at Scale 1 */
	constexpr float SparkSpeedMin = 260.f;
	constexpr float SparkSpeedMax = 480.f;
	constexpr float SparkDrag = 2.4f;
	constexpr float SparkGravity = 210.f;
	constexpr float SparkHead = 11.f;
	constexpr float SparkStreak = 4.f;
	constexpr int32 SparksPerBurst = 48;
	constexpr float FlashSize = 150.f;
	constexpr float FlashTime = 0.3f;

	/** Garden colors, bright enough to read over the menu's paper */
	const FLinearColor BurstColors[] =
	{
		FLinearColor(1.f, 0.68f, 0.05f),
		FLinearColor(0.95f, 0.18f, 0.35f),
		FLinearColor(0.15f, 0.55f, 1.f),
		FLinearColor(0.25f, 0.78f, 0.15f),
		FLinearColor(0.62f, 0.28f, 0.95f),
		FLinearColor(1.f, 0.42f, 0.05f),
	};
	const FLinearColor Glitter(1.f, 0.95f, 0.75f);

	class SFireworks : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFireworks) {} SLATE_END_ARGS()

		void Construct(const FArguments&, UFireworksWidget* InOwner)
		{
			Owner = InOwner;
			Dot.DrawAs = ESlateBrushDrawType::RoundedBox;
			Dot.TintColor = FSlateColor(FLinearColor::White);
			Dot.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
			Dot.OutlineSettings.Width = 0.f;
		}

		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }

		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&, FSlateWindowElementList& Elements,
			int32 Layer, const FWidgetStyle& Style, bool) const override
		{
			const UFireworksWidget* Fireworks = Owner.Get();
			if (!Fireworks)
			{
				return Layer;
			}
			const float Scale = Fireworks->GetScale();
			const float Opacity = Style.GetColorAndOpacityTint().A;
			auto DrawDot = [&](const FVector2D& Center, float Size, FLinearColor Color, int32 DotLayer)
			{
				Color.A *= Opacity;
				FSlateDrawElement::MakeBox(Elements, DotLayer, Geometry.ToPaintGeometry(FVector2D(Size),
					FSlateLayoutTransform(Center - FVector2D(Size * 0.5f))), &Dot, ESlateDrawEffect::None, Color);
			};

			for (const TPair<FVector2D, float>& Flash : Fireworks->GetFlashes())
			{
				DrawDot(Flash.Key, FlashSize * Scale * (1.4f - 0.4f * Flash.Value), FLinearColor(1.f, 0.97f, 0.85f, 0.45f * Flash.Value), Layer);
			}
			for (const UFireworksWidget::FSpark& Spark : Fireworks->GetSparks())
			{
				// Sparks fade over the last part of their life, glitter twinkling as it goes
				const float Left = Spark.Life - Spark.Age;
				float Alpha = FMath::Clamp(Left / (Spark.Life * 0.45f), 0.f, 1.f);
				if (!Spark.bRocket && Spark.Size < 0.8f)
				{
					Alpha *= 0.55f + 0.45f * FMath::Sin(Spark.Age * 38.f + Spark.Position.X);
				}
				if (Alpha <= 0.f)
				{
					continue;
				}
				FLinearColor Color = Spark.Color;
				Color.A = Alpha;
				const float Head = SparkHead * Scale * Spark.Size;

				// A streak behind it, longer the faster it goes
				const FVector2D Tail = Spark.Position - Spark.Velocity * (Spark.bRocket ? 0.04f : 0.045f);
				if (FVector2D::DistSquared(Tail, Spark.Position) > 1.f)
				{
					FLinearColor StreakColor = Color;
					StreakColor.A *= 0.8f * Opacity;
					const TArray<FVector2D> Points = { Tail, Spark.Position };
					FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None,
						StreakColor, true, SparkStreak * Scale * Spark.Size);
				}
				// A soft glow, then the bright head
				FLinearColor GlowColor = Color;
				GlowColor.A *= 0.3f;
				DrawDot(Spark.Position, Head * 2.8f, GlowColor, Layer);
				DrawDot(Spark.Position, Head, FMath::Lerp(Color, FLinearColor(1.f, 1.f, 1.f, Color.A), 0.2f), Layer + 1);
			}
			return Layer + 1;
		}

	private:
		TWeakObjectPtr<UFireworksWidget> Owner;
		FSlateBrush Dot;
	};
}

TSharedRef<SWidget> UFireworksWidget::RebuildWidget()
{
	SlateFireworks = SNew(SFireworks, this);
	return SlateFireworks.ToSharedRef();
}

void UFireworksWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	SlateFireworks.Reset();
}

void UFireworksWidget::Play(const TArray<FBurst>& InBursts, float InScale)
{
	Bursts = InBursts;
	Bursts.Sort([](const FBurst& A, const FBurst& B) { return A.Delay < B.Delay; });
	Sparks.Reset();
	Flashes.Reset();
	NextBurst = 0;
	Time = 0.f;
	Scale = FMath::Max(InScale, 0.05f);
	Random.GenerateNewSeed();
}

void UFireworksWidget::AddBursts(const TArray<FBurst>& InBursts, float InScale)
{
	if (!IsPlaying() && Flashes.Num() == 0)
	{
		Play(InBursts, InScale);
		return;
	}
	// The bursts already gone are done with; the colors carry on round from where they were
	const int32 Gone = NextBurst - NextBurst % UE_ARRAY_COUNT(BurstColors);
	Bursts.RemoveAt(0, Gone);
	NextBurst -= Gone;
	for (FBurst Burst : InBursts)
	{
		Burst.Delay += Time;
		Bursts.Add(Burst);
	}
	Bursts.Sort([](const FBurst& A, const FBurst& B) { return A.Delay < B.Delay; });
	Scale = FMath::Max(InScale, 0.05f);
}

void UFireworksWidget::Stop()
{
	Bursts.Reset();
	Sparks.Reset();
	Flashes.Reset();
	NextBurst = 0;
	if (SlateFireworks)
	{
		SlateFireworks->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

float UFireworksWidget::GetFirstBurstTime() const
{
	return Bursts.Num() > 0 ? Bursts[0].Delay : -1.f;
}

void UFireworksWidget::Explode(const FBurst& Burst, const FLinearColor& Color)
{
	Flashes.Emplace(Burst.Position, 1.f);
	for (int32 Index = 0; Index < SparksPerBurst; Index++)
	{
		// Evenly round the circle, a little uneven so it doesn't look stamped
		const float Angle = 2.f * PI * (Index + Random.FRandRange(-0.3f, 0.3f)) / SparksPerBurst;
		const float Speed = Random.FRandRange(SparkSpeedMin, SparkSpeedMax) * Scale;
		FSpark& Spark = Sparks.AddDefaulted_GetRef();
		Spark.Position = Burst.Position;
		Spark.Velocity = Speed * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
		const bool bGlitter = Index % 4 == 3;
		Spark.Color = bGlitter ? Glitter : Color;
		Spark.Size = bGlitter ? 0.7f : Random.FRandRange(0.85f, 1.15f);
		Spark.Life = Random.FRandRange(1.1f, 1.6f);
	}
}

void UFireworksWidget::Advance(float DeltaTime)
{
	if (!IsPlaying() && Flashes.Num() == 0)
	{
		return;
	}
	DeltaTime = FMath::Min(DeltaTime, 0.05f) * PlayRate;
	Time += DeltaTime;

	// Rockets rise, easing out, and burst on arrival
	Sparks.RemoveAll([](const FSpark& Spark) { return Spark.bRocket; });
	for (int32 Index = NextBurst; Index < Bursts.Num(); Index++)
	{
		const FBurst& Burst = Bursts[Index];
		if (Time >= Burst.Delay)
		{
			Explode(Burst, BurstColors[Index % UE_ARRAY_COUNT(BurstColors)]);
			NextBurst = Index + 1;
			continue;
		}
		const float RiseStart = Burst.Delay - RocketRiseTime;
		if (Burst.RocketRise <= 0.f || Time < RiseStart)
		{
			continue;
		}
		const float T = (Time - RiseStart) / RocketRiseTime;
		const float Height = Burst.RocketRise * FMath::Square(1.f - T);
		FSpark& Rocket = Sparks.AddDefaulted_GetRef();
		Rocket.bRocket = true;
		Rocket.Burst = Index;
		Rocket.Position = Burst.Position + FVector2D(0.f, Height);
		Rocket.Velocity = FVector2D(0.f, -2.f * Burst.RocketRise * (1.f - T) / RocketRiseTime);
		Rocket.Color = Glitter;
		Rocket.Size = 0.8f;
		Rocket.Life = 10.f;

		// It sheds a few sparks as it climbs
		FSpark& Trail = Sparks.AddDefaulted_GetRef();
		Trail.Position = Rocket.Position;
		Trail.Velocity = FVector2D(Random.FRandRange(-30.f, 30.f), Random.FRandRange(20.f, 60.f)) * Scale;
		Trail.Color = FLinearColor(1.f, 0.8f, 0.4f);
		Trail.Size = 0.45f;
		Trail.Life = Random.FRandRange(0.25f, 0.4f);
	}

	// Sparks slow, fall and burn out
	const float Drag = FMath::Exp(-SparkDrag * DeltaTime);
	for (FSpark& Spark : Sparks)
	{
		if (Spark.bRocket)
		{
			continue;
		}
		Spark.Age += DeltaTime;
		Spark.Velocity *= Drag;
		Spark.Velocity.Y += SparkGravity * Scale * DeltaTime;
		Spark.Position += Spark.Velocity * DeltaTime;
	}
	Sparks.RemoveAll([](const FSpark& Spark) { return !Spark.bRocket && Spark.Age >= Spark.Life; });
	for (TPair<FVector2D, float>& Flash : Flashes)
	{
		Flash.Value -= DeltaTime / FlashTime;
	}
	Flashes.RemoveAll([](const TPair<FVector2D, float>& Flash) { return Flash.Value <= 0.f; });

	if (SlateFireworks)
	{
		SlateFireworks->Invalidate(EInvalidateWidgetReason::Paint);
	}
}
