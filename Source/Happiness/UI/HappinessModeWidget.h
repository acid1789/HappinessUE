#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HappinessClassic/CampaignProgress.h"
#include "HappinessClassic/PuzzleSlots.h"
#include "HappinessModeWidget.generated.h"

class UButton;
class UConfirmDialogWidget;
class UFireworksWidget;
class ULessonsCompletePopup;
class USoundBase;
class UPlayerLevelBadge;
class UStreakSaverPanel;
class UTextBlock;
class USizeBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHappinessModeEvent);

/**
 * Happiness mode select, between the game select screen and playing: Classic (the classic puzzle flow)
 * or Lessons (the campaign tree). The owner reacts to the events; the screen just hides itself.
 * Each mode resumes its own puzzle in progress, if it has one (UPuzzleSlotSubsystem), rather than its menus.
 */
UCLASS(Abstract)
class HAPPINESS_API UHappinessModeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UHappinessModeWidget(const FObjectInitializer& ObjectInitializer);

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

	/**
	 * Every lesson just completed (the first time the screen shows after it): the congratulations
	 * (ULessonsCompletePopup) open, and this many seconds later fireworks burst over them
	 */
	UPROPERTY(EditAnywhere, Category = "Lessons Complete")
	float UnlockFireworksDelay = 0.35f;

	/** Played with the first burst */
	UPROPERTY(EditAnywhere, Category = "Lessons Complete")
	TObjectPtr<USoundBase> UnlockSound;

	/** The dialog that asks whether to resume or abandon a mode's puzzle in progress (WBP_ConfirmDialog) */
	UPROPERTY(EditAnywhere, Category = "Happiness Mode")
	TSoftClassPtr<UConfirmDialogWidget> ConfirmDialogClass;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

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

	/** Over the whole screen while every lesson's completion is celebrated (made the first time it's needed) */
	UPROPERTY(Transient)
	TObjectPtr<UFireworksWidget> Fireworks;

	/** The congratulations for completing every lesson (made the first time it's needed) */
	UPROPERTY(Transient)
	TObjectPtr<ULessonsCompletePopup> LessonsCompletePopup;

	/** Asks whether to resume a puzzle in progress, over the whole screen (made the first time it's needed) */
	UPROPERTY(Transient)
	TObjectPtr<UConfirmDialogWidget> ConfirmDialog;

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

	/** Moves the badge from this screen into the main menu around it, behind its screens (once it has been built) */
	void MoveBadgeToMenu();

	/** A new player's first start (no save at all): straight to the Lessons tree with Game Basics open */
	void StartNewPlayer();

	/** Make the Streak Saver button if the layout has none */
	void MakeStreakSaverButton();

	UFUNCTION()
	void HandleStreakSaverClicked();

	/** The Daily button (disabled once today's is solved) and the streak line */
	void RefreshDaily();

	void OpenTree(ECampaignMode Mode);

	/** PuzzleSlot's puzzle in progress, if it can be resumed: hide and play it. False if there's none. */
	bool ResumePuzzle(EPuzzleSlot PuzzleSlot);

	/**
	 * PuzzleSlot has a puzzle in progress: ask whether to resume it (OnResume) or abandon it (it's dropped from its
	 * slot, then OnAbandon). False, asking nothing, if there's none.
	 */
	bool AskResumeOrAbandon(EPuzzleSlot PuzzleSlot, TFunction<void()> OnResume, TFunction<void()> OnAbandon);
	UConfirmDialogWidget* GetConfirmDialog();

	/** Today's daily puzzle: its puzzle in progress if bResume and there is one, otherwise from the start */
	void PlayDaily(bool bResume);
	/** On to Free Play: its puzzle in progress, or its settings */
	void PlayFreePlay();
	/** The lesson tree in Mode */
	void ShowTree(ECampaignMode Mode);
	void RefreshCampaignLock();

	/** Every lesson's completion: the congratulations, with fireworks over them */
	void StartUnlockCelebration();
	void UpdateUnlockCelebration(float DeltaTime);
	/** Stop the fireworks (the congratulations stay open) */
	void EndUnlockCelebration();
	bool IsCelebratingUnlock() const { return UnlockTime >= 0.f; }
	/** Where the fireworks burst, over the congratulations' window: false until it has been laid out */
	bool PlayUnlockFireworks();
	void ShowLessonsCompletePopup();

	/** Seconds into the celebration (below 0: none) */
	float UnlockTime = -1.f;
	bool bUnlockFireworksStarted = false;
	/** When the first burst comes, for its sound (below 0: not known yet, or played) */
	float FirstBurstAt = -1.f;

	UFUNCTION()
	void HandleBackClicked();
};
