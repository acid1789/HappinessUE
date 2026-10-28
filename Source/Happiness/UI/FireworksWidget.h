#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "FireworksWidget.generated.h"

/**
 * Code-drawn fireworks: rockets rise and burst into sparks that slow, fall and fade. Place it over the area they may
 * fill (it draws anywhere inside itself) and call Play with where to burst; the owner advances it every frame
 * (Advance), so it costs nothing while idle. The mode screen plays it over the Campaign button when every lesson has
 * been completed (UHappinessModeWidget); the completed Master popup keeps it going while its reward waits
 * (ULessonPopupWidget, with AddBursts).
 */
UCLASS()
class HAPPINESS_API UFireworksWidget : public UWidget
{
	GENERATED_BODY()

public:
	/** One burst: where (in this widget's space), when after Play, and where its rocket starts (its height: below it) */
	struct FBurst
	{
		FVector2D Position = FVector2D::ZeroVector;
		float Delay = 0.f;
		float RocketRise = 0.f;
	};

	/** Start the bursts. Scale sizes the sparks and their spread (1: about a 500-unit-wide button's worth). */
	void Play(const TArray<FBurst>& InBursts, float InScale);

	/**
	 * More bursts after the ones playing (delays from now), keeping what is already in the air: call again when
	 * HasBurstsLeft is false to keep the fireworks going. Starts them, as Play, if nothing is playing.
	 */
	void AddBursts(const TArray<FBurst>& InBursts, float InScale);

	/** A rocket is still to burst */
	bool HasBurstsLeft() const { return NextBurst < Bursts.Num(); }

	/** Move the fireworks on by DeltaTime and redraw */
	void Advance(float DeltaTime);

	/** Something is still to burst or still showing */
	bool IsPlaying() const { return NextBurst < Bursts.Num() || Sparks.Num() > 0; }

	/** Seconds after Play until the first burst, or -1 for none */
	float GetFirstBurstTime() const;

	void Stop();

	/** How fast the fireworks play (1: normal; lower to watch them slowly) */
	UPROPERTY(EditAnywhere, Category = "Fireworks")
	float PlayRate = 1.f;

	struct FSpark
	{
		FVector2D Position;
		FVector2D Velocity;
		FLinearColor Color;
		float Age = 0.f;
		float Life = 1.f;
		float Size = 1.f;
		/** A rocket rising to its burst: drawn as a bright streak, sheds sparks */
		bool bRocket = false;
		int32 Burst = INDEX_NONE;
	};

	const TArray<FSpark>& GetSparks() const { return Sparks; }
	/** Burst flashes still showing: where, and how bright (1 to 0) */
	const TArray<TPair<FVector2D, float>>& GetFlashes() const { return Flashes; }
	float GetScale() const { return Scale; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	void Explode(const FBurst& Burst, const FLinearColor& Color);

	TArray<FBurst> Bursts;
	TArray<FSpark> Sparks;
	TArray<TPair<FVector2D, float>> Flashes;
	int32 NextBurst = 0;
	float Time = 0.f;
	float Scale = 1.f;
	FRandomStream Random;
	TSharedPtr<SWidget> SlateFireworks;
};
