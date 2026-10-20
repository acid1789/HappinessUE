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

/**
 * Clips candidate and resolved icons to the cell's rounded frame, and pulses the icon when it is set: a Niagara
 * effect (PulseSystem, in an overlay over the game viewport, at the cell) draws the icon growing to PulseScale and
 * back over PulseDuration, while the icon itself stays as it is. Cells already set when they first show don't pulse.
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

	/** The effect: user parameters IconMaterial, IconSize, PulseScale, PulseDuration (NS_IconPulse) */
	UPROPERTY(EditAnywhere, Category = "Pulse")
	TObjectPtr<UNiagaraSystem> PulseSystem;

	/** The effect's material: a dynamic instance of it per cell, its Icon texture the cell's icon */
	UPROPERTY(EditAnywhere, Category = "Pulse")
	TObjectPtr<UMaterialInterface> PulseMaterial;
protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> IconMaskMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UGameCellIconRetainer> IconRetainer;
	FVector2D MaskSize = FVector2D::ZeroVector;

	void UpdatePulse(float DeltaTime);
	void StartPulse();
	void RemovePulseOverlay();
	virtual void NativeDestruct() override;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraUIWidget> PulseEffect;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PulseMaterialInstance;
	/** The effect's place: a canvas over the game viewport, the effect at the cell's rectangle (PulseRect) */
	TSharedPtr<SWidget> PulseOverlay;
	FBox2D PulseRect = FBox2D(ForceInit);

	/** Whether the final icon showed last tick; unknown until the first tick */
	bool bKnowsFinalIcon = false;
	bool bShowedFinalIcon = false;
};
