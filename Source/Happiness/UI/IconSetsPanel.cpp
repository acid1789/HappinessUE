#include "UI/IconSetsPanel.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "IconSetsPanel"

namespace
{
	const FLinearColor SetsInk(.17f, .085f, .035f);
	const FLinearColor SetsIvory(1.f, .95f, .8f);
	const FLinearColor SetsWarning(.55f, .12f, .04f);
	const FVector2D SetsWindowSize(1600.f, 800.f);
	constexpr float SetsMessageDuration = 3.5f;

	UCanvasPanelSlot* SetsPlace(UCanvasPanel* Canvas, UWidget* Widget, FVector2D Position, FVector2D Size, int32 Layer)
	{
		UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetZOrder(Layer);
		return Slot;
	}

	UTextBlock* SetsText(UWidgetTree* Tree, UFont* Font, const FText& Text, int32 Points, const FLinearColor& Color)
	{
		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>();
		Label->SetText(Text);
		Label->SetFont(FSlateFontInfo(Font, Points));
		Label->SetColorAndOpacity(Color);
		Label->SetJustification(ETextJustify::Center);
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Label;
	}

	FButtonStyle SetsHitStyle(float Radius)
	{
		FButtonStyle Style;
		Style.Normal = Style.Disabled = FSlateRoundedBoxBrush(FLinearColor::Transparent, Radius);
		Style.Hovered = FSlateRoundedBoxBrush(FLinearColor(1.f, 1.f, .9f, .10f), Radius);
		Style.Pressed = FSlateRoundedBoxBrush(FLinearColor(.12f, .18f, .06f, .14f), Radius);
		Style.NormalPadding = Style.PressedPadding = FMargin(0.f);
		return Style;
	}
}

void UIconSetCardClick::HandleClicked()
{
	if (UIconSetsPanel* Owner = Panel.Get())
	{
		if (bAction) Owner->ToggleSet(Index);
		else Owner->SelectSet(Index);
	}
}

UIconSetsPanel::UIconSetsPanel(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> Panel(TEXT("/Game/General/Textures/T_IconSetsCatalogue.T_IconSetsCatalogue"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Buttons(TEXT("/Game/General/Textures/T_IconSetStates.T_IconSetStates"));
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
	if (WidgetTree && !WidgetTree->RootWidget) BuildLayout();
	return Super::RebuildWidget();
}

void UIconSetsPanel::BuildLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Root;
	UButton* Outside = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Dim;
	Dim.Normal = Dim.Hovered = Dim.Pressed = Dim.Disabled = FSlateRoundedBoxBrush(FLinearColor(0.f, 0.f, 0.f, .35f), 0.f);
	Dim.NormalPadding = Dim.PressedPadding = FMargin(0.f);
	Outside->SetStyle(Dim);
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
	UCanvasPanel* Window = WidgetTree->ConstructWidget<UCanvasPanel>();
	Size->AddChild(Window);
	UImage* Surface = WidgetTree->ConstructWidget<UImage>();
	Surface->SetBrushFromTexture(PanelTexture);
	SetsPlace(Window, Surface, FVector2D::ZeroVector, SetsWindowSize, 0);
	SetsPlace(Window, SetsText(WidgetTree, PanelFont, LOCTEXT("Title", "Icon Sets"), 48, SetsInk),
		FVector2D(630.f, 21.f), FVector2D(340.f, 75.f), 1);

	UButton* Close = WidgetTree->ConstructWidget<UButton>();
	Close->SetStyle(SetsHitStyle(48.f));
	Close->OnClicked.AddUniqueDynamic(this, &UIconSetsPanel::HandleCloseClicked);
	SetsPlace(Window, Close, FVector2D(1485.f, 43.f), FVector2D(96.f), 2);

	Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	Scroll->SetOrientation(Orient_Vertical);
	Scroll->SetScrollBarVisibility(ESlateVisibility::Visible);
	Scroll->SetAnimateWheelScrolling(true);
	Scroll->SetConsumeMouseWheel(EConsumeMouseWheel::Always);
	Scroll->SetScrollbarThickness(FVector2D(7.f, 7.f));
	FScrollBarStyle Bar = Scroll->GetWidgetBarStyle();
	Bar.NormalThumbImage = Bar.HoveredThumbImage = Bar.DraggedThumbImage = FSlateRoundedBoxBrush(FLinearColor(.38f, .51f, .16f), 3.f);
	Scroll->SetWidgetBarStyle(Bar);
	CardRows = WidgetTree->ConstructWidget<UVerticalBox>();
	Scroll->AddChild(CardRows);
	SetsPlace(Window, Scroll, FVector2D(76.f, 156.f), FVector2D(582.f, 520.f), 3);

	Detail = WidgetTree->ConstructWidget<UCanvasPanel>();
	Detail->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SetsPlace(Window, Detail, FVector2D(740.f, 142.f), FVector2D(782.f, 580.f), 4);
	StatusText = SetsText(WidgetTree, PanelFont, FText::GetEmpty(), 22, SetsInk);
	StatusText->SetAutoWrapText(true);
	SetsPlace(Window, StatusText, FVector2D(88.f, 690.f), FVector2D(550.f, 55.f), 5);
	Refresh();
#if WITH_EDITOR
	if (PreviewIconSetIndex >= 0) PreviewCatalogue(PreviewIconSetIndex);
#endif
}

TArray<FIconSetEntry> UIconSetsPanel::GetEntries() const
{
	if (!PreviewEntries.IsEmpty()) return PreviewEntries;
	const UIconSetSubsystem* Sets = UIconSetSubsystem::Get(this);
	return Sets ? Sets->GetEntries() : TArray<FIconSetEntry>();
}

FSlateBrush UIconSetsPanel::StateBrush(const FIconSetEntry& Entry) const
{
	const int32 Row = Entry.bUnlocked ? (Entry.bEnabled ? 0 : 1) : (Entry.ProductId.IsEmpty() ? 2 : 3);
	// Measured bounds of each painted face; exclude the atlas's transparent gutters.
	constexpr float Tops[] = {.083f, .305f, .524f, .745f};
	constexpr float Bottoms[] = {.284f, .510f, .731f, .954f};
	FSlateBrush Brush;
	Brush.SetResourceObject(ButtonTexture);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.SetUVRegion(FBox2f(FVector2f(.048f, Tops[Row]), FVector2f(.952f, Bottoms[Row])));
	return Brush;
}

FText UIconSetsPanel::StateLabel(const FIconSetEntry& Entry) const
{
	if (Entry.bUnlocked) return Entry.bEnabled ? LOCTEXT("Enabled", "Enabled") : LOCTEXT("Disabled", "Disabled");
	return Entry.ProductId.IsEmpty() ? Entry.LockedReason : LOCTEXT("BuySet", "Buy Set");
}

UButton* UIconSetsPanel::MakeStateButton(UCanvasPanel* Canvas, const FIconSetEntry& Entry, int32 Index,
	FVector2D Position, FVector2D Extent, int32 FontSize)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style;
	Style.Normal = Style.Hovered = Style.Pressed = Style.Disabled = StateBrush(Entry);
	Style.Hovered.TintColor = FLinearColor(1.08f, 1.08f, 1.04f);
	Style.Pressed.TintColor = FLinearColor(.85f, .88f, .8f);
	Style.NormalPadding = Style.PressedPadding = FMargin(0.f);
	Button->SetStyle(Style);
	Button->SetCursor(EMouseCursor::Hand);
	Button->SetTouchMethod(EButtonTouchMethod::PreciseTap);
	Button->SetClickMethod(EButtonClickMethod::PreciseClick);
	SetsPlace(Canvas, Button, Position, Extent, 3);
	UIconSetCardClick* Click = NewObject<UIconSetCardClick>(this);
	Click->Panel = this;
	Click->Index = Index;
	Click->bAction = true;
	Button->OnClicked.AddUniqueDynamic(Click, &UIconSetCardClick::HandleClicked);
	(Canvas == Detail ? DetailClicks : CardClicks).Add(Click);
	const FText LabelText = Canvas == Detail && !Entry.bUnlocked && Entry.ProductId.IsEmpty()
		? LOCTEXT("Locked", "Locked") : StateLabel(Entry);
	UTextBlock* Label = SetsText(WidgetTree, PanelFont, LabelText, FontSize,
		Entry.bUnlocked && Entry.bEnabled ? SetsIvory : SetsInk);
	Label->SetWrapTextAt(Extent.X * .7f);
	UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>();
	Fit->SetStretch(EStretch::ScaleToFit);
	Fit->SetStretchDirection(EStretchDirection::DownOnly);
	Fit->SetVisibility(ESlateVisibility::HitTestInvisible);
	Fit->AddChild(Label);
	SetsPlace(Canvas, Fit, Position + FVector2D(Extent.X * .23f, 4.f), FVector2D(Extent.X * .72f, Extent.Y - 8.f), 4);
	return Button;
}

UWidget* UIconSetsPanel::MakeRow(int32 Index, const FIconSetEntry& Entry)
{
	UCanvasPanel* Row = WidgetTree->ConstructWidget<UCanvasPanel>();
	UButton* Select = WidgetTree->ConstructWidget<UButton>();
	const bool bSelected = Entry.Id == SelectedId;
	FButtonStyle Style;
	Style.Normal = Style.Hovered = Style.Pressed = Style.Disabled = FSlateRoundedBoxBrush(
		bSelected ? FLinearColor(.76f, .91f, 1.f, .55f) : FLinearColor(1.f, .97f, .86f, .35f), 17.f,
		bSelected ? FLinearColor(.025f, .56f, 1.f) : FLinearColor(.65f, .45f, .18f, .55f), bSelected ? 2.5f : 1.f);
	Style.Hovered.TintColor = FLinearColor(1.06f, 1.06f, 1.02f);
	Style.NormalPadding = Style.PressedPadding = FMargin(0.f);
	Select->SetStyle(Style);
	Select->SetTouchMethod(EButtonTouchMethod::PreciseTap);
	Select->SetClickMethod(EButtonClickMethod::PreciseClick);
	Select->SetCursor(EMouseCursor::Hand);
	SetsPlace(Row, Select, FVector2D::ZeroVector, FVector2D(562.f, 99.f), 0);
	UIconSetCardClick* Click = NewObject<UIconSetCardClick>(this);
	Click->Panel = this;
	Click->Index = Index;
	Select->OnClicked.AddUniqueDynamic(Click, &UIconSetCardClick::HandleClicked);
	CardClicks.Add(Click);
	const TArray<UTexture2D*> Textures = UIconSetSubsystem::GetIconTextures(Entry.IconSet);
	UImage* Thumb = WidgetTree->ConstructWidget<UImage>();
	if (!Textures.IsEmpty()) Thumb->SetBrushFromTexture(Textures[0]);
	Thumb->SetVisibility(ESlateVisibility::HitTestInvisible);
	SetsPlace(Row, Thumb, FVector2D(12.f, 11.f), FVector2D(76.f), 1);
	UTextBlock* Name = SetsText(WidgetTree, PanelFont, Entry.Name, 25, SetsInk);
	Name->SetJustification(ETextJustify::Left);
	Name->SetWrapTextAt(191.f);
	UScaleBox* NameFit = WidgetTree->ConstructWidget<UScaleBox>();
	NameFit->SetStretch(EStretch::ScaleToFit);
	NameFit->SetStretchDirection(EStretchDirection::DownOnly);
	NameFit->SetVisibility(ESlateVisibility::HitTestInvisible);
	CastChecked<UScaleBoxSlot>(NameFit->AddChild(Name))->SetHorizontalAlignment(HAlign_Left);
	SetsPlace(Row, NameFit, FVector2D(100.f, 8.f), FVector2D(191.f, 83.f), 2);
	MakeStateButton(Row, Entry, Index, FVector2D(300.f, 22.f), FVector2D(250.f, 55.f), Entry.bUnlocked ? 24 : 20);
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(562.f);
	Size->SetHeightOverride(99.f);
	Size->AddChild(Row);
	return Size;
}

void UIconSetsPanel::Refresh()
{
	if (!CardRows) return;
	const float Offset = Scroll->GetScrollOffset();
	const TArray<FIconSetEntry> Entries = GetEntries();
	if (!Entries.ContainsByPredicate([this](const FIconSetEntry& E) { return E.Id == SelectedId; }))
		SelectedId = Entries.IsEmpty() ? NAME_None : Entries[0].Id;
	CardRows->ClearChildren();
	CardClicks.Reset();
	CardSets.Reset();
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		UVerticalBoxSlot* RowSlot = CardRows->AddChildToVerticalBox(MakeRow(Index, Entries[Index]));
		RowSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 10.f));
		CardSets.Add(Entries[Index].IconSet);
	}
	Scroll->SetScrollOffset(Offset);
	UpdateDetail();
	UpdateStatus();
}

void UIconSetsPanel::UpdateDetail()
{
	if (!Detail) return;
	Detail->ClearChildren();
	DetailClicks.Reset();
	const TArray<FIconSetEntry> Entries = GetEntries();
	const int32 Index = Entries.IndexOfByPredicate([this](const FIconSetEntry& E) { return E.Id == SelectedId; });
	if (Index == INDEX_NONE)
	{
		SetsPlace(Detail, SetsText(WidgetTree, PanelFont, LOCTEXT("NoSets", "No icon sets available"), 32, SetsInk),
			FVector2D(0.f, 80.f), FVector2D(782.f, 70.f), 0);
		return;
	}
	const FIconSetEntry& Entry = Entries[Index];
	UTextBlock* Title = SetsText(WidgetTree, PanelFont, Entry.Name, 42, SetsInk);
	UScaleBox* TitleFit = WidgetTree->ConstructWidget<UScaleBox>();
	TitleFit->SetStretch(EStretch::ScaleToFit);
	TitleFit->SetStretchDirection(EStretchDirection::DownOnly);
	TitleFit->SetVisibility(ESlateVisibility::HitTestInvisible);
	TitleFit->AddChild(Title);
	SetsPlace(Detail, TitleFit, FVector2D(10.f, 0.f), FVector2D(762.f, 70.f), 0);
	const TArray<UTexture2D*> Textures = UIconSetSubsystem::GetIconTextures(Entry.IconSet);
	const int32 Rows = FMath::Max(1, FMath::DivideAndRoundUp(Textures.Num(), 4));
	const float Side = FMath::Min(160.f, (355.f - 14.f * (Rows - 1)) / Rows);
	const int32 Columns = FMath::Min(4, Textures.Num());
	const float GridWidth = Columns * Side + FMath::Max(0, Columns - 1) * 14.f;
	const float GridHeight = Rows * Side + (Rows - 1) * 14.f;
	for (int32 Icon = 0; Icon < Textures.Num(); ++Icon)
	{
		UImage* Image = WidgetTree->ConstructWidget<UImage>();
		Image->SetBrushFromTexture(Textures[Icon]);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		SetsPlace(Detail, Image, FVector2D((782.f - GridWidth) * .5f + (Icon % 4) * (Side + 14.f),
			84.f + (355.f - GridHeight) * .5f + (Icon / 4) * (Side + 14.f)), FVector2D(Side), 1);
	}
	MakeStateButton(Detail, Entry, Index, FVector2D(191.f, 460.f), FVector2D(400.f, 88.f), 34);
	const FText Help = Entry.bUnlocked ? (Entry.bEnabled ? LOCTEXT("TapOff", "Tap to disable") : LOCTEXT("TapOn", "Tap to enable"))
		: Entry.ProductId.IsEmpty() ? Entry.LockedReason : LOCTEXT("TapBuy", "Tap to purchase this set");
	UTextBlock* Hint = SetsText(WidgetTree, PanelFont, Help, 21, SetsInk);
	Hint->SetAutoWrapText(true);
	SetsPlace(Detail, Hint, FVector2D(20.f, 557.f), FVector2D(742.f, 65.f), 5);
}

void UIconSetsPanel::SelectSet(int32 Index)
{
	const TArray<FIconSetEntry> Entries = GetEntries();
	if (!Entries.IsValidIndex(Index)) return;
	SelectedId = Entries[Index].Id;
	PlaySound(AcceptSound);
	Refresh();
}

void UIconSetsPanel::UpdateStatus()
{
	if (!StatusText || MessageSeconds > 0.f) return;
	int32 Enabled = 0;
	for (const FIconSetEntry& Entry : GetEntries()) Enabled += Entry.bEnabled ? 1 : 0;
	if (!PreviewEntries.IsEmpty()) Enabled = 10; // The render shows a sample of a larger collection.
	StatusText->SetText(FText::Format(LOCTEXT("Count", "{0} sets enabled  |  Minimum {1}"), Enabled, UIconSetSubsystem::MinEnabledSets));
	StatusText->SetColorAndOpacity(SetsInk);
}

void UIconSetsPanel::ToggleSet(int32 Index)
{
	const TArray<FIconSetEntry> Entries = GetEntries();
	if (!Entries.IsValidIndex(Index)) return;
	const FIconSetEntry& Entry = Entries[Index];
	SelectedId = Entry.Id;
	FText Message;
	if (!Entry.bUnlocked && !Entry.ProductId.IsEmpty())
	{
		if (OnPurchaseRequested.IsBound())
		{
			PlaySound(AcceptSound);
			OnPurchaseRequested.Broadcast(Entry.ProductId);
			return;
		}
		Message = LOCTEXT("StoreUnavailable", "Purchases are not available right now.");
	}
	else if (!Entry.bUnlocked) Message = Entry.LockedReason;
	else if (UIconSetSubsystem* Sets = UIconSetSubsystem::Get(this))
	{
		if (!Sets->SetEnabled(Entry.IconSet, !Entry.bEnabled))
			Message = FText::Format(LOCTEXT("TooFew", "At least {0} sets must stay enabled to play."), UIconSetSubsystem::MinEnabledSets);
	}
	if (!Message.IsEmpty())
	{
		PlaySound(CancelSound);
		MessageSeconds = SetsMessageDuration;
		Refresh();
		StatusText->SetText(Message);
		StatusText->SetColorAndOpacity(SetsWarning);
		return;
	}
	PlaySound(AcceptSound);
	MessageSeconds = 0.f;
	Refresh();
}

void UIconSetsPanel::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (MessageSeconds > 0.f && (MessageSeconds -= InDeltaTime) <= 0.f)
	{
		MessageSeconds = 0.f;
		UpdateStatus();
	}
}

void UIconSetsPanel::Show()
{
	MessageSeconds = 0.f;
	Refresh();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UIconSetsPanel::Hide() { SetVisibility(ESlateVisibility::Collapsed); }
void UIconSetsPanel::HandleCloseClicked() { PlaySound(CancelSound); Hide(); }
void UIconSetsPanel::PlaySound(USoundBase* Sound) const
{
	if (Sound) UGameplayStatics::PlaySound2D(this, Sound, 1.f, 1.f, 0.f, nullptr, nullptr, true);
}

#if WITH_EDITOR
void UIconSetsPanel::PreviewCatalogue(int32 Index)
{
	PreviewEntries.Reset();
	const TCHAR* Assets[] = { TEXT("IS_Flowers2"), TEXT("IS_Balls"), TEXT("IS_SeaLife"), TEXT("IS_MythicalCreatures"), TEXT("IS_GardenVisitors"), TEXT("IS_MasterRewardGems") };
	const TCHAR* Names[] = { TEXT("Flowers"), TEXT("Balls"), TEXT("Sea Life"), TEXT("Mythical Creatures"), TEXT("Garden Visitors"), TEXT("Master Reward Gems") };
	for (int32 I = 0; I < UE_ARRAY_COUNT(Assets); ++I)
	{
		const FString Path = FString::Printf(TEXT("/Game/Happiness/IconSets/%s.%s"), Assets[I], Assets[I]);
		FIconSetEntry Entry;
		Entry.IconSet = LoadObject<UPrimaryDataAsset>(nullptr, *Path);
		Entry.Id = FName(Assets[I]);
		Entry.Name = FText::FromString(Names[I]);
		Entry.bUnlocked = I != 2 && I != 3;
		Entry.bEnabled = Entry.bUnlocked && I != 1;
		if (I == 2) Entry.ProductId = TEXT("preview_only");
		if (I == 3) Entry.LockedReason = LOCTEXT("PreviewLock", "Complete Campaign");
		PreviewEntries.Add(Entry);
	}
	SelectedId = PreviewEntries[FMath::Clamp(Index, 0, PreviewEntries.Num() - 1)].Id;
	Refresh();
}
#endif

#undef LOCTEXT_NAMESPACE
