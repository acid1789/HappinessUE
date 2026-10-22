#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CompleteLessonsPanel.generated.h"

class UButton;
class UTextBlock;

/**
 * Parent of WBP_CompleteLessons, the Lessons screen's "Complete All Lessons" button (bottom left of WBP_CampaignTree,
 * Lessons mode only), for players who already know the game. It asks first with that screen's confirm dialog
 * (UConfirmDialogWidget::Ask) and, once that's confirmed, completes every lesson (UCampaignSubsystem::CompleteAllLessons),
 * which unlocks Campaign mode. Hidden once every lesson is completed.
 */
UCLASS()
class HAPPINESS_API UCompleteLessonsPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Shown only in Lessons mode while some lesson isn't completed (UCampaignTreeWidget calls this when it shows) */
	UFUNCTION()
	void Refresh();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text;

private:
	UFUNCTION()
	void HandleClicked();
};
