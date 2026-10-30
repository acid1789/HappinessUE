#include "Ads/IconSetStore.h"

#include "Ads/AdRemoval.h"
#include "HappinessClassic/IconSets.h"

#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlinePurchaseInterface.h"
#include "Interfaces/OnlineStoreInterfaceV2.h"
#include "Misc/CoreDelegates.h"
#include "Misc/FileHelper.h"
#include "OnlineError.h"
#include "OnlineSubsystem.h"

#if PLATFORM_ANDROID
// Google Play Billing's acknowledge, from the engine (Launch); Android links everything into one library
extern void AndroidThunkCpp_Iap_AcknowledgePurchase(const FString& PurchaseToken);
#endif

DEFINE_LOG_CATEGORY_STATIC(LogIconSetStore, Log, All);

namespace
{
	const TCHAR* OwnedFileName = TEXT("iconsets.owned");

	// Seconds after startup before the first store check: after Remove Ads' (3) and the streak savers' (8), so they
	// don't ask the store at the same moment
	const double IconSetFirstCheckDelay = 13.0;

#if !UE_BUILD_SHIPPING
	TAutoConsoleVariable<int32> CVarSimulateIconSetStore(
		TEXT("Happiness.IconSets.SimulateStore"),
		0,
		TEXT("1: act as a phone does, the Purchase icon sets locked until bought; buying grants them at once (no store). ")
		TEXT("Development builds only; reopen the icon sets screen after changing it."));
#endif

	/** Pretending to have a store, where there is none (development builds) */
	bool IsSimulatedStore()
	{
#if !UE_BUILD_SHIPPING && !PLATFORM_ANDROID && !PLATFORM_IOS
		return CVarSimulateIconSetStore.GetValueOnGameThread() != 0;
#else
		return false;
#endif
	}
}

void UIconSetStoreSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// The file decides, with or without a connection
	LoadOwned();
	NextCheck = FPlatformTime::Seconds() + IconSetFirstCheckDelay;
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UIconSetStoreSubsystem::Tick), 1.f);
	ForegroundHandle = FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this, &UIconSetStoreSubsystem::HandleForeground);
}

void UIconSetStoreSubsystem::Deinitialize()
{
	FTSTicker::RemoveTicker(TickHandle);
	FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(ForegroundHandle);
	Super::Deinitialize();
}

UIconSetStoreSubsystem* UIconSetStoreSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UIconSetStoreSubsystem>() : nullptr;
}

bool UIconSetStoreSubsystem::UsesStore()
{
#if PLATFORM_ANDROID || PLATFORM_IOS
	return true;
#else
	return IsSimulatedStore();
#endif
}

FString UIconSetStoreSubsystem::GetProductId(FName SetId) const
{
	return ProductPrefix + SetId.ToString().ToLower();
}

bool UIconSetStoreSubsystem::IsOwned(FName SetId) const
{
	return !SetId.IsNone() && Owned.Contains(GetProductId(SetId));
}

bool UIconSetStoreSubsystem::CanBuy() const
{
	if (IsSimulatedStore())
	{
		return PurchasingSet.IsNone();
	}
	return UsesStore() && bStoreReached && PurchasingSet.IsNone() && GetStore();
}

bool UIconSetStoreSubsystem::IsPending(FName SetId) const
{
	return Pending.Contains(GetProductId(SetId));
}

FText UIconSetStoreSubsystem::GetPriceText(FName SetId) const
{
	const FText* Price = Prices.Find(GetProductId(SetId));
	return Price ? *Price : FText::GetEmpty();
}

FString UIconSetStoreSubsystem::GetOwnedPath() const
{
	return UAdRemovalSubsystem::GetPrivateFilePath(OwnedFileName);
}

void UIconSetStoreSubsystem::LoadOwned()
{
	// One purchase a line: "<product> <transaction>"
	Owned.Reset();
	TArray<FString> Lines;
	if (FFileHelper::LoadFileToStringArray(Lines, *GetOwnedPath()))
	{
		for (const FString& Line : Lines)
		{
			FString Product, Transaction;
			if (!Line.Split(TEXT(" "), &Product, &Transaction))
			{
				Product = Line;
			}
			Product.TrimStartAndEndInline();
			if (!Product.IsEmpty())
			{
				Owned.Add(Product, Transaction.TrimStartAndEnd());
			}
		}
	}
	UE_LOG(LogIconSetStore, Log, TEXT("Icon sets: %d bought (%s)"), Owned.Num(), *GetOwnedPath());
}

void UIconSetStoreSubsystem::SaveOwned() const
{
	FString Text;
	for (const TPair<FString, FString>& Purchase : Owned)
	{
		Text += Purchase.Key + TEXT(" ") + Purchase.Value + TEXT("\n");
	}
	const FString Path = GetOwnedPath();
	if (Owned.Num() == 0)
	{
		IFileManager::Get().Delete(*Path);
	}
	else if (!FFileHelper::SaveStringToFile(Text, *Path))
	{
		UE_LOG(LogIconSetStore, Error, TEXT("Icon sets: couldn't write %s"), *Path);
	}
}

void UIconSetStoreSubsystem::SetOwned(const FString& ProductId, bool bOwned, const FString& TransactionId)
{
	Pending.Remove(ProductId);
	if (bOwned == Owned.Contains(ProductId))
	{
		return;
	}
	if (bOwned)
	{
		Owned.Add(ProductId, TransactionId.IsEmpty() ? TEXT("-") : TransactionId);
		UE_LOG(LogIconSetStore, Log, TEXT("Icon sets: %s owned"), *ProductId);
	}
	else
	{
		Owned.Remove(ProductId);
		UE_LOG(LogIconSetStore, Log, TEXT("Icon sets: %s not owned (refunded?), removed"), *ProductId);
	}
	SaveOwned();
	if (bOwned)
	{
		if (const FName* SetId = GetProducts().Find(ProductId))
		{
			OnSetOwned.Broadcast(*SetId);
		}
	}
	OnChanged.Broadcast();
}

TMap<FString, FName> UIconSetStoreSubsystem::GetProducts() const
{
	TMap<FString, FName> Products;
	if (const UIconSetSubsystem* IconSets = GetGameInstance() ? GetGameInstance()->GetSubsystem<UIconSetSubsystem>() : nullptr)
	{
		for (const FName SetId : IconSets->GetPurchaseSetIds())
		{
			Products.Add(GetProductId(SetId), SetId);
		}
	}
	return Products;
}

FString UIconSetStoreSubsystem::GetOfferProduct(const FString& OfferId)
{
	// Google Play may report it with its type in front ("inapp:iconset_birds")
	FString Type, Product;
	return OfferId.Split(TEXT(":"), &Type, &Product) ? Product : OfferId;
}

IOnlineSubsystem* UIconSetStoreSubsystem::GetStore() const
{
#if PLATFORM_ANDROID
	return IOnlineSubsystem::Get(TEXT("GooglePlay"));
#elif PLATFORM_IOS
	return IOnlineSubsystem::Get(TEXT("IOS"));
#else
	return nullptr;
#endif
}

TSharedPtr<const FUniqueNetId> UIconSetStoreSubsystem::GetUserId() const
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

bool UIconSetStoreSubsystem::IsOnline()
{
	return FPlatformMisc::GetNetworkConnectionType() != ENetworkConnectionType::None &&
		FPlatformMisc::GetNetworkConnectionType() != ENetworkConnectionType::AirplaneMode;
}

bool UIconSetStoreSubsystem::Tick(float DeltaTime)
{
	// Reach the store once per run, and again after each return to the foreground (more often while it can't be reached)
	if (GetStore() && !bStoreReached && !bChecking && FPlatformTime::Seconds() >= NextCheck)
	{
		CheckStore(false);
	}
	return true;
}

void UIconSetStoreSubsystem::HandleForeground()
{
	// Purchases may have changed while away (bought on another device, refunded, a pending payment cleared)
	bStoreReached = false;
	NextCheck = FPlatformTime::Seconds() + 1.0;
}

void UIconSetStoreSubsystem::CheckStore(bool bRestore)
{
	IOnlineSubsystem* Store = GetStore();
	const IOnlinePurchasePtr Purchase = Store ? Store->GetPurchaseInterface() : nullptr;
	const TSharedPtr<const FUniqueNetId> UserId = GetUserId();
	if (!Purchase.IsValid() || !UserId.IsValid())
	{
		NextCheck = FPlatformTime::Seconds() + RetryInterval;
		return;
	}
	if (bChecking)
	{
		return;
	}
	bChecking = true;
	TWeakObjectPtr<UIconSetStoreSubsystem> WeakThis(this);
	Purchase->QueryReceipts(*UserId, bRestore, FOnQueryReceiptsComplete::CreateLambda([WeakThis](const FOnlineError& Result)
	{
		UIconSetStoreSubsystem* This = WeakThis.Get();
		if (!This)
		{
			return;
		}
		This->bChecking = false;
		if (!Result.WasSuccessful())
		{
			// Offline or the store isn't ready: keep what we have (the file stays), try again later
			UE_LOG(LogIconSetStore, Log, TEXT("Icon sets: store not reached (%s); retrying later"), *Result.ToLogString());
			This->NextCheck = FPlatformTime::Seconds() + This->RetryInterval;
			This->OnChanged.Broadcast();
			return;
		}
		This->bStoreReached = true;
		const TSet<FString> Listed = This->HandleReceipts();
		This->Unconfirmed.Reset();
		for (const TPair<FString, FString>& Purchase : This->Owned)
		{
			if (!Listed.Contains(Purchase.Key))
			{
				This->Unconfirmed.Add(Purchase.Key);
			}
		}
		// Also the server check: removing a purchase waits for it
		This->QueryOffers();
		This->OnChanged.Broadcast();
	}));
}

TSet<FString> UIconSetStoreSubsystem::HandleReceipts()
{
	TSet<FString> Listed;
	IOnlineSubsystem* Store = GetStore();
	const IOnlinePurchasePtr Purchase = Store ? Store->GetPurchaseInterface() : nullptr;
	const TSharedPtr<const FUniqueNetId> UserId = GetUserId();
	if (!Purchase.IsValid() || !UserId.IsValid())
	{
		return Listed;
	}
	TArray<FPurchaseReceipt> Receipts;
	Purchase->GetReceipts(*UserId, Receipts);

	const TMap<FString, FName> Products = GetProducts();
	for (const FPurchaseReceipt& Receipt : Receipts)
	{
		for (const FPurchaseReceipt::FReceiptOfferEntry& Offer : Receipt.ReceiptOffers)
		{
			const FString ProductId = GetOfferProduct(Offer.OfferId);
			if (!Products.Contains(ProductId))
			{
				continue;
			}
			Listed.Add(ProductId);
			// Paid: the store gives it validation info; a pending (not yet paid) purchase has none
			const bool bPaid = Receipt.TransactionState == EPurchaseTransactionState::Purchased &&
				Offer.LineItems.ContainsByPredicate([](const FPurchaseReceipt::FLineItemInfo& Item) { return Item.IsRedeemable(); });
			if (bPaid)
			{
				// Acknowledged every time it's seen: one the app didn't get to acknowledge would be refunded in 3 days
				AcknowledgePurchase(Receipt.TransactionId);
				SetOwned(ProductId, true, Receipt.TransactionId);
			}
			else if (Receipt.TransactionState == EPurchaseTransactionState::Deferred || Receipt.TransactionState == EPurchaseTransactionState::Purchased)
			{
				if (!Owned.Contains(ProductId))
				{
					Pending.Add(ProductId);
				}
			}
		}
	}
	return Listed;
}

void UIconSetStoreSubsystem::AcknowledgePurchase(const FString& TransactionId)
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

void UIconSetStoreSubsystem::QueryOffers()
{
	IOnlineSubsystem* Store = GetStore();
	const IOnlineStoreV2Ptr StoreV2 = Store ? Store->GetStoreV2Interface() : nullptr;
	const TSharedPtr<const FUniqueNetId> UserId = GetUserId();
	TArray<FString> ProductIds;
	GetProducts().GetKeys(ProductIds);
	if (!StoreV2.IsValid() || !UserId.IsValid() || ProductIds.Num() == 0)
	{
		return;
	}
	TWeakObjectPtr<UIconSetStoreSubsystem> WeakThis(this);
	StoreV2->QueryOffersById(*UserId, ProductIds, FOnQueryOnlineStoreOffersComplete::CreateLambda(
		[WeakThis](bool bWasSuccessful, const TArray<FUniqueOfferId>& OfferIds, const FString& Error)
	{
		UIconSetStoreSubsystem* This = WeakThis.Get();
		IOnlineSubsystem* Store = This ? This->GetStore() : nullptr;
		const IOnlineStoreV2Ptr StoreV2 = Store ? Store->GetStoreV2Interface() : nullptr;
		if (!StoreV2.IsValid())
		{
			return;
		}
		TSet<FString> Found;
		for (const FUniqueOfferId& OfferId : OfferIds)
		{
			if (const TSharedPtr<FOnlineStoreOffer> Offer = StoreV2->GetOffer(OfferId))
			{
				const FString ProductId = GetOfferProduct(Offer->OfferId);
				This->Prices.Add(ProductId, Offer->GetDisplayPrice());
				Found.Add(ProductId);
			}
		}
		if (!bWasSuccessful)
		{
			UE_LOG(LogIconSetStore, Warning, TEXT("Icon sets: couldn't get the prices: %s"), *Error);
		}

		// A product's details came from the store's servers: its receipt list was current, so a missing purchase is gone
		const TSet<FString> Unconfirmed = MoveTemp(This->Unconfirmed);
		This->Unconfirmed.Reset();
		for (const FString& ProductId : Unconfirmed)
		{
			if (bWasSuccessful && Found.Contains(ProductId) && IsOnline())
			{
				This->SetOwned(ProductId, false, FString());
			}
			else
			{
				UE_LOG(LogIconSetStore, Log, TEXT("Icon sets: %s not in the receipts, but the store's servers weren't confirmed; keeping it"), *ProductId);
			}
		}
		This->OnChanged.Broadcast();
	}));
}

bool UIconSetStoreSubsystem::Buy(FName SetId)
{
	if (SetId.IsNone() || IsOwned(SetId) || !CanBuy())
	{
		return false;
	}
	const FString ProductId = GetProductId(SetId);
	if (IsSimulatedStore())
	{
		UE_LOG(LogIconSetStore, Display, TEXT("Icon sets: no store here, test purchase of %s"), *ProductId);
		SetOwned(ProductId, true, TEXT("test"));
		return true;
	}

	IOnlineSubsystem* Store = GetStore();
	const IOnlinePurchasePtr Purchase = Store ? Store->GetPurchaseInterface() : nullptr;
	const TSharedPtr<const FUniqueNetId> UserId = GetUserId();
	if (!Purchase.IsValid() || !UserId.IsValid())
	{
		return false;
	}
	FPurchaseCheckoutRequest Request;
	Request.AddPurchaseOffer(TEXT(""), ProductId, 1, false);
	PurchasingSet = SetId;
	OnChanged.Broadcast();

	TWeakObjectPtr<UIconSetStoreSubsystem> WeakThis(this);
	Purchase->Checkout(*UserId, Request, FOnPurchaseCheckoutComplete::CreateLambda(
		[WeakThis, ProductId](const FOnlineError& Result, const TSharedRef<FPurchaseReceipt>& Receipt)
	{
		UIconSetStoreSubsystem* This = WeakThis.Get();
		if (!This)
		{
			return;
		}
		This->PurchasingSet = NAME_None;
		UE_LOG(LogIconSetStore, Log, TEXT("Icon sets: purchase of %s finished: %s, state %d"), *ProductId, *Result.ToLogString(),
			static_cast<int32>(Receipt->TransactionState));
		if (Receipt->TransactionState == EPurchaseTransactionState::Purchased)
		{
			// The store also lists it as owned now
			This->HandleReceipts();
			if (!This->Owned.Contains(ProductId))
			{
				This->AcknowledgePurchase(Receipt->TransactionId);
				This->SetOwned(ProductId, true, Receipt->TransactionId);
			}
		}
		else if (Receipt->TransactionState == EPurchaseTransactionState::Deferred)
		{
			// Paid later (e.g. cash at a shop): the next store check finds it once it clears
			This->Pending.Add(ProductId);
			This->bStoreReached = false;
			This->NextCheck = FPlatformTime::Seconds() + This->RetryInterval;
		}
		This->OnChanged.Broadcast();
	}));
	return true;
}

void UIconSetStoreSubsystem::RestorePurchases()
{
	if (!bChecking && GetStore())
	{
		CheckStore(true);
	}
}

void UIconSetStoreSubsystem::DebugSetOwned(FName SetId, bool bOwned)
{
	if (!SetId.IsNone())
	{
		SetOwned(GetProductId(SetId), bOwned, TEXT("debug"));
		return;
	}
	for (const TPair<FString, FName>& Product : GetProducts())
	{
		SetOwned(Product.Key, bOwned, TEXT("debug"));
	}
}

#if !UE_BUILD_SHIPPING
static FAutoConsoleCommandWithWorldAndArgs GIconSetOwnedCommand(
	TEXT("Happiness.IconSets.Owned"),
	TEXT("Happiness.IconSets.Owned <SetId|all> <1|0>: own a Purchase icon set (or all of them) or not, as if bought or refunded. ")
	TEXT("No argument: check the store now. Dev builds only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		UIconSetStoreSubsystem* IconSetStore = UIconSetStoreSubsystem::Get(World);
		if (!IconSetStore)
		{
			return;
		}
		if (Args.Num() == 0)
		{
			IconSetStore->DebugCheckStore();
			return;
		}
		const FName SetId = Args[0].Equals(TEXT("all"), ESearchCase::IgnoreCase) ? NAME_None : FName(*Args[0]);
		IconSetStore->DebugSetOwned(SetId, Args.Num() < 2 || FCString::Atoi(*Args[1]) != 0);
		UE_LOG(LogIconSetStore, Display, TEXT("Icon sets: %s %s"), *Args[0], Args.Num() < 2 || FCString::Atoi(*Args[1]) != 0 ? TEXT("owned") : TEXT("not owned"));
	}));
#endif
