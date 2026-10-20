#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/RetainerBox.h"
#include "GameCellWidget.generated.h"

class UMaterialInstanceDynamic;
class UNiagaraSystem;
class UNiagaraUIWidget;

/** Cache static icon content, redrawing when an icon or its layout changes. */
UCLASS()
class HAPPINESS_API UGameCellIconRetainer : public URetainerBox
{
	GENERATED_BODY()
public:
	UGameCellIconRetainer(const FObjectInitializer& ObjectInitializer);
};

/** One Niagara effect a cell plays, drawn in the cell's overlay over the game viewport at Rect */
USTRUCT()
struct FCellEffect
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraUIWidget> Widget;

	/** Its own material instance, showing its icon */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	/** Where it plays, in viewport units */
	FBox2D Rect = FBox2D(ForceInit);
};

/**
 * Clips candidate and resolved icons to the cell's rounded frame, and plays Niagara effects over its icons, in an
 * overlay over the whole screen (the icons themselves left as they are). The overlay goes in the root canvas of
 * the screen holding the board (UGamePanelWidget) at EffectZOrder (WBP_Happiness: over the board, under the end
 * screens and dialogs), or over everything in the game viewport if there's no such canvas:
 * - when its icon is set, a pulse (PulseSystem): the icon grows to PulseScale and back over PulseDuration;
 * - when a candidate icon is eliminated, it breaks into pieces that fall (ShatterSystem, over ShatterDuration);
 * - when the puzzle is solved, its icon bursts into confetti (FireworkSystem; started by UGamePanelWidget).
 * Nothing plays for what the cell shows when it first appears.
 */
UCLASS(Abstract)
class HAPPINESS_API UGameCellWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UGameCellWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, Category = "Pulse")
	float PulseScale = 1.25f;

	/** Seconds to grow and shrink back */
	UPROPERTY(EditAnywhere, Category = "Pulse")
	float PulseDuration = 0.5f;

	/** The pulse: user parameters IconMaterial, IconSize, PulseScale, PulseDuration (NS_IconPulse) */
	UPROPERTY(EditAnywhere, Category = "Pulse")
	TObjectPtr<UNiagaraSystem> PulseSystem;

	/**
	 * The pulse leaves out the icon's frame: what's outside an octagon this far in from each edge (a fraction of the
	 * icon's size), with corners cut by PulseFrameChamfer (M_IconPulse's FrameInset and FrameChamfer). 0: all of it.
	 */
	UPROPERTY(EditAnywhere, Category = "Pulse")
	float PulseFrameInset = 0.065f;

	UPROPERTY(EditAnywhere, Category = "Pulse")
	float PulseFrameChamfer = 0.12f;

	/** The effects' material: a dynamic instance of it per effect, its Icon texture the effect's icon */
	UPROPERTY(EditAnywhere, Category = "Pulse")
	TObjectPtr<UMaterialInterface> PulseMaterial;

	/** Seconds the pieces of an eliminated icon fall for */
	UPROPERTY(EditAnywhere, Category = "Shatter")
	float ShatterDuration = 1.f;

	/** The break: user parameters IconMaterial, IconSize, ShatterDuration (NS_IconShatter) */
	UPROPERTY(EditAnywhere, Category = "Shatter")
	TObjectPtr<UNiagaraSystem> ShatterSystem;

	/** Seconds the confetti of a solved puzzle's icon lasts (each piece 70-100% of it) */
	UPROPERTY(EditAnywhere, Category = "Firework")
	float FireworkDuration = 1.8f;

	/** The solved puzzle's burst: user parameters IconMaterial, IconSize, ShatterDuration (NS_IconFirework) */
	UPROPERTY(EditAnywhere, Category = "Firework")
	TObjectPtr<UNiagaraSystem> FireworkSystem;

	/**
	 * Makes the firework's effect widget and places it over the icon, ready to play: a new effect widget is drawn at
	 * the top left of the screen for the frame it's added in, so it's made at least a frame before PlayFirework.
	 */
	void PrepareFirework();

	/** The cell's icon bursts into confetti and is hidden. False if it shows no icon. */
	bool PlayFirework();

	/** Shows the icon PlayFirework hid */
	void EndFirework();

	/** Z order of the effects' overlay in the screen's root canvas: above the board (0), below the ad banner (5) */
	UPROPERTY(EditAnywhere, Category = "Pulse")
	int32 EffectZOrder = 1;
protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> IconMaskMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UGameCellIconRetainer> IconRetainer;
	FVector2D MaskSize = FVector2D::ZeroVector;

	/** Starts the effects for what changed since the last tick */
	void UpdateEffects();
	/** Plays effect Index (0: the pulse, 1 + n: candidate icon n breaking) over the widget Over, showing Icon */
	class UNiagaraComponent* PlayEffect(int32 Index, UNiagaraSystem* System, UTexture* Icon, const UWidget& Over);
	void RemoveEffects();

	UPROPERTY(Transient)
	TArray<FCellEffect> Effects;
	/** The canvas over the screen the effects play in */
	TSharedPtr<class SCanvas> EffectOverlay;

	/** Holds EffectOverlay in the screen's root canvas; null when the overlay is in the game viewport */
	UPROPERTY(Transient)
	TObjectPtr<class UNativeWidgetHost> EffectHost;

	/** What showed last tick: the final icon, and each candidate icon; unknown until the first tick */
	bool bKnowsShown = false;
	bool bShowedFinalIcon = false;
	TArray<bool> ShowedCandidates;
};
