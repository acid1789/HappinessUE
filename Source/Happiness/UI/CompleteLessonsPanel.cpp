#include "UI/CompleteLessonsPanel.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "HappinessClassic/CampaignProgress.h"
#include "UI/ConfirmDialogWidget.h"

#define LOCTEXT_NAMESPACE "CompleteLessons"

void UCompleteLessonsPanel::NativeConstruct()
{
	Super::NativeConstruct();

	Button->OnClicked.AddUniqueDynamic(this, &UCompleteLessonsPanel::HandleClicked);
	if (UCampaignSubsystem* Campaign = UCampaignSubsystem::Get(this))
	{
		Campaign->OnProgressChanged.AddUniqueDynamic(this, &UCompleteLessonsPanel::Refresh);
	}
	Refresh();
}

void UCompleteLessonsPanel::NativeDestruct()
{
	if (UCampaignSubsystem* Campaign = UCampaignSubsystem::Get(this))
	{
		Campaign->OnProgressChanged.RemoveDynamic(this, &UCompleteLessonsPanel::Refresh);
	}
	Super::NativeDestruct();
}

void UCompleteLessonsPanel::HandleClicked()
{
	// The confirm dialog of the screen this is placed in (WBP_CampaignTree's ConfirmDialog)
	UConfirmDialogWidget* Dialog = nullptr;
	for (const UUserWidget* Owner = GetTypedOuter<UUserWidget>(); Owner && !Dialog; Owner = Owner->GetTypedOuter<UUserWidget>())
	{
		Dialog = Owner->WidgetTree ? Cast<UConfirmDialogWidget>(Owner->WidgetTree->FindWidget(TEXT("ConfirmDialog"))) : nullptr;
	}
	if (!Dialog)
	{
		return;
	}
	Dialog->Ask(LOCTEXT("Question", "Complete all lessons?"),
		LOCTEXT("Description", "Every lesson is marked complete.").ToString(),
		[WeakThis = TWeakObjectPtr<UCompleteLessonsPanel>(this)]()
		{
			if (UCampaignSubsystem* Campaign = WeakThis.IsValid() ? UCampaignSubsystem::Get(WeakThis.Get()) : nullptr)
			{
				Campaign->CompleteAllLessons();
			}
		});
}

void UCompleteLessonsPanel::Refresh()
{
	const UCampaignSubsystem* Campaign = UCampaignSubsystem::Get(this);
	const bool bShow = Campaign && UCampaignSubsystem::GetMode() == ECampaignMode::Lessons && !Campaign->IsCampaignModeUnlocked();

	Text->SetText(LOCTEXT("Complete", "Complete All Lessons"));
	SetVisibility(bShow ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

#undef LOCTEXT_NAMESPACE
