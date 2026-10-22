#include "UI/ClueDescriptionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

UClueDescriptionWidget::UClueDescriptionWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	ReadingFont = Font.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> Panel(TEXT("/Game/Happiness/UI/Textures/T_HintReadingPanel.T_HintReadingPanel"));
	ReadingPanelTexture = Panel.Object;
	PanelBrush.SetResourceObject(ReadingPanelTexture);
	PanelBrush.ImageType = ESlateBrushImageType::FullColor;
	PanelBrush.ImageSize = FVector2D(256, 128);
	PanelBrush.DrawAs = ESlateBrushDrawType::Box;
	PanelBrush.Margin = FMargin(.07f, .14f);
}

void UClueDescriptionWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (UBorder* Frame = Cast<UBorder>(GetWidgetFromName(TEXT("ReadingFrame"))))
	{
		Frame->SetBrush(PanelBrush);
		Frame->SetPadding(FMargin(12.f, 0.f));
	}
	if (!GetGameInstance())
	{
		if (UHorizontalBox* Line = Cast<UHorizontalBox>(GetWidgetFromName(TEXT("HorizontalBox"))))
		{
			if (Line->GetChildrenCount() == 0)
			{
			FSlateBrush IconBrush;
			IconBrush.SetResourceObject(LoadObject<UTexture2D>(nullptr, TEXT("/Game/Happiness/IconSets/Icons/Toys/Bear.Bear")));
			IconBrush.ImageSize = FVector2D(30.f);
			UImage* FirstIcon = WidgetTree->ConstructWidget<UImage>();
			FirstIcon->SetBrush(IconBrush);
			Line->AddChildToHorizontalBox(FirstIcon);
				UTextBlock* Sample = WidgetTree->ConstructWidget<UTextBlock>();
			Sample->SetText(FText::FromString(TEXT(" is directly left of ")));
				Line->AddChildToHorizontalBox(Sample);
			UImage* SecondIcon = WidgetTree->ConstructWidget<UImage>();
			SecondIcon->SetBrush(IconBrush);
			Line->AddChildToHorizontalBox(SecondIcon);
			}
		}
	}
	StylePieces();
}

void UClueDescriptionWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	StylePieces();
}

void UClueDescriptionWidget::StylePieces()
{
	UHorizontalBox* Line = Cast<UHorizontalBox>(GetWidgetFromName(TEXT("HorizontalBox")));
	if (!Line || !ReadingFont) return;
	bool bHasContent = false;
	const TArray<UWidget*> Pieces = Line->GetAllChildren();
	for (int32 Index = 0; Index < Pieces.Num(); ++Index)
	{
		UWidget* Piece = Pieces[Index];
		UTextBlock* Text = Cast<UTextBlock>(Piece);
		UImage* Icon = Cast<UImage>(Piece);
		if (UUserWidget* Word = Cast<UUserWidget>(Piece))
		{
			Text = Cast<UTextBlock>(Word->GetWidgetFromName(TEXT("TextBlock_54")));
			Icon = Cast<UImage>(Word->GetWidgetFromName(TEXT("Image_28")));
		}
		if (UHorizontalBoxSlot* PieceSlot = Cast<UHorizontalBoxSlot>(Piece->Slot))
		{
			// Keep the existing 30x30 icon at its desired size, independent of text height.
			PieceSlot->SetVerticalAlignment(VAlign_Center);
			PieceSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			PieceSlot->SetPadding(Icon ? FMargin(Index > 0 ? 4.f : 0.f, 0.f,
				Index + 1 < Pieces.Num() ? 4.f : 0.f, 0.f) : FMargin(0.f));
		}
		if (Icon)
		{
			Icon->SetDesiredSizeOverride(FVector2D(30.f, 30.f));
			bHasContent |= Piece->IsVisible();
		}
		if (Text)
			bHasContent |= Piece->IsVisible() && !Text->GetText().ToString().TrimStartAndEnd().IsEmpty();
		if (!Text || (Text->GetFont().FontObject == ReadingFont && Text->GetFont().Size == 20)) continue;
		Text->SetFont(FSlateFontInfo(ReadingFont, 20));
		Text->SetColorAndOpacity(FLinearColor(.17f, .085f, .035f));
		Text->SetShadowOffset(FVector2D::ZeroVector);
	}
	if (UBorder* Frame = Cast<UBorder>(GetWidgetFromName(TEXT("ReadingFrame"))))
		Frame->SetVisibility(bHasContent ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}
