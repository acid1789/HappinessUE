#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Containers/Ticker.h"
#include "HintSparkleIconWidget.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNativeWidgetHost;
class UNiagaraSystem;
class UNiagaraUIWidget;
class UTexture2D;
class UImage;
class SCanvas;

/**
 * Moves Niagara sparkle sprites around the highlighted icon or clue.
 *
 * A clue's sparkles orbit the whole clue and are run by its first icon (the first in its box). Clues sit in their panel's scroll box,
 * which stops painting, and so ticking, the clues scrolled out of view: those icons are run by a core ticker instead,
 * which scrolls the clue into view when its hint starts, follows it as the panel scrolls, and hides the sparkles
 * that leave the panel.
 *
 * When a hint starts, its sparkles stream in so the player's eye follows them: first out of the hint button to the clue,
 * then, once that ring has formed, off the clue's ring to the board icon. Each sparkle flies into its place in the
 * orbit.
 */
UCLASS(Abstract)
class HAPPINESS_API UHintSparkleIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UHintSparkleIconWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;
	virtual void BeginDestroy() override;

private:
	bool IsShowingHint() const;
	void HideOldOutline();
	/** The clue this icon is in (none for a board icon) */
	UUserWidget* FindClue() const;
	/** A clue's first icon, each frame: runs its clue's sparkles */
	bool TickClue(float DeltaTime);
	void StopTickers();
	/** A board icon's sparkles, each frame: stop them once the icon itself stops ticking (collapsed or hidden) */
	bool TickBoardWatch(float DeltaTime);
	/** Move and fade the sparkles around Geometry (in the clue's scroll box, whose area clips them, if Scroll) */
	void UpdateSparkles(float DeltaTime, const FGeometry& Geometry, bool bWholeClue, const FGeometry* Scroll = nullptr);
	void StartSparkles();
	void StopSparkles();
	void UpdateSparklePosition(const FGeometry& Geometry, bool bWholeClue);
	/** Geometry's rectangle in the sparkle canvas */
	bool ToSparkleSpace(const FGeometry& Geometry, FVector2D& OutTopLeft, FVector2D& OutBottomRight) const;
	/** Starts a sparkle; its index, or INDEX_NONE if all are in use */
	int32 SpawnSparkle();
	/** An icon of the cell dialog: the dialog is the board's cell up close, so its ring is there from the start */
	bool IsInCellDialog() const;
	/** Fill the ring at once, as many sparkles as it settles at, each partway through its life */
	void PrefillRing();
	FVector2D GetSparklePosition(int32 Index) const;
	/** Where Index's sparkle is in the orbit (its center) */
	FVector2D GetOrbitPosition(int32 Index) const;
	/** Where a streamed sparkle starts, in the sparkle canvas: by the hint button for a clue, on the clue's ring for a board icon (by the hint button if no clue has one) */
	bool GetStreamSource(FVector2D& OutFrom) const;
	float GetSparkleSize(int32 Index) const;
	UTexture2D* MakeSparkleTexture(const FLinearColor& Color);

	struct FOrbitSparkle
	{
		float Age = 0.f;
		float Lifetime = 0.f;
		float Phase = 0.f;
		float Speed = 0.f;
		float Scale = 0.f;
		float NormalOffset = 0.f;
		/** Streamed: seconds it flies from From (by the hint button) into the orbit, on a curve bent by Bend */
		float Travel = 0.f;
		FVector2D From = FVector2D::ZeroVector;
		float Bend = 0.f;
		bool bActive = false;
	};

	static constexpr int32 SparkleCount = 400;
	static constexpr float SpawnInterval = .008f;
	/** How long the stream leaves the hint button, how many sparkles leave per spawn, and their flight time */
	static constexpr float StreamSeconds = .45f;
	static constexpr int32 StreamSpawns = 3;
	static constexpr float TravelSeconds = .65f;
	/** A board icon's stream (from the clue) waits this long after the hint starts, for the clue's ring to form */
	static constexpr float CellStreamDelay = .85f;
	UPROPERTY() TObjectPtr<UNiagaraSystem> SparkleSystem;
	UPROPERTY() TObjectPtr<UMaterialInterface> SparkleMaterial;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> PurpleTexture;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> SilverTexture;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> PurpleMaterial;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> SilverMaterial;
	UPROPERTY(Transient) TArray<TObjectPtr<UNiagaraUIWidget>> Sparkles;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> SparkleImages;
	UPROPERTY(Transient) TObjectPtr<UNativeWidgetHost> SparkleHost;
	TSharedPtr<SCanvas> SparkleCanvas;
	TArray<FOrbitSparkle> SparkleStates;
	FBox2D SparkleRect = FBox2D(ForceInit);
	/** Sparkles outside this (in the sparkle canvas) are hidden; invalid: none are */
	FBox2D SparkleClip = FBox2D(ForceInit);
	/** A clue's first icon: its clue, and its ticker */
	TWeakObjectPtr<UUserWidget> TickedClue;
	FTSTicker::FDelegateHandle ClueTicker;
	/** A board icon with sparkles: its watch ticker, and the frame it last ticked */
	FTSTicker::FDelegateHandle BoardWatch;
	uint64 LastTickFrame = 0;
	/** Its clue has been scrolled into view for this hint */
	bool bClueScrolled = false;
	/** The hint button the stream comes from; how long it has been streaming; whether this hint has streamed */
	TWeakObjectPtr<UWidget> HintButton;
	float StreamTime = StreamSeconds;
	bool bStreamed = false;
	/** The clue icon whose sparkles are showing, for the board icon's stream to come from */
	static TWeakObjectPtr<UHintSparkleIconWidget> ShownClue;
	float OrbitTime = 0.f;
	float SpawnClock = 0.f;
	float SpriteSize = 20.f;
};
