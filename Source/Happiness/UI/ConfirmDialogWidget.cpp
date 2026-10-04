#include "UI/ConfirmDialogWidget.h"

#include "Components/TextBlock.h"

void UConfirmDialogWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetDescription(FString());
}

void UConfirmDialogWidget::SetDescription(FString Description)
{
	if (DescriptionText)
	{
		DescriptionText->SetText(FText::FromString(Description));
		DescriptionText->SetVisibility(Description.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UConfirmDialogWidget::SetVisibility(ESlateVisibility InVisibility)
{
	Super::SetVisibility(InVisibility);
	// Closed: the next dialog starts without a description
	if (InVisibility == ESlateVisibility::Hidden || InVisibility == ESlateVisibility::Collapsed)
	{
		SetDescription(FString());
	}
}
