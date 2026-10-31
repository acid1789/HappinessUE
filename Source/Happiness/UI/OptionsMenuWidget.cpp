#include "UI/OptionsMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateNoResource.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/BackgroundBlur.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "OptionsGarden"

namespace
{
	const FLinearColor OptionsInk(.17f, .085f, .035f);
	const FLinearColor OptionsGold(.65f, .36f, .07f);
	const FLinearColor OptionsIvory(1.f, .95f, .8f);

	FSlateBrush OptionsImage(UTexture2D* Texture)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.SetResourceObject(Texture);
		return Brush;
	}
}

UOptionsMenuWidget::UOptionsMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> Panel(TEXT("/Game/General/Textures/T_OptionsGarden.T_OptionsGarden"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Buttons(TEXT("/Game/General/Textures/T_LessonNotebookButtons.T_LessonNotebookButtons"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Empty(TEXT("/Game/General/Textures/T_OptionsUnchecked.T_OptionsUnchecked"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Check(TEXT("/Game/General/Textures/T_OptionsChecked.T_OptionsChecked"));
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	PanelTexture = Panel.Object;
	ButtonsTexture = Buttons.Object;
	UncheckedTexture = Empty.Object;
	CheckedTexture = Check.Object;
	OptionsFont = Font.Object;
}

void UOptionsMenuWidget::BuildLayout()
{
	if (SettingsRows || !WidgetTree) return;
	EffectsSlider = Cast<USlider>(GetWidgetFromName(TEXT("SoundEffectVolume")));
	MusicSlider = Cast<USlider>(GetWidgetFromName(TEXT("MusicVolume")));
	SleepToggle = Cast<UUserWidget>(GetWidgetFromName(TEXT("CB_PreventSleepOnIdle")));
	HintToggle = Cast<UUserWidget>(GetWidgetFromName(TEXT("CB_HintAnimation")));
	CancelAction = Cast<UButton>(GetWidgetFromName(TEXT("Button_Cancel")));
	ApplyAction = Cast<UButton>(GetWidgetFromName(TEXT("Button_Confirm")));
	ExtraSettingsHost = Cast<UVerticalBox>(GetWidgetFromName(TEXT("ExtraSettings")));
	if (!EffectsSlider || !MusicSlider) return;
	UCanvasPanel* Root = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Root) return;
	// Keep the original controls, including their Blueprint delegates and settings bindings.
	for (UWidget* Control : {static_cast<UWidget*>(EffectsSlider), static_cast<UWidget*>(MusicSlider),
		static_cast<UWidget*>(SleepToggle), static_cast<UWidget*>(HintToggle),
		static_cast<UWidget*>(CancelAction), static_cast<UWidget*>(ApplyAction), static_cast<UWidget*>(ExtraSettingsHost)})
		if (Control) Control->RemoveFromParent();
	Root->ClearChildren();

	auto Place = [](UCanvasPanel* Canvas, UWidget* Child, FVector2D Position, FVector2D Extent, int32 Z = 1)
	{
		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Child);
		CanvasSlot->SetPosition(Position); CanvasSlot->SetSize(Extent); CanvasSlot->SetZOrder(Z);
		return CanvasSlot;
	};
	auto Stretch = [&](UWidget* Child, int32 Z)
	{
		UCanvasPanelSlot* CanvasSlot = Place(Root, Child, {}, {}, Z);
		CanvasSlot->SetAnchors(FAnchors(0, 0, 1, 1)); CanvasSlot->SetOffsets(FMargin(0));
	};
	UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>();
	Blur->SetBlurStrength(7); Blur->SetVisibility(ESlateVisibility::HitTestInvisible); Stretch(Blur, 0);
	UImage* Dim = WidgetTree->ConstructWidget<UImage>();
	Dim->SetBrush(FSlateRoundedBoxBrush(FLinearColor::White, 0.f)); Dim->SetColorAndOpacity(FLinearColor(0, 0, 0, .3f));
	Stretch(Dim, 1);
	UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>(); Fit->SetStretch(EStretch::ScaleToFit);
	Fit->SetStretchDirection(EStretchDirection::DownOnly);
	UCanvasPanelSlot* FitSlot = Place(Root, Fit, {}, {}, 2);
	FitSlot->SetAnchors(FAnchors(0, 0, 1, 1)); FitSlot->SetOffsets(FMargin(40));
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(); Size->SetWidthOverride(1400); Size->SetHeightOverride(700); Fit->AddChild(Size);
	UCanvasPanel* Panel = WidgetTree->ConstructWidget<UCanvasPanel>(); Size->AddChild(Panel);

	auto Image = [&](UCanvasPanel* Canvas, FSlateBrush Brush, FVector2D Position, FVector2D Extent, int32 Z = 1)
	{
		UImage* Art = WidgetTree->ConstructWidget<UImage>(); Art->SetBrush(Brush);
		Art->SetVisibility(ESlateVisibility::HitTestInvisible); Place(Canvas, Art, Position, Extent, Z); return Art;
	};
	auto Text = [&](const FText& Caption, int32 Points, FLinearColor Color)
	{
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(); Label->SetText(Caption);
		Label->SetFont(FSlateFontInfo(OptionsFont, Points)); Label->SetColorAndOpacity(Color);
		Label->SetVisibility(ESlateVisibility::HitTestInvisible); return Label;
	};
	auto Face = [&](bool Primary)
	{
		FSlateBrush Brush = OptionsImage(ButtonsTexture);
		Brush.SetUVRegion(FBox2f(FVector2f(.03f, Primary ? .525f : .06f), FVector2f(.98f, Primary ? .963f : .485f)));
		return Brush;
	};
	Image(Panel, OptionsImage(PanelTexture), {0, 0}, {1400, 700}, 0);
	UTextBlock* Title = Text(LOCTEXT("Options", "Options"), 48, OptionsInk);
	Title->SetJustification(ETextJustify::Center); Place(Panel, Title, {450, 18}, {500, 72}, 2);

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("SettingsScroll"));
	Scroll->SetOrientation(Orient_Vertical); Scroll->SetScrollbarThickness({10, 10});
	Scroll->SetAlwaysShowScrollbar(false); Scroll->SetAllowOverscroll(false);
	Scroll->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
	FScrollBarStyle ScrollStyle = Scroll->GetWidgetBarStyle();
	ScrollStyle.NormalThumbImage = FSlateRoundedBoxBrush(OptionsGold, 5.f);
	ScrollStyle.HoveredThumbImage = FSlateRoundedBoxBrush(FLinearColor(.85f, .52f, .12f), 5.f);
	ScrollStyle.DraggedThumbImage = ScrollStyle.HoveredThumbImage;
	Scroll->SetWidgetBarStyle(ScrollStyle);
	Place(Panel, Scroll, {70, 128}, {1260, 420}, 3);
	SettingsRows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsRows"));
	Scroll->AddChild(SettingsRows);

	auto Leaves = [&](UCanvasPanel* Canvas)
	{
		FSlateBrush Left = OptionsImage(ButtonsTexture);
		Left.SetUVRegion(FBox2f(FVector2f(.043f, .125f), FVector2f(.125f, .42f)));
		Image(Canvas, Left, {15, 16}, {38, 62}, 2);
		FSlateBrush Right = OptionsImage(ButtonsTexture);
		Right.SetUVRegion(FBox2f(FVector2f(.89f, .125f), FVector2f(.972f, .42f)));
		Image(Canvas, Right, {1190, 16}, {38, 62}, 2);
	};
	auto Row = [&]()
	{
		USizeBox* RowSize = WidgetTree->ConstructWidget<USizeBox>(); RowSize->SetHeightOverride(94);
		UVerticalBoxSlot* RowSlot = SettingsRows->AddChildToVerticalBox(RowSize); RowSlot->SetPadding(FMargin(0, 0, 0, 8));
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(); RowSize->AddChild(Canvas);
		FSlateBrush Surface = FSlateRoundedBoxBrush(FLinearColor(1.f, .94f, .79f, .44f), 14.f, FLinearColor(.72f, .45f, .16f, .72f), 1.4f);
		UImage* Background = Image(Canvas, Surface, {}, {1244, 94}, 0);
		CastChecked<UCanvasPanelSlot>(Background->Slot)->SetAnchors(FAnchors(0, 0, 1, 1));
		CastChecked<UCanvasPanelSlot>(Background->Slot)->SetOffsets(FMargin(0));
		Leaves(Canvas); return Canvas;
	};
	auto Audio = [&](USlider* Slider, const FText& Caption)
	{
		UCanvasPanel* Canvas = Row();
		Place(Canvas, Text(Caption, 32, OptionsInk), {75, 23}, {460, 55}, 2);
		UProgressBar* Fill = WidgetTree->ConstructWidget<UProgressBar>();
		FProgressBarStyle FillStyle;
		FillStyle.BackgroundImage = FSlateRoundedBoxBrush(FLinearColor(.91f, .82f, .62f), 12.f, OptionsGold, 2.f);
		FillStyle.FillImage = FSlateRoundedBoxBrush(FLinearColor(.19f, .36f, .08f), 10.f);
		Fill->SetWidgetStyle(FillStyle); Fill->SetFillColorAndOpacity(FLinearColor::White);
		Fill->SetBorderPadding({3, 3}); Fill->SetBarFillStyle(EProgressBarFillStyle::Scale);
		Fill->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Canvas, Fill, {549, 35}, {597, 24}, 2);
		FSliderStyle SliderStyle;
		SliderStyle.NormalBarImage = SliderStyle.HoveredBarImage = SliderStyle.DisabledBarImage = FSlateNoResource();
		SliderStyle.NormalThumbImage = FSlateRoundedBoxBrush(OptionsIvory, 24.f, OptionsGold, 3.f);
		SliderStyle.NormalThumbImage.ImageSize = FVector2f(48, 48);
		SliderStyle.HoveredThumbImage = SliderStyle.NormalThumbImage;
		SliderStyle.HoveredThumbImage.TintColor = FLinearColor(1.07f, 1.04f, 1.f);
		SliderStyle.DisabledThumbImage = SliderStyle.NormalThumbImage; SliderStyle.BarThickness = 24;
		Slider->SetWidgetStyle(SliderStyle); Slider->SetSliderHandleColor(FLinearColor::White);
		Slider->SetIndentHandle(true); Slider->SetCursor(EMouseCursor::Hand);
		Place(Canvas, Slider, {525, 7}, {645, 80}, 3);
		return Fill;
	};
	EffectsFill = Audio(EffectsSlider, LOCTEXT("Effects", "Sound Effect Volume"));
	MusicFill = Audio(MusicSlider, LOCTEXT("Music", "Music Volume"));

	auto Checkbox = [&](UUserWidget* Control, const FText& Caption)
	{
		if (!Control) return;
		UCanvasPanel* Canvas = Row();
		UButton* Button = Cast<UButton>(Control->GetWidgetFromName(TEXT("Button_53")));
		UImage* Empty = Cast<UImage>(Control->GetWidgetFromName(TEXT("CheckBox")));
		UImage* Check = Cast<UImage>(Control->GetWidgetFromName(TEXT("CheckMark")));
		if (Button && Empty && Check)
		{
			Empty->RemoveFromParent(); Check->RemoveFromParent(); Button->ClearChildren();
			FButtonStyle Style;
			Style.Normal = Style.Pressed = Style.Disabled = FSlateNoResource();
			Style.Hovered = FSlateRoundedBoxBrush(FLinearColor(1.f, .91f, .58f, .2f), 14.f);
			Style.NormalPadding = Style.PressedPadding = FMargin(0); Button->SetStyle(Style);
			Button->SetCursor(EMouseCursor::Hand);
			UCanvasPanel* Content = WidgetTree->ConstructWidget<UCanvasPanel>();
			UButtonSlot* ContentSlot = CastChecked<UButtonSlot>(Button->AddChild(Content));
			ContentSlot->SetHorizontalAlignment(HAlign_Fill); ContentSlot->SetVerticalAlignment(VAlign_Fill);
			Place(Content, Text(Caption, 32, OptionsInk), {75, 23}, {930, 55}, 1);
			Empty->SetBrush(OptionsImage(UncheckedTexture)); Empty->SetColorAndOpacity(FLinearColor::White);
			Check->SetBrush(OptionsImage(CheckedTexture)); Check->SetColorAndOpacity(FLinearColor::White);
			Place(Content, Empty, {1116, 17}, {60, 60}, 2); Place(Content, Check, {1116, 17}, {60, 60}, 3);
		}
		UCanvasPanelSlot* ControlSlot = Place(Canvas, Control, {}, {}, 3);
		ControlSlot->SetAnchors(FAnchors(0, 0, 1, 1)); ControlSlot->SetOffsets(FMargin(0));
	};
#if PLATFORM_ANDROID || PLATFORM_IOS
	Checkbox(SleepToggle, LOCTEXT("Sleep", "Prevent Sleep on Idle"));
#endif
	Checkbox(HintToggle, LOCTEXT("HintAnimation", "Animate Hint Button"));
	if (ExtraSettingsHost)
	{
		// Future WBP_CheckBox children receive the same row styling and scrolling automatically.
		for (UWidget* Child : ExtraSettingsHost->GetAllChildren())
		{
			UUserWidget* Control = Cast<UUserWidget>(Child);
			if (!Control || !Control->GetWidgetFromName(TEXT("Button_53"))) continue;
			if (FTextProperty* Caption = FindFProperty<FTextProperty>(Control->GetClass(), TEXT("Text")))
				Checkbox(Control, Caption->GetPropertyValue_InContainer(Control));
		}
		SettingsRows->AddChildToVerticalBox(ExtraSettingsHost);
	}

	auto Action = [&](UButton* Button, const FText& Caption, bool Primary, float X)
	{
		if (!Button) return;
		Button->ClearChildren(); FButtonStyle Style;
		Style.Normal = Style.Hovered = Style.Pressed = Style.Disabled = Face(Primary);
		Style.Hovered.TintColor = FLinearColor(1.06f, 1.06f, 1.02f);
		Style.Pressed.TintColor = FLinearColor(.82f, .86f, .76f);
		Style.NormalPadding = Style.PressedPadding = FMargin(48, 8);
		Button->SetStyle(Style); Button->SetBackgroundColor(FLinearColor::White); Button->SetCursor(EMouseCursor::Hand);
		UButtonSlot* ContentSlot = CastChecked<UButtonSlot>(Button->AddChild(Text(Caption, 37, Primary ? OptionsIvory : OptionsInk)));
		ContentSlot->SetHorizontalAlignment(HAlign_Center); ContentSlot->SetVerticalAlignment(VAlign_Center);
		Place(Panel, Button, {X, 573}, {400, 90}, 4);
	};
	Action(CancelAction, LOCTEXT("Cancel", "Cancel"), false, 285);
	Action(ApplyAction, LOCTEXT("Apply", "Apply"), true, 715);
}

void UOptionsMenuWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	BuildLayout();
	if (EffectsFill) EffectsFill->SetPercent(EffectsSlider->GetValue());
	if (MusicFill) MusicFill->SetPercent(MusicSlider->GetValue());
#if WITH_EDITOR
	if (GetWorld() && !GetWorld()->IsGameWorld()) SetVisibility(ESlateVisibility::Visible);
#endif
}

void UOptionsMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// Show sets the original sliders programmatically, without broadcasting OnValueChanged.
	if (EffectsFill && EffectsFill->GetPercent() != EffectsSlider->GetValue()) EffectsFill->SetPercent(EffectsSlider->GetValue());
	if (MusicFill && MusicFill->GetPercent() != MusicSlider->GetValue()) MusicFill->SetPercent(MusicSlider->GetValue());
}

#undef LOCTEXT_NAMESPACE
