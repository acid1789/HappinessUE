#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AdsSubsystem.generated.h"

/**
 * Ads through the PloxTools AdMob plugin: full-screen (interstitial) ads between puzzles and a banner on the puzzle
 * screen. At startup it asks for consent where the law requires it (Google's UMP form), initializes AdMob and loads
 * both ads. Ads never stop the game: with nothing to show (no ad loaded, no consent, no network, or not on a phone)
 * everything carries on without them.
 *
 * - Interstitials: every end-of-puzzle screen's buttons (restart, exit, next) play one, then do their thing
 *   (ShowInterstitialThen).
 * - Banner: a native Android/iOS view drawn over the game where the puzzle screen's UAdBannerSlot is. The slot asks
 *   for it every frame it wants it (RequestBanner); when the requests stop (screen closed, overlay up) it hides.
 * - On/off: AreAdsEnabled, saved with the game. Off: no interstitials, no banner, and the puzzle screen's panels use
 *   the banner's space. "Happiness.Ads 0/1" in dev builds.
 *
 * Ad units: Google's test units unless InterstitialAdUnitID / BannerAdUnitID are set ([/Script/Happiness.AdsSubsystem]
 * in DefaultGame.ini), and always the test units outside Shipping builds, so development never shows live ads.
 */
UCLASS(Config = Game)
class HAPPINESS_API UAdsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	/** The game's own interstitial ad unit (from AdMob); used in Shipping builds only. Empty: Google's test unit. */
	UPROPERTY(Config)
	FString InterstitialAdUnitID;

	/** The game's own banner ad unit (from AdMob); used in Shipping builds only. Empty: Google's test unit. */
	UPROPERTY(Config)
	FString BannerAdUnitID;

	/** The puzzle screen's banner: the 320x100 Large Banner instead of the standard 320x50 */
	UPROPERTY(Config)
	bool bLargeBanner = false;

	/** The banner's size, in dp */
	FVector2D GetBannerSizeDp() const { return FVector2D(320.0, bLargeBanner ? 100.0 : 50.0); }

	/** Switch between the 320x50 and 320x100 banner (reloads it) */
	void SetLargeBanner(bool bLarge);

	/** Seconds to wait for an ad to start before giving up on it and running the action */
	UPROPERTY(Config)
	float ShowTimeout = 5.f;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static UAdsSubsystem* Get(const UObject* WorldContextObject);
	static UAdsSubsystem* GetInstance() { return Instance.Get(); }

	/** Whether the game shows ads (saved with the game) */
	UFUNCTION(BlueprintPure, Category = "Ads")
	bool AreAdsEnabled() const { return bAdsEnabled; }

	/** Turn all ads on or off, and save it */
	UFUNCTION(BlueprintCallable, Category = "Ads")
	void SetAdsEnabled(bool bEnabled);

	/** For the save game: true when ads are off */
	bool GetAdsDisabledSaveData() const { return !bAdsEnabled; }

	/** Play an interstitial if one is ready, then run Action (at once if not, or with ads off) */
	void ShowInterstitialThen(FSimpleDelegate Action);

	/** Blueprint version: play an interstitial, then broadcast Target's event dispatcher DispatcherName (no params) */
	UFUNCTION(BlueprintCallable, Category = "Ads", meta = (WorldContext = "WorldContextObject"))
	static void ShowInterstitialThenBroadcast(const UObject* WorldContextObject, UObject* Target, FName DispatcherName);

	/** Show the banner with its top left corner at this viewport pixel, this frame. It hides when not asked for. */
	void RequestBanner(const FVector2D& ViewportPixel);

	/** Take the banner off the screen now (it also goes when RequestBanner stops being called) */
	void HideBanner();

	/** UMG units per dp at this geometry: how big a dp-sized ad is in the layout */
	float GetUnitsPerDp(const FGeometry& Geometry) const;

#if !UE_BUILD_SHIPPING
	/** Happiness.TestAd: play an ad (when one is loaded) with an action that only logs */
	void RunTestAd();
#endif

private:
	FString GetInterstitialAdUnitID() const;
	FString GetBannerAdUnitID() const;
	void StartAds();
	void LoadInterstitial();
	void LoadBanner();
	bool Tick(float DeltaTime);
	/** Run the waiting action once (from the ad closing, failing, or timing out) */
	void FinishShow();
	/** The size of the view the banner is placed in, in pixels, and pixels per dp; false where there isn't one */
	bool GetScreenMetrics(FVector2D& OutScreenPixels, float& OutDensity) const;

	static TWeakObjectPtr<UAdsSubsystem> Instance;

	bool bAdsEnabled = true;

	FSimpleDelegate PendingAction;
	bool bShowing = false;
	bool bStarted = false;
	FTimerHandle TimeoutTimer;
	bool bTestAdPending = false;

	bool bAdMobReady = false;
	bool bBannerLoading = false;
	bool bBannerReady = false;
	bool bBannerShown = false;
	FIntPoint BannerShownAt = FIntPoint(-1, -1);
	double LastBannerRequest = 0.0;
	double NextBannerLoad = 0.0;
	FTSTicker::FDelegateHandle TickHandle;

	mutable FVector2D CachedScreenPixels = FVector2D::ZeroVector;
	mutable float CachedDensity = 0.f;
	mutable FVector2D CachedForViewport = FVector2D(-1.0, -1.0);
};
