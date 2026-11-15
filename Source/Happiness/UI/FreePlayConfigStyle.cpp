#include "UI/FreePlayConfigWidget.h"

#include "UI/ClueSelectWidget.h"
#include "UI/PauseMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateNoResource.h"
#include "Components/BackgroundBlur.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"
#if WITH_EDITOR
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"
#endif

#define LOCTEXT_NAMESPACE "FreePlayGarden"

namespace
{
	const FLinearColor FreePlayInk(.17f, .085f, .035f);
	const FLinearColor FreePlayIvory(1.f, .95f, .8f);

	FSlateBrush FreePlayImage(UTexture2D* Texture)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		return Brush;
	}

	FSlateBrush FreePlayField(UTexture2D* Texture)
	{
		FSlateBrush Brush = FreePlayImage(Texture);
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.Margin = FMargin(.035f, .25f);
		return Brush;
	}

	FButtonStyle FreePlayButton(const FSlateBrush& Face)
	{
		FButtonStyle Style;
		Style.Normal = Style.Hovered = Style.Pressed = Style.Disabled = Face;
		Style.Hovered.TintColor = FLinearColor(1.08f, 1.08f, 1.04f);
		Style.Pressed.TintColor = FLinearColor(.8f, .84f, .88f);
		Style.Disabled.TintColor = FLinearColor(.55f, .55f, .55f);
		Style.NormalPadding = Style.PressedPadding = FMargin(8, 4);
		return Style;
	}

	int32 FreePlaySelectionIndex(const UUserWidget* Selection)
	{
		const FIntProperty* Index = Selection ? FindFProperty<FIntProperty>(Selection->GetClass(), TEXT("SelectedIndex")) : nullptr;
		return Index ? Index->GetPropertyValue_InContainer(Selection) : 0;
	}
}

UFreePlayConfigWidget::UFreePlayConfigWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> Panel(TEXT("/Game/General/Textures/T_OptionsGarden.T_OptionsGarden"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Actions(TEXT("/Game/General/Textures/T_LessonNotebookButtons.T_LessonNotebookButtons"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Field(TEXT("/Game/General/Textures/T_FreePlayField.T_FreePlayField"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Selected(TEXT("/Game/General/Textures/T_FreePlaySelected.T_FreePlaySelected"));
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	GardenTexture = Panel.Object;
	ActionTexture = Actions.Object;
	FieldTexture = Field.Object;
	SelectedTexture = Selected.Object;
	GardenFont = Font.Object;
}

void UFreePlayOptionToggle::HandleClicked()
{
	if (UFreePlayConfigWidget* Widget = Owner.Get()) Widget->HandleOptionSelected(bDifficulty, Index);
}

void UFreePlayConfigWidget::HandleOptionSelected(bool bDifficulty, int32 Index)
{
	SelectOption(bDifficulty ? DifficultySelection : SizeSelection, Index);
	UGameplayStatics::PlaySound2D(this, LoadObject<USoundBase>(nullptr, TEXT("/Game/General/Audio/Cues/MenuAccept.MenuAccept")));
	RefreshSegments();
}

void UFreePlayConfigWidget::FitToMenuCanvas()
{
	// GameSelect's legacy popup slot auto-sizes from desired size. This root now stretches,
	// so its desired size is zero; give the modal the actual menu canvas instead.
	// Show can configure the slot after Construct, hence the conditional check on tick too.
	if (UCanvasPanelSlot* Host = Cast<UCanvasPanelSlot>(Slot))
	{
		const FAnchors Fill(0, 0, 1, 1);
		if (Host->GetAutoSize() || Host->GetAnchors() != Fill || Host->GetOffsets() != FMargin(0) || Host->GetAlignment() != FVector2D::ZeroVector)
		{
			Host->SetAutoSize(false);
			Host->SetAnchors(Fill);
			Host->SetAlignment(FVector2D::ZeroVector);
			Host->SetOffsets(FMargin(0));
		}
	}
}

void UFreePlayConfigWidget::BuildGardenLayout()
{
	if (RandomButton || !WidgetTree) return;
	UCanvasPanel* Root = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	UCanvasPanel* Dialog = Cast<UCanvasPanel>(GetWidgetFromName(TEXT("CanvasPanel_26")));
	USizeBox* Size = Cast<USizeBox>(GetWidgetFromName(TEXT("SizeBox_26")));
	UButton* Cancel = Cast<UButton>(GetWidgetFromName(TEXT("Button_Cancel")));
	UButton* Start = Cast<UButton>(GetWidgetFromName(TEXT("Button_Start")));
	UEditableTextBox* Number = Cast<UEditableTextBox>(GetWidgetFromName(TEXT("PuzzleNumberBox")));
	UClueSelectWidget* Clues = Cast<UClueSelectWidget>(GetWidgetFromName(TEXT("ClueSelect")));
	SizeSelection = Cast<UUserWidget>(GetWidgetFromName(TEXT("PuzzleSize")));
	DifficultySelection = Cast<UUserWidget>(GetWidgetFromName(TEXT("PuzzleDifficulty")));
	if (!Root || !Dialog || !Size || !Cancel || !Start || !Number || !Clues || !SizeSelection || !DifficultySelection) return;

	auto Place = [](UCanvasPanel* Canvas, UWidget* Widget, FVector2D Position, FVector2D Extent, int32 Z = 10)
	{
		Widget->RemoveFromParent();
		UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
		Slot->SetPosition(Position); Slot->SetSize(Extent); Slot->SetZOrder(Z);
		return Slot;
	};
	// Leave the original selectors in their hidden parents: Show, Select, delegates and unlock checks still use them.
	for (UWidget* Child : Dialog->GetAllChildren())
		if (Child != Number && Child != Clues) Child->SetVisibility(ESlateVisibility::Collapsed);
	Size->RemoveFromParent();
	Size->SetWidthOverride(1280); Size->SetHeightOverride(850);
	UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>();
	Fit->SetStretch(EStretch::ScaleToFit); Fit->AddChild(Size);
	UCanvasPanelSlot* FitSlot = Place(Root, Fit, {}, {}, 2);
	FitSlot->SetAnchors(FAnchors(0, 0, 1, 1)); FitSlot->SetOffsets(FMargin(40));
	UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>();
	Blur->SetBlurStrength(7); Blur->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* BlurSlot = Place(Root, Blur, {}, {}, 0);
	BlurSlot->SetAnchors(FAnchors(0, 0, 1, 1)); BlurSlot->SetOffsets(FMargin(0));
	UImage* Dim = WidgetTree->ConstructWidget<UImage>();
	Dim->SetBrush(FSlateBrush()); Dim->SetColorAndOpacity(FLinearColor(0, 0, 0, .28f));
	Dim->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* DimSlot = Place(Root, Dim, {}, {}, 1);
	DimSlot->SetAnchors(FAnchors(0, 0, 1, 1)); DimSlot->SetOffsets(FMargin(0));

	UImage* Garden = WidgetTree->ConstructWidget<UImage>();
	Garden->SetBrush(FreePlayImage(GardenTexture)); Garden->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(Dialog, Garden, {}, {1280, 850}, 0);
	auto Text = [&](const FText& Caption, int32 Points, FLinearColor Color = FreePlayInk)
	{
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
		Label->SetText(Caption); Label->SetFont(FSlateFontInfo(GardenFont, Points)); Label->SetColorAndOpacity(Color);
		Label->SetVisibility(ESlateVisibility::HitTestInvisible); return Label;
	};
	UTextBlock* Heading = Text(LOCTEXT("Title", "Free Play"), 50);
	Heading->SetJustification(ETextJustify::Center); Place(Dialog, Heading, {390, 24}, {500, 100});
	auto Label = [&](const FText& Caption, float Y)
	{
		Place(Dialog, Text(Caption, 34), {100, Y + 14}, {335, 62});
	};
	Label(LOCTEXT("Number", "Puzzle Number"), 240);
	Label(LOCTEXT("Size", "Puzzle Size"), 340);
	Label(LOCTEXT("Difficulty", "Difficulty"), 440);
	// The Blueprint still controls these images when the selected size/difficulty is locked.
	for (const auto& Lock : {TPair<const TCHAR*, float>(TEXT("SizeLock"), 364.f), TPair<const TCHAR*, float>(TEXT("DifficultyLock"), 464.f)})
		if (UWidget* LockImage = GetWidgetFromName(Lock.Key)) Place(Dialog, LockImage, {405, Lock.Value}, {32, 32}, 20);
	FEditableTextBoxStyle NumberStyle = Number->GetWidgetStyle();
	NumberStyle.BackgroundImageNormal = NumberStyle.BackgroundImageHovered = NumberStyle.BackgroundImageFocused = FreePlayField(FieldTexture);
	NumberStyle.TextStyle.SetFont(FSlateFontInfo(GardenFont, 34));
	NumberStyle.ForegroundColor = FreePlayInk; NumberStyle.FocusedForegroundColor = FreePlayInk;
	NumberStyle.Padding = FMargin(18, 10); Number->SetWidgetStyle(NumberStyle); Number->SetForegroundColor(FreePlayInk);
	Number->SetJustification(ETextJustify::Center); Place(Dialog, Number, {450, 240}, {745, 80});

	auto Action = [&](UButton* Button, const FText& Caption, int32 Symbol, bool Primary, FVector2D Position, FVector2D Extent, int32 Points)
	{
		Button->ClearChildren();
		FSlateBrush Face = FreePlayImage(ActionTexture);
		Face.SetUVRegion(FBox2f(FVector2f(.03f, Primary ? .525f : .06f), FVector2f(.98f, Primary ? .963f : .485f)));
		Button->SetStyle(FreePlayButton(Face)); Button->SetBackgroundColor(FLinearColor::White);
		Button->SetCursor(EMouseCursor::Hand); Button->SetVisibility(ESlateVisibility::Visible);
		UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>();
		if (Symbol >= 0)
		{
			USizeBox* GlyphSize = WidgetTree->ConstructWidget<USizeBox>(); GlyphSize->SetWidthOverride(42); GlyphSize->SetHeightOverride(42);
			UPauseMenuGlyph* Glyph = WidgetTree->ConstructWidget<UPauseMenuGlyph>(); Glyph->Symbol = Symbol;
			Glyph->Color = Primary ? FreePlayIvory : FreePlayInk; GlyphSize->AddChild(Glyph);
			UHorizontalBoxSlot* GlyphSlot = Content->AddChildToHorizontalBox(GlyphSize);
			GlyphSlot->SetVerticalAlignment(VAlign_Center); GlyphSlot->SetPadding(FMargin(0, 0, 16, 0));
		}
		UHorizontalBoxSlot* LabelSlot = Content->AddChildToHorizontalBox(Text(Caption, Points, Primary ? FreePlayIvory : FreePlayInk));
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		UButtonSlot* ContentSlot = CastChecked<UButtonSlot>(Button->AddChild(Content));
		ContentSlot->SetHorizontalAlignment(HAlign_Center); ContentSlot->SetVerticalAlignment(VAlign_Center);
		Place(Dialog, Button, Position, Extent, 15);
	};
	Action(Cancel, LOCTEXT("Cancel", "Cancel"), 9, false, {90, 680}, {520, 112}, 40);
	Action(Start, LOCTEXT("Start", "Start Puzzle"), 8, true, {660, 680}, {530, 112}, 40);
	RandomButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_Random"));
	Action(RandomButton, LOCTEXT("Random", "Random"), 10, false, {950, 156}, {245, 76}, 28);

	for (int32 Group = 0; Group < 2; ++Group)
	{
		const int32 Count = Group == 0 ? 6 : 3;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			UButton* Button = WidgetTree->ConstructWidget<UButton>();
			Button->SetCursor(EMouseCursor::Hand); Button->SetBackgroundColor(FLinearColor::White);
			const FText Caption = Group == 0 ? FText::Format(LOCTEXT("SizeChoice", "{0}\u00d7{0}"), Index + 3)
				: Index == 0 ? LOCTEXT("Easy", "Easy") : Index == 1 ? LOCTEXT("Normal", "Normal") : LOCTEXT("Hard", "Hard");
			UTextBlock* SegmentLabel = Text(Caption, 30); SegmentLabel->SetJustification(ETextJustify::Center);
			UButtonSlot* ContentSlot = CastChecked<UButtonSlot>(Button->AddChild(SegmentLabel));
			ContentSlot->SetHorizontalAlignment(HAlign_Center); ContentSlot->SetVerticalAlignment(VAlign_Center);
			const float CellWidth = 745.f / Count;
			Place(Dialog, Button, {450 + CellWidth * Index, Group == 0 ? 340.f : 440.f}, {CellWidth, 80}, 15 + Index);
			UFreePlayOptionToggle* Handler = NewObject<UFreePlayOptionToggle>(this);
			Handler->Owner = this; Handler->bDifficulty = Group == 1; Handler->Index = Index;
			Button->OnClicked.AddDynamic(Handler, &UFreePlayOptionToggle::HandleClicked);
			SegmentButtons.Add(Button); SegmentLabels.Add(SegmentLabel); SegmentHandlers.Add(Handler);
		}
	}

	// Keep the clue selector and its existing popup; only replace the trigger's presentation.
	if (UWidget* OldLabel = Clues->GetWidgetFromName(TEXT("ClueLabel"))) OldLabel->SetVisibility(ESlateVisibility::Collapsed);
	UTextBlock* ClueCaption = Text(LOCTEXT("ClueTypes", "Clue Types"), 34);
	Place(Dialog, ClueCaption, {100, 554}, {335, 62});
	UButton* Open = Clues->OpenButton;
	Clues->SummaryText->RemoveFromParent(); Open->ClearChildren();
	Open->SetStyle(FreePlayButton(FreePlayField(SelectedTexture))); Open->SetBackgroundColor(FLinearColor::White);
	Open->SetCursor(EMouseCursor::Hand);
	UCanvasPanel* Trigger = WidgetTree->ConstructWidget<UCanvasPanel>();
	UButtonSlot* TriggerSlot = CastChecked<UButtonSlot>(Open->AddChild(Trigger));
	TriggerSlot->SetHorizontalAlignment(HAlign_Fill); TriggerSlot->SetVerticalAlignment(VAlign_Fill);
	Clues->SummaryText->SetFont(FSlateFontInfo(GardenFont, 30));
	Clues->SummaryText->SetColorAndOpacity(FreePlayIvory); Clues->SummaryText->SetJustification(ETextJustify::Center);
	Place(Trigger, Clues->SummaryText, {45, 11}, {620, 58}, 1);
	Place(Trigger, Text(FText::FromString(TEXT("\u203a")), 40, FreePlayIvory), {674, 1}, {40, 65}, 2);
	UCanvasPanelSlot* OpenSlot = CastChecked<UCanvasPanelSlot>(Open->Slot);
	OpenSlot->SetAnchors(FAnchors(0, 0)); OpenSlot->SetAlignment({0, 0});
	OpenSlot->SetPosition({450, 540}); OpenSlot->SetSize({745, 80});
	CastChecked<UCanvasPanelSlot>(Clues->Slot)->SetZOrder(40);
}

void UFreePlayConfigWidget::RefreshSegments()
{
	if (SegmentButtons.Num() != 9) return;
	int32 SizeIndex = FreePlaySelectionIndex(SizeSelection);
	int32 DifficultyIndex = FreePlaySelectionIndex(DifficultySelection);
#if WITH_EDITOR
	if (GetWorld() && !GetWorld()->IsGameWorld()) { SizeIndex = 3; DifficultyIndex = 1; }
#endif
	if (SizeIndex == LastSizeSelection && DifficultyIndex == LastDifficultySelection) return;
	LastSizeSelection = SizeIndex; LastDifficultySelection = DifficultyIndex;
	for (int32 Index = 0; Index < SegmentButtons.Num(); ++Index)
	{
		const bool bSelected = Index < 6 ? Index == SizeIndex : Index - 6 == DifficultyIndex;
		SegmentButtons[Index]->SetStyle(FreePlayButton(FreePlayField(bSelected ? SelectedTexture : FieldTexture)));
		SegmentLabels[Index]->SetColorAndOpacity(bSelected ? FreePlayIvory : FreePlayInk);
	}
}

#undef LOCTEXT_NAMESPACE

#if WITH_EDITOR
namespace
{
	// Reproduce the menu's old auto-sized host after construction, without clicking through UI or changing a save.
	TWeakObjectPtr<UFreePlayConfigWidget> FreePlayHostTest;
	FAutoConsoleCommandWithWorldAndArgs FreePlayHostTestCommand(
		TEXT("Happiness.FreePlay.TestSetup"),
		TEXT("Open Free Play in the legacy auto-sized slot; optional clues opens its popup. Editor only."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
			if (!PC) return;
			TArray<UUserWidget*> Widgets;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UUserWidget::StaticClass(), false);
			UCanvasPanel* Menu = nullptr;
			for (UUserWidget* Widget : Widgets)
				if (Widget->FindFunction(TEXT("ShowHappinessConfig")))
				{
					Menu = Cast<UCanvasPanel>(Widget->GetWidgetFromName(TEXT("MainCanvas")));
					if (!Menu) Menu = Cast<UCanvasPanel>(Widget->WidgetTree->RootWidget);
					break;
				}
			UClass* ConfigClass = LoadClass<UFreePlayConfigWidget>(nullptr, TEXT("/Game/Happiness/UI/WBP_HappinessConfig.WBP_HappinessConfig_C"));
			if (!Menu || !ConfigClass) return;
			if (FreePlayHostTest.IsValid()) FreePlayHostTest->RemoveFromParent();
			UFreePlayConfigWidget* Config = CreateWidget<UFreePlayConfigWidget>(PC, ConfigClass);
			UCanvasPanelSlot* Host = Menu->AddChildToCanvas(Config);
			Host->SetAnchors(FAnchors(.5f, .5f)); Host->SetAlignment({.5f, .5f}); Host->SetOffsets(FMargin(0));
			Host->SetAutoSize(true); Host->SetZOrder(200);
			Config->SetVisibility(ESlateVisibility::Visible);
			FreePlayHostTest = Config;
			if (Args.Contains(TEXT("clues")))
				if (UClueSelectWidget* Clues = Cast<UClueSelectWidget>(Config->GetWidgetFromName(TEXT("ClueSelect")))) Clues->OpenPanel();
			FTimerHandle Report;
			World->GetTimerManager().SetTimer(Report, FTimerDelegate::CreateWeakLambda(Config, [Config, World]()
			{
				const UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Config->Slot);
				UE_LOG(LogTemp, Display, TEXT("FreePlayHostTest: size=%s autoSize=%d anchors=%s/%s viewport=%s"),
					*Config->GetCachedGeometry().GetLocalSize().ToString(), Slot ? Slot->GetAutoSize() : -1,
					Slot ? *Slot->GetAnchors().Minimum.ToString() : TEXT("?"), Slot ? *Slot->GetAnchors().Maximum.ToString() : TEXT("?"),
					*UWidgetLayoutLibrary::GetViewportSize(World).ToString());
			}), .5f, false);
		}));
}
#endif
