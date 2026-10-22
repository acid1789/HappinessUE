#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "ClueDescriptionWidget.generated.h"

class UFont;
class UHorizontalBox;
class UTexture2D;

/** Compact, content-sized reading plaque for the currently selected clue. */
UCLASS(Abstract)
class HAPPINESS_API UClueDescriptionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UClueDescriptionWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void StylePieces();

	UPROPERTY() TObjectPtr<UFont> ReadingFont;
	UPROPERTY() TObjectPtr<UTexture2D> ReadingPanelTexture;
	FSlateBrush PanelBrush;
};
