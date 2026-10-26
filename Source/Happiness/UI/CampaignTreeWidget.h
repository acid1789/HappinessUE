#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HappinessClassic/CampaignLesson.h"
#include "Styling/SlateTypes.h"
#include "UI/LessonPopupWidget.h"
#include "CampaignTreeWidget.generated.h"

class UButton;
class UBorder;
class UCanvasPanel;
class UCampaignTreeWidget;
class UCompleteLessonsPanel;
class ULessonPopupWidget;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UProgressBar;
class UTexture2D;
class UFont;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCampaignLessonSelected, ECampaignLesson, Lesson);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCampaignTreeClosed);

/**
 * Draws the campaign tree's connectors. Lives inside TreeCanvas behind the nodes, so anything layered above
 * the tree (the lesson popup) covers the lines too.
 */
UCLASS()
class UCampaignTreeLines : public UWidget
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<UCampaignTreeWidget> Tree;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
};

/** Opaque painted medallion and readable label, including the disabled state. */
UCLASS()
class HAPPINESS_API ULessonMedallion : public UWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> Texture;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> LockTexture;
	FSlateFontInfo LabelFont;
	ECampaignLesson Lesson = ECampaignLesson::VerticalTwo;
	bool bLocked = false;
	bool bCompleted = false;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
};

/** Bright completion seal placed on the lesson card itself, separate from its clue diagram. */
UCLASS()
class HAPPINESS_API ULessonCompletionBadge : public UWidget
{
    GENERATED_BODY()
public:
    bool bCompleted = false;
    /** While it's celebrated: where a band of light sweeping across it is (0 to 1), or below 0 for none */
    float Shimmer = -1.f;
    /** While it's celebrated: how strongly it glows (0 to 1) */
    float Glow = 0.f;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
};

/** Abstract, code-drawn clue diagram: its geometry reflects the lesson, independent of puzzle artwork. */
UCLASS()
class HAPPINESS_API ULessonClueDiagram : public UWidget
{
	GENERATED_BODY()
public:
	ECampaignLesson Lesson = ECampaignLesson::VerticalTwo;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
};

/** Routes one node button's click back to the tree with its lesson (UButton::OnClicked has no parameters) */
UCLASS()
class UCampaignTreeNodeClick : public UObject
{
	GENERATED_BODY()

public:
	ECampaignLesson Lesson = ECampaignLesson::Given;
	TWeakObjectPtr<UCampaignTreeWidget> Tree;
	TWeakObjectPtr<UButton> Button;

	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleHovered();

	UFUNCTION()
	void HandleUnhovered();
};

/**
 * Full-screen campaign lesson tree. Builds one button per lesson from UCampaignTree's columns inside
 * a horizontally draggable frosted viewport, with connectors between required lesson groups.
 * A lesson is unlocked once every lesson in the columns before it is complete.
 *
 * TreeCanvas belongs to this widget: its children are rebuilt, so put other UI (title, back button)
 * outside it. Style everything through the Campaign Tree properties in the Blueprint's defaults.
 */
UCLASS(Abstract)
class HAPPINESS_API UCampaignTreeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCampaignTreeWidget(const FObjectInitializer& ObjectInitializer);

	/** Canvas the lesson nodes are placed in; the tree spans its full area */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCanvasPanel> TreeCanvas;

	/** Optional. Hides this screen and fires OnClosed. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackButton;

	/** Optional. Opened for the clicked lesson; place it on top of the tree, collapsed. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<ULessonPopupWidget> LessonPopup;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OverallStatus;

	/** Optional. WBP_CompleteLessons, shown in Lessons mode until every lesson is completed; it asks with ConfirmDialog */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCompleteLessonsPanel> CompleteLessons;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> OverallProgress;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign Tree")
	TObjectPtr<UTexture2D> MedallionTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign Tree")
	TObjectPtr<UTexture2D> GroupBadgeTexture;

	UPROPERTY()
	TObjectPtr<UTexture2D> StatusPlaqueTexture;

	UPROPERTY()
	TObjectPtr<UTexture2D> ViewportFrameTexture;

	UPROPERTY()
	TObjectPtr<UFont> LessonSerifFont;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign Tree")
	FVector2D NodeSize = FVector2D(96.f, 100.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign Tree")
	FButtonStyle NodeStyle;

	/** Font for lesson names. Leave the font object empty to use the default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign Tree")
	FSlateFontInfo NodeFont;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign Tree")
	FSlateColor NodeTextColor = FSlateColor(FLinearColor(0.02f, 0.02f, 0.05f, 1.f));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign Tree")
	FLinearColor LockedTint = FLinearColor(0.3f, 0.3f, 0.3f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign Tree")
	FLinearColor AvailableTint = FLinearColor(1.f, 1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign Tree")
	FLinearColor CompletedTint = FLinearColor(0.45f, 0.85f, 0.45f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign Tree")
	FLinearColor LineColor = FLinearColor(0.41f, 0.f, 0.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign Tree")
	float LineThickness = 6.f;

	/**
	 * Lesson completion, played when the tree reopens after a lesson's final: the lesson's check pops in, starting
	 * small, growing past its size and settling, with a shimmer across it. Then if its group is complete, the
	 * group's glow moves on to the next group while the tree scrolls to center it, and that group unlocks.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson Completion")
	float CheckDelay = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson Completion")
	float CheckDuration = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson Completion")
	float CheckStartScale = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson Completion")
	float CheckPeakScale = 1.4f;

	/** Seconds between the check settling and the glow moving on */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson Completion")
	float GlowMoveDelay = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson Completion")
	float GlowMoveDuration = 1.f;

	/** After the last lesson's completion: seconds the tree stays before it closes, back to the mode screen, which
	 *  celebrates every lesson being completed (UHappinessModeWidget) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson Completion")
	float CloseAfterLastLessonDelay = 0.6f;

	/** Played as the check starts to pop in */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lesson Completion")
	TObjectPtr<USoundBase> CheckSound;

	/** The game rules screen (WBP_GameRules, a UGameRulesWidget) the lesson popup's Learn More opens over the tree */
	UPROPERTY(EditAnywhere, Category = "Campaign Tree")
	TSoftClassPtr<UUserWidget> RulesClass;

	/** Fired when the player picks an unlocked lesson */
	UPROPERTY(BlueprintAssignable, Category = "Campaign Tree")
	FOnCampaignLessonSelected OnLessonSelected;

	/** Relayed from LessonPopup when the player picks a stage to play (the session is already started) */
	UPROPERTY(BlueprintAssignable, Category = "Campaign Tree")
	FOnLessonStageChosen OnLessonStageChosen;

	/** Show this screen, with the lesson popup open for the current lesson session if there is one */
	UFUNCTION(BlueprintCallable, Category = "Campaign Tree")
	void ShowForLessonSession();

	/** Fired after the screen hides itself via BackButton or Close() */
	UPROPERTY(BlueprintAssignable, Category = "Campaign Tree")
	FOnCampaignTreeClosed OnClosed;

	/** Hide this screen (collapsed) and fire OnClosed */
	UFUNCTION(BlueprintCallable, Category = "Campaign Tree")
	void Close();

	virtual void SetVisibility(ESlateVisibility InVisibility) override;

	/** Replace the set of completed lessons (e.g. from the save game) and refresh the nodes */
	UFUNCTION(BlueprintCallable, Category = "Campaign Tree")
	void SetCompletedLessons(const TArray<ECampaignLesson>& Lessons);

	UFUNCTION(BlueprintCallable, Category = "Campaign Tree")
	void SetLessonCompleted(ECampaignLesson Lesson, bool bCompleted = true);

	UFUNCTION(BlueprintPure, Category = "Campaign Tree")
	bool IsLessonCompleted(ECampaignLesson Lesson) const;

	/** True once every lesson in the earlier columns is complete */
	UFUNCTION(BlueprintPure, Category = "Campaign Tree")
	bool IsLessonUnlocked(ECampaignLesson Lesson) const;

	/** Recreate the nodes, e.g. after changing NodeSize or NodeStyle at runtime */
	UFUNCTION(BlueprintCallable, Category = "Campaign Tree")
	void RebuildTree();

	void NotifyNodeClicked(ECampaignLesson Lesson);

	/** Draws the connectors between columns into Geometry's space (called by the lines widget behind the nodes) */
	int32 PaintConnectors(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry,
												  const FPointerEvent& InMouseEvent) override;

private:
	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleProgressChanged();

	UFUNCTION()
	void HandlePopupStageChosen(ECampaignLesson Lesson, int32 Stage);

	/** Learn More: the rules, open at the lesson's clue */
	UFUNCTION()
	void HandleLearnMore(ECampaignLesson Lesson);

	void LoadProgress();

	/** Plays the lesson completion for Lesson (already completed in the progress) */
	void StartCelebration(ECampaignLesson Lesson);
	void UpdateCelebration(float DeltaTime, float ViewportWidth, float ScrollEnd);
	void EndCelebration();
	bool IsCelebrating() const { return CelebrationTime >= 0.f; }
	/** Completed, as shown: the celebrated lesson only once its check appears */
	bool IsLessonShownCompleted(ECampaignLesson Lesson) const;
	/** The group shown as current (glowing, unlocked last, centered): the celebrated one until the glow moves on */
	int32 GetShownColumn() const;

	void RefreshNodeStates();
	void RefreshOverallStatus();
	int32 GetCurrentColumn() const;
	UButton* FindNode(ECampaignLesson Lesson) const;

	/** Full-size child of TreeCanvas, behind the nodes, that draws the connectors */
	UPROPERTY(Transient)
	TObjectPtr<UCampaignTreeLines> Lines;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> NodeButtons;

	/** Made on the first Learn More */
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> RulesWidget;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCampaignTreeNodeClick>> NodeClicks;

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> TreeScroll;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> ScrollContent;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> NodeCanvas;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULessonMedallion>> Medallions;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULessonCompletionBadge>> CompletionBadges;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> ColumnStatusLabels;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> ColumnStatusPanels;

	/** The lesson completion playing: seconds into it (below 0: none) */
	float CelebrationTime = -1.f;
	ECampaignLesson CelebrationLesson = ECampaignLesson::Given;
	int32 CelebrationFromColumn = 0;
	int32 CelebrationToColumn = 0;
	bool bCelebrationCheckShown = false;
	bool bCelebrationMoving = false;
	float CelebrationScrollFrom = 0.f;
	float CelebrationScrollTo = 0.f;
	/** How far the glow has moved from CelebrationFromColumn to CelebrationToColumn (0 to 1, eased) */
	float GlowMove = 0.f;
	/** Every lesson is completed: once the celebration ends, seconds until the tree closes (below 0: it stays) */
	float CloseCountdown = -1.f;
	bool bCloseAfterCelebration = false;

	float LastViewportWidth = 0.f;
	int32 LastCompletedColumns = INDEX_NONE;
	bool bCenterCurrentColumn = true;
	bool bPanCandidate = false;
	bool bDraggingTree = false;
	/** The pointer that pressed (a finger, or the mouse cursor): touches don't move the cursor */
	int32 PanUserIndex = 0;
	uint32 PanPointerIndex = 0;
	FVector2D PanStart;
	float PanStartOffset = 0.f;

	TArray<ECampaignLesson> NodeLessons;
	TSet<ECampaignLesson> CompletedLessons;
};
