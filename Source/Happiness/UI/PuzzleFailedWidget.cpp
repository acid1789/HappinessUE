#include "UI/PuzzleFailedWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Styling/SlateBrush.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "PuzzleFailed"

UPuzzleFailedWidget::UPuzzleFailedWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Accept(TEXT("/Game/General/Audio/Cues/MenuAccept.MenuAccept"));
	MessageFont = FSlateFontInfo(Font.Object, 30);
	ButtonFont = FSlateFontInfo(Font.Object, 26);
	ClickSound = Accept.Object;

	auto Fill = [](const FLinearColor& Color)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Color);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(14.f, 14.f, 14.f, 14.f),
			FSlateColor(FLinearColor(1.f, 0.85f, 0.45f, 0.9f)), 2.f);
		return Brush;
	};
	ButtonStyle.SetNormal(Fill(FLinearColor(0.12f, 0.1f, 0.08f, 0.75f)))
		.SetHovered(Fill(FLinearColor(0.25f, 0.2f, 0.12f, 0.85f)))
		.SetPressed(Fill(FLinearColor(0.4f, 0.32f, 0.16f, 0.9f)));
	ButtonStyle.SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f));

	SetVisibility(ESlateVisibility::Collapsed);
}

TSharedRef<SWidget> UPuzzleFailedWidget::RebuildWidget()
{
	// No Blueprint layout: the widgets are made here, also for the designer preview
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UPuzzleFailedWidget::BuildLayout()
{
	// Full size and clear, but Visible: it takes the clicks meant for the board
	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ClickBlocker"));
	Root->SetBrushColor(FLinearColor::Transparent);
	Root->SetPadding(PanelMargin);
	Root->SetHorizontalAlignment(HAlign_Center);
	Root->SetVerticalAlignment(VAlign_Bottom);
	WidgetTree->RootWidget = Root;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	FSlateBrush PanelBrush;
	PanelBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	PanelBrush.TintColor = FSlateColor(PanelColor);
	PanelBrush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(18.f, 18.f, 18.f, 18.f));
	Panel->SetBrush(PanelBrush);
	Panel->SetPadding(FMargin(28.f, 14.f, 28.f, 18.f));
	Root->SetContent(Panel);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Column);

	UTextBlock* Message = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Message"));
	Message->SetText(LOCTEXT("Message", "Some icons are wrong"));
	Message->SetFont(MessageFont);
	Message->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.95f, 0.85f)));
	Message->SetShadowOffset(FVector2D(1.f, 1.f));
	Message->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* MessageSlot = Column->AddChildToVerticalBox(Message))
	{
		MessageSlot->SetHorizontalAlignment(HAlign_Center);
		MessageSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	}

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	if (UVerticalBoxSlot* RowSlot = Column->AddChildToVerticalBox(Row))
	{
		RowSlot->SetHorizontalAlignment(HAlign_Center);
	}
	UndoButton = MakeButton(LOCTEXT("Undo", "Undo"));
	RestartButton = MakeButton(LOCTEXT("Restart", "Restart"));
	for (UButton* Button : { UndoButton.Get(), RestartButton.Get() })
	{
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(ButtonSize.X);
		Size->SetHeightOverride(ButtonSize.Y);
		Size->SetContent(Button);
		if (UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(Size))
		{
			ButtonSlot->SetPadding(FMargin(12.f, 0.f));
		}
	}
	UndoButton->OnClicked.AddUniqueDynamic(this, &UPuzzleFailedWidget::HandleUndo);
	RestartButton->OnClicked.AddUniqueDynamic(this, &UPuzzleFailedWidget::HandleRestart);
}

UButton* UPuzzleFailedWidget::MakeButton(const FText& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	Button->SetStyle(ButtonStyle);
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(Label);
	Text->SetFont(ButtonFont);
	Text->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.92f, 0.75f)));
	Text->SetShadowOffset(FVector2D(1.f, 1.f));
	Button->SetContent(Text);
	return Button;
}

void UPuzzleFailedWidget::Show()
{
	SetVisibility(ESlateVisibility::Visible);
}

void UPuzzleFailedWidget::Hide()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UPuzzleFailedWidget::HandleUndo()
{
	UGameplayStatics::PlaySound2D(this, ClickSound);
	Hide();
	OnUndo.Broadcast();
}

void UPuzzleFailedWidget::HandleRestart()
{
	UGameplayStatics::PlaySound2D(this, ClickSound);
	Hide();
	OnRestart.Broadcast();
}

#undef LOCTEXT_NAMESPACE
