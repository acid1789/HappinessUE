#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AdRemoval.generated.h"

UENUM(BlueprintType)
enum class ERemoveAdsState : uint8
{
	Unavailable,	// no store to talk to (offline, store not ready, or not on a phone)
	Available,		// can be bought
	Purchasing,		// the store's purchase flow is open
	Pending,		// bought, waiting for payment to clear (the store finishes it later)
	Purchased,		// ads are removed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRemoveAdsChanged);

/**
 * The "Remove Ads" in-app purchase (a one-time, non-consumable product), made to work offline:
 *
 * - Buying it writes a token file to the app's private storage (on Android its internal files folder). While the
 *   token exists, ads are off, with or without a connection.
 * - No token: when the store can be reached, ask it whether the player owns the product (a reinstall, a new phone);
 *   if so, write the token and turn ads off.
 * - Token: when the store's servers can be reached, check again; if the product isn't owned any more (refunded),
 *   delete the token and ads come back. Only a check that really reached the servers can remove it (the phone is
 *   online and the product's details came back from the store): Google Play answers "nothing owned" from its offline
 *   cache too, so that answer alone could take the purchase away from an offline player.
 *
 * Store: Unreal's online purchase interface (Google Play Billing on Android, StoreKit on iOS). The Google Play
 * purchase is acknowledged, never consumed (consuming would make it look unowned), so the engine's own
 * acknowledge-and-consume is turned off in DefaultEngine.ini ([OnlineSubsystemGooglePlay.Store]
 * bDisableLocalAcknowledgeAndConsume).
 */
UCLASS(Config = Game)
class HAPPINESS_API UAdRemovalSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** The store's product id (Play Console in-app product / App Store Connect non-consumable) */
	UPROPERTY(Config)
	FString ProductId = TEXT("remove_ads");

	/** Seconds between attempts to reach the store while it can't be reached */
	UPROPERTY(Config)
	float RetryInterval = 30.f;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "Ads", meta = (WorldContext = "WorldContextObject"))
	static UAdRemovalSubsystem* Get(const UObject* WorldContextObject);

	/** Ads are removed (the token exists) */
	UFUNCTION(BlueprintPure, Category = "Ads")
	bool AreAdsRemoved() const { return bHasToken; }

	UFUNCTION(BlueprintPure, Category = "Ads")
	ERemoveAdsState GetState() const;

	/** The store's localized price, once it has been reached; empty until then */
	UFUNCTION(BlueprintPure, Category = "Ads")
	FText GetPriceText() const { return PriceText; }

	/** True while a restore (store check asked for by the player) is running */
	UFUNCTION(BlueprintPure, Category = "Ads")
	bool IsRestoring() const { return bRestoring; }

	/** Start the store's purchase flow */
	UFUNCTION(BlueprintCallable, Category = "Ads")
	void BuyRemoveAds();

	/** Ask the store for the player's purchases now (the App Store requires a button for this) */
	UFUNCTION(BlueprintCallable, Category = "Ads")
	void RestorePurchases();

	/** Something above changed (state, price, restore result) */
	UPROPERTY(BlueprintAssignable, Category = "Ads")
	FOnRemoveAdsChanged OnChanged;

	/** Testing: create or delete the token as if bought or refunded */
	void DebugSetToken(bool bOwned);
	/** Testing: check the store again now */
	void DebugCheckStore() { CheckStore(false); }

private:
	FString GetTokenPath() const;
	void SetOwned(bool bOwned, const FString& TransactionId);
	void ApplyToAds() const;

	/** Ask the store which products the player owns (and the price); retries later when it can't be reached */
	void CheckStore(bool bRestore);
	void QueryPrice();
	bool Tick(float DeltaTime);
	/** Go through the store's receipt list: own the product if it's paid for. True if it's there (paid or pending). */
	bool HandleReceipts();
	/** The phone has a network connection */
	static bool IsOnline();
	/** Tell the store the purchase was delivered (Google refunds unacknowledged purchases after 3 days) */
	void AcknowledgePurchase(const FString& TransactionId);

	class IOnlineSubsystem* GetStore() const;
	TSharedPtr<const class FUniqueNetId> GetUserId() const;

	bool bHasToken = false;
	bool bStoreReached = false;
	bool bChecking = false;
	bool bRestoring = false;
	bool bPurchasing = false;
	bool bPending = false;
	/** The last receipt list didn't have the product although the token exists: revoke if the servers confirm */
	bool bRevokeIfConfirmed = false;
	FText PriceText;
	double NextCheck = 0.0;
	FTSTicker::FDelegateHandle TickHandle;
};
