#include "Ads/AdBannerSlot.h"

#include "Ads/AdsSubsystem.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

void UAdBannerSlot::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

#if WITH_EDITOR
void UAdBannerSlot::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	// The designer shows the banner's size on a typical phone
	if (IsDesignTime())
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
		{
			CanvasSlot->SetSize(GetDefault<UAdsSubsystem>()->GetBannerSizeDp() * DesignUnitsPerDp);
		}
	}
}
#endif

void UAdBannerSlot::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UAdsSubsystem* Ads = UAdsSubsystem::Get(this);
	const bool bAdsOn = Ads && Ads->AreAdsEnabled();
	const float UnitsPerDp = Ads ? Ads->GetUnitsPerDp(MyGeometry) : DesignUnitsPerDp;
	const FVector2D BannerSizeDp = Ads ? Ads->GetBannerSizeDp() : GetDefault<UAdsSubsystem>()->GetBannerSizeDp();
	if (!bLaidOut || bAdsOn != bLaidOutWithAds || !FMath::IsNearlyEqual(UnitsPerDp, LaidOutUnitsPerDp, 0.01f) ||
		BannerSizeDp != LaidOutBannerSizeDp)
	{
		LaidOutBannerSizeDp = BannerSizeDp;
		ApplyLayout(bAdsOn, UnitsPerDp);
		return; // place the banner once the new layout has been arranged
	}

	if (!bAdsOn || IsCovered())
	{
		if (Ads)
		{
			Ads->HideBanner();
		}
	}
	else
	{
		// The banner hangs from the slot's top left corner, in viewport pixels
		FVector2D PixelPosition, ViewportPosition;
		USlateBlueprintLibrary::LocalToViewport(this, MyGeometry, FVector2D::ZeroVector, PixelPosition, ViewportPosition);
		Ads->RequestBanner(PixelPosition);
	}
}

void UAdBannerSlot::ApplyLayout(bool bAdsOn, float UnitsPerDp)
{
	bLaidOut = true;
	bLaidOutWithAds = bAdsOn;
	LaidOutUnitsPerDp = UnitsPerDp;

	const FVector2D BannerSize = LaidOutBannerSizeDp * UnitsPerDp;
	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot);
	if (!CanvasSlot)
	{
		return;
	}
	CanvasSlot->SetSize(BannerSize);

	// The banner's left and top edges, as distances from the canvas's right and bottom edges (anchored bottom right:
	// the offsets place the alignment point, which is the banner's bottom right corner)
	const FMargin Offsets = CanvasSlot->GetOffsets();
	const FVector2D Alignment = CanvasSlot->GetAlignment();
	const float Gap = GapDp * UnitsPerDp;
	const float LeftEdge = -Offsets.Left + BannerSize.X * Alignment.X;
	const float TopEdge = -Offsets.Top + BannerSize.Y * Alignment.Y;
	for (const FName& Name : MakeRoomLeft)
	{
		MakeRoom(Name, true, bAdsOn ? LeftEdge + Gap : -1.f);
	}
	for (const FName& Name : MakeRoomAbove)
	{
		MakeRoom(Name, false, bAdsOn ? TopEdge + Gap : -1.f);
	}
}

void UAdBannerSlot::MakeRoom(FName Name, bool bRight, float Margin)
{
	UWidget* Panel = FindSibling(Name);
	UCanvasPanelSlot* PanelSlot = Panel ? Cast<UCanvasPanelSlot>(Panel->Slot) : nullptr;
	if (!PanelSlot)
	{
		return;
	}
	// Stretched panels: Right and Bottom are the margins from the canvas's right and bottom edges
	FMargin Offsets = PanelSlot->GetOffsets();
	float& Edge = bRight ? Offsets.Right : Offsets.Bottom;
	const float Designed = DesignedOffsets.FindOrAdd(Name, Edge);
	Edge = Margin >= 0.f ? FMath::Max(Designed, Margin) : Designed;
	PanelSlot->SetOffsets(Offsets);
}

UWidget* UAdBannerSlot::FindSibling(FName Name) const
{
	if (const UPanelWidget* Parent = GetParent())
	{
		for (UWidget* Child : Parent->GetAllChildren())
		{
			if (Child && Child->GetFName() == Name)
			{
				return Child;
			}
		}
	}
	return nullptr;
}

bool UAdBannerSlot::IsCovered() const
{
	for (const FName& Name : CoveredBy)
	{
		if (const UWidget* Overlay = FindSibling(Name))
		{
			if (Overlay->IsVisible())
			{
				return true;
			}
		}
	}
	return false;
}

int32 UAdBannerSlot::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	// In the designer: an outline where the banner goes (in the game the banner itself is drawn by Android)
	if (IsDesignTime())
	{
		const FVector2D Size = AllottedGeometry.GetLocalSize();
		const FLinearColor Color(0.1f, 1.f, 0.2f);
		const TArray<FVector2D> Outline = { FVector2D(0, 0), FVector2D(Size.X, 0), Size, FVector2D(0, Size.Y), FVector2D(0, 0) };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Outline,
			ESlateDrawEffect::None, Color, true, 3.f);
		const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", 16);
		const FVector2D SizeDp = GetDefault<UAdsSubsystem>()->GetBannerSizeDp();
		FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2D(8, 4))),
			FString::Printf(TEXT("Banner ad %.0fx%.0f"), SizeDp.X, SizeDp.Y), Font, ESlateDrawEffect::None, Color);
		LayerId += 1;
	}
	return LayerId;
}
