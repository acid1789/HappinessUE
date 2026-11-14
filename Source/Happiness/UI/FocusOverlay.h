#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "FocusOverlay.generated.h"

class SFocusOverlay;
class FFocusInputFilter;

/**
 * A full-screen "focus" effect: everything is dimmed (Color at Opacity, 75% black) except any number of circles,
 * which stay clear. Circles may overlap. Place it over what it dims, stretched to fill (a canvas slot anchored 0,0 to
 * 1,1, or AddToViewport with a high Z order).
 *
 * A circle is either fixed (AddFocusCircle, in this widget's own units) or follows a widget (AddFocusOnWidget): it
 * goes round that widget's box, wherever the widget is drawn, every frame. Either can grow (or shrink) in as it's
 * added: from StartRadius to its radius over Duration seconds. A moving circle (AddFocusMove) goes from round one set
 * of widgets to round another, its center and radius both changing over Duration.
 *
 * The dim is drawn as a mesh: rows one pixel high, each dimmed outside the circles' spans in that row, with a soft
 * edge (EdgeSoftness pixels) where a row meets a circle. Nothing is drawn into a texture, so it costs no render target.
 * It takes no input by default (HitTestInvisible): taps go through to what's under it, dimmed or not. With
 * SetBlocksInput it takes every tap and click instead, circles included, so only what's drawn above it can be used.
 * With SetInputWindow, taps and clicks only reach the given widgets (and nothing else, above or below it).
 *
 * Dev builds: Happiness.Focus x,y,r[,StartRadius,Duration] ... shows one over the game (x, y as fractions of the
 * screen's width and height, the radii of its height); Happiness.FocusWidget <WidgetName> [Padding] [StartRadius]
 * [Duration] adds a circle round a widget on screen (StartRadius in this widget's units); Happiness.Focus alone
 * removes it.
 */
UCLASS()
class HAPPINESS_API UFocusOverlay : public UWidget
{
	GENERATED_BODY()

public:
	UFocusOverlay();

	/** The dim's color; its alpha is replaced by Opacity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Focus")
	FLinearColor Color = FLinearColor::Black;

	/** How opaque the dim is outside the circles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Focus", meta = (ClampMin = "0", ClampMax = "1"))
	float Opacity = 0.75f;

	/** Width of the soft edge round each circle, in screen pixels (0: a hard edge) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Focus", meta = (ClampMin = "0"))
	float EdgeSoftness = 2.f;

	/**
	 * A clear circle at Center with Radius, in this widget's units (its top left is 0,0). With a Duration, it starts
	 * at StartRadius and changes to Radius over Duration seconds.
	 */
	UFUNCTION(BlueprintCallable, Category = "Focus")
	void AddFocusCircle(FVector2D Center, float Radius, float StartRadius = 0.f, float Duration = 0.f);

	/**
	 * A clear circle round Target, following it: centered on its box, through the box's corners, plus Padding (in
	 * Target's units). Nothing while Target isn't drawn. With a Duration, it starts at StartRadius (in this widget's
	 * units) and changes to that radius over Duration seconds.
	 */
	UFUNCTION(BlueprintCallable, Category = "Focus")
	void AddFocusOnWidget(UWidget* Target, float Padding = 8.f, float StartRadius = 0.f, float Duration = 0.f);

	/**
	 * A clear circle that starts round From and moves to round To over Duration seconds (center and radius both, in a
	 * straight line), then stays round To. Round a set of widgets: centered on the box holding them all, through its
	 * corners, plus a padding (FromPadding at the start, Padding at the end, in the widgets' units). Tight (bTight at
	 * the end, bFromTight at the start): half the box's longer side instead, so the circle meets the box's long ends
	 * and cuts its corners (round a clue's two stacked icons: centered between them, one icon's size). Both ends follow
	 * their widgets. Nothing while none of To is drawn; at once round To if none of From is.
	 */
	UFUNCTION(BlueprintCallable, Category = "Focus")
	void AddFocusMove(const TArray<UWidget*>& From, const TArray<UWidget*>& To, float Padding = 8.f, float Duration = 1.f,
		float FromPadding = 0.f, bool bTight = false, bool bFromTight = false);

	/** No circles: all dimmed */
	UFUNCTION(BlueprintCallable, Category = "Focus")
	void ClearFocus();

	/** Take every tap and click (true), so nothing under it can be used, or let them through (false, the default) */
	UFUNCTION(BlueprintCallable, Category = "Focus")
	void SetBlocksInput(bool bBlock);

	/**
	 * Taps and clicks only reach Widgets (each one while it's shown, over its whole box): anywhere else on screen
	 * they're dropped before any widget sees them (an input preprocessor), whatever is drawn there. Only while this is
	 * on screen. Empty: input goes through as usual. The overlay itself doesn't take input meanwhile.
	 */
	UFUNCTION(BlueprintCallable, Category = "Focus")
	void SetInputWindow(const TArray<UWidget*>& Widgets);

	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void BeginDestroy() override;

	/** SetInputWindow is on, this is on screen and Position (desktop space) is on none of its widgets */
	bool ShouldDropInput(const FVector2D& ScreenPosition) const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	struct FFocus
	{
		FVector2D Center = FVector2D::ZeroVector;
		float Radius = 0.f;
		/** Widgets it goes round (none: fixed at Center, Radius) */
		TArray<TWeakObjectPtr<UWidget>> Targets;
		float Padding = 0.f;
		/** Widgets it starts round (AddFocusMove; none: it starts at StartRadius, already at its center) */
		TArray<TWeakObjectPtr<UWidget>> StartTargets;
		float StartPadding = 0.f;
		/** Tight circles (AddFocusMove's bTight, bFromTight) */
		bool bTight = false;
		bool bStartTight = false;
		float StartRadius = 0.f;
		float Duration = 0.f;
		/** When it was added (Slate's clock), for its animation */
		double StartTime = 0.0;
	};

	static double Now();
	TArray<FFocus> Focuses;

	TSharedPtr<SFocusOverlay> MyOverlay;

	/** SetInputWindow's widgets, and the preprocessor that drops input elsewhere while there are some */
	TArray<TWeakObjectPtr<UWidget>> InputWindow;
	TSharedPtr<FFocusInputFilter> InputFilter;
	void StopInputFilter();

	void PushFocuses();
	friend class SFocusOverlay;
};
