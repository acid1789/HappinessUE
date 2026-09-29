#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "HappinessClassic/CampaignLesson.h"
#include "UI/LessonPopupWidget.h"
#include "CampaignTreeWidget.generated.h"

class UButton;
class UCanvasPanel;
class UCampaignTreeWidget;
class ULessonPopupWidget;

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

/** Routes one node button's click back to the tree with its lesson (UButton::OnClicked has no parameters) */
UCLASS()
class UCampaignTreeNodeClick : public UObject
{
	GENERATED_BODY()

public:
	ECampaignLesson Lesson = ECampaignLesson::Given;
	TWeakObjectPtr<UCampaignTreeWidget> Tree;

	UFUNCTION()
	void HandleClicked();
};

/**
 * Full-screen campaign lesson tree. Builds one button per lesson from UCampaignTree's columns inside
 * TreeCanvas, spread evenly across it left to right, and draws the connectors between columns.
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

private:
	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleProgressChanged();

	UFUNCTION()
	void HandlePopupStageChosen(ECampaignLesson Lesson, int32 Stage);

	void LoadProgress();

	void RefreshNodeStates();
	UButton* FindNode(ECampaignLesson Lesson) const;

	/** Full-size child of TreeCanvas, behind the nodes, that draws the connectors */
	UPROPERTY(Transient)
	TObjectPtr<UCampaignTreeLines> Lines;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> NodeButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCampaignTreeNodeClick>> NodeClicks;

	TArray<ECampaignLesson> NodeLessons;
	TSet<ECampaignLesson> CompletedLessons;
};
