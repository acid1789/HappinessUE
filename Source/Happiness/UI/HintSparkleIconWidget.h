#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HintSparkleIconWidget.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNativeWidgetHost;
class UNiagaraSystem;
class UNiagaraUIWidget;
class UTexture2D;
class UImage;
class SCanvas;

/** Moves Niagara sparkle sprites around the highlighted icon or clue. */
UCLASS(Abstract)
class HAPPINESS_API UHintSparkleIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UHintSparkleIconWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

private:
	void StartSparkles();
	void StopSparkles();
	void UpdateSparklePosition(const FGeometry& Geometry, bool bWholeClue);
	void SpawnSparkle();
	FVector2D GetSparklePosition(int32 Index) const;
	float GetSparkleSize(int32 Index) const;
	UTexture2D* MakeSparkleTexture(const FLinearColor& Color);

	struct FOrbitSparkle
	{
		float Age = 0.f;
		float Lifetime = 0.f;
		float Phase = 0.f;
		float Speed = 0.f;
		float Scale = 0.f;
		float NormalOffset = 0.f;
		bool bActive = false;
	};

	static constexpr int32 SparkleCount = 400;
	static constexpr float SpawnInterval = .008f;
	UPROPERTY() TObjectPtr<UNiagaraSystem> SparkleSystem;
	UPROPERTY() TObjectPtr<UMaterialInterface> SparkleMaterial;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> PurpleTexture;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> SilverTexture;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> PurpleMaterial;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> SilverMaterial;
	UPROPERTY(Transient) TArray<TObjectPtr<UNiagaraUIWidget>> Sparkles;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> SparkleImages;
	UPROPERTY(Transient) TObjectPtr<UNativeWidgetHost> SparkleHost;
	TSharedPtr<SCanvas> SparkleCanvas;
	TArray<FOrbitSparkle> SparkleStates;
	FBox2D SparkleRect = FBox2D(ForceInit);
	float OrbitTime = 0.f;
	float SpawnClock = 0.f;
	float SpriteSize = 20.f;
};
