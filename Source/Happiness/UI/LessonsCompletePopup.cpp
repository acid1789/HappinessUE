#include "UI/LessonsCompletePopup.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "UI/ConfirmDialogWidget.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "LessonsComplete"

namespace
{
	// The confirm dialog's plaque colors
	const FLinearColor PopupInk(.17f, .085f, .035f);
	const FLinearColor PopupIvory(1.f, .95f, .8f);
	const FLinearColor PopupSage(.16f, .29f, .075f);
	const FLinearColor PopupGold(.55f, .36f, .08f);
	constexpr float CardArtSize = 118.f;
}

ULessonsCompletePopup::ULessonsCompletePopup(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> Plaque(TEXT("/Game/General/Textures/T_ConfirmGardenPlaque.T_ConfirmGardenPlaque"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Button(TEXT("/Game/General/Textures/T_PauseQuitButton.T_PauseQuitButton"));
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Bee(TEXT("/Game/Happiness/IconSets/Icons/GardenVisitors/T_Bee.T_Bee"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Ladybug(TEXT("/Game/Happiness/IconSets/Icons/GardenVisitors/T_Ladybug.T_Ladybug"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Butterfly(TEXT("/Game/Happiness/IconSets/Icons/GardenVisitors/T_Butterfly.T_Butterfly"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Snail(TEXT("/Game/Happiness/IconSets/Icons/GardenVisitors/T_Snail.T_Snail"));
	PlaqueTexture = Plaque.Object;
	ButtonTexture = Button.Object;
	PlaqueFont = Font.Object;
	for (UTexture2D* Visitor : { Bee.Object, Ladybug.Object, Butterfly.Object, Snail.Object })
	{
		if (Visitor)
		{
			VisitorTextures.Add(Visitor);
		}
	}
}

TSharedRef<SWidget> ULessonsCompletePopup::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void ULessonsCompletePopup::BuildLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PopupCanvas"));
	WidgetTree->RootWidget = Root;

	// The screen behind blurs and dims, and takes no taps
	UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>();
	Blur->SetBlurStrength(4.f);
	UCanvasPanelSlot* BlurSlot = Root->AddChildToCanvas(Blur);
	BlurSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BlurSlot->SetOffsets(FMargin(0.f));
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>();
	Dim->SetBrush(FSlateRoundedBoxBrush(FLinearColor::White, 0.f));
	Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, .32f));
	UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim);
	DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	DimSlot->SetOffsets(FMargin(0.f));

	UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>();
	Fit->SetStretch(EStretch::ScaleToFit);
	Fit->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UCanvasPanelSlot* FitSlot = Root->AddChildToCanvas(Fit);
	FitSlot->SetAnchors(FAnchors(.1f, .07f, .9f, .93f));
	FitSlot->SetOffsets(FMargin(0.f));
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(1400.f);
	Size->SetMinDesiredHeight(900.f);
	Fit->AddChild(Size);
	Window = Size;
	Window->SetRenderTransformPivot(FVector2D(.5f));

	UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>();
	Size->AddChild(Layers);
	UImage* Surface = WidgetTree->ConstructWidget<UImage>();
	Surface->SetBrushFromTexture(PlaqueTexture);
	UOverlaySlot* SurfaceSlot = Layers->AddChildToOverlay(Surface);
	SurfaceSlot->SetHorizontalAlignment(HAlign_Fill);
	SurfaceSlot->SetVerticalAlignment(VAlign_Fill);

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
	UOverlaySlot* ContentSlot = Layers->AddChildToOverlay(Content);
	ContentSlot->SetHorizontalAlignment(HAlign_Fill);
	ContentSlot->SetVerticalAlignment(VAlign_Center);
	ContentSlot->SetPadding(FMargin(120.f, 105.f, 120.f, 60.f));

	auto MakeText = [this](const FText& Text, int32 Points, const FLinearColor& Color, float WrapAt = 0.f)
	{
		UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>();
		Block->SetText(Text);
		Block->SetFont(FSlateFontInfo(PlaqueFont, Points));
		Block->SetColorAndOpacity(Color);
		Block->SetJustification(ETextJustify::Center);
		if (WrapAt > 0.f)
		{
			// A fixed width: auto wrap in a centered slot wraps to its own last width, which can stay too narrow
			Block->SetWrapTextAt(WrapAt);
		}
		return Block;
	};

	Content->AddChildToVerticalBox(MakeText(LOCTEXT("Title", "Lessons Complete!"), 58, PopupInk))->SetHorizontalAlignment(HAlign_Center);

	// The dialog's leaf divider
	USizeBox* DividerSize = WidgetTree->ConstructWidget<USizeBox>();
	DividerSize->SetWidthOverride(600.f);
	DividerSize->SetHeightOverride(55.f);
	UCanvasPanel* Divider = WidgetTree->ConstructWidget<UCanvasPanel>();
	DividerSize->AddChild(Divider);
	for (float X : { 0.f, 360.f })
	{
		UImage* Line = WidgetTree->ConstructWidget<UImage>();
		Line->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.44f, .28f, .10f, .45f), 0.f));
		UCanvasPanelSlot* LineSlot = Divider->AddChildToCanvas(Line);
		LineSlot->SetPosition(FVector2D(X, 27.f));
		LineSlot->SetSize(FVector2D(240.f, 1.f));
	}
	UConfirmDialogGlyph* Leaf = WidgetTree->ConstructWidget<UConfirmDialogGlyph>();
	Leaf->Symbol = 2;
	Leaf->Color = PopupSage;
	UCanvasPanelSlot* LeafSlot = Divider->AddChildToCanvas(Leaf);
	LeafSlot->SetPosition(FVector2D(270.f, 0.f));
	LeafSlot->SetSize(FVector2D(60.f, 55.f));
	Content->AddChildToVerticalBox(DividerSize)->SetHorizontalAlignment(HAlign_Center);

	Content->AddChildToVerticalBox(MakeText(LOCTEXT("Message", "You've learned every clue. Two new things are yours:"),
		30, PopupInk, 1140.f))->SetHorizontalAlignment(HAlign_Center);

	// One card per unlock: a picture, its name and what it is
	UHorizontalBox* Cards = WidgetTree->ConstructWidget<UHorizontalBox>();
	auto AddCard = [&](UWidget* Art, const FText& Name, const FText& Line, bool bFirst)
	{
		UBorder* Card = WidgetTree->ConstructWidget<UBorder>();
		Card->SetBrush(FSlateRoundedBoxBrush(FLinearColor(1.f, .97f, .86f, .65f), 26.f, FLinearColor(.78f, .6f, .25f, .9f), 2.f));
		Card->SetPadding(FMargin(24.f, 16.f));
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Card->SetContent(Column);
		Column->AddChildToVerticalBox(MakeText(LOCTEXT("Unlocked", "Unlocked"), 22, PopupGold))->SetHorizontalAlignment(HAlign_Center);
		USizeBox* ArtSize = WidgetTree->ConstructWidget<USizeBox>();
		ArtSize->SetHeightOverride(CardArtSize);
		ArtSize->AddChild(Art);
		UVerticalBoxSlot* ArtSlot = Column->AddChildToVerticalBox(ArtSize);
		ArtSlot->SetHorizontalAlignment(HAlign_Center);
		ArtSlot->SetPadding(FMargin(0.f, 8.f));
		Column->AddChildToVerticalBox(MakeText(Name, 40, PopupSage))->SetHorizontalAlignment(HAlign_Center);
		Column->AddChildToVerticalBox(MakeText(Line, 23, PopupInk, 452.f))->SetHorizontalAlignment(HAlign_Center);
		USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>();
		CardSize->SetWidthOverride(500.f);
		CardSize->SetMinDesiredHeight(300.f);
		CardSize->AddChild(Card);
		Cards->AddChildToHorizontalBox(CardSize)->SetPadding(FMargin(bFirst ? 0.f : 44.f, 0.f, 0.f, 0.f));
	};

	UHorizontalBox* Visitors = WidgetTree->ConstructWidget<UHorizontalBox>();
	for (int32 Index = 0; Index < VisitorTextures.Num(); Index++)
	{
		UImage* Visitor = WidgetTree->ConstructWidget<UImage>();
		FSlateBrush VisitorBrush;
		VisitorBrush.SetResourceObject(VisitorTextures[Index]);
		VisitorBrush.ImageSize = FVector2D(96.f);
		Visitor->SetBrush(VisitorBrush);
		UHorizontalBoxSlot* VisitorSlot = Visitors->AddChildToHorizontalBox(Visitor);
		VisitorSlot->SetVerticalAlignment(VAlign_Center);
		VisitorSlot->SetPadding(FMargin(Index == 0 ? 0.f : 6.f, 0.f, 0.f, 0.f));
	}
	AddCard(Visitors, LOCTEXT("GardenVisitors", "Garden Visitors"), LOCTEXT("GardenVisitorsLine", "A new icon set for your puzzles"), true);

	CampaignImage = WidgetTree->ConstructWidget<UImage>();
	AddCard(CampaignImage, LOCTEXT("Campaign", "Campaign"), LOCTEXT("CampaignLine", "Every clue again, on bigger boards"), false);
	UVerticalBoxSlot* CardsSlot = Content->AddChildToVerticalBox(Cards);
	CardsSlot->SetHorizontalAlignment(HAlign_Center);
	CardsSlot->SetPadding(FMargin(0.f, 26.f, 0.f, 34.f));

	// Continue, in the dialog's Confirm style
	ContinueButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ContinueButton"));
	FSlateBrush Face;
	Face.DrawAs = ESlateBrushDrawType::Image;
	Face.SetResourceObject(ButtonTexture);
	Face.SetUVRegion(FBox2f(FVector2f(.024f, .19f), FVector2f(.976f, .782f)));
	FButtonStyle Style;
	Style.Normal = Style.Hovered = Style.Pressed = Style.Disabled = Face;
	Style.Hovered.TintColor = FLinearColor(1.08f, 1.08f, 1.04f);
	Style.Pressed.TintColor = FLinearColor(.83f, .88f, .78f);
	Style.NormalPadding = Style.PressedPadding = FMargin(35.f, 8.f);
	ContinueButton->SetStyle(Style);
	ContinueButton->SetCursor(EMouseCursor::Hand);
	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	USizeBox* CheckSize = WidgetTree->ConstructWidget<USizeBox>();
	CheckSize->SetWidthOverride(64.f);
	CheckSize->SetHeightOverride(64.f);
	UConfirmDialogGlyph* Check = WidgetTree->ConstructWidget<UConfirmDialogGlyph>();
	Check->Symbol = 1;
	Check->Color = PopupIvory;
	CheckSize->AddChild(Check);
	UHorizontalBoxSlot* CheckSlot = ButtonRow->AddChildToHorizontalBox(CheckSize);
	CheckSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
	CheckSlot->SetVerticalAlignment(VAlign_Center);
	ButtonRow->AddChildToHorizontalBox(MakeText(LOCTEXT("Continue", "Continue"), 36, PopupIvory))->SetVerticalAlignment(VAlign_Center);
	UButtonSlot* RowSlot = CastChecked<UButtonSlot>(ContinueButton->AddChild(ButtonRow));
	RowSlot->SetHorizontalAlignment(HAlign_Center);
	RowSlot->SetVerticalAlignment(VAlign_Center);
	USizeBox* ButtonSize = WidgetTree->ConstructWidget<USizeBox>();
	ButtonSize->SetWidthOverride(470.f);
	ButtonSize->SetHeightOverride(130.f);
	ButtonSize->AddChild(ContinueButton);
	Content->AddChildToVerticalBox(ButtonSize)->SetHorizontalAlignment(HAlign_Center);
	ContinueButton->OnClicked.AddUniqueDynamic(this, &ULessonsCompletePopup::HandleContinueClicked);

	// Only the button takes taps: the rest just blocks the screen behind
	for (UWidget* Widget : { static_cast<UWidget*>(Surface), static_cast<UWidget*>(Content), static_cast<UWidget*>(Cards) })
	{
		Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void ULessonsCompletePopup::Show(const FSlateBrush* CampaignArt)
{
	if (CampaignImage)
	{
		if (CampaignArt)
		{
			FSlateBrush Art = *CampaignArt;
			Art.ImageSize = FVector2D(CardArtSize);
			CampaignImage->SetBrush(Art);
		}
		CampaignImage->SetVisibility(CampaignArt ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	SetVisibility(ESlateVisibility::Visible);
	FadeTime = 0.f;
	SetRenderOpacity(0.f);
	if (Window)
	{
		Window->SetRenderScale(FVector2D(.9f));
	}
}

void ULessonsCompletePopup::Hide()
{
	FadeTime = -1.f;
	SetVisibility(ESlateVisibility::Collapsed);
}

void ULessonsCompletePopup::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (FadeTime < 0.f)
	{
		return;
	}
	FadeTime += InDeltaTime;
	const float T = FMath::Clamp(FadeTime / FMath::Max(FadeInTime, .01f), 0.f, 1.f);
	SetRenderOpacity(T);
	if (Window)
	{
		// It grows into place, just past its size and back
		const float Ease = 1.f + 2.7f * FMath::Pow(T - 1.f, 3.f) + 1.7f * FMath::Pow(T - 1.f, 2.f);
		Window->SetRenderScale(FVector2D(FMath::Lerp(.9f, 1.f, Ease)));
	}
	if (T >= 1.f)
	{
		FadeTime = -1.f;
	}
}

void ULessonsCompletePopup::HandleContinueClicked()
{
	Hide();
}

#undef LOCTEXT_NAMESPACE
