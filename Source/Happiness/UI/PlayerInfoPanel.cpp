#include "UI/PlayerInfoPanel.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "HappinessClassic/CampaignProgress.h"
#include "HappinessClassic/CampaignTree.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UI/GameRulesWidget.h"
#include "UI/IconSetsPanel.h"
#include "UI/LessonPopupWidget.h"
#include "UI/PauseMenuWidget.h"
#include "UI/PlayerLevelBadge.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "PlayerInfo"

namespace
{
	// The pause menu's garden colors
	const FLinearColor InfoInk(.17f, .085f, .035f);
	const FLinearColor InfoSage(.16f, .29f, .075f);
	const FVector2D InfoWindowSize(1200.f, 600.f);
	constexpr float InfoRewardSize = 68.f;
	constexpr float InfoRewardGap = 10.f;
}

UPlayerInfoPanel::UPlayerInfoPanel(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> Panel(TEXT("/Game/General/Textures/T_PauseGardenPanel.T_PauseGardenPanel"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Buttons(TEXT("/Game/General/Textures/T_LessonNotebookButtons.T_LessonNotebookButtons"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> RewardArt(TEXT("/Game/General/Textures/T_MasterRewardGems.T_MasterRewardGems"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Daisy(TEXT("/Game/Happiness/IconSets/Icons/Flowers/Daisy.Daisy"));
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Accept(TEXT("/Game/General/Audio/Cues/MenuAccept.MenuAccept"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Cancel(TEXT("/Game/General/Audio/Cues/MenuCancel.MenuCancel"));
	PanelTexture = Panel.Object;
	ButtonTexture = Buttons.Object;
	RewardAtlasTexture = RewardArt.Object;
	IconSetsTexture = Daisy.Object;
	PanelFont = Font.Object;
	AcceptSound = Accept.Object;
	CancelSound = Cancel.Object;
	OptionsClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/Happiness/UI/WBP_Options.WBP_Options_C")));
	RulesClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/Happiness/UI/WBP_GameRules.WBP_GameRules_C")));
}

TSharedRef<SWidget> UPlayerInfoPanel::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UPlayerInfoPanel::BuildLayout()
{
	Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("InfoCanvas"));
	WidgetTree->RootWidget = Root;

	// The menu behind dims; a tap on it closes the panel
	UButton* Outside = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("OutsideButton"));
	FButtonStyle DimStyle;
	DimStyle.Normal = DimStyle.Hovered = DimStyle.Pressed = DimStyle.Disabled = FSlateRoundedBoxBrush(FLinearColor(0.f, 0.f, 0.f, .35f), 0.f);
	DimStyle.NormalPadding = DimStyle.PressedPadding = FMargin(0.f);
	Outside->SetStyle(DimStyle);
	Outside->OnClicked.AddUniqueDynamic(this, &UPlayerInfoPanel::HandleCloseClicked);
	UCanvasPanelSlot* OutsideSlot = Root->AddChildToCanvas(Outside);
	OutsideSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	OutsideSlot->SetOffsets(FMargin(0.f));

	// In the middle, as wide as it can be while keeping clear of the sun (top right) on either side
	UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>();
	Fit->SetStretch(EStretch::ScaleToFit);
	Fit->SetStretchDirection(EStretchDirection::DownOnly);
	Fit->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UCanvasPanelSlot* FitSlot = Root->AddChildToCanvas(Fit);
	FitSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	const float SunClearance = UPlayerLevelBadge::BadgeSize + 40.f;
	FitSlot->SetOffsets(FMargin(SunClearance, 60.f, SunClearance, 60.f));
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(InfoWindowSize.X);
	Size->SetHeightOverride(InfoWindowSize.Y);
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
	auto MakeText = [&](const FText& Text, int32 Points, const FLinearColor& Color)
	{
		UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>();
		Block->SetText(Text);
		Block->SetFont(FSlateFontInfo(PanelFont, Points));
		Block->SetColorAndOpacity(Color);
		Block->SetJustification(ETextJustify::Center);
		Block->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Block;
	};
	// The notebook buttons' ivory face (the pause menu's buttons)
	FSlateBrush Face;
	Face.DrawAs = ESlateBrushDrawType::Image;
	Face.SetResourceObject(ButtonTexture);
	Face.SetUVRegion(FBox2f(FVector2f(.03f, .06f), FVector2f(.98f, .485f)));

	// The window takes the taps on it, so they don't close it
	UImage* Surface = WidgetTree->ConstructWidget<UImage>();
	FSlateBrush SurfaceBrush;
	SurfaceBrush.SetResourceObject(PanelTexture);
	SurfaceBrush.DrawAs = ESlateBrushDrawType::Image;
	SurfaceBrush.SetUVRegion(FBox2f(FVector2f(.008f, .05f), FVector2f(.992f, .93f)));
	Surface->SetBrush(SurfaceBrush);
	Place(Surface, FVector2D::ZeroVector, InfoWindowSize);

	UImage* TitlePlate = WidgetTree->ConstructWidget<UImage>();
	TitlePlate->SetBrush(Face);
	TitlePlate->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(TitlePlate, FVector2D(390.f, 50.f), FVector2D(420.f, 96.f));
	Place(MakeText(LOCTEXT("Title", "Player Info"), 48, InfoInk), FVector2D(390.f, 66.f), FVector2D(420.f, 70.f));

	// Mastered Clues: the badge's rewards in a row, the earned ones in full
	Place(MakeText(LOCTEXT("MasteredClues", "Mastered Clues"), 36, InfoSage), FVector2D(100.f, 182.f), FVector2D(1000.f, 50.f));
	UImage* Tray = WidgetTree->ConstructWidget<UImage>();
	Tray->SetBrush(FSlateRoundedBoxBrush(FLinearColor(1.f, .97f, .86f, .65f), 26.f, FLinearColor(.78f, .6f, .25f, .9f), 2.f));
	Tray->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(Tray, FVector2D(110.f, 240.f), FVector2D(980.f, 104.f))->SetZOrder(1);
	UHorizontalBox* RewardRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	RewardRow->SetVisibility(ESlateVisibility::HitTestInvisible);
	Rewards.Reset();
	const int32 NumRewards = UCampaignTree::GetNumColumns();
	for (int32 Index = 0; Index < NumRewards; Index++)
	{
		UImage* Reward = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), *FString::Printf(TEXT("Reward%d"), Index));
		Reward->SetBrush(UPlayerLevelBadge::MakeRewardBrush(RewardAtlasTexture, Index, InfoRewardSize));
		UHorizontalBoxSlot* RewardSlot = RewardRow->AddChildToHorizontalBox(Reward);
		RewardSlot->SetVerticalAlignment(VAlign_Center);
		RewardSlot->SetPadding(FMargin(Index == 0 ? 0.f : InfoRewardGap, 0.f, 0.f, 0.f));
		Rewards.Add(Reward);
	}
	const float RowWidth = NumRewards * InfoRewardSize + FMath::Max(NumRewards - 1, 0) * InfoRewardGap;
	// Explicitly over the tray: the canvas doesn't keep the order of children at the same Z, and the tray's cream
	// over the rewards washed them out
	Place(RewardRow, FVector2D((InfoWindowSize.X - RowWidth) * .5f, 258.f), FVector2D(RowWidth, InfoRewardSize))->SetZOrder(2);

	UImage* Divider = WidgetTree->ConstructWidget<UImage>();
	Divider->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.44f, .28f, .1f, .35f), 0.f));
	Divider->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(Divider, FVector2D(110.f, 376.f), FVector2D(980.f, 1.f));

	// The three buttons, each with an icon in the pause menu's style
	auto MakeButton = [&](const TCHAR* Name, const FText& Label, UWidget* Icon, float X)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		FButtonStyle Style;
		Style.Normal = Style.Hovered = Style.Pressed = Style.Disabled = Face;
		Style.Hovered.TintColor = FLinearColor(1.08f, 1.08f, 1.04f);
		Style.Pressed.TintColor = FLinearColor(.83f, .88f, .78f);
		Style.Disabled.TintColor = FLinearColor(.77f, .76f, .68f);
		// Inside the leaves at the face's ends
		Style.NormalPadding = Style.PressedPadding = FMargin(44.f, 8.f);
		Button->SetStyle(Style);
		Button->SetCursor(EMouseCursor::Hand);
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>();
		IconSize->SetWidthOverride(40.f);
		IconSize->SetHeightOverride(40.f);
		Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		IconSize->AddChild(Icon);
		UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconSize);
		IconSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
		IconSlot->SetVerticalAlignment(VAlign_Center);
		Row->AddChildToHorizontalBox(MakeText(Label, 28, InfoInk))->SetVerticalAlignment(VAlign_Center);
		UButtonSlot* ContentSlot = CastChecked<UButtonSlot>(Button->AddChild(Row));
		ContentSlot->SetHorizontalAlignment(HAlign_Center);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
		Place(Button, FVector2D(X, 408.f), FVector2D(316.f, 106.f));
		return Button;
	};
	UPauseMenuGlyph* Gear = WidgetTree->ConstructWidget<UPauseMenuGlyph>();
	Gear->Symbol = 3;
	Gear->Color = InfoInk;
	OptionsButton = MakeButton(TEXT("OptionsButton"), LOCTEXT("Options", "Options"), Gear, 110.f);
	ULessonPopupGlyph* Book = WidgetTree->ConstructWidget<ULessonPopupGlyph>();
	Book->Symbol = 0;
	Book->Color = InfoInk;
	RulesButton = MakeButton(TEXT("RulesButton"), LOCTEXT("Rules", "Game Rules"), Book, 442.f);
	UImage* Flower = WidgetTree->ConstructWidget<UImage>();
	Flower->SetBrushFromTexture(IconSetsTexture);
	IconSetsButton = MakeButton(TEXT("IconSetsButton"), LOCTEXT("IconSets", "Icon Sets"), Flower, 774.f);
	OptionsButton->OnClicked.AddUniqueDynamic(this, &UPlayerInfoPanel::HandleOptionsClicked);
	RulesButton->OnClicked.AddUniqueDynamic(this, &UPlayerInfoPanel::HandleRulesClicked);
	IconSetsButton->OnClicked.AddUniqueDynamic(this, &UPlayerInfoPanel::HandleIconSetsClicked);

	// Close: a round ivory button in the top right corner
	CloseButton = MakeCloseButton(*WidgetTree);
	CloseButton->OnClicked.AddUniqueDynamic(this, &UPlayerInfoPanel::HandleCloseClicked);
	// Right of the corner's leaves, inside the frame
	Place(CloseButton, FVector2D(1088.f, 80.f), FVector2D(66.f, 66.f));

	Refresh();
}

UButton* UPlayerInfoPanel::MakeCloseButton(UWidgetTree& Tree, FName Name)
{
	UButton* Button = Tree.ConstructWidget<UButton>(UButton::StaticClass(), Name);
	FButtonStyle Style;
	Style.Normal = Style.Hovered = Style.Pressed = Style.Disabled =
		FSlateRoundedBoxBrush(FLinearColor(1.f, .97f, .86f), 33.f, FLinearColor(.78f, .6f, .25f), 3.f);
	Style.Hovered.TintColor = FLinearColor(1.08f, 1.08f, 1.04f);
	Style.Pressed.TintColor = FLinearColor(.83f, .88f, .78f);
	Style.NormalPadding = Style.PressedPadding = FMargin(0.f);
	Button->SetStyle(Style);
	Button->SetCursor(EMouseCursor::Hand);
	UPauseMenuGlyph* Cross = Tree.ConstructWidget<UPauseMenuGlyph>();
	Cross->Symbol = 9;
	Cross->Color = InfoInk;
	Cross->SetVisibility(ESlateVisibility::HitTestInvisible);
	USizeBox* CrossSize = Tree.ConstructWidget<USizeBox>();
	CrossSize->SetWidthOverride(30.f);
	CrossSize->SetHeightOverride(30.f);
	CrossSize->AddChild(Cross);
	UButtonSlot* CrossSlot = CastChecked<UButtonSlot>(Button->AddChild(CrossSize));
	CrossSlot->SetHorizontalAlignment(HAlign_Center);
	CrossSlot->SetVerticalAlignment(VAlign_Center);
	return Button;
}

void UPlayerInfoPanel::Refresh()
{
	const UCampaignSubsystem* Campaign = UCampaignSubsystem::Get(this);
	for (int32 Index = 0; Index < Rewards.Num(); Index++)
	{
		const bool bUnlocked = Campaign && Campaign->IsMasterColumnCompleted(Index);
		Rewards[Index]->SetRenderOpacity(bUnlocked ? 1.f : LockedRewardOpacity);
	}
}

void UPlayerInfoPanel::Show()
{
	Refresh();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UPlayerInfoPanel::Hide()
{
	if (!IsVisible())
	{
		return;
	}
	SetVisibility(ESlateVisibility::Collapsed);
	OnClosed.Broadcast();
}

void UPlayerInfoPanel::HandleCloseClicked()
{
	PlaySound(CancelSound);
	Hide();
}

UUserWidget* UPlayerInfoPanel::OpenScreen(TObjectPtr<UUserWidget>& Screen, UClass* Class)
{
	if (!Screen)
	{
		Screen = Class ? CreateWidget<UUserWidget>(this, Class) : nullptr;
		// Over the whole screen, in the menu above the raised badge (in the panel the sun would cover the screen's
		// buttons); closing it returns to the panel
		UCanvasPanel* Canvas = Cast<UCanvasPanel>(GetParent()) ? Cast<UCanvasPanel>(GetParent()) : Root.Get();
		if (UCanvasPanelSlot* ScreenSlot = Screen && Canvas ? Canvas->AddChildToCanvas(Screen) : nullptr)
		{
			ScreenSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			ScreenSlot->SetOffsets(FMargin(0.f));
			ScreenSlot->SetZOrder(300);
		}
	}
	if (Screen)
	{
		Screen->SetVisibility(ESlateVisibility::Visible);
	}
	return Screen;
}

void UPlayerInfoPanel::HandleOptionsClicked()
{
	PlaySound(AcceptSound);
	UUserWidget* Options = OpenScreen(OptionsWidget, OptionsClass.LoadSynchronous());
	// WBP_Options.Show(PC) loads the current settings into it (as from the pause menu)
	if (UFunction* ShowFunction = Options ? Options->FindFunction(TEXT("Show")) : nullptr)
	{
		uint8* Params = static_cast<uint8*>(FMemory_Alloca(ShowFunction->ParmsSize));
		FMemory::Memzero(Params, ShowFunction->ParmsSize);
		if (const FObjectPropertyBase* PCParam = CastField<FObjectPropertyBase>(ShowFunction->ChildProperties))
		{
			PCParam->SetObjectPropertyValue_InContainer(Params, GetOwningPlayer());
		}
		Options->ProcessEvent(ShowFunction, Params);
	}
}

void UPlayerInfoPanel::HandleRulesClicked()
{
	PlaySound(AcceptSound);
	if (UGameRulesWidget* Rules = Cast<UGameRulesWidget>(OpenScreen(RulesWidget, RulesClass.LoadSynchronous())))
	{
		Rules->Show(GetOwningPlayer());
	}
}

void UPlayerInfoPanel::HandleIconSetsClicked()
{
	PlaySound(AcceptSound);
	if (UIconSetsPanel* IconSets = Cast<UIconSetsPanel>(OpenScreen(IconSetsWidget, UIconSetsPanel::StaticClass())))
	{
		IconSets->Show();
	}
}

void UPlayerInfoPanel::PlaySound(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound, 1.f, 1.f, 0.f, nullptr, nullptr, true);
	}
}

#undef LOCTEXT_NAMESPACE
