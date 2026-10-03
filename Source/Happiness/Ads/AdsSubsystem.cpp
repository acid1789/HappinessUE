#include "Ads/AdsSubsystem.h"

#include "Containers/Ticker.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "HappinessClassic/HappinessSaveGame.h"
#include "TimerManager.h"

#if PLATFORM_ANDROID || PLATFORM_IOS
#include "Interface/AdMobCPPLibrary.h"
#define HAPPINESS_WITH_ADS 1
#else
#define HAPPINESS_WITH_ADS 0
#endif

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogHappinessAds, Log, All);

TWeakObjectPtr<UAdsSubsystem> UAdsSubsystem::Instance;

namespace
{
	// Google's always-available test ad units
#if PLATFORM_IOS
	const TCHAR* TestInterstitialAdUnitID = TEXT("ca-app-pub-3940256099942544/4411468910");
	const TCHAR* TestBannerAdUnitID = TEXT("ca-app-pub-3940256099942544/2934735716");
#else
	const TCHAR* TestInterstitialAdUnitID = TEXT("ca-app-pub-3940256099942544/1033173712");
	const TCHAR* TestBannerAdUnitID = TEXT("ca-app-pub-3940256099942544/6300978111");
#endif

#if !HAPPINESS_WITH_ADS
	// Away from a phone the game has no ads; this lays the puzzle screen out as if it had (banner space)
	TAutoConsoleVariable<bool> CVarPreviewAdLayout(
		TEXT("Happiness.PreviewAdLayout"),
		false,
		TEXT("Editor/PC: lay the puzzle screen out with the banner ad's space, as on a phone with ads on"));
#endif

	// The banner hides when nothing has asked for it for this long (its screen closed)
	const double BannerRequestTimeout = 0.25;
	// Seconds before trying again after a banner failed to load (offline, no fill)
	const double BannerRetryDelay = 30.0;

	// Ad callbacks: handle them on the game thread, on a later frame. Never inside the plugin's own callback: it
	// calls us from inside its delegate and unbinds that delegate right after, and running game actions in there
	// (which can create or destroy widgets and delegates) corrupted memory and crashed after an interstitial.
	// The core ticker is thread safe, so this also works from the Java thread.
	void OnGameThread(TFunction<void()> Work)
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Work = MoveTemp(Work)](float)
		{
			Work();
			return false; // once
		}));
	}
}

void UAdsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Instance = this;

	if (const UHappinessSaveGame* SaveGame = UHappinessSaveGame::LoadFromSlot())
	{
		bAdsEnabled = !SaveGame->bAdsDisabled;
	}

	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UAdsSubsystem::Tick));
	StartAds();
}

void UAdsSubsystem::Deinitialize()
{
	FTSTicker::RemoveTicker(TickHandle);
	HideBanner();
	if (Instance == this)
	{
		Instance = nullptr;
	}
	Super::Deinitialize();
}

UAdsSubsystem* UAdsSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UAdsSubsystem>() : nullptr;
}

void UAdsSubsystem::SetAdsEnabled(bool bEnabled)
{
	if (bAdsEnabled == bEnabled)
	{
		return;
	}
	bAdsEnabled = bEnabled;
	UE_LOG(LogHappinessAds, Log, TEXT("Ads: %s"), bAdsEnabled ? TEXT("on") : TEXT("off"));
	if (bAdsEnabled)
	{
		LoadBanner();
	}
	else
	{
		HideBanner();
	}
	UHappinessSaveGame::SaveCurrentSettings();
}

bool UAdsSubsystem::AreAdsEnabled() const
{
#if HAPPINESS_WITH_ADS
	return bAdsEnabled && !bAdsRemoved;
#else
	return CVarPreviewAdLayout.GetValueOnGameThread() && bAdsEnabled && !bAdsRemoved;
#endif
}

void UAdsSubsystem::SetAdsRemoved(bool bRemoved)
{
	if (bAdsRemoved == bRemoved)
	{
		return;
	}
	bAdsRemoved = bRemoved;
	UE_LOG(LogHappinessAds, Log, TEXT("Ads: %s by the Remove Ads purchase"), bAdsRemoved ? TEXT("removed") : TEXT("back"));
	if (AreAdsEnabled())
	{
		LoadBanner();
	}
	else
	{
		HideBanner();
	}
}

FString UAdsSubsystem::GetInterstitialAdUnitID() const
{
#if UE_BUILD_SHIPPING
	if (!InterstitialAdUnitID.IsEmpty())
	{
		return InterstitialAdUnitID;
	}
#endif
	return TestInterstitialAdUnitID;
}

FString UAdsSubsystem::GetBannerAdUnitID() const
{
#if UE_BUILD_SHIPPING
	if (!BannerAdUnitID.IsEmpty())
	{
		return BannerAdUnitID;
	}
#endif
	return TestBannerAdUnitID;
}

void UAdsSubsystem::StartAds()
{
#if HAPPINESS_WITH_ADS
	// Consent first (the form only shows where it's required, and only until answered), then AdMob, then the ads
	TWeakObjectPtr<UAdsSubsystem> WeakThis(this);
	auto StartAdMob = [WeakThis]()
	{
		OnGameThread([WeakThis]()
		{
			if (!WeakThis.IsValid() || !UAdMobCPPLibrary::CanRequestAds())
			{
				UE_LOG(LogHappinessAds, Log, TEXT("Ads: can't request ads (no consent)"));
				return;
			}
			UAdMobCPPLibrary::InitializeAdMob(FOnAdMobInitializedNative::CreateLambda([WeakThis]()
			{
				OnGameThread([WeakThis]()
				{
					if (WeakThis.IsValid())
					{
						WeakThis->bAdMobReady = true;
						WeakThis->LoadInterstitial();
						WeakThis->LoadBanner();
					}
				});
			}));
		});
	};
	UAdMobCPPLibrary::RequestConsentForm(false,
		FOnConsentFormCompletedNative::CreateLambda([StartAdMob](EConsentStatus) { StartAdMob(); }),
		FOnConsentFormFailedNative::CreateLambda([StartAdMob](EFormErrorCode, const FString& Message)
		{
			// Consent may still be on record from an earlier run
			UE_LOG(LogHappinessAds, Warning, TEXT("Ads: consent form failed: %s"), *Message);
			StartAdMob();
		}));
#endif
}

void UAdsSubsystem::LoadInterstitial()
{
#if HAPPINESS_WITH_ADS
	UAdMobCPPLibrary::LoadInterstitial(GetInterstitialAdUnitID(),
		FOnInterstitialLoadedNative::CreateWeakLambda(this, [this]()
		{
			UE_LOG(LogHappinessAds, Log, TEXT("Ads: interstitial loaded"));
#if !UE_BUILD_SHIPPING
			OnGameThread([WeakThis = TWeakObjectPtr<UAdsSubsystem>(this)]()
			{
				if (WeakThis.IsValid() && WeakThis->bTestAdPending)
				{
					WeakThis->RunTestAd();
				}
			});
#endif
		}),
		FOnInterstitialLoadFailedNative::CreateLambda([](EAdErrorCode, const FString& Message)
		{
			UE_LOG(LogHappinessAds, Warning, TEXT("Ads: interstitial failed to load: %s"), *Message);
		}));
#endif
}

void UAdsSubsystem::ShowInterstitialThen(FSimpleDelegate Action)
{
	// An ad is already playing for an earlier press: ignore this one (one action per ad)
	if (bShowing)
	{
		return;
	}
#if HAPPINESS_WITH_ADS
	if (AreAdsEnabled() && UAdMobCPPLibrary::IsAdMobReady() && UAdMobCPPLibrary::IsInterstitialReady())
	{
		PendingAction = MoveTemp(Action);
		bShowing = true;
		bStarted = false;

		// If the ad never starts, don't leave the player waiting
		if (const UWorld* World = GetGameInstance()->GetWorld())
		{
			World->GetTimerManager().SetTimer(TimeoutTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (bShowing && !bStarted)
				{
					UE_LOG(LogHappinessAds, Warning, TEXT("Ads: interstitial didn't start; carrying on"));
					FinishShow();
				}
			}), ShowTimeout, false);
		}

		TWeakObjectPtr<UAdsSubsystem> WeakThis(this);
		auto Finish = [WeakThis]()
		{
			OnGameThread([WeakThis]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->FinishShow();
				}
			});
		};
		UAdMobCPPLibrary::ShowInterstitial(true, true,
			FOnInterstitialStartedNative::CreateLambda([WeakThis]()
			{
				OnGameThread([WeakThis]() { if (WeakThis.IsValid()) { WeakThis->bStarted = true; } });
			}),
			FOnInterstitialCompletedNative::CreateLambda(Finish),
			FOnInterstitialShowFailedNative::CreateLambda([Finish](EAdErrorCode, const FString& Message)
			{
				UE_LOG(LogHappinessAds, Warning, TEXT("Ads: interstitial failed to show: %s"), *Message);
				Finish();
			}),
			FOnInterstitialEventNative());
		return;
	}

	// Nothing to show now: try to have one ready next time
	if (AreAdsEnabled() && UAdMobCPPLibrary::IsAdMobReady())
	{
		LoadInterstitial();
	}
#endif
	Action.ExecuteIfBound();
}

void UAdsSubsystem::FinishShow()
{
	if (!bShowing)
	{
		return;
	}
	bShowing = false;
	if (const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		World->GetTimerManager().ClearTimer(TimeoutTimer);
	}
	// A copy, not a move, then clear the member: the action may start another ad
	const FSimpleDelegate Action = PendingAction;
	PendingAction.Unbind();
	Action.ExecuteIfBound();
}

void UAdsSubsystem::LoadBanner()
{
#if HAPPINESS_WITH_ADS
	if (!AreAdsEnabled() || !bAdMobReady || bBannerLoading || bBannerReady || FPlatformTime::Seconds() < NextBannerLoad)
	{
		return;
	}
	bBannerLoading = true;
	TWeakObjectPtr<UAdsSubsystem> WeakThis(this);
	UAdMobCPPLibrary::LoadBanner(GetBannerAdUnitID(), bLargeBanner ? EBannerAdSize::LargeBanner : EBannerAdSize::Banner,
		FOnBannerLoadedNative::CreateLambda([WeakThis]()
		{
			OnGameThread([WeakThis]()
			{
				if (WeakThis.IsValid())
				{
					UE_LOG(LogHappinessAds, Log, TEXT("Ads: banner loaded"));
					WeakThis->bBannerLoading = false;
					WeakThis->bBannerReady = true;
				}
			});
		}),
		FOnBannerLoadFailedNative::CreateLambda([WeakThis](EAdErrorCode, const FString& Message)
		{
			UE_LOG(LogHappinessAds, Warning, TEXT("Ads: banner failed to load: %s"), *Message);
			OnGameThread([WeakThis]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->bBannerLoading = false;
					WeakThis->NextBannerLoad = FPlatformTime::Seconds() + BannerRetryDelay;
				}
			});
		}),
		FOnBannerEventNative());
#endif
}

void UAdsSubsystem::SetLargeBanner(bool bLarge)
{
	if (bLargeBanner == bLarge)
	{
		return;
	}
	bLargeBanner = bLarge;
	// The loaded banner has the old size: drop it and load one of the new size
	HideBanner();
#if HAPPINESS_WITH_ADS
	if (bBannerReady || bBannerLoading)
	{
		UAdMobCPPLibrary::DestroyBanner();
	}
#endif
	bBannerReady = false;
	bBannerLoading = false;
	NextBannerLoad = 0.0;
	LoadBanner();
}

void UAdsSubsystem::RequestBanner(const FVector2D& ViewportPixel)
{
	if (!AreAdsEnabled())
	{
		return;
	}
	LastBannerRequest = FPlatformTime::Seconds();
#if HAPPINESS_WITH_ADS
	if (!bBannerReady)
	{
		LoadBanner();
		return;
	}

	// Viewport pixels (the game may render at a lower resolution) to the pixels of the view the banner is placed in
	FVector2D ViewportSize = FVector2D::ZeroVector;
	if (UGameViewportClient* Viewport = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr)
	{
		Viewport->GetViewportSize(ViewportSize);
	}
	FVector2D ScreenPixels;
	float Density;
	if (ViewportSize.X <= 0.0 || ViewportSize.Y <= 0.0 || !GetScreenMetrics(ScreenPixels, Density))
	{
		return;
	}
	const FIntPoint At(FMath::RoundToInt(ViewportPixel.X * ScreenPixels.X / ViewportSize.X),
		FMath::RoundToInt(ViewportPixel.Y * ScreenPixels.Y / ViewportSize.Y));
	if (bBannerShown && At == BannerShownAt)
	{
		return;
	}
	bBannerShown = true;
	BannerShownAt = At;
	// The plugin sets the top margin to minus OffsetY for the top gravities
	TWeakObjectPtr<UAdsSubsystem> WeakThis(this);
	UAdMobCPPLibrary::ShowBanner(EBannerGravity::TopLeft, At.X, -At.Y, FOnBannerShownNative(),
		FOnBannerShowFailedNative::CreateLambda([WeakThis](EAdErrorCode, const FString& Message)
		{
			UE_LOG(LogHappinessAds, Warning, TEXT("Ads: banner failed to show: %s"), *Message);
			OnGameThread([WeakThis]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->bBannerShown = false;
					WeakThis->bBannerReady = false;
				}
			});
		}));
#endif
}

void UAdsSubsystem::HideBanner()
{
	if (!bBannerShown)
	{
		return;
	}
	bBannerShown = false;
	BannerShownAt = FIntPoint(-1, -1);
#if HAPPINESS_WITH_ADS
	UAdMobCPPLibrary::HideBanner();
#endif
}

bool UAdsSubsystem::Tick(float DeltaTime)
{
	// Nothing asked for the banner lately: its screen closed or something covers it
	if (bBannerShown && (!AreAdsEnabled() || FPlatformTime::Seconds() - LastBannerRequest > BannerRequestTimeout))
	{
		HideBanner();
	}
	return true;
}

float UAdsSubsystem::GetUnitsPerDp(const FGeometry& Geometry) const
{
	FVector2D ViewportSize = FVector2D::ZeroVector;
	if (UGameViewportClient* Viewport = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr)
	{
		Viewport->GetViewportSize(ViewportSize);
	}
	const float Scale = Geometry.GetAccumulatedLayoutTransform().GetScale();
	if (ViewportSize.X <= 0.0 || ViewportSize.Y <= 0.0 || Scale <= 0.f)
	{
		return 2.f;
	}

	// On a phone: its pixels per dp over its pixels per layout unit
	FVector2D ScreenPixels;
	float Density;
	if (GetScreenMetrics(ScreenPixels, Density))
	{
		const float PixelsPerUnit = Scale * ScreenPixels.X / ViewportSize.X;
		return Density / PixelsPerUnit;
	}

	// Elsewhere (editor, PIE): as on a typical phone, whose short side is about 360 dp
	return ViewportSize.Y / Scale / 360.f;
}

bool UAdsSubsystem::GetScreenMetrics(FVector2D& OutScreenPixels, float& OutDensity) const
{
#if PLATFORM_ANDROID
	FVector2D ViewportSize = FVector2D::ZeroVector;
	if (UGameViewportClient* Viewport = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr)
	{
		Viewport->GetViewportSize(ViewportSize);
	}
	if (ViewportSize == CachedForViewport && CachedDensity > 0.f)
	{
		OutScreenPixels = CachedScreenPixels;
		OutDensity = CachedDensity;
		return true;
	}

	// The activity's content view (android.R.id.content), which the plugin adds the banner to, and the density
	JNIEnv* Env = FAndroidApplication::GetJavaEnv();
	jobject Activity = FAndroidApplication::GetGameActivityThis();
	if (!Env || !Activity)
	{
		return false;
	}
	jint Width = 0, Height = 0;
	jfloat Density = 0.f;
	jclass ActivityClass = Env->GetObjectClass(Activity);
	jobject Content = Env->CallObjectMethod(Activity, Env->GetMethodID(ActivityClass, "findViewById", "(I)Landroid/view/View;"), 16908290);
	jobject Resources = Env->CallObjectMethod(Activity, Env->GetMethodID(ActivityClass, "getResources", "()Landroid/content/res/Resources;"));
	if (Content && Resources && !Env->ExceptionCheck())
	{
		jclass ViewClass = Env->GetObjectClass(Content);
		Width = Env->CallIntMethod(Content, Env->GetMethodID(ViewClass, "getWidth", "()I"));
		Height = Env->CallIntMethod(Content, Env->GetMethodID(ViewClass, "getHeight", "()I"));
		jclass ResourcesClass = Env->GetObjectClass(Resources);
		jobject Metrics = Env->CallObjectMethod(Resources, Env->GetMethodID(ResourcesClass, "getDisplayMetrics", "()Landroid/util/DisplayMetrics;"));
		if (Metrics && !Env->ExceptionCheck())
		{
			jclass MetricsClass = Env->GetObjectClass(Metrics);
			Density = Env->GetFloatField(Metrics, Env->GetFieldID(MetricsClass, "density", "F"));
			Env->DeleteLocalRef(MetricsClass);
		}
		if (Metrics)
		{
			Env->DeleteLocalRef(Metrics);
		}
		Env->DeleteLocalRef(ResourcesClass);
		Env->DeleteLocalRef(ViewClass);
	}
	if (Env->ExceptionCheck())
	{
		Env->ExceptionClear();
		Width = 0;
	}
	if (Content)
	{
		Env->DeleteLocalRef(Content);
	}
	if (Resources)
	{
		Env->DeleteLocalRef(Resources);
	}
	Env->DeleteLocalRef(ActivityClass);

	// Before the first layout the view has no size yet
	if (Width <= 0 || Height <= 0 || Density <= 0.f)
	{
		return false;
	}
	CachedScreenPixels = FVector2D(Width, Height);
	CachedDensity = Density;
	CachedForViewport = ViewportSize;
	OutScreenPixels = CachedScreenPixels;
	OutDensity = CachedDensity;
	return true;
#else
	return false;
#endif
}

#if !UE_BUILD_SHIPPING
void UAdsSubsystem::RunTestAd()
{
	// Plays an ad with an action that only logs, to tell the ad's own effects from the end-screen buttons'
#if HAPPINESS_WITH_ADS
	if (!UAdMobCPPLibrary::IsAdMobReady() || !UAdMobCPPLibrary::IsInterstitialReady())
	{
		UE_LOG(LogHappinessAds, Log, TEXT("Ads: test ad waits for an ad to load"));
		bTestAdPending = true;
		return;
	}
#endif
	bTestAdPending = false;
	UE_LOG(LogHappinessAds, Log, TEXT("Ads: test ad showing"));
	ShowInterstitialThen(FSimpleDelegate::CreateLambda([]() { UE_LOG(LogHappinessAds, Log, TEXT("Ads: test ad action ran")); }));
}

static FAutoConsoleCommandWithWorld GTestAdCommand(
	TEXT("Happiness.TestAd"),
	TEXT("Play an interstitial (as soon as one is loaded) followed by an action that only logs. Dev builds only."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (UAdsSubsystem* Ads = UAdsSubsystem::Get(World))
		{
			Ads->RunTestAd();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GLargeBannerCommand(
	TEXT("Happiness.LargeBanner"),
	TEXT("The puzzle screen's banner: 1 the 320x100 Large Banner, 0 the standard 320x50. Not saved (bLargeBanner in DefaultGame.ini sets it). Dev builds only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (UAdsSubsystem* Ads = UAdsSubsystem::Get(World))
		{
			if (Args.Num() > 0)
			{
				Ads->SetLargeBanner(FCString::Atoi(*Args[0]) != 0);
			}
			UE_LOG(LogHappinessAds, Display, TEXT("Banner is %s"), Ads->bLargeBanner ? TEXT("320x100") : TEXT("320x50"));
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GAdsCommand(
	TEXT("Happiness.Ads"),
	TEXT("Turn ads on (1) or off (0): the end-screen interstitials and the puzzle screen's banner. Saved. Dev builds only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (UAdsSubsystem* Ads = UAdsSubsystem::Get(World))
		{
			if (Args.Num() > 0)
			{
				Ads->SetAdsEnabled(FCString::Atoi(*Args[0]) != 0);
			}
			UE_LOG(LogHappinessAds, Display, TEXT("Ads are %s"), Ads->AreAdsEnabled() ? TEXT("on") : TEXT("off"));
		}
	}));
#endif

void UAdsSubsystem::ShowInterstitialThenBroadcast(const UObject* WorldContextObject, UObject* Target, FName DispatcherName)
{
	TWeakObjectPtr<UObject> WeakTarget(Target);
	FSimpleDelegate Broadcast = FSimpleDelegate::CreateLambda([WeakTarget, DispatcherName]()
	{
		UObject* Object = WeakTarget.Get();
		const FMulticastDelegateProperty* Dispatcher = Object ? FindFProperty<FMulticastDelegateProperty>(Object->GetClass(), DispatcherName) : nullptr;
		if (!Dispatcher)
		{
			UE_LOG(LogHappinessAds, Warning, TEXT("Ads: no event dispatcher %s to call after the ad"), *DispatcherName.ToString());
			return;
		}
		// A Blueprint event dispatcher without parameters. GetMulticastDelegate takes the property's value address,
		// not the object: passing the object read and wrote the widget's own memory as a delegate (crashed on device)
		if (const FMulticastScriptDelegate* Delegate = Dispatcher->GetMulticastDelegate(Dispatcher->ContainerPtrToValuePtr<void>(Object)))
		{
			Delegate->ProcessDelegate<UObject>(nullptr);
		}
	});

	if (UAdsSubsystem* Ads = Get(WorldContextObject))
	{
		Ads->ShowInterstitialThen(MoveTemp(Broadcast));
	}
	else
	{
		Broadcast.ExecuteIfBound();
	}
}
