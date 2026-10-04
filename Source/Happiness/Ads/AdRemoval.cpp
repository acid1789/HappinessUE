#include "Ads/AdRemoval.h"

#include "Ads/AdsSubsystem.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlinePurchaseInterface.h"
#include "Interfaces/OnlineStoreInterfaceV2.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "OnlineError.h"
#include "OnlineSubsystem.h"

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
// Google Play Billing's acknowledge, from the engine (Launch); Android links everything into one library
extern void AndroidThunkCpp_Iap_AcknowledgePurchase(const FString& PurchaseToken);
#endif

DEFINE_LOG_CATEGORY_STATIC(LogHappinessPurchase, Log, All);

namespace
{
	const TCHAR* TokenFileName = TEXT("remove_ads.token");

	// Seconds after startup before the first store check (the store connects in the background)
	const double FirstCheckDelay = 3.0;

	bool IsProduct(const FString& OfferId, const FString& ProductId)
	{
		// Google Play may report it with its type in front ("inapp:remove_ads")
		return OfferId == ProductId || OfferId.EndsWith(TEXT(":") + ProductId);
	}
}

void UAdRemovalSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<UAdsSubsystem>();
	Super::Initialize(Collection);

	// The token decides, with or without a connection
	bHasToken = IFileManager::Get().FileExists(*GetTokenPath());
	UE_LOG(LogHappinessPurchase, Log, TEXT("Remove Ads: token %s (%s)"), bHasToken ? TEXT("found") : TEXT("not found"), *GetTokenPath());
	ApplyToAds();

	NextCheck = FPlatformTime::Seconds() + FirstCheckDelay;
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UAdRemovalSubsystem::Tick), 1.f);
}

void UAdRemovalSubsystem::Deinitialize()
{
	FTSTicker::RemoveTicker(TickHandle);
	Super::Deinitialize();
}

UAdRemovalSubsystem* UAdRemovalSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UAdRemovalSubsystem>() : nullptr;
}

ERemoveAdsState UAdRemovalSubsystem::GetState() const
{
	if (bHasToken)
	{
		return ERemoveAdsState::Purchased;
	}
	if (bPurchasing)
	{
		return ERemoveAdsState::Purchasing;
	}
	if (bPending)
	{
		return ERemoveAdsState::Pending;
	}
	return bStoreReached && GetStore() ? ERemoveAdsState::Available : ERemoveAdsState::Unavailable;
}

FString UAdRemovalSubsystem::GetTokenPath() const
{
#if PLATFORM_ANDROID
	// The app's internal files folder (Context.getFilesDir()): private to the app, removed with it
	static FString FilesDir;
	if (FilesDir.IsEmpty())
	{
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			jobject Activity = FAndroidApplication::GetGameActivityThis();
			jclass ActivityClass = Env->GetObjectClass(Activity);
			jobject Dir = Env->CallObjectMethod(Activity, Env->GetMethodID(ActivityClass, "getFilesDir", "()Ljava/io/File;"));
			if (Dir && !Env->ExceptionCheck())
			{
				jclass FileClass = Env->GetObjectClass(Dir);
				jstring Path = (jstring)Env->CallObjectMethod(Dir, Env->GetMethodID(FileClass, "getAbsolutePath", "()Ljava/lang/String;"));
				if (Path && !Env->ExceptionCheck())
				{
					FilesDir = FJavaHelper::FStringFromLocalRef(Env, Path);
				}
				Env->DeleteLocalRef(FileClass);
				Env->DeleteLocalRef(Dir);
			}
			if (Env->ExceptionCheck())
			{
				Env->ExceptionClear();
			}
			Env->DeleteLocalRef(ActivityClass);
		}
	}
	if (!FilesDir.IsEmpty())
	{
		return FilesDir / TokenFileName;
	}
#endif
	// iOS: the app's own sandbox; elsewhere (editor, PC): the project's Saved folder
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TokenFileName);
}

void UAdRemovalSubsystem::SetOwned(bool bOwned, const FString& TransactionId)
{
	if (bOwned == bHasToken)
	{
		return;
	}
	const FString Path = GetTokenPath();
	if (bOwned)
	{
		const FString Token = FString::Printf(TEXT("product=%s\ntransaction=%s\nwritten=%s\n"), *ProductId, *TransactionId,
			*FDateTime::UtcNow().ToIso8601());
		if (!FFileHelper::SaveStringToFile(Token, *Path))
		{
			UE_LOG(LogHappinessPurchase, Error, TEXT("Remove Ads: couldn't write the token %s"), *Path);
		}
		UE_LOG(LogHappinessPurchase, Log, TEXT("Remove Ads: owned, token written"));
	}
	else
	{
		IFileManager::Get().Delete(*Path);
		UE_LOG(LogHappinessPurchase, Log, TEXT("Remove Ads: not owned (refunded?), token deleted"));
	}
	bHasToken = bOwned;
	bPending = false;
	ApplyToAds();
	OnChanged.Broadcast();
}

void UAdRemovalSubsystem::ApplyToAds() const
{
	if (UAdsSubsystem* Ads = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAdsSubsystem>() : nullptr)
	{
		Ads->SetAdsRemoved(bHasToken);
	}
}

IOnlineSubsystem* UAdRemovalSubsystem::GetStore() const
{
#if PLATFORM_ANDROID
	return IOnlineSubsystem::Get(TEXT("GooglePlay"));
#elif PLATFORM_IOS
	return IOnlineSubsystem::Get(TEXT("IOS"));
#else
	return nullptr;
#endif
}

TSharedPtr<const FUniqueNetId> UAdRemovalSubsystem::GetUserId() const
{
	// Purchases belong to the device's store account; the online subsystem only needs an id of its own type
	const IOnlineSubsystem* Store = GetStore();
	const IOnlineIdentityPtr Identity = Store ? Store->GetIdentityInterface() : nullptr;
	if (!Identity.IsValid())
	{
		return nullptr;
	}
	FUniqueNetIdPtr UserId = Identity->GetUniquePlayerId(0);
	return UserId.IsValid() ? UserId : Identity->CreateUniquePlayerId(TEXT("0"));
}

bool UAdRemovalSubsystem::Tick(float DeltaTime)
{
	// Reach the store once per run (more often while it can't be reached)
	if (!bStoreReached && !bChecking && FPlatformTime::Seconds() >= NextCheck)
	{
		CheckStore(false);
	}
	return true;
}

void UAdRemovalSubsystem::CheckStore(bool bRestore)
{
	IOnlineSubsystem* Store = GetStore();
	const IOnlinePurchasePtr Purchase = Store ? Store->GetPurchaseInterface() : nullptr;
	const TSharedPtr<const FUniqueNetId> UserId = GetUserId();
	if (!Purchase.IsValid() || !UserId.IsValid())
	{
		bRestoring = false;
		NextCheck = FPlatformTime::Seconds() + RetryInterval;
		return;
	}
	if (bChecking)
	{
		return;
	}
	bChecking = true;
	TWeakObjectPtr<UAdRemovalSubsystem> WeakThis(this);
	Purchase->QueryReceipts(*UserId, bRestore, FOnQueryReceiptsComplete::CreateLambda([WeakThis](const FOnlineError& Result)
	{
		UAdRemovalSubsystem* This = WeakThis.Get();
		if (!This)
		{
			return;
		}
		This->bChecking = false;
		if (!Result.WasSuccessful())
		{
			// Offline or the store isn't ready: keep what we have (the token stays), try again later
			UE_LOG(LogHappinessPurchase, Log, TEXT("Remove Ads: store not reached (%s); retrying later"), *Result.ToLogString());
			This->bRestoring = false;
			This->NextCheck = FPlatformTime::Seconds() + This->RetryInterval;
			This->OnChanged.Broadcast();
			return;
		}
		This->bStoreReached = true;
		const bool bListed = This->HandleReceipts();
		This->bRevokeIfConfirmed = This->bHasToken && !bListed;
		This->bRestoring = false;
		// Also the server check: removing a token waits for it
		This->QueryPrice();
		This->OnChanged.Broadcast();
	}));
}

bool UAdRemovalSubsystem::IsOnline()
{
	return FPlatformMisc::GetNetworkConnectionType() != ENetworkConnectionType::None &&
		FPlatformMisc::GetNetworkConnectionType() != ENetworkConnectionType::AirplaneMode;
}

bool UAdRemovalSubsystem::HandleReceipts()
{
	IOnlineSubsystem* Store = GetStore();
	const IOnlinePurchasePtr Purchase = Store ? Store->GetPurchaseInterface() : nullptr;
	const TSharedPtr<const FUniqueNetId> UserId = GetUserId();
	if (!Purchase.IsValid() || !UserId.IsValid())
	{
		return false;
	}
	TArray<FPurchaseReceipt> Receipts;
	Purchase->GetReceipts(*UserId, Receipts);

	bool bOwned = false;
	bool bFoundPending = false;
	FString TransactionId;
	for (const FPurchaseReceipt& Receipt : Receipts)
	{
		for (const FPurchaseReceipt::FReceiptOfferEntry& Offer : Receipt.ReceiptOffers)
		{
			if (!IsProduct(Offer.OfferId, ProductId))
			{
				continue;
			}
			// Paid: the store gives it validation info; a pending (not yet paid) purchase has none
			const bool bPaid = Receipt.TransactionState == EPurchaseTransactionState::Purchased &&
				Offer.LineItems.ContainsByPredicate([](const FPurchaseReceipt::FLineItemInfo& Item) { return Item.IsRedeemable(); });
			if (bPaid)
			{
				bOwned = true;
				TransactionId = Receipt.TransactionId;
			}
			else if (Receipt.TransactionState == EPurchaseTransactionState::Deferred || Receipt.TransactionState == EPurchaseTransactionState::Purchased)
			{
				bFoundPending = true;
			}
		}
	}

	bPending = !bOwned && bFoundPending;
	if (bOwned)
	{
		AcknowledgePurchase(TransactionId);
		SetOwned(true, TransactionId);
	}
	return bOwned || bFoundPending;
}

void UAdRemovalSubsystem::AcknowledgePurchase(const FString& TransactionId)
{
	if (TransactionId.IsEmpty())
	{
		return;
	}
#if PLATFORM_ANDROID
	// Acknowledge, don't consume: a consumed product is no longer owned
	AndroidThunkCpp_Iap_AcknowledgePurchase(TransactionId);
#else
	// StoreKit: finish the transaction (non-consumables stay owned)
	IOnlineSubsystem* Store = GetStore();
	const IOnlinePurchasePtr Purchase = Store ? Store->GetPurchaseInterface() : nullptr;
	const TSharedPtr<const FUniqueNetId> UserId = GetUserId();
	if (Purchase.IsValid() && UserId.IsValid())
	{
		Purchase->FinalizePurchase(*UserId, TransactionId);
	}
#endif
}

void UAdRemovalSubsystem::QueryPrice()
{
	IOnlineSubsystem* Store = GetStore();
	const IOnlineStoreV2Ptr StoreV2 = Store ? Store->GetStoreV2Interface() : nullptr;
	const TSharedPtr<const FUniqueNetId> UserId = GetUserId();
	if (!StoreV2.IsValid() || !UserId.IsValid() || (bHasToken && !bRevokeIfConfirmed))
	{
		return;
	}
	TWeakObjectPtr<UAdRemovalSubsystem> WeakThis(this);
	StoreV2->QueryOffersById(*UserId, { ProductId }, FOnQueryOnlineStoreOffersComplete::CreateLambda(
		[WeakThis](bool bWasSuccessful, const TArray<FUniqueOfferId>& OfferIds, const FString& Error)
	{
		UAdRemovalSubsystem* This = WeakThis.Get();
		IOnlineSubsystem* Store = This ? This->GetStore() : nullptr;
		const IOnlineStoreV2Ptr StoreV2 = Store ? Store->GetStoreV2Interface() : nullptr;
		if (!StoreV2.IsValid())
		{
			return;
		}
		bool bFoundOffer = false;
		for (const FUniqueOfferId& OfferId : OfferIds)
		{
			if (const TSharedPtr<FOnlineStoreOffer> Offer = StoreV2->GetOffer(OfferId))
			{
				This->PriceText = Offer->GetDisplayPrice();
				bFoundOffer = true;
			}
		}
		if (!bWasSuccessful)
		{
			UE_LOG(LogHappinessPurchase, Warning, TEXT("Remove Ads: couldn't get the price: %s"), *Error);
		}

		// The product's details came from the store's servers: its receipt list was current, so a missing purchase is gone
		if (This->bRevokeIfConfirmed)
		{
			This->bRevokeIfConfirmed = false;
			if (bWasSuccessful && bFoundOffer && IsOnline())
			{
				This->SetOwned(false, FString());
			}
			else
			{
				UE_LOG(LogHappinessPurchase, Log, TEXT("Remove Ads: not in the receipts, but the store's servers weren't confirmed; keeping the token"));
			}
		}
		This->OnChanged.Broadcast();
	}));
}

void UAdRemovalSubsystem::BuyRemoveAds()
{
	if (GetState() != ERemoveAdsState::Available)
	{
		return;
	}
	IOnlineSubsystem* Store = GetStore();
	const IOnlinePurchasePtr Purchase = Store ? Store->GetPurchaseInterface() : nullptr;
	const TSharedPtr<const FUniqueNetId> UserId = GetUserId();
	if (!Purchase.IsValid() || !UserId.IsValid())
	{
		return;
	}

	FPurchaseCheckoutRequest Request;
	Request.AddPurchaseOffer(TEXT(""), ProductId, 1, false);
	bPurchasing = true;
	OnChanged.Broadcast();

	TWeakObjectPtr<UAdRemovalSubsystem> WeakThis(this);
	Purchase->Checkout(*UserId, Request, FOnPurchaseCheckoutComplete::CreateLambda(
		[WeakThis](const FOnlineError& Result, const TSharedRef<FPurchaseReceipt>& Receipt)
	{
		UAdRemovalSubsystem* This = WeakThis.Get();
		if (!This)
		{
			return;
		}
		This->bPurchasing = false;
		UE_LOG(LogHappinessPurchase, Log, TEXT("Remove Ads: purchase finished: %s, state %d"), *Result.ToLogString(),
			static_cast<int32>(Receipt->TransactionState));
		if (Receipt->TransactionState == EPurchaseTransactionState::Purchased)
		{
			// The store also lists it as owned now
			This->HandleReceipts();
			if (!This->bHasToken)
			{
				This->AcknowledgePurchase(Receipt->TransactionId);
				This->SetOwned(true, Receipt->TransactionId);
			}
		}
		else if (Receipt->TransactionState == EPurchaseTransactionState::Deferred)
		{
			// Paid later (e.g. cash at a shop): the next store check finds it once it clears
			This->bPending = true;
			This->bStoreReached = false;
			This->NextCheck = FPlatformTime::Seconds() + This->RetryInterval;
		}
		This->OnChanged.Broadcast();
	}));
}

void UAdRemovalSubsystem::RestorePurchases()
{
	if (bRestoring || bHasToken)
	{
		return;
	}
	bRestoring = true;
	OnChanged.Broadcast();
	CheckStore(true);
}

void UAdRemovalSubsystem::DebugSetToken(bool bOwned)
{
	SetOwned(bOwned, TEXT("debug"));
}

#if !UE_BUILD_SHIPPING
static FAutoConsoleCommandWithWorldAndArgs GRemoveAdsTokenCommand(
	TEXT("Happiness.RemoveAdsToken"),
	TEXT("Create (1) or delete (0) the Remove Ads token, as if bought or refunded. No argument: check the store now. Dev builds only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (UAdRemovalSubsystem* AdRemoval = UAdRemovalSubsystem::Get(World))
		{
			if (Args.Num() > 0)
			{
				AdRemoval->DebugSetToken(FCString::Atoi(*Args[0]) != 0);
			}
			else
			{
				AdRemoval->DebugCheckStore();
			}
			UE_LOG(LogHappinessPurchase, Display, TEXT("Remove Ads: %s, state %d"), AdRemoval->AreAdsRemoved() ? TEXT("removed") : TEXT("not removed"),
				static_cast<int32>(AdRemoval->GetState()));
		}
	}));
#endif
