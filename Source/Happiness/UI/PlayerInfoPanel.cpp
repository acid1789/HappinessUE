#include "UI/PlayerInfoPanel.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
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
#include "UI/PauseMenuWidget.h"
#include "UI/PlayerLevelBadge.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "PlayerInfo"

namespace
{
	// The pause menu's garden colors
	const FLinearColor InfoInk(.17f, .085f, .035f);
	const FVector2D InfoWindowSize(1500.f, 750.f);
	constexpr float InfoRewardSize = 116.f;
}

UPlayerInfoPanel::UPlayerInfoPanel(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> Panel(TEXT("/Game/General/Textures/T_PlayerInfoCabinet.T_PlayerInfoCabinet"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> RewardArt(TEXT("/Game/General/Textures/T_MasterRewardGems.T_MasterRewardGems"));
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Accept(TEXT("/Game/General/Audio/Cues/MenuAccept.MenuAccept"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Cancel(TEXT("/Game/General/Audio/Cues/MenuCancel.MenuCancel"));
	PanelTexture = Panel.Object;
	RewardAtlasTexture = RewardArt.Object;
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
	// Artwork is tightly cropped. Coordinates follow its two-row cabinet and three button faces.
	UImage* Surface = WidgetTree->ConstructWidget<UImage>();
	Surface->SetBrushFromTexture(PanelTexture);
	Place(Surface, FVector2D::ZeroVector, InfoWindowSize)->SetZOrder(0);

	Place(MakeText(LOCTEXT("Title", "Player Info"), 52, InfoInk),
		FVector2D(565.f, 54.f), FVector2D(370.f, 82.f))->SetZOrder(1);
	Place(MakeText(LOCTEXT("MasteredClues", "Mastered Clues"), 34, InfoInk),
		FVector2D(285.f, 247.f), FVector2D(440.f, 58.f))->SetZOrder(2);

	Rewards.Reset();
	constexpr float RewardCentersX[] = {146.f, 284.f, 423.f, 561.f, 700.f, 838.f};
	constexpr float RewardCentersY[] = {405.f, 553.f};
	const int32 NumRewards = UCampaignTree::GetNumColumns();
	for (int32 Index = 0; Index < NumRewards; ++Index)
	{
		UImage* Reward = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),
			*FString::Printf(TEXT("Reward%d"), Index));
		Reward->SetBrush(UPlayerLevelBadge::MakeRewardBrush(RewardAtlasTexture, Index, InfoRewardSize));
		Reward->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Reward, FVector2D(RewardCentersX[Index % 6], RewardCentersY[Index / 6]) - FVector2D(InfoRewardSize * .5f),
			FVector2D(InfoRewardSize))->SetZOrder(3);
		Rewards.Add(Reward);
	}

	// Transparent hit areas cover the painted faces, with a soft hover/press response.
	auto MakeButton = [&](const TCHAR* Name, const FText& Label, float Y, float LabelX, float LabelWidth, int32 LabelPoints)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		FButtonStyle Style;
		Style.Normal = Style.Disabled = FSlateRoundedBoxBrush(FLinearColor::Transparent, 20.f);
		Style.Hovered = FSlateRoundedBoxBrush(FLinearColor(1.f, 1.f, .9f, .10f), 20.f);
		Style.Pressed = FSlateRoundedBoxBrush(FLinearColor(.12f, .18f, .06f, .16f), 20.f);
		Style.NormalPadding = Style.PressedPadding = FMargin(0.f);
		Button->SetStyle(Style);
		Button->SetCursor(EMouseCursor::Hand);
		Place(Button, FVector2D(991.f, Y), FVector2D(455.f, 137.f))->SetZOrder(4);
		Place(MakeText(Label, LabelPoints, InfoInk), FVector2D(LabelX, Y + 43.f), FVector2D(LabelWidth, 55.f))->SetZOrder(5);
		return Button;
	};
	OptionsButton = MakeButton(TEXT("OptionsButton"), LOCTEXT("Options", "Options"), 214.f, 1180.f, 205.f, 31);
	RulesButton = MakeButton(TEXT("RulesButton"), LOCTEXT("Rules", "Game Rules"), 383.f, 1248.f, 175.f, 26);
	IconSetsButton = MakeButton(TEXT("IconSetsButton"), LOCTEXT("IconSets", "Icon Sets"), 552.f, 1260.f, 145.f, 26);
	OptionsButton->OnClicked.AddUniqueDynamic(this, &UPlayerInfoPanel::HandleOptionsClicked);
	RulesButton->OnClicked.AddUniqueDynamic(this, &UPlayerInfoPanel::HandleRulesClicked);
	IconSetsButton->OnClicked.AddUniqueDynamic(this, &UPlayerInfoPanel::HandleIconSetsClicked);

	CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
	FButtonStyle CloseStyle;
	CloseStyle.Normal = CloseStyle.Disabled = FSlateRoundedBoxBrush(FLinearColor::Transparent, 55.f);
	CloseStyle.Hovered = FSlateRoundedBoxBrush(FLinearColor(1.f, 1.f, .9f, .10f), 55.f);
	CloseStyle.Pressed = FSlateRoundedBoxBrush(FLinearColor(.12f, .18f, .06f, .16f), 55.f);
	CloseStyle.NormalPadding = CloseStyle.PressedPadding = FMargin(0.f);
	CloseButton->SetStyle(CloseStyle);
	CloseButton->SetCursor(EMouseCursor::Hand);
	CloseButton->OnClicked.AddUniqueDynamic(this, &UPlayerInfoPanel::HandleCloseClicked);
	Place(CloseButton, FVector2D(1344.f, 79.f), FVector2D(113.f))->SetZOrder(4);

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
