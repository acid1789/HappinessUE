#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ClueWidget.generated.h"

class UGameCellIconRetainer;

/**
 * Parent of WBP_ClueBase (WBP_HorizontalClue, WBP_VerticalClue). Clips the clue's icons to the rounded corners of
 * its frame, as UGameCellWidget does for the grid cells: the icon box (the parent of "HorizontalBox" or
 * "VerticalBox") goes into a retainer masked by M_CellRoundedMask, in the box's place in the layout.
 */
UCLASS(Abstract)
class HAPPINESS_API UClueWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UClueWidget(const FObjectInitializer& ObjectInitializer);

	/** Corner radius of the icon mask, in layout pixels (the frame's rounded corners) */
	UPROPERTY(EditAnywhere, Category = "Clue")
	float IconCornerRadius = 6.f;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> IconMaskMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UGameCellIconRetainer> IconRetainer;
	FVector2D MaskSize = FVector2D::ZeroVector;
};
