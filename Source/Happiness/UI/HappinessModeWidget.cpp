#include "UI/HappinessModeWidget.h"

#include "Components/Button.h"

void UHappinessModeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ClassicButton)
	{
		ClassicButton->OnClicked.AddUniqueDynamic(this, &UHappinessModeWidget::HandleClassicClicked);
	}
	if (LessonsButton)
	{
		LessonsButton->OnClicked.AddUniqueDynamic(this, &UHappinessModeWidget::HandleLessonsClicked);
	}
	if (CampaignButton)
	{
		CampaignButton->OnClicked.AddUniqueDynamic(this, &UHappinessModeWidget::HandleCampaignClicked);
	}
	RefreshCampaignLock();
	if (BackButton)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &UHappinessModeWidget::HandleBackClicked);
	}
}

void UHappinessModeWidget::Show()
{
	RefreshCampaignLock();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UHappinessModeWidget::RefreshCampaignLock()
{
	if (CampaignButton)
	{
		const UCampaignSubsystem* Campaign = UCampaignSubsystem::Get(this);
		CampaignButton->SetIsEnabled(Campaign && Campaign->IsCampaignModeUnlocked());
	}
}

void UHappinessModeWidget::Hide()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UHappinessModeWidget::HandleClassicClicked()
{
	Hide();
	OnClassicChosen.Broadcast();
}

void UHappinessModeWidget::HandleLessonsClicked()
{
	OpenTree(ECampaignMode::Lessons);
}

void UHappinessModeWidget::HandleCampaignClicked()
{
	OpenTree(ECampaignMode::Campaign);
}

void UHappinessModeWidget::OpenTree(ECampaignMode Mode)
{
	if (UCampaignSubsystem* Campaign = UCampaignSubsystem::Get(this))
	{
		Campaign->SetMode(Mode);
	}
	Hide();
	OnLessonsChosen.Broadcast();
}

void UHappinessModeWidget::HandleBackClicked()
{
	Hide();
	OnClosed.Broadcast();
}
