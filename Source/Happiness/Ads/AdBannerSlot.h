#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AdBannerSlot.generated.h"

/**
 * Where the puzzle screen's banner ad goes. Place it in the screen's canvas anchored to the bottom right corner
 * (anchor and alignment 1,1; its offsets are the margins from that corner); it sets its own size to the banner's.
 *
 * While ads are on it makes room: the panels in MakeRoomLeft end left of the banner, and the panels in MakeRoomAbove
 * end above it (each with a gap). With ads off they get their designed layout back. The banner itself is a native
 * view drawn over the game (UAdsSubsystem::RequestBanner): it shows while this slot is on screen and none of the
 * overlays in CoveredBy is up.
 */
UCLASS()
class HAPPINESS_API UAdBannerSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Sibling panels (stretched across) whose right edge moves left of the banner while ads are on */
	UPROPERTY(EditAnywhere, Category = "Banner")
	TArray<FName> MakeRoomLeft = { TEXT("VerticalCluePanel"), TEXT("HintInfo") };

	/** Sibling panels (stretched down) whose bottom edge moves above the banner while ads are on */
	UPROPERTY(EditAnywhere, Category = "Banner")
	TArray<FName> MakeRoomAbove = { TEXT("HorizontalCluePanel") };

	/** Sibling overlays that cover the banner's spot: the banner hides while one of them is showing */
	UPROPERTY(EditAnywhere, Category = "Banner")
	TArray<FName> CoveredBy = { TEXT("EndScreen"), TEXT("LessonEndScreen"), TEXT("CampaignEndScreen"), TEXT("PauseMenu"),
		TEXT("CellDialog"), TEXT("ConfirmDialog") };

	/** Space between the banner and the panels that make room for it, in dp */
	UPROPERTY(EditAnywhere, Category = "Banner")
	float GapDp = 8.f;

	/** UMG units per dp in the designer, where there's no screen to measure (a typical phone's) */
	UPROPERTY(EditAnywhere, Category = "Banner")
	float DesignUnitsPerDp = 2.f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
#if WITH_EDITOR
	virtual void SynchronizeProperties() override;
#endif

private:
	/** Size the slot and move the panels for ads on or off */
	void ApplyLayout(bool bAdsOn, float UnitsPerDp);
	UWidget* FindSibling(FName Name) const;
	bool IsCovered() const;

	/** Move one panel's right or bottom edge to Margin from the canvas edge (with ads on), or back to its designed place */
	void MakeRoom(FName Name, bool bRight, float Margin);

	/** The panels' designed right or bottom offsets, before any room was made */
	TMap<FName, float> DesignedOffsets;
	bool bLaidOut = false;
	bool bLaidOutWithAds = false;
	float LaidOutUnitsPerDp = 0.f;
	FVector2D LaidOutBannerSizeDp = FVector2D::ZeroVector;
};
