#include "UI/MenuTipsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "HappinessClassic/IconSets.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "MenuTips"

namespace
{
	// The clue description's ink (UClueDescriptionWidget), on the same frame
	const FLinearColor TipsInk(.17f, .085f, .035f);
	constexpr int32 TipsPoints = 26;
	constexpr float TipsIconSize = 52.f;
	// Longer tips wrap onto a second line here (the widget is a little wider)
	constexpr float TipsWrapWidth = 680.f;

	/** The written tips */
	TArray<FText> GetWrittenTips()
	{
		return {
			LOCTEXT("HideClue", "When you are done with a clue, you can hide it with the Hide Clue button."),
			LOCTEXT("UndoHide", "Hid a clue by mistake? The Undo button brings it back."),
			LOCTEXT("UnhideAll", "You can bring back every hidden clue with Unhide Clues in the Pause menu."),
			LOCTEXT("Hint", "Stuck? The Hint button finds a useful clue and explains why it helps."),
			LOCTEXT("Horizontal", "Horizontal clues compare columns left to right, no matter which row each icon is in."),
			LOCTEXT("Vertical", "Vertical clues describe icons that share a column."),
			LOCTEXT("Rows", "Each row is one category. Every icon in it belongs in exactly one column."),
			LOCTEXT("Campaign", "Finish every lesson to unlock Campaign mode."),
			LOCTEXT("Master", "Complete a clue's Campaign final to unlock its Master Mode."),
			LOCTEXT("Daily", "There is a new Daily Puzzle every day. Play each day to build your streak."),
			LOCTEXT("StreakSaver", "Missed a day? A Streak Saver keeps your daily streak going."),
			LOCTEXT("DailyPuzzleClues", "Daily Puzzles only use clues you have learned in the Lessons."),
			LOCTEXT("FixPuzzle", "When you discover a mistake, Fix Puzzle in the Pause menu can help."),
			LOCTEXT("PlayerInfo", "Access the Player Info by tapping on the Sun in the top right of the main screen."),
			LOCTEXT("GameRules", "You can access Game Rules from the Pause menu or Player Info."),
			LOCTEXT("IconSets", "You can choose which icon sets get used in puzzles in the Icon Sets."),
			LOCTEXT("HintButton", "The Hint button animation can be disabled in the options."),
			LOCTEXT("FreePlayIcons", "You can choose which clues are used on a puzzle in Free Play."),
			LOCTEXT("ClueInfo", "Tapping on a clue shows a description of what the clue means."),
		};
	}
}

UMenuTipsWidget::UMenuTipsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// The frame of the clue description on the puzzle screen (UClueDescriptionWidget's PanelBrush)
	static ConstructorHelpers::FObjectFinder<UTexture2D> Panel(TEXT("/Game/Happiness/UI/Textures/T_HintReadingPanel.T_HintReadingPanel"));
	FrameTexture = Panel.Object;
}

TSharedRef<SWidget> UMenuTipsWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UMenuTipsWidget::BuildLayout()
{
	TipFont = LoadObject<UFont>(nullptr, TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));

	// The frame fills the widget, so it keeps its size from tip to tip. Centered in it: the icon (icon tips only),
	// then the text.
	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TipsFrame"));
	FSlateBrush Frame;
	Frame.SetResourceObject(FrameTexture);
	Frame.ImageType = ESlateBrushImageType::FullColor;
	Frame.ImageSize = FVector2D(256, 128);
	Frame.DrawAs = ESlateBrushDrawType::Box;
	Frame.Margin = FMargin(.07f, .14f);
	Root->SetBrush(Frame);
	Root->SetPadding(FMargin(20.f, 8.f));
	Root->SetHorizontalAlignment(HAlign_Center);
	Root->SetVerticalAlignment(VAlign_Center);
	Root->SetVisibility(ESlateVisibility::HitTestInvisible);
	WidgetTree->RootWidget = Root;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	Root->SetContent(Row);
	Content = Row;

	IconBox = WidgetTree->ConstructWidget<USizeBox>();
	IconBox->SetWidthOverride(TipsIconSize);
	IconBox->SetHeightOverride(TipsIconSize);
	Icon = WidgetTree->ConstructWidget<UImage>();
	IconBox->AddChild(Icon);
	if (UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconBox))
	{
		IconSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
		IconSlot->SetVerticalAlignment(VAlign_Center);
	}

	TipText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TipText"));
	TipText->SetFont(FSlateFontInfo(TipFont, TipsPoints));
	TipText->SetColorAndOpacity(FSlateColor(TipsInk));
	TipText->SetJustification(ETextJustify::Center);
	TipText->SetWrapTextAt(TipsWrapWidth);
	if (UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(TipText))
	{
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UMenuTipsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TipTime = 0.f;
	bSwapped = false;
	ShowNextTip();
}

void UMenuTipsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!Content || TipSeconds <= 0.f)
	{
		return;
	}

	// Fade in, show, fade out; the next tip comes in at the end of the fade out
	TipTime += InDeltaTime;
	if (TipTime >= TipSeconds)
	{
		TipTime = FMath::Fmod(TipTime, TipSeconds);
		ShowNextTip();
	}
	const float Fade = FMath::Max(FadeSeconds, KINDA_SMALL_NUMBER);
	const float Opacity = FMath::Clamp(FMath::Min(TipTime / Fade, (TipSeconds - TipTime) / Fade), 0.f, 1.f);
	Content->SetRenderOpacity(Opacity);
}

void UMenuTipsWidget::ShowNextTip()
{
	if (!TipText || !IconBox || !Icon)
	{
		return;
	}

	UTexture2D* Texture = nullptr;
	FText IconName;
	if (FMath::FRand() < IconTipChance && PickIcon(Texture, IconName))
	{
		// "[Cosmos] - Cosmos"
		Icon->SetBrushFromTexture(Texture);
		IconBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		TipText->SetText(FText::Format(LOCTEXT("IconTip", "- {0}"), IconName));
		LastTip = INDEX_NONE;
		return;
	}

	const TArray<FText> Tips = GetWrittenTips();
	int32 Index = FMath::RandRange(0, Tips.Num() - 1);
	if (Tips.Num() > 1 && Index == LastTip)
	{
		Index = (Index + 1 + FMath::RandRange(0, Tips.Num() - 2)) % Tips.Num();
	}
	LastTip = Index;
	IconBox->SetVisibility(ESlateVisibility::Collapsed);
	TipText->SetText(Tips[Index]);
}

bool UMenuTipsWidget::PickIcon(UTexture2D*& OutTexture, FText& OutName) const
{
	const UIconSetSubsystem* IconSets = UIconSetSubsystem::Get(this);
	if (!IconSets)
	{
		return false;
	}
	TArray<UTexture2D*> Textures;
	for (const FIconSetEntry& Entry : IconSets->GetEntries())
	{
		if (Entry.bEnabled && Entry.IconSet)
		{
			for (UTexture2D* Texture : UIconSetSubsystem::GetIconTextures(Entry.IconSet))
			{
				if (Texture)
				{
					Textures.Add(Texture);
				}
			}
		}
	}
	if (Textures.Num() == 0)
	{
		return false;
	}
	OutTexture = Textures[FMath::RandRange(0, Textures.Num() - 1)];
	// The texture is named for the icon (its Id), many with the texture prefix: "T_BlueWildFlower" -> "Blue Wild Flower"
	FString Name = OutTexture->GetName();
	Name.RemoveFromStart(TEXT("T_"));
	OutName = FText::FromString(FName::NameToDisplayString(Name, false));
	return true;
}

#undef LOCTEXT_NAMESPACE
