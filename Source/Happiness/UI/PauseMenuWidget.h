#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class APlayerController;
class UButton;
class USoundBase;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPauseMenuAction);

/**
 * Parent of WBP_PauseMenu2, the puzzle screen's pause menu: a column of buttons in the middle of the screen (no
 * background, no help slides). While it shows, the rest of the puzzle screen is hidden: every sibling that was
 * visible when it opened is collapsed, and restored when it closes. Dialogs opened from the menu (confirmations,
 * Options) aren't touched, as they were hidden when it opened. Each button broadcasts its event; WBP_Happiness binds them to its
 * actions (most ask for confirmation first). Options opens the Options screen over the menu itself.
 *
 * Quit App is hidden on iOS, where apps aren't supposed to quit themselves.
 */
UCLASS()
class HAPPINESS_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPauseMenuWidget(const FObjectInitializer& ObjectInitializer);

	/** Show the menu (the puzzle is paused by its owner) */
	UFUNCTION(BlueprintCallable, Category = "Pause Menu")
	void Show(APlayerController* PlayerController);

	/** Shown or hidden (WBP_Happiness's Pause / UnPause): hide or restore the rest of the screen */
	virtual void SetVisibility(ESlateVisibility InVisibility) override;

	UPROPERTY(BlueprintAssignable, Category = "Pause Menu")
	FOnPauseMenuAction OnResumeGame;

	UPROPERTY(BlueprintAssignable, Category = "Pause Menu")
	FOnPauseMenuAction OnResetPuzzle;

	UPROPERTY(BlueprintAssignable, Category = "Pause Menu")
	FOnPauseMenuAction OnFixPuzzle;

	UPROPERTY(BlueprintAssignable, Category = "Pause Menu")
	FOnPauseMenuAction OnUnhideClues;

	/** Game Rules: the rules screen is still to come (nothing is bound to this yet) */
	UPROPERTY(BlueprintAssignable, Category = "Pause Menu")
	FOnPauseMenuAction OnGameRules;

	UPROPERTY(BlueprintAssignable, Category = "Pause Menu")
	FOnPauseMenuAction OnMainMenu;

	UPROPERTY(BlueprintAssignable, Category = "Pause Menu")
	FOnPauseMenuAction OnQuitApp;

	/** The game rules screen (WBP_GameRules, a UGameRulesWidget), over the menu. Soft, like OptionsClass. */
	UPROPERTY(EditAnywhere, Category = "Pause Menu")
	TSoftClassPtr<UUserWidget> RulesClass;

	/**
	 * The Options screen (its Show(PC) is called when it opens). A soft reference, loaded when first opened: loading
	 * it while this class's default object is made (at startup) loads WBP_Happiness, whose pause menu is of this
	 * class, and hangs the editor.
	 */
	UPROPERTY(EditAnywhere, Category = "Pause Menu")
	TSoftClassPtr<UUserWidget> OptionsClass;

	UPROPERTY(EditAnywhere, Category = "Pause Menu")
	TObjectPtr<USoundBase> AcceptSound;

	UPROPERTY(EditAnywhere, Category = "Pause Menu")
	TObjectPtr<USoundBase> CancelSound;

protected:
	virtual void NativeConstruct() override;

	/** The puzzle's time so far, set when the menu opens */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TimeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ResetButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> FixButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> UnhideButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RulesButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> OptionsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MainMenuButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitAppButton;

private:
	UFUNCTION()
	void HandleResume();
	UFUNCTION()
	void HandleReset();
	UFUNCTION()
	void HandleFix();
	UFUNCTION()
	void HandleUnhide();
	UFUNCTION()
	void HandleRules();
	UFUNCTION()
	void HandleOptions();
	UFUNCTION()
	void HandleMainMenu();
	UFUNCTION()
	void HandleQuitApp();

	void PlaySound(USoundBase* Sound) const;

	UPROPERTY()
	TObjectPtr<APlayerController> PC;

	/** Show the puzzle screen's time (its PuzzleTime, saved when it paused) as the puzzle formats it */
	void UpdateTime();

	/** Hide the puzzle screen around the menu, or bring back what was hidden */
	void HideScreen(bool bHide);

	/** The siblings hidden while the menu shows, with the visibility each had */
	TMap<TWeakObjectPtr<UWidget>, ESlateVisibility> HiddenScreen;

	/** Open a screen over the menu, made the first time and reused */
	UUserWidget* OpenOverMenu(TObjectPtr<UUserWidget>& Screen, TSoftClassPtr<UUserWidget>& Class, bool& bNew);

	/** The rules screen, made the first time and reused (it hides itself when closed) */
	UPROPERTY()
	TObjectPtr<UUserWidget> RulesWidget;

	/** The Options screen, made the first time and reused (it hides itself when closed) */
	UPROPERTY()
	TObjectPtr<UUserWidget> OptionsWidget;
};
