#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "HappinessClassic/CampaignLesson.h"
#include "LessonPopupWidget.generated.h"

class UButton;
class UPanelWidget;
class UProgressBar;
class UTextBlock;
class UBorder;
class UImage;
class ULessonProgressTicks;
class ULessonPopupWidget;
class UCampaignSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLessonStageChosen, ECampaignLesson, Lesson, int32, Stage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLessonPopupClosed);

/** Routes one stage button's click to the popup (UButton::OnClicked has no parameters) */
UCLASS()
class ULessonStageClick : public UObject
{
	GENERATED_BODY()

public:
	int32 Stage = 0;
	TWeakObjectPtr<ULessonPopupWidget> Popup;

	UFUNCTION()
	void HandleClicked();
};

/**
 * Popup for one campaign lesson: its name and explanation, a progress bar toward the final, and one button
 * per stage (3x3 easy, 3x3 normal, 4x4 easy, 4x4 normal, final). Unlocked stages can always be replayed.
 * Play starts the stage that currently earns points (or the final once it is unlocked).
 *
 * Choosing a stage calls UCampaignSubsystem::RequestLessonPuzzle and fires OnStageChosen.
 */
UCLASS(Abstract)
class HAPPINESS_API ULessonPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	ULessonPopupWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UProgressBar> LessonProgress;

	/** e.g. "12 / 36 points - 3x3 Normal" */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ProgressText;

	/** Stage buttons are created in here, left to right */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPanelWidget> StageBox;

	/** Master Mode's stage buttons (Campaign), shown once the clue's final is completed */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> MasterBox;

	/** Tick marks over LessonProgress; switched to the Master Mode track once that's shown */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<ULessonProgressTicks> ProgressTicks;

	// Master Mode reveal: the first time the player opens a cleared clue, the bar drains, an eraser wipes the
	// progress line from right to left, the Master line appears, then the Master buttons and bar. EraseCover and
	// Eraser sit over ProgressText (in an Overlay, aligned right, filling its height); EraseCover is painted in
	// Panel's color so the text disappears as it grows.

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Panel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> EraseCover;

	/** The eraser; a plain rectangle until it gets art */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Eraser;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson Popup|Master Reveal")
	float RevealDrainTime = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson Popup|Master Reveal")
	float RevealEraseTime = 3.f;

	/** Seconds the Master line shows before the Master buttons and bar come in */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson Popup|Master Reveal")
	float RevealTextHold = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson Popup|Master Reveal")
	float EraserWidth = 28.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson Popup|Master Reveal")
	FLinearColor EraserColor = FLinearColor(0.85f, 0.45f, 0.5f, 1.f);

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> PlayButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	/** Optional label of CloseButton; reads "Return to Lessons" once the lesson is complete (Play is hidden then) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CloseLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson Popup")
	FVector2D StageButtonSize = FVector2D(150.f, 60.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson Popup")
	FButtonStyle StageButtonStyle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson Popup")
	FSlateFontInfo StageFont;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson Popup")
	FSlateColor StageTextColor = FSlateColor(FLinearColor(0.02f, 0.02f, 0.05f, 1.f));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson Popup")
	FLinearColor LockedTint = FLinearColor(0.3f, 0.3f, 0.3f, 1.f);

	/** Unlocked stage that no longer earns points (replay) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson Popup")
	FLinearColor PassedTint = FLinearColor(0.45f, 0.85f, 0.45f, 1.f);

	/** The stage that currently earns points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson Popup")
	FLinearColor CurrentTint = FLinearColor(1.f, 0.85f, 0.3f, 1.f);

	/** Fired when the player picks a stage (Play or a stage button), after the subsystem request */
	UPROPERTY(BlueprintAssignable, Category = "Lesson Popup")
	FOnLessonStageChosen OnStageChosen;

	UPROPERTY(BlueprintAssignable, Category = "Lesson Popup")
	FOnLessonPopupClosed OnClosed;

	/** Fill the popup for Lesson and show it */
	UFUNCTION(BlueprintCallable, Category = "Lesson Popup")
	void ShowLesson(ECampaignLesson Lesson);

	/** Show the lesson again after playing one of its puzzles. Unlike ShowLesson (the player opening it), this
	 *  doesn't reveal a newly unlocked Master Mode. */
	UFUNCTION(BlueprintCallable, Category = "Lesson Popup")
	void ShowLessonAfterPuzzle(ECampaignLesson Lesson);

	/** Re-read progress for the shown lesson */
	UFUNCTION(BlueprintCallable, Category = "Lesson Popup")
	void Refresh();

	UFUNCTION(BlueprintCallable, Category = "Lesson Popup")
	void Close();

	UFUNCTION(BlueprintCallable, Category = "Lesson Popup")
	void ChooseStage(int32 Stage);

	UFUNCTION(BlueprintPure, Category = "Lesson Popup")
	ECampaignLesson GetLesson() const { return Lesson; }

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION()
	void HandlePlayClicked();

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleProgressChanged();

	UCampaignSubsystem* GetCampaign() const;
	void BuildStageButtons();
	UButton* AddStageButton(UPanelWidget* Box, int32 Stage, const FText& Name, float Width, float Spacing);

	/** Master Mode is open and revealed for the shown clue: the bar, text and Play button follow it */
	bool IsShowingMaster() const;

	FText GetMasterProgressText() const;

	enum class ERevealPhase : uint8
	{
		None,
		Drain,	// the completed campaign bar empties
		Erase,	// the eraser wipes the progress line, right to left
		Text,	// the Master line shows, before the Master buttons and bar
	};
	ERevealPhase RevealPhase = ERevealPhase::None;
	float RevealElapsed = 0.f;

	void TickReveal(float DeltaTime);
	/** 0: nothing erased, 1: the whole line covered */
	void SetEraseProgress(float Alpha);
	void HideEraser();

	ECampaignLesson Lesson = ECampaignLesson::VerticalTwo;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> StageButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> MasterButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULessonStageClick>> StageClicks;
};
