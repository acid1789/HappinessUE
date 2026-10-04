#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConfirmDialogWidget.generated.h"

class UTextBlock;

/**
 * Parent of WBP_ConfirmDialog, the puzzle screen's yes/no dialog. Its question is the Blueprint's Message; this adds
 * an optional description above it (DescriptionText: smaller, white) saying what the action does. The description
 * is cleared whenever the dialog closes, so dialogs that don't set one show none.
 */
UCLASS()
class HAPPINESS_API UConfirmDialogWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Show this description above the question (empty: none). Set it after opening the dialog. */
	UFUNCTION(BlueprintCallable, Category = "Confirm Dialog")
	void SetDescription(FString Description);

	virtual void SetVisibility(ESlateVisibility InVisibility) override;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;
};
