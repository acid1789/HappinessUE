#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ButtonPanelWidget.generated.h"

class UWidget;

/**
 * The puzzle's side button panel. When the player hasn't touched anything for a while, the hint button gives a
 * small wiggle (a little left, a little right, back), again every IdleSeconds while they stay idle.
 */
UCLASS(Abstract)
class HAPPINESS_API UButtonPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Button_H;

	/** Seconds without input before each wiggle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint Wiggle")
	float IdleSeconds = 5.f;

	/** How far the button turns each way, in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint Wiggle")
	float WiggleDegrees = 12.f;

	/** How long one wiggle takes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint Wiggle")
	float WiggleSeconds = 0.6f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	double ShownTime = 0.0;
};
