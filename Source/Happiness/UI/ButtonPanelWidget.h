#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ButtonPanelWidget.generated.h"

class UWidget;

/**
 * The puzzle's side button panel. When the player hasn't touched anything for a while, the hint button gives a
 * small wiggle (a little left, a little right, back), again every IdleSeconds while they stay idle, unless the
 * player turned "Animate Hint Button" off in the options. On Master Mode stages hints and Fix Puzzle aren't allowed: the
 * hint button is disabled and keeps still, and the Fix button is disabled (UCampaignSubsystem::AreAssistsAllowed).
 *
 * On a phone held with its notch (or Dynamic Island) on the left, over this panel, the panel moves right by the
 * screen's left safe-area inset, and the panels next to it (NotchMakeRoom: the board and the help line above it) give
 * up that much width. With the notch on the right, over the clues, nothing moves.
 */
UCLASS(Abstract)
class HAPPINESS_API UButtonPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Button_H;

	/** Fix Puzzle: disabled on Master Mode stages, like the hint button */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Button_Fix;

	/** Seconds without input before each wiggle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint Wiggle")
	float IdleSeconds = 5.f;

	/** How far the button turns each way, in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint Wiggle")
	float WiggleDegrees = 12.f;

	/** How long one wiggle takes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint Wiggle")
	float WiggleSeconds = 0.6f;

	/** Siblings (by name, in the same canvas) that start right of this panel and give up width when it moves for a
	 * left notch: their left edges move right with it */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notch")
	TArray<FName> NotchMakeRoom = { TEXT("GamePanel"), TEXT("HelpPanel") };

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** The "Animate Hint Button" option (PC_Happiness Happiness_HintAnimation); on if the controller doesn't have it */
	bool IsAnimationEnabled() const;

	double ShownTime = 0.0;

	/** The screen's safe-area inset on the left, in widget units at this layout scale: 0 unless the notch is there */
	float GetLeftNotchInset(float Scale) const;

	/** Moves this panel and NotchMakeRoom right by the left notch inset, when it changes (the phone turned around) */
	void UpdateNotchShift(float Scale);

	float AppliedNotchInset = 0.f;
	/** iOS: when to read the notch from UIKit again */
	mutable double NextNotchReadTime = 0.0;
	/** The left offsets as designed, before any notch shift, by widget name */
	TMap<FName, float> DesignedLeftOffsets;
};
