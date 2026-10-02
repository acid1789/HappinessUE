#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HappinessClassic/CampaignProgress.h"
#include "LessonEndScreenWidget.generated.h"

class UButton;
class UProgressBar;
class USoundBase;
class UTextBlock;
class UPuzzle;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLessonEndScreenEvent);

/**
 * End of puzzle screen for lesson mode, in place of the classic end screen (no experience or level).
 *
 * Staged like the classic end screen: after a short delay the time and the score lines appear one at a time
 * (completion points, the hint penalties, the total), then the lesson progress bar fills with the points earned
 * over a fixed time. The buttons are enabled once that is done.
 *
 * Show records the finished puzzle with the campaign (UCampaignSubsystem::FinishLessonPuzzle). The buttons hide
 * the screen and fire their event; the owner restarts the puzzle, plays the next one or returns to the lessons.
 */
UCLASS(Abstract)
class HAPPINESS_API ULessonEndScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	ULessonEndScreenWidget(const FObjectInitializer& ObjectInitializer);

	/** Lesson name and stage, e.g. "Gap - 3x3 Easy" */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> LessonText;

	/** "SUCCESS!" or "INCORRECT" */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> ResultText;

	// Values revealed one at a time; their labels stay visible

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> TimeText;

	/** 3 when solved */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CompletionText;

	/** -1 if any hint was used, else 0 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> HintPenaltyText;

	/** Another -1 if a hint was used on the lesson's clue, else 0 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> LessonHintPenaltyText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> TotalText;

	/** Why no points are added (practice stage, already scored, mistakes), shown with the total; empty otherwise */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NoteText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UProgressBar> LessonProgress;

	/** "12 / 36" */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ProgressText;

	/** A newly unlocked stage, or the lesson completed; appears when the bar reaches it */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UnlockText;

	/** Stage the next puzzle comes from, above the Next Puzzle button */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NextPuzzleText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> RestartButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> ReturnButton;

	/** Hidden when the board had mistakes (the same puzzle would come up again) and when the lesson was just completed */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> NextPuzzleButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson End Screen")
	FSlateColor SolvedColor = FSlateColor(FLinearColor(0.f, 0.5f, 0.f, 1.f));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson End Screen")
	FSlateColor IncorrectColor = FSlateColor(FLinearColor(1.f, 0.f, 0.f, 1.f));

	/** Seconds before the first line appears */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson End Screen")
	float StartDelay = 1.f;

	/** Seconds between lines, and before the bar fills */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson End Screen")
	float StepTime = 0.5f;

	/** Seconds for the bar to fill with the points earned */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson End Screen")
	float FillTime = 2.f;

	/** Played as each line appears */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson End Screen")
	TObjectPtr<USoundBase> StepSound;

	/** Played every frame while the bar fills */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson End Screen")
	TObjectPtr<USoundBase> FillSound;

	/** Played when a stage unlocks or the lesson completes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson End Screen")
	TObjectPtr<USoundBase> UnlockSound;

	/** Played when a button is pressed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson End Screen")
	TObjectPtr<USoundBase> ButtonSound;

	UPROPERTY(BlueprintAssignable, Category = "Lesson End Screen")
	FOnLessonEndScreenEvent OnRestartPuzzle;

	UPROPERTY(BlueprintAssignable, Category = "Lesson End Screen")
	FOnLessonEndScreenEvent OnReturnToLessons;

	UPROPERTY(BlueprintAssignable, Category = "Lesson End Screen")
	FOnLessonEndScreenEvent OnNextPuzzle;

	/** Record the finished lesson puzzle and show its result */
	UFUNCTION(BlueprintCallable, Category = "Lesson End Screen")
	void Show(UPuzzle* Puzzle, float PuzzleSeconds);

	UFUNCTION(BlueprintCallable, Category = "Lesson End Screen")
	void Hide();

	UFUNCTION(BlueprintPure, Category = "Lesson End Screen")
	FLessonPuzzleResult GetResult() const { return Result; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	enum class EAnimState : uint8
	{
		Idle,
		Revealing,	// showing the lines one at a time
		Filling,	// filling the progress bar
		Done,
	};

	UFUNCTION()
	void HandleRestartClicked();

	UFUNCTION()
	void HandleReturnClicked();

	UFUNCTION()
	void HandleNextClicked();

	FText GetNoteText() const;
	FText GetUnlockText() const;
	TArray<UWidget*> GetRevealedWidgets() const;
	void SetShownPoints(float Points);
	void ShowUnlock();
	void Finish();
	void SetButtonsEnabled(bool bEnabled);
	void PlaySound(USoundBase* Sound) const;

	/** A full-screen ad first (UAdsSubsystem), then Action */
	void PlayAdThen(TFunction<void()> Action);

	FLessonPuzzleResult Result;
	int32 NextStage = 0;
	EAnimState AnimState = EAnimState::Idle;
	float AnimTimeRemaining = 0.f;
	int32 RevealedCount = 0;
	float FillElapsed = 0.f;
	bool bUnlockShown = false;
};
