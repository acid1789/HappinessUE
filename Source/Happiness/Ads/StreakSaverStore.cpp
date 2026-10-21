#include "Ads/StreakSaverStore.h"

#include "HappinessClassic/DailyPuzzle.h"

#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlinePurchaseInterface.h"
#include "Interfaces/OnlineStoreInterfaceV2.h"
#include "Misc/Guid.h"
#include "OnlineError.h"
#include "OnlineSubsystem.h"

#if PLATFORM_ANDROID
// Google Play Billing's consume, from the engine (Launch); Android links everything into one library
extern void AndroidThunkCpp_Iap_ConsumePurchase(const FString& PurchaseToken);
#endif

DEFINE_LOG_CATEGORY_STATIC(LogStreakSaver, Log, All);

namespace
{
	// Seconds after startup before the first store check: after Remove Ads' own (UAdRemovalSubsystem), so the two
	// don't ask the store at the same moment
	const double FirstCheckDelay = 8.0;

	const EStreakSaverTier AllTiers[] = { EStreakSaverTier::OneDay, EStreakSaverTier::SevenDays, EStreakSaverTier::ThirtyDays };

	bool IsProduct(const FString& OfferId, const FString& ProductId)
	{
		// Google Play may report it with its type in front ("inapp:streak_saver_7")
		return OfferId == ProductId || OfferId.EndsWith(TEXT(":") + ProductId);
	}

	/** No store to buy from here, so development builds hand out the days directly */
	bool UsesTestPurchases()
	{
#if !UE_BUILD_SHIPPING && !PLATFORM_ANDROID && !PLATFORM_IOS
		return true;
#else
		return false;
#endif
	}
}

void UStreakSaverStoreSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<UDailySubsystem>();
	Super::Initialize(Collection);
	NextCheck = FPlatformTime::Seconds() + FirstCheckDelay;
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UStreakSaverStoreSubsystem::Tick), 1.f);
}

void UStreakSaverStoreSubsystem::Deinitialize()
{
	FTSTicker::RemoveTicker(TickHandle);
	Super::Deinitialize();
}

UStreakSaverStoreSubsystem* UStreakSaverStoreSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UStreakSaverStoreSubsystem>() : nullptr;
}

int32 UStreakSaverStoreSubsystem::GetTierDays(EStreakSaverTier Tier)
{
	switch (Tier)
	{
	case EStreakSaverTier::SevenDays: return 7;
	case EStreakSaverTier::ThirtyDays: return 30;
	default: return 1;
	}
}

const FString& UStreakSaverStoreSubsystem::GetProductId(EStreakSaverTier Tier) const
{
	switch (Tier)
	{
	case EStreakSaverTier::SevenDays: return SevenDaysProductId;
	case EStreakSaverTier::ThirtyDays: return ThirtyDaysProductId;
	default: return OneDayProductId;
	}
}

bool UStreakSaverStoreSubsystem::FindTier(const FString& OfferId, EStreakSaverTier& OutTier) const
{
	for (EStreakSaverTier Tier : AllTiers)
	{
		if (IsProduct(OfferId, GetProductId(Tier)))
		{
			OutTier = Tier;
			return true;
		}
	}
	return false;
}

bool UStreakSaverStoreSubsystem::CanBuy() const
{
	return !bPurchasing && (UsesTestPurchases() || (bStoreReached && GetStore()));
}

FText UStreakSaverStoreSubsystem::GetPriceText(EStreakSaverTier Tier) const
{
	if (const FText* Price = Prices.Find(Tier))
	{
		return *Price;
	}
	return UsesTestPurchases() ? NSLOCTEXT("StreakSaver", "TestPrice", "Test") : FText::GetEmpty();
}

IOnlineSubsystem* UStreakSaverStoreSubsystem::GetStore() const
{
#if PLATFORM_ANDROID
	return IOnlineSubsystem::Get(TEXT("GooglePlay"));
#elif PLATFORM_IOS
	return IOnlineSubsystem::Get(TEXT("IOS"));
#else
	return nullptr;
#endif
}

TSharedPtr<const FUniqueNetId> UStreakSaverStoreSubsystem::GetUserId() const
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

bool UStreakSaverStoreSubsystem::Tick(float DeltaTime)
{
	// Reach the store once per run (more often while it can't be reached)
	if (!bStoreReached && !bChecking && GetStore() && FPlatformTime::Seconds() >= NextCheck)
	{
		CheckStore();
	}
	return true;
}

void UStreakSaverStoreSubsystem::CheckStore()
{
	IOnlineSubsystem* Store = GetStore();
	const IOnlinePurchasePtr Purchase = Store ? Store->GetPurchaseInterface() : nullptr;
	const TSharedPtr<const FUniqueNetId> UserId = GetUserId();
	if (!Purchase.IsValid() || !UserId.IsValid())
	{
		NextCheck = FPlatformTime::Seconds() + RetryInterval;
		return;
	}
	bChecking = true;
	TWeakObjectPtr<UStreakSaverStoreSubsystem> WeakThis(this);
	Purchase->QueryReceipts(*UserId, false, FOnQueryReceiptsComplete::CreateLambda([WeakThis](const FOnlineError& Result)
	{
		UStreakSaverStoreSubsystem* This = WeakThis.Get();
		if (!This)
		{
			return;
		}
		This->bChecking = false;
		if (!Result.WasSuccessful())
		{
			UE_LOG(LogStreakSaver, Log, TEXT("Streak Saver: store not reached (%s); retrying later"), *Result.ToLogString());
			This->NextCheck = FPlatformTime::Seconds() + This->RetryInterval;
			This->OnChanged.Broadcast();
			return;
		}
		This->bStoreReached = true;
		This->HandleReceipts();
		This->QueryPrices();
		This->OnChanged.Broadcast();
	}));
}

void UStreakSaverStoreSubsystem::QueryPrices()
{
	IOnlineSubsystem* Store = GetStore();
	const IOnlineStoreV2Ptr StoreV2 = Store ? Store->GetStoreV2Interface() : nullptr;
	const TSharedPtr<const FUniqueNetId> UserId = GetUserId();
	if (!StoreV2.IsValid() || !UserId.IsValid())
	{
		return;
	}
	TWeakObjectPtr<UStreakSaverStoreSubsystem> WeakThis(this);
	StoreV2->QueryOffersById(*UserId, { OneDayProductId, SevenDaysProductId, ThirtyDaysProductId }, FOnQueryOnlineStoreOffersComplete::CreateLambda(
		[WeakThis](bool bWasSuccessful, const TArray<FUniqueOfferId>& OfferIds, const FString& Error)
	{
		UStreakSaverStoreSubsystem* This = WeakThis.Get();
		IOnlineSubsystem* Store = This ? This->GetStore() : nullptr;
		const IOnlineStoreV2Ptr StoreV2 = Store ? Store->GetStoreV2Interface() : nullptr;
		if (!StoreV2.IsValid())
		{
			return;
		}
		if (!bWasSuccessful)
		{
			UE_LOG(LogStreakSaver, Warning, TEXT("Streak Saver: couldn't get the prices: %s"), *Error);
		}
		for (const FUniqueOfferId& OfferId : OfferIds)
		{
			EStreakSaverTier Tier;
			const TSharedPtr<FOnlineStoreOffer> Offer = StoreV2->GetOffer(OfferId);
			if (Offer.IsValid() && This->FindTier(OfferId, Tier))
			{
				This->Prices.Add(Tier, Offer->GetDisplayPrice());
			}
		}
		This->OnChanged.Broadcast();
	}));
}

void UStreakSaverStoreSubsystem::HandleReceipts()
{
	IOnlineSubsystem* Store = GetStore();
	const IOnlinePurchasePtr Purchase = Store ? Store->GetPurchaseInterface() : nullptr;
	const TSharedPtr<const FUniqueNetId> UserId = GetUserId();
	if (!Purchase.IsValid() || !UserId.IsValid())
	{
		return;
	}
	TArray<FPurchaseReceipt> Receipts;
	Purchase->GetReceipts(*UserId, Receipts);

	bool bFoundPending = false;
	for (const FPurchaseReceipt& Receipt : Receipts)
	{
		for (const FPurchaseReceipt::FReceiptOfferEntry& Offer : Receipt.ReceiptOffers)
		{
			EStreakSaverTier Tier;
			if (!FindTier(Offer.OfferId, Tier))
			{
				continue;
			}
			// Paid: the store gives it validation info; a pending (not yet paid) purchase has none
			const bool bPaid = Receipt.TransactionState == EPurchaseTransactionState::Purchased &&
				Offer.LineItems.ContainsByPredicate([](const FPurchaseReceipt::FLineItemInfo& Item) { return Item.IsRedeemable(); });
			if (bPaid)
			{
				Deliver(Receipt.TransactionId, Tier, Offer.Quantity);
			}
			else if (Receipt.TransactionState == EPurchaseTransactionState::Deferred || Receipt.TransactionState == EPurchaseTransactionState::Purchased)
			{
				bFoundPending = true;
			}
		}
	}
	bPending = bFoundPending;
}

void UStreakSaverStoreSubsystem::Deliver(const FString& TransactionId, EStreakSaverTier Tier, int32 Quantity)
{
	if (TransactionId.IsEmpty())
	{
		return;
	}
	if (UDailySubsystem* Daily = GetGameInstance() ? GetGameInstance()->GetSubsystem<UDailySubsystem>() : nullptr)
	{
		// The daily data remembers the transaction: a purchase the store reports again (not consumed yet) adds nothing
		if (!Daily->HasSaverTransaction(TransactionId))
		{
			const int32 Days = GetTierDays(Tier) * FMath::Max(Quantity, 1);
			Daily->AddSaverDays(Days, TransactionId);
			UE_LOG(LogStreakSaver, Log, TEXT("Streak Saver: %d days added (%s)"), Days, *GetProductId(Tier));
		}
		Consume(TransactionId);
	}
}

void UStreakSaverStoreSubsystem::Consume(const FString& TransactionId)
{
#if PLATFORM_ANDROID
	// Consumed, it can be bought again
	AndroidThunkCpp_Iap_ConsumePurchase(TransactionId);
#else
	// StoreKit: finish the transaction
	IOnlineSubsystem* Store = GetStore();
	const IOnlinePurchasePtr Purchase = Store ? Store->GetPurchaseInterface() : nullptr;
	const TSharedPtr<const FUniqueNetId> UserId = GetUserId();
	if (Purchase.IsValid() && UserId.IsValid())
	{
		Purchase->FinalizePurchase(*UserId, TransactionId);
	}
#endif
}

void UStreakSaverStoreSubsystem::Buy(EStreakSaverTier Tier)
{
	if (!CanBuy())
	{
		return;
	}
	if (UsesTestPurchases())
	{
		UE_LOG(LogStreakSaver, Display, TEXT("Streak Saver: no store here, test purchase of %s"), *GetProductId(Tier));
		if (UDailySubsystem* Daily = GetGameInstance() ? GetGameInstance()->GetSubsystem<UDailySubsystem>() : nullptr)
		{
			Daily->AddSaverDays(GetTierDays(Tier), TEXT("test-") + FGuid::NewGuid().ToString());
		}
		OnChanged.Broadcast();
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
	Request.AddPurchaseOffer(TEXT(""), GetProductId(Tier), 1, true);
	bPurchasing = true;
	OnChanged.Broadcast();

	TWeakObjectPtr<UStreakSaverStoreSubsystem> WeakThis(this);
	Purchase->Checkout(*UserId, Request, FOnPurchaseCheckoutComplete::CreateLambda(
		[WeakThis, Tier](const FOnlineError& Result, const TSharedRef<FPurchaseReceipt>& Receipt)
	{
		UStreakSaverStoreSubsystem* This = WeakThis.Get();
		if (!This)
		{
			return;
		}
		This->bPurchasing = false;
		UE_LOG(LogStreakSaver, Log, TEXT("Streak Saver: purchase finished: %s, state %d"), *Result.ToLogString(),
			static_cast<int32>(Receipt->TransactionState));
		if (Receipt->TransactionState == EPurchaseTransactionState::Purchased)
		{
			This->Deliver(Receipt->TransactionId, Tier, 1);
		}
		else if (Receipt->TransactionState == EPurchaseTransactionState::Deferred)
		{
			// Paid later: the next store check finds it once it clears
			This->bPending = true;
			This->bStoreReached = false;
			This->NextCheck = FPlatformTime::Seconds() + This->RetryInterval;
		}
		This->OnChanged.Broadcast();
	}));
}
