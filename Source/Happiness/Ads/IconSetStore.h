#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "IconSetStore.generated.h"

/**
 * Buying icon sets on phones: each DA_IconSetUnlocks set with the Purchase condition is a one-time (non-consumable)
 * in-app product, ProductPrefix + its set Id in lower case ("iconset_arcticanimals"). Made to work offline, like
 * UAdRemovalSubsystem:
 *
 * - A purchase is written to a file in the app's private storage (iconsets.owned, one product a line). The sets it
 *   lists are unlocked, with or without a connection.
 * - When the store can be reached (at startup, and each time the app comes back to the foreground), its receipts are
 *   compared with the file: a paid purchase missing from the file is added (a reinstall, a new phone, a purchase the
 *   app didn't get to finish); one in the file but not in the store's list is removed (refunded), but only when the
 *   store's servers were really reached (the phone is online and the products' details came back): Google Play
 *   answers "nothing owned" from its offline cache too, which alone could take a set away from an offline player.
 *
 * Purchases are acknowledged, never consumed (see UAdRemovalSubsystem). Only phones have a store (UsesStore): on PC
 * the Purchase sets are simply unlocked. Development builds can pretend to have one (Happiness.IconSets.SimulateStore
 * 1), buying at once without a store.
 */
UCLASS(Config = Game)
class HAPPINESS_API UIconSetStoreSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Product id of a set: this + its Id in lower case (Play Console in-app products / App Store Connect non-consumables) */
	UPROPERTY(Config)
	FString ProductPrefix = TEXT("iconset_");

	/** Seconds between attempts to reach the store while it can't be reached */
	UPROPERTY(Config)
	float RetryInterval = 30.f;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static UIconSetStoreSubsystem* Get(const UObject* WorldContextObject);

	/** Purchase sets are bought here (phones), rather than unlocked from the start (PC) */
	static bool UsesStore();

	FString GetProductId(FName SetId) const;

	/** The set was bought (it's in the purchases file) */
	bool IsOwned(FName SetId) const;

	/** The store can take a purchase now */
	bool CanBuy() const;

	/** This set's purchase flow is open */
	bool IsPurchasing(FName SetId) const { return PurchasingSet == SetId && !SetId.IsNone(); }

	/** Bought, waiting for the payment to clear */
	bool IsPending(FName SetId) const;

	/** The store's localized price, once it has been reached; empty until then */
	FText GetPriceText(FName SetId) const;

	/** Start the store's purchase flow for a set. False if the store can't take it now. */
	bool Buy(FName SetId);

	/** Ask the store for the player's purchases now (the App Store's Restore Purchases) */
	void RestorePurchases();

	/** Something above changed: owned sets, prices, a purchase started or ended */
	FSimpleMulticastDelegate OnChanged;

	/** A set just became owned (bought, or found in the store's receipts) */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSetOwned, FName);
	FOnSetOwned OnSetOwned;

	/** Testing: own a set (or every Purchase set, None) or not, as if bought or refunded */
	void DebugSetOwned(FName SetId, bool bOwned);
	/** Testing: check the store again now */
	void DebugCheckStore() { CheckStore(false); }

private:
	FString GetOwnedPath() const;
	void LoadOwned();
	void SaveOwned() const;
	void SetOwned(const FString& ProductId, bool bOwned, const FString& TransactionId);

	/** The Purchase sets' product ids, by product id their set Id (DA_IconSetUnlocks) */
	TMap<FString, FName> GetProducts() const;
	/** "inapp:iconset_x" -> "iconset_x" */
	static FString GetOfferProduct(const FString& OfferId);

	void CheckStore(bool bRestore);
	/** Go through the store's receipts: own the paid sets. Returns the products listed (paid or pending). */
	TSet<FString> HandleReceipts();
	/** Prices, and the server check: removes the Unconfirmed products if the store's servers say they aren't owned */
	void QueryOffers();
	void AcknowledgePurchase(const FString& TransactionId);
	bool Tick(float DeltaTime);
	void HandleForeground();
	static bool IsOnline();

	class IOnlineSubsystem* GetStore() const;
	TSharedPtr<const class FUniqueNetId> GetUserId() const;

	/** Owned products, with the transaction that bought each */
	TMap<FString, FString> Owned;
	TSet<FString> Pending;
	/** Owned here, but the last receipt list didn't have them: removed if the servers confirm */
	TSet<FString> Unconfirmed;
	TMap<FString, FText> Prices;
	FName PurchasingSet;
	bool bStoreReached = false;
	bool bChecking = false;
	double NextCheck = 0.0;
	FTSTicker::FDelegateHandle TickHandle;
	FDelegateHandle ForegroundHandle;
};
