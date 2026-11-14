#include "UI/TutorialPanel.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "HappinessClassic/CampaignProgress.h"
#include "UI/CampaignTreeWidget.h"

#define LOCTEXT_NAMESPACE "Tutorial"

void UTutorialPanel::NativeConstruct()
{
	Super::NativeConstruct();

	Button->OnClicked.AddUniqueDynamic(this, &UTutorialPanel::HandleClicked);
	Refresh();
}

void UTutorialPanel::HandleClicked()
{
	// It scrolls with the tree: a press that panned the tree isn't a press of the button
	const UCampaignTreeWidget* Tree = GetTypedOuter<UCampaignTreeWidget>();
	if (Tree && Tree->IsDraggingTree())
	{
		return;
	}
	OnTutorialRequested.Broadcast();
}

void UTutorialPanel::Refresh()
{
	const bool bShow = UCampaignSubsystem::GetMode() == ECampaignMode::Lessons;

	Text->SetText(LOCTEXT("Tutorial", "Tutorial"));
	SetVisibility(bShow ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

#undef LOCTEXT_NAMESPACE
