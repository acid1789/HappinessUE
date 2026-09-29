#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "LessonProgressTicks.generated.h"

class SLessonProgressTicks;

/**
 * Tick marks for a lesson progress bar: a vertical line at the points that unlock 3x3 Normal, 4x4 Easy and
 * 4x4 Normal (the Final unlocks at the bar's end), spread across the widget's width. Place it over the bar with
 * the same horizontal extent; make it taller than the bar for ticks that overhang it.
 */
UCLASS()
class HAPPINESS_API ULessonProgressTicks : public UWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ticks")
	FLinearColor TickColor = FLinearColor(0.02f, 0.02f, 0.05f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ticks")
	float TickThickness = 2.f;

	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<SLessonProgressTicks> Ticks;
};
