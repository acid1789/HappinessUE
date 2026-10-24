#include "UI/PlayerLevelBadge.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "HappinessClassic/CampaignProgress.h"
#include "HappinessClassic/CampaignTree.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor SunGold(1.f, 0.78f, 0.3f, 1.f);
	const FLinearColor SunRim(0.85f, 0.55f, 0.15f, 1.f);
	const FLinearColor PlayerLevelInk(0.10f, 0.25f, 0.10f, 1.f);
	constexpr float SunSize = 144.f;
	constexpr float IconSize = 60.f;
	constexpr float RewardRadius = 114.f;

	/** A circle: a rounded box with corners of half its size */
	FSlateBrush MakeDisc(const FLinearColor& Fill, const FLinearColor& Rim, float Size)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.ImageSize = FVector2D(Size);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(Size * 0.5f, Size * 0.5f, Size * 0.5f, Size * 0.5f), FSlateColor(Rim), 3.f);
		return Brush;
	}
}

UPlayerLevelBadge::UPlayerLevelBadge(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> SunArt(TEXT("/Game/General/Textures/T_PlayerLevelSun.T_PlayerLevelSun"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> RewardArt(TEXT("/Game/General/Textures/T_MasterRewardGems.T_MasterRewardGems"));
	SunTexture = SunArt.Object;
	RewardAtlasTexture = RewardArt.Object;
}

TSharedRef<SWidget> UPlayerLevelBadge::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UPlayerLevelBadge::BuildLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BadgeCanvas"));
	WidgetTree->RootWidget = Root;
	const FVector2D Center(BadgeSize * 0.5f);

	UImage* Sun = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Sun"));
	if (SunTexture)
	{
		Sun->SetBrushFromTexture(SunTexture);
	}
	else
	{
		Sun->SetBrush(MakeDisc(SunGold, SunRim, SunSize));
	}
	if (UCanvasPanelSlot* SunSlot = Root->AddChildToCanvas(Sun))
	{
		SunSlot->SetAlignment(FVector2D(0.5f));
		SunSlot->SetPosition(Center);
		SunSlot->SetSize(FVector2D(SunSize));
	}

	LevelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LevelText"));
	if (const UFont* Font = LoadObject<UFont>(nullptr, TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font")))
	{
		LevelText->SetFont(FSlateFontInfo(Font, 50));
	}
	LevelText->SetColorAndOpacity(FSlateColor(PlayerLevelInk));
	LevelText->SetJustification(ETextJustify::Center);
	if (UCanvasPanelSlot* TextSlot = Root->AddChildToCanvas(LevelText))
	{
		TextSlot->SetAlignment(FVector2D(0.5f));
		TextSlot->SetPosition(Center);
		TextSlot->SetAutoSize(true);
	}

	// Clockwise from the top, between the sun's rim and the badge's edge
	const int32 NumIcons = UCampaignTree::GetNumColumns();
	Icons.Reset();
	for (int32 Index = 0; Index < NumIcons; Index++)
	{
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), *FString::Printf(TEXT("Icon%d"), Index));
		if (IconTextures.IsValidIndex(Index) && IconTextures[Index])
		{
			Icon->SetBrushFromTexture(IconTextures[Index]);
		}
		else if (RewardAtlasTexture && Index < 12)
		{
			const int32 Column = Index % 4;
			const int32 Row = Index / 4;
			constexpr float Inset = 0.02f;
			FSlateBrush Brush;
			Brush.SetResourceObject(RewardAtlasTexture);
			Brush.DrawAs = ESlateBrushDrawType::Image;
			Brush.ImageSize = FVector2D(IconSize);
			Brush.SetUVRegion(FBox2f(
				FVector2f((Column + Inset) / 4.f, (Row + Inset) / 3.f),
				FVector2f((Column + 1.f - Inset) / 4.f, (Row + 1.f - Inset) / 3.f)));
			Icon->SetBrush(Brush);
		}
		else
		{
			Icon->SetBrush(MakeDisc(FLinearColor(1.f, 0.93f, 0.7f, 1.f), SunRim, IconSize));
		}
		const float Angle = 2.f * PI * Index / NumIcons;
		if (UCanvasPanelSlot* IconSlot = Root->AddChildToCanvas(Icon))
		{
			IconSlot->SetAlignment(FVector2D(0.5f));
			IconSlot->SetPosition(Center + RewardRadius * FVector2D(FMath::Sin(Angle), -FMath::Cos(Angle)));
			IconSlot->SetSize(FVector2D(IconSize));
		}
		Icons.Add(Icon);
	}
	Refresh();
}

void UPlayerLevelBadge::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCampaignSubsystem* Campaign = UCampaignSubsystem::Get(this))
	{
		Campaign->OnProgressChanged.AddUniqueDynamic(this, &UPlayerLevelBadge::HandleProgressChanged);
	}
	Refresh();
}

void UPlayerLevelBadge::NativeDestruct()
{
	if (UCampaignSubsystem* Campaign = UCampaignSubsystem::Get(this))
	{
		Campaign->OnProgressChanged.RemoveDynamic(this, &UPlayerLevelBadge::HandleProgressChanged);
	}
	Super::NativeDestruct();
}

void UPlayerLevelBadge::HandleProgressChanged()
{
	Refresh();
}

void UPlayerLevelBadge::Refresh()
{
	if (LevelText)
	{
		// The level lives on the player controller (PC_Happiness), which loads and saves it
		const APlayerController* PC = GetOwningPlayer();
		const FIntProperty* LevelProperty = PC ? FindFProperty<FIntProperty>(PC->GetClass(), TEXT("HappinessLevel")) : nullptr;
		const int32 Level = LevelProperty ? LevelProperty->GetPropertyValue_InContainer(PC) : 1;
		LevelText->SetText(FText::AsNumber(Level));
	}
	const UCampaignSubsystem* Campaign = UCampaignSubsystem::Get(this);
	for (int32 Index = 0; Index < Icons.Num(); Index++)
	{
		const bool bUnlocked = Campaign && Campaign->IsMasterColumnCompleted(Index);
		Icons[Index]->SetVisibility(bUnlocked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}
