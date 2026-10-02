#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HappinessClassic/CampaignProgress.h"
#include "HappinessClassic/FreePlaySettings.h"
#include "CampaignEndScreenWidget.generated.h"

class UButton;
class UProgressBar;
class USoundBase;
class UTextBlock;
class UPuzzle;
class UWidget;
class ULessonProgressTicks;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCampaignEndScreenEvent);

/**
 * End of puzzle screen for Campaign mode, laid out like the Free Play end screen. Scored like Free Play
 * (UFreePlaySubsystem::FinishFreePlayPuzzle): the time against a par from the puzzle's rating and the player's pace,
 * completion experience and a time bonus, which fill the player's level bar. Also shows the clue and stage
 * ("Game Basics - 5x5 Normal") and the clue's whole campaign track ("7 / 15", ticks at the stages).
 *
 * Staged like the other end screens: the lines appear one at a time, then the level bar fills with the experience
 * (rolling over on a level up), then the campaign bar fills if the puzzle was a step. The buttons are enabled once
 * that is done; they hide the screen and fire their event.
 */
UCLASS(Abstract)
class HAPPINESS_API UCampaignEndScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCampaignEndScreenWidget(const FObjectInitializer& ObjectInitializer);

	/** Clue and stage, e.g. "Game Basics - 5x5 Normal" */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> LessonText;

	/** "SUCCESS!" or "INCORRECT" */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> ResultText;

	// Values revealed one at a time; their labels stay visible
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> TimeText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> ParTimeText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CompletionExpText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> BonusExpText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> TotalText;

	/** Mistakes or a stage already done; shown with the total, empty otherwise */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NoteText;

	/** The player's level and progress to the next, as on the Free Play end screen */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UProgressBar> ExpBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText;

	/** "123 / 456" */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ExpText;

	/** The clue's whole campaign track; a LessonProgressTicks over it marks the stages */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UProgressBar> StageProgress;

	/** The bar's label: "Campaign:", or "Master:" for a Master Mode puzzle */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StageLabel;

	/** Tick marks over StageProgress: the campaign's stages, or Master Mode's */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<ULessonProgressTicks> ProgressTicks;

	/** "7 / 15" */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ProgressText;

	/** The next stage unlocked, or the clue completed; appears when the bar fills */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UnlockText;

	/** Stage the next puzzle comes from, above the Next Puzzle button */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NextPuzzleText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> RestartButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> ReturnButton;

	/** Hidden when the board had mistakes and when the clue was just completed */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> NextPuzzleButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign End Screen")
	FSlateColor SolvedColor = FSlateColor(FLinearColor(0.f, 0.5f, 0.f, 1.f));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign End Screen")
	FSlateColor IncorrectColor = FSlateColor(FLinearColor(1.f, 0.f, 0.f, 1.f));

	/** Seconds before the first line appears */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign End Screen")
	float StartDelay = 1.f;

	/** Seconds between lines, and before the bar fills */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign End Screen")
	float StepTime = 0.5f;

	/** Seconds for the level bar to take the experience (the Free Play screen's rate) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign End Screen")
	float ExpFillTime = 2.f;

	/** Seconds for the campaign bar to fill */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign End Screen")
	float FillTime = 1.f;

	/** Played as each line appears */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign End Screen")
	TObjectPtr<USoundBase> StepSound;

	/** Played every frame while a bar fills */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign End Screen")
	TObjectPtr<USoundBase> FillSound;

	/** Played when a stage unlocks, the clue completes or a level is gained */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign End Screen")
	TObjectPtr<USoundBase> UnlockSound;

	/** Played when a button is pressed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign End Screen")
	TObjectPtr<USoundBase> ButtonSound;

	UPROPERTY(BlueprintAssignable, Category = "Campaign End Screen")
	FOnCampaignEndScreenEvent OnRestartPuzzle;

	UPROPERTY(BlueprintAssignable, Category = "Campaign End Screen")
	FOnCampaignEndScreenEvent OnReturnToLessons;

	UPROPERTY(BlueprintAssignable, Category = "Campaign End Screen")
	FOnCampaignEndScreenEvent OnNextPuzzle;

	/** Record and score the finished campaign puzzle and show its result */
	UFUNCTION(BlueprintCallable, Category = "Campaign End Screen")
	void Show(UPuzzle* Puzzle, float PuzzleSeconds);

	UFUNCTION(BlueprintCallable, Category = "Campaign End Screen")
	void Hide();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	enum class EAnimState : uint8
	{
		Idle,
		Revealing,		// showing the lines one at a time
		FillingExp,		// the level bar taking the experience
		FillingStage,	// the campaign bar taking the step
		Done,
	};

	UFUNCTION()
	void HandleRestartClicked();

	UFUNCTION()
	void HandleReturnClicked();

	UFUNCTION()
	void HandleNextClicked();

	/** Adds the experience to the player controller's level (HappinessExp / HappinessLevel); the bar starts from
	 *  the old values */
	void AddPlayerExp(int32 Exp);

	/** Experience from one level to the next, as the Free Play end screen has it: ln(level + 3) x 1000 */
	static int32 GetExpForNextLevel(int32 Level);

	FText GetNoteText() const;
	FText GetUnlockText() const;
	TArray<UWidget*> GetRevealedWidgets() const;
	void SetShownProgress(float Points);
	void SetShownExp();
	void ShowUnlock();
	void Finish();
	void SetButtonsEnabled(bool bEnabled);
	void PlaySound(USoundBase* Sound) const;

	FLessonPuzzleResult Result;
	FFreePlayScore Score;
	int32 NextStage = 0;

	// The level bar as shown: level, experience into it, and experience still to add
	int32 ShownLevel = 0;
	float ShownExp = 0.f;
	float ExpToAdd = 0.f;
	float ExpRate = 0.f;

	EAnimState AnimState = EAnimState::Idle;
	float AnimTimeRemaining = 0.f;
	int32 RevealedCount = 0;
	float FillElapsed = 0.f;
	bool bUnlockShown = false;
};
