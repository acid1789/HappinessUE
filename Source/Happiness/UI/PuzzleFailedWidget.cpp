#include "UI/PuzzleFailedWidget.h"

#include "UI/ConfirmDialogWidget.h"
#include "UI/LessonEndScreenWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Styling/SlateBrush.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "PuzzleFailed"

namespace
{
	const FLinearColor FailedInk(.10f, .05f, .02f);
}

UPuzzleFailedWidget::UPuzzleFailedWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> Frame(TEXT("/Game/Happiness/UI/Textures/T_HintReadingPanel.T_HintReadingPanel"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Buttons(TEXT("/Game/General/Textures/T_LessonNotebookButtons.T_LessonNotebookButtons"));
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Accept(TEXT("/Game/General/Audio/Cues/MenuAccept.MenuAccept"));
	FrameTexture = Frame.Object;
	ButtonTexture = Buttons.Object;
	GardenFont = Font.Object;
	ClickSound = Accept.Object;
	SetVisibility(ESlateVisibility::Collapsed);
}

TSharedRef<SWidget> UPuzzleFailedWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UPuzzleFailedWidget::BuildLayout()
{
	// The clear root blocks puzzle input. The banner alone covers the grid's upper border.
	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ClickBlocker"));
	Root->SetBrushColor(FLinearColor::Transparent);
	// GamePanel spans x=98 to viewport-right-200, so its center is 51 units left of the viewport center.
	Root->SetPadding(FMargin(0.f, 0.f, 51.f, 0.f));
	Root->SetHorizontalAlignment(HAlign_Center);
	Root->SetVerticalAlignment(VAlign_Top);
	WidgetTree->RootWidget = Root;

	USizeBox* BannerSize = WidgetTree->ConstructWidget<USizeBox>();
	BannerSize->SetWidthOverride(748.f);
	BannerSize->SetHeightOverride(50.f);
	Root->SetContent(BannerSize);

	UBorder* Banner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Banner"));
	FSlateBrush FrameBrush;
	FrameBrush.SetResourceObject(FrameTexture);
	FrameBrush.DrawAs = ESlateBrushDrawType::Box;
	FrameBrush.Margin = FMargin(.07f, .14f);
	FrameBrush.TintColor = FSlateColor(FLinearColor(1.f, 1.f, 1.f, .75f));
	Banner->SetBrush(FrameBrush);
	Banner->SetPadding(FMargin(17.f, 4.f));
	BannerSize->SetContent(Banner);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	Banner->SetContent(Row);
	UndoButton = MakeButton(LOCTEXT("Undo", "Undo"), false);
	RestartButton = MakeButton(LOCTEXT("Restart", "Restart"), true);

	USizeBox* UndoSize = WidgetTree->ConstructWidget<USizeBox>();
	UndoSize->SetWidthOverride(139.f);
	UndoSize->SetHeightOverride(42.f);
	UndoSize->SetContent(UndoButton);
	Row->AddChildToHorizontalBox(UndoSize)->SetVerticalAlignment(VAlign_Center);

	UTextBlock* Message = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Message"));
	Message->SetText(LOCTEXT("Message", "Some Icons Are Wrong"));
	Message->SetFont(FSlateFontInfo(GardenFont, 25));
	Message->SetColorAndOpacity(FailedInk);
	Message->SetJustification(ETextJustify::Center);
	Message->SetShadowOffset(FVector2D::ZeroVector);
	USizeBox* MessageSize = WidgetTree->ConstructWidget<USizeBox>();
	MessageSize->SetWidthOverride(436.f);
	MessageSize->SetContent(Message);
	Row->AddChildToHorizontalBox(MessageSize)->SetVerticalAlignment(VAlign_Center);

	USizeBox* RestartSize = WidgetTree->ConstructWidget<USizeBox>();
	RestartSize->SetWidthOverride(139.f);
	RestartSize->SetHeightOverride(42.f);
	RestartSize->SetContent(RestartButton);
	Row->AddChildToHorizontalBox(RestartSize)->SetVerticalAlignment(VAlign_Center);

	UndoButton->OnClicked.AddUniqueDynamic(this, &UPuzzleFailedWidget::HandleUndo);
	RestartButton->OnClicked.AddUniqueDynamic(this, &UPuzzleFailedWidget::HandleRestart);
}

UButton* UPuzzleFailedWidget::MakeButton(const FText& Label, bool bRestart)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	FSlateBrush Face;
	Face.SetResourceObject(ButtonTexture);
	Face.DrawAs = ESlateBrushDrawType::Image;
	Face.SetUVRegion(FBox2f(FVector2f(.03f, .06f), FVector2f(.98f, .485f)));
	FButtonStyle Style;
	Style.Normal = Style.Hovered = Style.Pressed = Style.Disabled = Face;
	Style.Hovered.TintColor = FLinearColor(1.08f, 1.08f, 1.04f);
	Style.Pressed.TintColor = FLinearColor(.83f, .88f, .78f);
	Style.NormalPadding = Style.PressedPadding = FMargin(0.f);
	Button->SetStyle(Style);
	Button->SetCursor(EMouseCursor::Hand);

	UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>();
	USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>();
	IconSize->SetWidthOverride(21.f);
	IconSize->SetHeightOverride(21.f);
	if (bRestart)
	{
		ULessonResultGlyph* Icon = WidgetTree->ConstructWidget<ULessonResultGlyph>();
		Icon->Symbol = 0;
		Icon->Color = FailedInk;
		Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		IconSize->SetContent(Icon);
	}
	else
	{
		UConfirmDialogGlyph* Icon = WidgetTree->ConstructWidget<UConfirmDialogGlyph>();
		Icon->Symbol = 0;
		Icon->Color = FailedInk;
		Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		IconSize->SetContent(Icon);
	}
	Content->AddChildToHorizontalBox(IconSize)->SetVerticalAlignment(VAlign_Center);
	UTextBlock* Caption = WidgetTree->ConstructWidget<UTextBlock>();
	Caption->SetText(Label);
	Caption->SetFont(FSlateFontInfo(GardenFont, 20));
	Caption->SetColorAndOpacity(FailedInk);
	Caption->SetShadowOffset(FVector2D::ZeroVector);
	UHorizontalBoxSlot* CaptionSlot = Content->AddChildToHorizontalBox(Caption);
	CaptionSlot->SetPadding(FMargin(5.f, 0.f, 0.f, 0.f));
	CaptionSlot->SetVerticalAlignment(VAlign_Center);
	UButtonSlot* ButtonSlot = CastChecked<UButtonSlot>(Button->AddChild(Content));
	ButtonSlot->SetHorizontalAlignment(HAlign_Center);
	ButtonSlot->SetVerticalAlignment(VAlign_Center);
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
