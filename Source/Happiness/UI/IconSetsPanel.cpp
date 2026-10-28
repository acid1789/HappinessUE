#include "UI/IconSetsPanel.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "HappinessClassic/IconSets.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UI/PauseMenuWidget.h"
#include "UI/PlayerInfoPanel.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "IconSetsPanel"

namespace
{
	// The pause menu's garden colors
	const FLinearColor SetsInk(.17f, .085f, .035f);
	const FLinearColor SetsSage(.16f, .29f, .075f);
	const FLinearColor SetsIvory(1.f, .95f, .8f);
	const FLinearColor SetsWarning(.55f, .12f, .04f);
	const FLinearColor SetsGold(.78f, .6f, .25f);
	// The player info's window, half as big again (same shape, so the frame art keeps its proportions)
	const FVector2D SetsWindowSize(1560.f, 780.f);
	const FVector2D SetsCardSize(290.f, 430.f);
	constexpr float SetsCardGap = 24.f;
	constexpr float SetsIconSize = 56.f;
	constexpr float SetsIconGap = 10.f;
	constexpr int32 SetsIconColumns = 4;
	constexpr float SetsMessageDuration = 3.5f;
	constexpr float SetsDisabledIconOpacity = .4f;
	constexpr float SetsLockedIconOpacity = .25f;
}

void UIconSetCardClick::HandleClicked()
{
	if (UIconSetsPanel* Owner = Panel.Get())
	{
		Owner->ToggleSet(Index);
	}
}

UIconSetsPanel::UIconSetsPanel(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> Panel(TEXT("/Game/General/Textures/T_PauseGardenPanel.T_PauseGardenPanel"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Buttons(TEXT("/Game/General/Textures/T_LessonNotebookButtons.T_LessonNotebookButtons"));
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Accept(TEXT("/Game/General/Audio/Cues/MenuAccept.MenuAccept"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Cancel(TEXT("/Game/General/Audio/Cues/MenuCancel.MenuCancel"));
	PanelTexture = Panel.Object;
	ButtonTexture = Buttons.Object;
	PanelFont = Font.Object;
	AcceptSound = Accept.Object;
	CancelSound = Cancel.Object;
}

TSharedRef<SWidget> UIconSetsPanel::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UIconSetsPanel::BuildLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SetsCanvas"));
	WidgetTree->RootWidget = Root;

	// The menu behind dims; a tap on it closes the screen
	UButton* Outside = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("OutsideButton"));
	FButtonStyle DimStyle;
	DimStyle.Normal = DimStyle.Hovered = DimStyle.Pressed = DimStyle.Disabled = FSlateRoundedBoxBrush(FLinearColor(0.f, 0.f, 0.f, .35f), 0.f);
	DimStyle.NormalPadding = DimStyle.PressedPadding = FMargin(0.f);
	Outside->SetStyle(DimStyle);
	Outside->OnClicked.AddUniqueDynamic(this, &UIconSetsPanel::HandleCloseClicked);
	UCanvasPanelSlot* OutsideSlot = Root->AddChildToCanvas(Outside);
	OutsideSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	OutsideSlot->SetOffsets(FMargin(0.f));

	UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>();
	Fit->SetStretch(EStretch::ScaleToFit);
	Fit->SetStretchDirection(EStretchDirection::DownOnly);
	Fit->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UCanvasPanelSlot* FitSlot = Root->AddChildToCanvas(Fit);
	FitSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	FitSlot->SetOffsets(FMargin(40.f));
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(SetsWindowSize.X);
	Size->SetHeightOverride(SetsWindowSize.Y);
	Fit->AddChild(Size);
	UCanvasPanel* Window = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Window"));
	Size->AddChild(Window);

	auto Place = [&](UWidget* Widget, FVector2D Position, FVector2D Extent)
	{
		UCanvasPanelSlot* WindowSlot = Window->AddChildToCanvas(Widget);
		WindowSlot->SetPosition(Position);
		WindowSlot->SetSize(Extent);
		return WindowSlot;
	};

	// The window takes the taps on it, so they don't close it
	UImage* Surface = WidgetTree->ConstructWidget<UImage>();
	FSlateBrush SurfaceBrush;
	SurfaceBrush.SetResourceObject(PanelTexture);
	SurfaceBrush.DrawAs = ESlateBrushDrawType::Image;
	SurfaceBrush.SetUVRegion(FBox2f(FVector2f(.008f, .05f), FVector2f(.992f, .93f)));
	Surface->SetBrush(SurfaceBrush);
	Place(Surface, FVector2D::ZeroVector, SetsWindowSize);

	// The title on the notebook buttons' ivory face, as the player info's
	FSlateBrush Face;
	Face.DrawAs = ESlateBrushDrawType::Image;
	Face.SetResourceObject(ButtonTexture);
	Face.SetUVRegion(FBox2f(FVector2f(.03f, .06f), FVector2f(.98f, .485f)));
	UImage* TitlePlate = WidgetTree->ConstructWidget<UImage>();
	TitlePlate->SetBrush(Face);
	TitlePlate->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(TitlePlate, FVector2D((SetsWindowSize.X - 440.f) * .5f, 64.f), FVector2D(440.f, 100.f));
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>();
	Title->SetText(LOCTEXT("Title", "Icon Sets"));
	Title->SetFont(FSlateFontInfo(PanelFont, 50));
	Title->SetColorAndOpacity(SetsInk);
	Title->SetJustification(ETextJustify::Center);
	Title->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(Title, FVector2D((SetsWindowSize.X - 440.f) * .5f, 80.f), FVector2D(440.f, 72.f));

	UButton* Close = UPlayerInfoPanel::MakeCloseButton(*WidgetTree);
	Close->OnClicked.AddUniqueDynamic(this, &UIconSetsPanel::HandleCloseClicked);
	// Right of the corner's leaves, inside the frame (the player info's place, scaled with the window)
	Place(Close, FVector2D(1414.f, 104.f), FVector2D(76.f, 76.f));

	StatusText = WidgetTree->ConstructWidget<UTextBlock>();
	StatusText->SetFont(FSlateFontInfo(PanelFont, 30));
	StatusText->SetJustification(ETextJustify::Center);
	StatusText->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(StatusText, FVector2D(150.f, 184.f), FVector2D(SetsWindowSize.X - 300.f, 46.f));

	// The cards, scrolled left and right (dragged, or with the arrows either side)
	Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("SetsScroll"));
	Scroll->SetOrientation(Orient_Horizontal);
	Scroll->SetScrollBarVisibility(ESlateVisibility::Collapsed);
	Scroll->SetAnimateWheelScrolling(true);
	Scroll->SetConsumeMouseWheel(EConsumeMouseWheel::Always);
	CardRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CardRow"));
	Scroll->AddChild(CardRow);
	const float ScrollLeft = 160.f;
	Place(Scroll, FVector2D(ScrollLeft, 250.f), FVector2D(SetsWindowSize.X - 2.f * ScrollLeft, SetsCardSize.Y));

	auto MakeArrow = [&](const TCHAR* Name, bool bLeft)
	{
		// The close button's round face, with an arrow for the cross
		UButton* Arrow = UPlayerInfoPanel::MakeCloseButton(*WidgetTree, Name);
		Arrow->SetClickMethod(EButtonClickMethod::PreciseClick);
		USizeBox* GlyphSize = CastChecked<USizeBox>(Arrow->GetChildAt(0));
		UPauseMenuGlyph* Glyph = WidgetTree->ConstructWidget<UPauseMenuGlyph>();
		// The play triangle, mirrored for the left
		Glyph->Symbol = 8;
		Glyph->Color = SetsInk;
		Glyph->SetVisibility(ESlateVisibility::HitTestInvisible);
		Glyph->SetRenderTransformPivot(FVector2D(.5f));
		Glyph->SetRenderScale(FVector2D(bLeft ? -1.f : 1.f, 1.f));
		GlyphSize->SetContent(Glyph);
		GlyphSize->SetWidthOverride(36.f);
		GlyphSize->SetHeightOverride(36.f);
		const float X = bLeft ? ScrollLeft - 110.f : SetsWindowSize.X - ScrollLeft + 26.f;
		Place(Arrow, FVector2D(X, 250.f + (SetsCardSize.Y - 84.f) * .5f), FVector2D(84.f, 84.f));
		return Arrow;
	};
	LeftButton = MakeArrow(TEXT("LeftButton"), true);
	RightButton = MakeArrow(TEXT("RightButton"), false);
	LeftButton->OnClicked.AddUniqueDynamic(this, &UIconSetsPanel::HandleLeftClicked);
	RightButton->OnClicked.AddUniqueDynamic(this, &UIconSetsPanel::HandleRightClicked);

	Refresh();
}

UWidget* UIconSetsPanel::MakeCard(int32 Index, const FIconSetEntry& Entry)
{
	// The card: a cream tile, greyed while the set is off or locked
	UButton* Card = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *FString::Printf(TEXT("Card%d"), Index));
	const FLinearColor Fill = Entry.bEnabled ? FLinearColor(1.f, .97f, .86f) : FLinearColor(.86f, .84f, .78f);
	const FLinearColor Rim = Entry.bEnabled ? SetsGold : FLinearColor(.6f, .56f, .48f);
	FButtonStyle Style;
	Style.Normal = Style.Hovered = Style.Pressed = Style.Disabled = FSlateRoundedBoxBrush(Fill, 26.f, Rim, Entry.bEnabled ? 4.f : 2.f);
	Style.Hovered.TintColor = FLinearColor(1.05f, 1.05f, 1.02f);
	Style.Pressed.TintColor = FLinearColor(.88f, .9f, .84f);
	Style.NormalPadding = Style.PressedPadding = FMargin(18.f, 22.f);
	Card->SetStyle(Style);
	Card->SetCursor(EMouseCursor::Hand);
	// A drag across the cards scrolls them rather than tapping one
	Card->SetTouchMethod(EButtonTouchMethod::PreciseTap);
	Card->SetClickMethod(EButtonClickMethod::PreciseClick);
	UIconSetCardClick* Click = NewObject<UIconSetCardClick>(this);
	Click->Panel = this;
	Click->Index = Index;
	Card->OnClicked.AddUniqueDynamic(Click, &UIconSetCardClick::HandleClicked);
	CardClicks.Add(Click);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Column->SetVisibility(ESlateVisibility::HitTestInvisible);
	UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>();
	Name->SetText(Entry.Name);
	Name->SetFont(FSlateFontInfo(PanelFont, 34));
	Name->SetColorAndOpacity(Entry.bUnlocked ? SetsInk : SetsInk.CopyWithNewOpacity(.6f));
	Name->SetJustification(ETextJustify::Center);
	Name->SetAutoWrapText(true);
	USizeBox* NameSize = WidgetTree->ConstructWidget<USizeBox>();
	NameSize->SetHeightOverride(96.f);
	NameSize->AddChild(Name);
	UVerticalBoxSlot* NameSlot = Column->AddChildToVerticalBox(NameSize);
	NameSlot->SetHorizontalAlignment(HAlign_Fill);

	UUniformGridPanel* Icons = WidgetTree->ConstructWidget<UUniformGridPanel>();
	Icons->SetSlotPadding(FMargin(SetsIconGap * .5f));
	const TArray<UTexture2D*> Textures = UIconSetSubsystem::GetIconTextures(Entry.IconSet);
	for (int32 Icon = 0; Icon < Textures.Num(); Icon++)
	{
		UImage* Image = WidgetTree->ConstructWidget<UImage>();
		if (Textures[Icon])
		{
			Image->SetBrushFromTexture(Textures[Icon]);
		}
		Image->SetRenderOpacity(Entry.bEnabled ? 1.f : Entry.bUnlocked ? SetsDisabledIconOpacity : SetsLockedIconOpacity);
		USizeBox* ImageSize = WidgetTree->ConstructWidget<USizeBox>();
		ImageSize->SetWidthOverride(SetsIconSize);
		ImageSize->SetHeightOverride(SetsIconSize);
		ImageSize->AddChild(Image);
		Icons->AddChildToUniformGrid(ImageSize, Icon / SetsIconColumns, Icon % SetsIconColumns);
	}
	UVerticalBoxSlot* IconsSlot = Column->AddChildToVerticalBox(Icons);
	IconsSlot->SetHorizontalAlignment(HAlign_Center);
	IconsSlot->SetVerticalAlignment(VAlign_Center);
	IconsSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	IconsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// At the bottom: On / Off, or what unlocks it
	UBorder* Pill = WidgetTree->ConstructWidget<UBorder>();
	const FLinearColor PillFill = Entry.bEnabled ? SetsSage : Entry.bUnlocked ? FLinearColor(.7f, .67f, .6f) : FLinearColor(.44f, .28f, .1f, .18f);
	Pill->SetBrush(FSlateRoundedBoxBrush(PillFill, 24.f));
	Pill->SetPadding(FMargin(12.f, 6.f));
	Pill->SetHorizontalAlignment(HAlign_Center);
	Pill->SetVerticalAlignment(VAlign_Center);
	UTextBlock* State = WidgetTree->ConstructWidget<UTextBlock>();
	State->SetText(Entry.bEnabled ? LOCTEXT("On", "Enabled") : Entry.bUnlocked ? LOCTEXT("Off", "Disabled")
		: FText::Format(LOCTEXT("Locked", "Locked: {0}"), Entry.LockedReason));
	State->SetFont(FSlateFontInfo(PanelFont, Entry.bUnlocked ? 28 : 22));
	State->SetColorAndOpacity(Entry.bEnabled ? SetsIvory : SetsInk);
	State->SetJustification(ETextJustify::Center);
	State->SetAutoWrapText(true);
	Pill->SetContent(State);
	USizeBox* PillSize = WidgetTree->ConstructWidget<USizeBox>();
	PillSize->SetMinDesiredHeight(54.f);
	PillSize->AddChild(Pill);
	UVerticalBoxSlot* PillSlot = Column->AddChildToVerticalBox(PillSize);
	PillSlot->SetHorizontalAlignment(HAlign_Fill);

	UButtonSlot* ContentSlot = CastChecked<UButtonSlot>(Card->AddChild(Column));
	ContentSlot->SetHorizontalAlignment(HAlign_Fill);
	ContentSlot->SetVerticalAlignment(VAlign_Fill);

	USizeBox* Sized = WidgetTree->ConstructWidget<USizeBox>();
	Sized->SetWidthOverride(SetsCardSize.X);
	Sized->SetHeightOverride(SetsCardSize.Y);
	Sized->AddChild(Card);
	return Sized;
}

void UIconSetsPanel::Refresh()
{
	if (!CardRow)
	{
		return;
	}
	const UIconSetSubsystem* IconSets = UIconSetSubsystem::Get(this);
	const TArray<FIconSetEntry> Entries = IconSets ? IconSets->GetEntries() : TArray<FIconSetEntry>();
	CardRow->ClearChildren();
	Cards.Reset();
	CardClicks.Reset();
	CardSets.Reset();
	for (int32 Index = 0; Index < Entries.Num(); Index++)
	{
		if (UWidget* Card = MakeCard(Index, Entries[Index]))
		{
			UHorizontalBoxSlot* CardSlot = CardRow->AddChildToHorizontalBox(Card);
			CardSlot->SetPadding(FMargin(Index == 0 ? 0.f : SetsCardGap, 0.f, 0.f, 0.f));
			Cards.Add(Card);
			CardSets.Add(Entries[Index].IconSet);
		}
	}
	UpdateStatus();
}

void UIconSetsPanel::UpdateStatus()
{
	if (!StatusText || MessageSeconds > 0.f)
	{
		return;
	}
	const UIconSetSubsystem* IconSets = UIconSetSubsystem::Get(this);
	int32 Unlocked = 0;
	for (const FIconSetEntry& Entry : IconSets ? IconSets->GetEntries() : TArray<FIconSetEntry>())
	{
		Unlocked += Entry.bUnlocked ? 1 : 0;
	}
	StatusText->SetText(FText::Format(LOCTEXT("Count", "{0} of {1} sets enabled. Tap a set to enable or disable it."),
		IconSets ? IconSets->GetNumEnabled() : 0, Unlocked));
	StatusText->SetColorAndOpacity(SetsSage);
}

void UIconSetsPanel::ToggleSet(int32 Index)
{
	UIconSetSubsystem* IconSets = UIconSetSubsystem::Get(this);
	const TArray<FIconSetEntry> Entries = IconSets ? IconSets->GetEntries() : TArray<FIconSetEntry>();
	if (!Entries.IsValidIndex(Index))
	{
		return;
	}
	const FIconSetEntry& Entry = Entries[Index];
	FText Message;
	if (!Entry.bUnlocked)
	{
		Message = FText::Format(LOCTEXT("LockedMessage", "{0} is locked. {1} to unlock it."), Entry.Name, Entry.LockedReason);
	}
	else if (!IconSets->SetEnabled(Entry.IconSet, !Entry.bEnabled))
	{
		Message = FText::Format(LOCTEXT("TooFew", "At least {0} sets must stay enabled to play."), UIconSetSubsystem::MinEnabledSets);
	}
	if (!Message.IsEmpty())
	{
		PlaySound(CancelSound);
		MessageSeconds = SetsMessageDuration;
		StatusText->SetText(Message);
		StatusText->SetColorAndOpacity(SetsWarning);
		return;
	}
	PlaySound(AcceptSound);
	MessageSeconds = 0.f;
	// Remake the cards, keeping the scroll where it is
	const float Offset = Scroll ? Scroll->GetScrollOffset() : 0.f;
	Refresh();
	if (Scroll)
	{
		Scroll->SetScrollOffset(Offset);
	}
}

void UIconSetsPanel::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (MessageSeconds > 0.f && (MessageSeconds -= InDeltaTime) <= 0.f)
	{
		MessageSeconds = 0.f;
		UpdateStatus();
	}
	// The arrows fade at the ends of the row
	if (Scroll && LeftButton && RightButton)
	{
		const float Offset = Scroll->GetScrollOffset();
		const float End = Scroll->GetScrollOffsetOfEnd();
		LeftButton->SetRenderOpacity(Offset > 1.f ? 1.f : .35f);
		RightButton->SetRenderOpacity(Offset < End - 1.f ? 1.f : .35f);
	}
}

void UIconSetsPanel::ScrollBy(int32 CardCount)
{
	if (!Scroll || Cards.Num() == 0)
	{
		return;
	}
	const float Step = SetsCardSize.X + SetsCardGap;
	const int32 First = FMath::RoundToInt(Scroll->GetScrollOffset() / Step);
	const int32 Target = FMath::Clamp(First + CardCount, 0, Cards.Num() - 1);
	Scroll->ScrollWidgetIntoView(Cards[Target], true, EDescendantScrollDestination::TopOrLeft);
}

void UIconSetsPanel::HandleLeftClicked()
{
	ScrollBy(-3);
}

void UIconSetsPanel::HandleRightClicked()
{
	ScrollBy(3);
}

void UIconSetsPanel::Show()
{
	MessageSeconds = 0.f;
	Refresh();
	if (Scroll)
	{
		Scroll->ScrollToStart();
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UIconSetsPanel::Hide()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UIconSetsPanel::HandleCloseClicked()
{
	PlaySound(CancelSound);
	Hide();
}

void UIconSetsPanel::PlaySound(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound, 1.f, 1.f, 0.f, nullptr, nullptr, true);
	}
}

#undef LOCTEXT_NAMESPACE
