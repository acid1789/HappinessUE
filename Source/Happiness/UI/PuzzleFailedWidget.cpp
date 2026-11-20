#include "UI/PuzzleFailedWidget.h"

#include "UI/ConfirmDialogWidget.h"
#include "HappinessClassic/CampaignProgress.h"
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
	const FLinearColor PuzzleFailedInk(.10f, .05f, .02f);
	// Undo / Fix Puzzle and Restart, both the width "Fix Puzzle" needs clear of the button's end vines; the message
	// between them fits "Impossible Puzzle State"
	constexpr float ButtonWidth = 190.f;
	constexpr float MessageWidth = 400.f;
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

void UPuzzleFailedWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (!GetGameInstance())
	{
		SetVisibility(ESlateVisibility::Visible);
	}
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
	BannerSize->SetWidthOverride(2.f * ButtonWidth + MessageWidth + 34.f);	// 17 padding each side
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
	UTextBlock* Caption = nullptr;
	UndoButton = MakeButton(LOCTEXT("Fix", "Fix Puzzle"), false, &Caption);
	UndoCaption = Caption;
	RestartButton = MakeButton(LOCTEXT("Restart", "Restart"), true);

	USizeBox* UndoSize = WidgetTree->ConstructWidget<USizeBox>();
	UndoSize->SetWidthOverride(ButtonWidth);
	UndoSize->SetHeightOverride(42.f);
	UndoSize->SetContent(UndoButton);
	Row->AddChildToHorizontalBox(UndoSize)->SetVerticalAlignment(VAlign_Center);

	UTextBlock* Message = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Message"));
	MessageText = Message;
	Message->SetText(LOCTEXT("Message", "Some Icons Are Wrong"));
	Message->SetFont(FSlateFontInfo(GardenFont, 25));
	Message->SetColorAndOpacity(PuzzleFailedInk);
	Message->SetJustification(ETextJustify::Center);
	Message->SetShadowOffset(FVector2D::ZeroVector);
	USizeBox* MessageSize = WidgetTree->ConstructWidget<USizeBox>();
	MessageSize->SetWidthOverride(MessageWidth);
	MessageSize->SetContent(Message);
	Row->AddChildToHorizontalBox(MessageSize)->SetVerticalAlignment(VAlign_Center);

	USizeBox* RestartSize = WidgetTree->ConstructWidget<USizeBox>();
	RestartSize->SetWidthOverride(ButtonWidth);
	RestartSize->SetHeightOverride(42.f);
	RestartSize->SetContent(RestartButton);
	Row->AddChildToHorizontalBox(RestartSize)->SetVerticalAlignment(VAlign_Center);

	UndoButton->OnClicked.AddUniqueDynamic(this, &UPuzzleFailedWidget::HandleUndo);
	RestartButton->OnClicked.AddUniqueDynamic(this, &UPuzzleFailedWidget::HandleRestart);
}

UButton* UPuzzleFailedWidget::MakeButton(const FText& Label, bool bRestart, UTextBlock** OutCaption)
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
	USizeBox* GlyphSizeBox = WidgetTree->ConstructWidget<USizeBox>();
	GlyphSizeBox->SetWidthOverride(21.f);
	GlyphSizeBox->SetHeightOverride(21.f);
	if (bRestart)
	{
		ULessonResultGlyph* Icon = WidgetTree->ConstructWidget<ULessonResultGlyph>();
		Icon->Symbol = 0;
		Icon->Color = PuzzleFailedInk;
		Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		GlyphSizeBox->SetContent(Icon);
	}
	else
	{
		UConfirmDialogGlyph* Icon = WidgetTree->ConstructWidget<UConfirmDialogGlyph>();
		Icon->Symbol = 0;
		Icon->Color = PuzzleFailedInk;
		Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		GlyphSizeBox->SetContent(Icon);
	}
	Content->AddChildToHorizontalBox(GlyphSizeBox)->SetVerticalAlignment(VAlign_Center);
	UTextBlock* Caption = WidgetTree->ConstructWidget<UTextBlock>();
	Caption->SetText(Label);
	if (OutCaption)
	{
		*OutCaption = Caption;
	}
	Caption->SetFont(FSlateFontInfo(GardenFont, 20));
	Caption->SetColorAndOpacity(PuzzleFailedInk);
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
	SetImpossible(false);
	SetVisibility(ESlateVisibility::Visible);
}

void UPuzzleFailedWidget::ShowImpossible()
{
	SetImpossible(true);
	SetVisibility(ESlateVisibility::Visible);
}

void UPuzzleFailedWidget::SetImpossible(bool bInImpossible)
{
	bImpossible = bInImpossible;
	if (MessageText)
	{
		MessageText->SetText(bImpossible ? LOCTEXT("Impossible", "Impossible Puzzle State") : LOCTEXT("Message", "Some Icons Are Wrong"));
	}
	// Fix Puzzle, except for mistakes on a Master Mode stage, which has no fixing (UPauseMenuWidget::Show): Undo
	bFix = bImpossible || UCampaignSubsystem::AreAssistsAllowed(this);
	if (UndoCaption)
	{
		UndoCaption->SetText(bFix ? LOCTEXT("Fix", "Fix Puzzle") : LOCTEXT("Undo", "Undo"));
	}
}

void UPuzzleFailedWidget::RunFix()
{
	// The puzzle screen this banner is on
	UUserWidget* Screen = GetTypedOuter<UUserWidget>();
	while (Screen && !Screen->FindFunction(TEXT("ConfirmConfirm")))
	{
		Screen = Screen->GetTypedOuter<UUserWidget>();
	}
	UFunction* Confirm = Screen ? Screen->FindFunction(TEXT("ConfirmConfirm")) : nullptr;
	const FProperty* Type = Screen ? Screen->GetClass()->FindPropertyByName(TEXT("ConfirmType")) : nullptr;
	if (!Confirm || !Type)
	{
		return;
	}
	void* Value = Type->ContainerPtrToValuePtr<void>(Screen);
	const int64 Fix = static_cast<int64>(EHappinessConfirmType::FixPuzzle);
	if (const FEnumProperty* EnumType = CastField<FEnumProperty>(Type))
	{
		EnumType->GetUnderlyingProperty()->SetIntPropertyValue(Value, Fix);
	}
	else if (const FByteProperty* ByteType = CastField<FByteProperty>(Type))
	{
		ByteType->SetIntPropertyValue(Value, Fix);
	}
	else
	{
		return;
	}
	Screen->ProcessEvent(Confirm, nullptr);
}

void UPuzzleFailedWidget::Hide()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UPuzzleFailedWidget::HandleUndo()
{
	UGameplayStatics::PlaySound2D(this, ClickSound);
	Hide();
	if (bFix)
	{
		RunFix();
		return;
	}
	OnUndo.Broadcast();
}

void UPuzzleFailedWidget::HandleRestart()
{
	UGameplayStatics::PlaySound2D(this, ClickSound);
	Hide();
	OnRestart.Broadcast();
}

#undef LOCTEXT_NAMESPACE
