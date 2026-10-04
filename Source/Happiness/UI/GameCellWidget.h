#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/RetainerBox.h"
#include "GameCellWidget.generated.h"

/** Cache static icon content, redrawing when an icon or its layout changes. */
UCLASS()
class HAPPINESS_API UGameCellIconRetainer : public URetainerBox
{
	GENERATED_BODY()
public:
	UGameCellIconRetainer(const FObjectInitializer& ObjectInitializer);
};

/** Clips candidate and resolved icons to the cell's rounded frame. */
UCLASS(Abstract)
class HAPPINESS_API UGameCellWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UGameCellWidget(const FObjectInitializer& ObjectInitializer);
protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> IconMaskMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UGameCellIconRetainer> IconRetainer;
	FVector2D MaskSize = FVector2D::ZeroVector;
};
