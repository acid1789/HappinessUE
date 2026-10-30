#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerInfoPanel.generated.h"

class UButton;
class UCanvasPanel;
class UFont;
class UImage;
class UTexture2D;
class USoundBase;
class UWidgetTree;

/**
 * The player info panel, opened by tapping the player level badge's sun in the main menu (UHappinessModeWidget makes
 * it the first time). A window in the middle of the screen, clear of the sun (which stays up, above the dimmed menu):
 * Mastered Clues, two rows of the badge's twelve Master rewards (faint until earned), then Options (WBP_Options),
 * Game Rules (WBP_GameRules) and Icon Sets (UIconSetsPanel). The close button (top right), tapping outside the window, or the sun
 * again, closes it.
 * Built in code, in the pause menu's garden style.
 */
UCLASS()
class HAPPINESS_API UPlayerInfoPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	UPlayerInfoPanel(const FObjectInitializer& ObjectInitializer);

	/** Reads the Master rewards again and shows the panel */
	void Show();

	void Hide();

	bool IsOpen() const { return IsVisible(); }

	/** The panel's close button: a round ivory button with an ink cross (the icon sets screen's too) */
	static UButton* MakeCloseButton(UWidgetTree& Tree, FName Name = TEXT("CloseButton"));

	/** Called when the panel closes */
	FSimpleMulticastDelegate OnClosed;

	/** A Master reward's look: locked rewards show at this opacity */
	UPROPERTY(EditAnywhere, Category = "Player Info")
	float LockedRewardOpacity = 0.25f;

	/** The options screen (WBP_Options) and the game rules screen (WBP_GameRules), opened over the panel */
	UPROPERTY(EditAnywhere, Category = "Player Info")
	TSoftClassPtr<UUserWidget> OptionsClass;

	UPROPERTY(EditAnywhere, Category = "Player Info")
	TSoftClassPtr<UUserWidget> RulesClass;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildLayout();
	void Refresh();
	/** Class's screen, made over the whole menu the first time (Screen keeps it), then shown */
	UUserWidget* OpenScreen(TObjectPtr<UUserWidget>& Screen, UClass* Class);
	void PlaySound(USoundBase* Sound) const;

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleOptionsClicked();

	UFUNCTION()
	void HandleRulesClicked();

	UFUNCTION()
	void HandleIconSetsClicked();

	UPROPERTY() TObjectPtr<UTexture2D> PanelTexture;
	UPROPERTY() TObjectPtr<UTexture2D> RewardAtlasTexture;
	UPROPERTY() TObjectPtr<UFont> PanelFont;
	UPROPERTY() TObjectPtr<USoundBase> AcceptSound;
	UPROPERTY() TObjectPtr<USoundBase> CancelSound;

	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Root;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> Rewards;
	UPROPERTY(Transient) TObjectPtr<UButton> OptionsButton;
	UPROPERTY(Transient) TObjectPtr<UButton> RulesButton;
	UPROPERTY(Transient) TObjectPtr<UButton> IconSetsButton;
	UPROPERTY(Transient) TObjectPtr<UButton> CloseButton;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> OptionsWidget;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> RulesWidget;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> IconSetsWidget;
};
