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
#include "UI/LessonPopupWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "UI/CampaignTreeWidget.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"
#include "Layout/ArrangedChildren.h"

namespace
{
	const FLinearColor SunGold(1.f, 0.78f, 0.3f, 1.f);
	const FLinearColor SunRim(0.85f, 0.55f, 0.15f, 1.f);
	const FLinearColor PlayerLevelInk(0.10f, 0.25f, 0.10f, 1.f);
	constexpr float SunSize = 144.f;
	// The twelve rewards ring the sun just inside its ray tips (72 from its center), as close as they go without
	// overlapping: neighbors are 2 * RewardRadius * sin(15 deg) apart, and the art fills about 96% of its icon
	constexpr float IconSize = 50.f;
	constexpr float RewardRadius = 92.f;

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

void UPlayerLevelBadge::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// Transparent rather than hidden, so it keeps ticking and comes back when the puzzle screen goes
	const bool bNowCovered = CoveredBy.IsValid() && CoveredBy->IsVisible();
	if (bNowCovered != bCovered)
	{
		bCovered = bNowCovered;
		SetRenderOpacity(bCovered ? 0.f : 1.f);
		if (!bCovered)
		{
			Refresh();
		}
	}
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
	const bool bRewardsInitialized = KnownRewards.Num() == Icons.Num();
	KnownRewards.SetNum(Icons.Num());
	for (int32 Index = 0; Index < Icons.Num(); Index++)
	{
		const bool bUnlocked = Campaign && Campaign->IsMasterColumnCompleted(Index);
		if (bRewardsInitialized && bUnlocked && !KnownRewards[Index]) PendingRewards.AddUnique(Index);
		KnownRewards[Index] = bUnlocked;
		Icons[Index]->SetVisibility((bUnlocked || Index == PreviewRewardIndex) && !PendingRewards.Contains(Index) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UPlayerLevelBadge::PreviewMasterReward(int32 Column)
{
	PendingRewards.Reset();
	PreviewRewardIndex = Column;
	PendingRewards.Add(Column);
	Refresh();
}

#if !UE_BUILD_SHIPPING
static FAutoConsoleCommandWithWorldAndArgs GPreviewMasterReward(
	TEXT("Happiness.PreviewMasterReward"),
	TEXT("Happiness.PreviewMasterReward [column 1-12]: opens the completed Master popup; press Return to Campaign to collapse it into its gem. Menu only; never changes saved progress."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const int32 Column = Args.IsEmpty() ? 0 : FCString::Atoi(*Args[0]) - 1;
		if (Column < 0 || Column >= UCampaignTree::GetNumColumns())
		{
			UE_LOG(LogTemp, Warning, TEXT("Use Happiness.PreviewMasterReward [column 1-12]"));
			return;
		}
		TArray<UUserWidget*> Trees, Badges;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Trees, UCampaignTreeWidget::StaticClass(), false);
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Badges, UPlayerLevelBadge::StaticClass(), false);
		if (Trees.IsEmpty() || Badges.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("Open the main Happiness menu before previewing a Master reward."));
			return;
		}
		UCampaignTreeWidget* Tree = CastChecked<UCampaignTreeWidget>(Trees[0]);
		if (!Tree->LessonPopup) return;
		Tree->SetVisibility(ESlateVisibility::Visible);
		Tree->LessonPopup->PreviewMasterCompletion(UCampaignTree::GetColumnLessons(Column)[0]);
		CastChecked<UPlayerLevelBadge>(Badges[0])->PreviewMasterReward(Column);
		if (Args.Num() > 1 && Args[1].Equals(TEXT("close"), ESearchCase::IgnoreCase))
		{
			FTimerHandle Timer;
			TWeakObjectPtr<ULessonPopupWidget> Popup = Tree->LessonPopup.Get();
			World->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateLambda([Popup]
			{
				if (Popup.IsValid()) Popup->PreviewMasterReturn();
			}), .3f, false);
		}
		UE_LOG(LogTemp, Display, TEXT("Previewing Master reward for column %d (no saved progress changed)."), Column + 1);
	}));
#endif

bool UPlayerLevelBadge::GetPendingMasterReward(int32 Column, FGeometry& OutGeometry) const
{
	if (!PendingRewards.Contains(Column) || !Icons.IsValidIndex(Column) || bCovered) return false;
	// Arrange the actual Slate canvas, including its hidden children. This uses exactly the same
	// anchors, DPI, parent transform and pixel snapping as the gem's eventual paint, without
	// relying on a hidden image's old geometry or reconstructing its placement from UMG slots.
	const TSharedPtr<SWidget> Canvas = GetRootWidget() ? GetRootWidget()->GetCachedWidget() : nullptr;
	const TSharedPtr<SWidget> Icon = Icons[Column]->GetCachedWidget();
	if (!Canvas || !Icon) return false;
	const FGeometry& CanvasGeometry = Canvas->GetPaintSpaceGeometry();
	if (CanvasGeometry.GetLocalSize().IsNearlyZero()) return false;
	FArrangedChildren Children(EVisibility::All);
	Canvas->ArrangeChildren(CanvasGeometry, Children);
	for (int32 Index = 0; Index < Children.Num(); ++Index)
	{
		if (Children[Index].Widget == Icon.ToSharedRef())
		{
			OutGeometry = Children[Index].Geometry;
			return true;
		}
	}
	return false;
}

void UPlayerLevelBadge::RevealMasterReward(int32 Column)
{
#if !UE_BUILD_SHIPPING
	FGeometry TargetGeometry;
	const bool bCheckAnchor = GetPendingMasterReward(Column, TargetGeometry);
	const FVector2D Expected = TargetGeometry.LocalToAbsolute(TargetGeometry.GetLocalSize() * .5f);
#endif
	PendingRewards.Remove(Column);
	Refresh();
#if !UE_BUILD_SHIPPING
	if (bCheckAnchor && GetWorld())
	{
		FTimerHandle Timer;
		TWeakObjectPtr<UPlayerLevelBadge> WeakThis(this);
		GetWorld()->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateLambda([WeakThis, Column, Expected]
		{
			if (!WeakThis.IsValid() || !WeakThis->Icons.IsValidIndex(Column)) return;
			const FGeometry& Painted = WeakThis->Icons[Column]->GetPaintSpaceGeometry();
			const FVector2D Actual = Painted.LocalToAbsolute(Painted.GetLocalSize() * .5f);
			UE_LOG(LogTemp, Display, TEXT("Master reward %d anchor: target=%s painted=%s error=%.2f"),
				Column + 1, *Expected.ToString(), *Actual.ToString(), FVector2D::Distance(Expected, Actual));
		}), .15f, false);
	}
#endif
}
