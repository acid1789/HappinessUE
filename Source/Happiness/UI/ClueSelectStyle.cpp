#include "UI/ClueSelectWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateNoResource.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "ClueSelectGarden"

namespace
{
	const FLinearColor ClueSelectInk(.17f, .085f, .035f);
	const FLinearColor ClueSelectIvory(1.f, .95f, .8f);

	FSlateBrush ClueSelectImage(UTexture2D* Texture)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		return Brush;
	}

	UCanvasPanelSlot* ClueSelectPlace(UCanvasPanel* Canvas, UWidget* Widget, FVector2D Position, FVector2D Extent, int32 Z)
	{
		Widget->RemoveFromParent();
		UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
		Slot->SetPosition(Position); Slot->SetSize(Extent); Slot->SetZOrder(Z);
		return Slot;
	}
}

void UClueSelectWidget::BuildGardenPanel()
{
	UCanvasPanel* Popup = Cast<UCanvasPanel>(Panel);
	if (bGardenPanelBuilt || !Popup || !ClueGrid) return;
	bGardenPanelBuilt = true;
	Columns = 3;
	for (UWidget* Child : Popup->GetAllChildren())
		if (Child != ClueGrid && Child != AllButton && Child != NoneButton && Child != DoneButton)
			Child->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* PopupSlot = Cast<UCanvasPanelSlot>(Popup->Slot))
	{
		PopupSlot->SetAnchors(FAnchors(0, 0, 1, 1)); PopupSlot->SetAlignment({0, 0});
		PopupSlot->SetOffsets(FMargin(0)); PopupSlot->SetZOrder(50);
	}
	UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>(); Fit->SetStretch(EStretch::ScaleToFit);
	UCanvasPanelSlot* FitSlot = ClueSelectPlace(Popup, Fit, {}, {}, 1);
	FitSlot->SetAnchors(FAnchors(0, 0, 1, 1)); FitSlot->SetOffsets(FMargin(0));
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(); Size->SetWidthOverride(1280); Size->SetHeightOverride(850);
	Fit->AddChild(Size);
	UCanvasPanel* Surface = WidgetTree->ConstructWidget<UCanvasPanel>(); Size->AddChild(Surface);
	UImage* Art = WidgetTree->ConstructWidget<UImage>(); Art->SetBrush(ClueSelectImage(ClueGardenTexture));
	// The full panel catches hits so setup controls behind its unused areas cannot be pressed.
	Art->SetVisibility(ESlateVisibility::Visible); ClueSelectPlace(Surface, Art, {}, {1280, 850}, 0);
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>();
	Title->SetText(LOCTEXT("Title", "Clue Types")); Title->SetFont(FSlateFontInfo(ClueGardenFont, 48));
	Title->SetColorAndOpacity(ClueSelectInk); Title->SetJustification(ETextJustify::Center);
	Title->SetVisibility(ESlateVisibility::HitTestInvisible);
	ClueSelectPlace(Surface, Title, {390, 24}, {500, 100}, 2);
	ClueSelectPlace(Surface, ClueGrid, {72, 150}, {1136, 480}, 5);

	auto Action = [&](UButton* Button, const FText& Caption, bool Primary, float X)
	{
		if (!Button) return;
		Button->ClearChildren();
		FSlateBrush Face = ClueSelectImage(ClueActionTexture);
		Face.SetUVRegion(FBox2f(FVector2f(.03f, Primary ? .525f : .06f), FVector2f(.98f, Primary ? .963f : .485f)));
		FButtonStyle Style; Style.Normal = Style.Hovered = Style.Pressed = Style.Disabled = Face;
		Style.Hovered.TintColor = FLinearColor(1.06f, 1.06f, 1.02f); Style.Pressed.TintColor = FLinearColor(.82f, .86f, .76f);
		Style.NormalPadding = Style.PressedPadding = FMargin(24, 6);
		Button->SetStyle(Style); Button->SetBackgroundColor(FLinearColor::White); Button->SetCursor(EMouseCursor::Hand);
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
		Label->SetText(Caption); Label->SetFont(FSlateFontInfo(ClueGardenFont, 30));
		Label->SetColorAndOpacity(Primary ? ClueSelectIvory : ClueSelectInk);
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		UButtonSlot* LabelSlot = CastChecked<UButtonSlot>(Button->AddChild(Label));
		LabelSlot->SetHorizontalAlignment(HAlign_Center); LabelSlot->SetVerticalAlignment(VAlign_Center);
		ClueSelectPlace(Surface, Button, {X, 680}, {350, 112}, 10);
	};
	Action(AllButton, LOCTEXT("All", "Select All"), false, 75);
	Action(NoneButton, LOCTEXT("None", "Clear All"), false, 465);
	Action(DoneButton, LOCTEXT("Done", "Done"), true, 855);
}

void UClueSelectWidget::StyleCheckBox(UUserWidget* CheckBox, float Width, float Height)
{
	UButton* Button = Cast<UButton>(CheckBox->GetWidgetFromName(TEXT("Button_53")));
	UImage* Empty = Cast<UImage>(CheckBox->GetWidgetFromName(TEXT("CheckBox")));
	UImage* Check = Cast<UImage>(CheckBox->GetWidgetFromName(TEXT("CheckMark")));
	UTextBlock* Label = Cast<UTextBlock>(CheckBox->GetWidgetFromName(TEXT("TextBlock_67")));
	if (!Button || !Empty || !Check || !Label) return;
	// Preserve these very same images/button so WBP_CheckBox's Check function and click delegates still work.
	Empty->RemoveFromParent(); Check->RemoveFromParent(); Label->RemoveFromParent(); Button->RemoveFromParent();
	Button->ClearChildren();
	USizeBox* RowSize = CheckBox->WidgetTree->ConstructWidget<USizeBox>();
	RowSize->SetWidthOverride(Width); RowSize->SetHeightOverride(Height); RowSize->AddChild(Button);
	CheckBox->WidgetTree->RootWidget = RowSize;
	FButtonStyle Style;
	Style.Normal = FSlateRoundedBoxBrush(FLinearColor(1.f, .94f, .79f, .42f), 12.f, FLinearColor(.72f, .45f, .16f, .55f), 1.f);
	Style.Hovered = FSlateRoundedBoxBrush(FLinearColor(1.f, .88f, .55f, .55f), 12.f, FLinearColor(.72f, .45f, .16f), 1.f);
	Style.Pressed = Style.Normal; Style.Disabled = Style.Normal;
	Style.NormalPadding = Style.PressedPadding = FMargin(0);
	Button->SetStyle(Style); Button->SetBackgroundColor(FLinearColor::White); Button->SetCursor(EMouseCursor::Hand);
	UCanvasPanel* Content = CheckBox->WidgetTree->ConstructWidget<UCanvasPanel>();
	UButtonSlot* ContentSlot = CastChecked<UButtonSlot>(Button->AddChild(Content));
	ContentSlot->SetHorizontalAlignment(HAlign_Fill); ContentSlot->SetVerticalAlignment(VAlign_Fill);
	Label->SetFont(FSlateFontInfo(ClueGardenFont, 23)); Label->SetColorAndOpacity(ClueSelectInk);
	Label->SetShadowOffset({0, 0}); Label->SetVisibility(ESlateVisibility::HitTestInvisible);
	ClueSelectPlace(Content, Label, {72, (Height - 38) / 2}, {Width - 82, 38}, 1);
	Empty->SetBrush(ClueSelectImage(ClueUncheckedTexture)); Empty->SetColorAndOpacity(FLinearColor::White);
	Check->SetBrush(ClueSelectImage(ClueCheckedTexture)); Check->SetColorAndOpacity(FLinearColor::White);
	Empty->SetVisibility(ESlateVisibility::HitTestInvisible);
	ClueSelectPlace(Content, Empty, {12, (Height - 46) / 2}, {46, 46}, 2);
	ClueSelectPlace(Content, Check, {12, (Height - 46) / 2}, {46, 46}, 3);
}

#undef LOCTEXT_NAMESPACE
