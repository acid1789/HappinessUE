#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HappinessClassic/CampaignProgress.h"
#include "HappinessModeWidget.generated.h"

class UButton;
class UPlayerLevelBadge;
class UStreakSaverPanel;
class UTextBlock;
class USizeBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHappinessModeEvent);

/**
 * Happiness mode select, between the game select screen and playing: Classic (the classic puzzle flow)
 * or Lessons (the campaign tree). The owner reacts to the events; the screen just hides itself.
 */
UCLASS(Abstract)
class HAPPINESS_API UHappinessModeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> ClassicButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> LessonsButton;

	/** Campaign mode: the lesson tree again on bigger boards. Disabled until every lesson is completed. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> CampaignButton;

	/** Today's daily puzzle */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> DailyButton;

	/** The daily streak, under the Daily button */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StreakText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackButton;

	UPROPERTY(BlueprintAssignable, Category = "Happiness Mode")
	FOnHappinessModeEvent OnClassicChosen;

	/** Lessons or Campaign chosen; the campaign subsystem's mode says which. Both open the lesson tree. */
	UPROPERTY(BlueprintAssignable, Category = "Happiness Mode")
	FOnHappinessModeEvent OnLessonsChosen;

	/** Daily chosen. The mode screen starts the puzzle itself, through its owner's PlayHappiness(Number, Size,
	 *  Difficulty) (WBP_GameSelect), the same way Free Play starts one. */
	UPROPERTY(BlueprintAssignable, Category = "Happiness Mode")
	FOnHappinessModeEvent OnDailyChosen;

	/** Back pressed: return to the game select screen */
	UPROPERTY(BlueprintAssignable, Category = "Happiness Mode")
	FOnHappinessModeEvent OnClosed;

	UFUNCTION(BlueprintCallable, Category = "Happiness Mode")
	void Show();

	UFUNCTION(BlueprintCallable, Category = "Happiness Mode")
	void Hide();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RemoveAdsButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> RemoveAdsCard;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> AdsPurchaseDialog;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ClosePurchaseButton;

	/** Opens the streak saver shop. Made in the Daily button's canvas, just above it, unless the layout has one. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> StreakSaverButton;

	/** Its label: the saver days left, once there are some */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StreakSaverLabel;

	/** The player's level and Master Mode icons, top right. Made in the screen's root canvas unless the layout has one. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPlayerLevelBadge> PlayerLevelBadge;

	/** The streak saver shop, over the whole screen (made the first time it's needed) */
	UPROPERTY(Transient)
	TObjectPtr<UStreakSaverPanel> StreakSaverPanel;

private:
	UFUNCTION()
	void RefreshAdRemoval();

	UFUNCTION()
	void HandleRemoveAdsClicked();

	UFUNCTION()
	void HandleClosePurchaseClicked();

	UFUNCTION()
	void HandleClassicClicked();

	UFUNCTION()
	void HandleLessonsClicked();

	UFUNCTION()
	void HandleCampaignClicked();

	UFUNCTION()
	void HandleDailyClicked();

	UFUNCTION()
	void RefreshStreak();

	/** Make the level badge if the layout has none */
	void MakePlayerLevelBadge();

	/** Make the Streak Saver button if the layout has none */
	void MakeStreakSaverButton();

	UFUNCTION()
	void HandleStreakSaverClicked();

	/** The Daily button (disabled once today's is solved) and the streak line */
	void RefreshDaily();

	void OpenTree(ECampaignMode Mode);
	void RefreshCampaignLock();

	UFUNCTION()
	void HandleBackClicked();
};
