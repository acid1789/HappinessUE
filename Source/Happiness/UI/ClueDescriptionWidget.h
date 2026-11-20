#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "ClueDescriptionWidget.generated.h"

class UFont;
class UHorizontalBox;
class UImage;
class UTextBlock;
class UTexture2D;

/**
 * Compact, content-sized reading plaque for the currently selected clue. While no clue is selected it shows the
 * tips instead (UMenuTipsWidget's pool), one at a time, changing every TipSeconds with a short fade, unless the
 * Show Tips option is off.
 */
UCLASS(Abstract)
class HAPPINESS_API UClueDescriptionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UClueDescriptionWidget(const FObjectInitializer& ObjectInitializer);

	/** Seconds each tip shows while no clue is selected, its fades included */
	UPROPERTY(EditAnywhere, Category = "Tips")
	float TipSeconds = 10.f;

	/** Seconds of each fade (out, then the next tip in) */
	UPROPERTY(EditAnywhere, Category = "Tips")
	float FadeSeconds = 0.3f;

	/** How often a tip is an icon and its name rather than a written tip (0 to 1) */
	UPROPERTY(EditAnywhere, Category = "Tips")
	float IconTipChance = 0.35f;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void StylePieces();
	/** No clue selected (the line is empty, or holds only the tip): show a tip on the line, and change it in time */
	void UpdateTip(float InDeltaTime);
	/** Put another tip on the line, not the one just shown */
	void ShowNextTip(UHorizontalBox& Line);

	UPROPERTY() TObjectPtr<UFont> ReadingFont;
	UPROPERTY() TObjectPtr<UTexture2D> ReadingPanelTexture;
	FSlateBrush PanelBrush;

	/** The tip's pieces on the line: the icon (icon tips only) and the text */
	UPROPERTY(Transient) TObjectPtr<UImage> TipIcon;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TipText;
	/** Seconds into the current tip */
	float TipTime = 0.f;
	int32 LastTip = INDEX_NONE;
};
