#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TutorialPanel.generated.h"

class UButton;
class UTextBlock;

/**
 * Parent of WBP_Tutorial, the Lessons screen's "Tutorial" button (in WBP_CampaignTree's scrolling content, above the
 * first column, Lessons mode only), styled like WBP_CompleteLessons. Pressing it broadcasts OnTutorialRequested;
 * the tutorial itself isn't built yet.
 */
UCLASS()
class HAPPINESS_API UTutorialPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTutorialRequested);

	/** The button was pressed */
	UPROPERTY(BlueprintAssignable)
	FOnTutorialRequested OnTutorialRequested;

	/** Shown only in Lessons mode (UCampaignTreeWidget calls this when it shows) */
	UFUNCTION()
	void Refresh();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text;

private:
	UFUNCTION()
	void HandleClicked();
};
