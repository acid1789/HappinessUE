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
	if (BackButton)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &UHappinessModeWidget::HandleBackClicked);
	}
}

void UHappinessModeWidget::Show()
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
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
	Hide();
	OnLessonsChosen.Broadcast();
}

void UHappinessModeWidget::HandleBackClicked()
{
	Hide();
	OnClosed.Broadcast();
}
