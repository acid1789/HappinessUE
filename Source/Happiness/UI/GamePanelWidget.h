#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GamePanelWidget.generated.h"

class UCanvasPanel;

/** Keeps the puzzle's columns sized to its two rows of candidate icons. */
UCLASS(Abstract)
class HAPPINESS_API UGamePanelWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCanvasPanel> Border_44;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	bool bSavedHintLayout = false;
	float NormalPanelTop = 0.f;
	ESlateVisibility NormalHintVisibility = ESlateVisibility::SelfHitTestInvisible;
};
