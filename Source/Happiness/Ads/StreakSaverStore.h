#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StreakSaverStore.generated.h"

/** The streak saver products: how many missed days each covers */
UENUM(BlueprintType)
enum class EStreakSaverTier : uint8
{
	OneDay,
	SevenDays,
	ThirtyDays,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStreakSaverStoreChanged);

/**
 * Buying streak savers: three consumable in-app products (1, 7 and 30 days; prices are set in the stores). A purchase
 * becomes saver days in the daily streak (UDailySubsystem::AddSaverDays, which remembers the transaction so it is never
 * counted twice) and is then consumed, so it can be bought again. A purchase the app didn't get to finish (closed
 * mid-purchase, or a payment that cleared later) is picked up the next time the store is reached.
 *
 * Store: as UAdRemovalSubsystem (Google Play Billing on Android, StoreKit on iOS). The engine's own consume is turned
 * off for Remove Ads, so these are consumed here. Without a store (editor, PC), development builds grant the days at
 * once so the flow can be tried.
 */
UCLASS(Config = Game)
class HAPPINESS_API UStreakSaverStoreSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** The store product ids (Play Console in-app products / App Store Connect consumables) */
	UPROPERTY(Config)
	FString OneDayProductId = TEXT("streak_saver_1");

	UPROPERTY(Config)
	FString SevenDaysProductId = TEXT("streak_saver_7");

	UPROPERTY(Config)
	FString ThirtyDaysProductId = TEXT("streak_saver_30");

	/** Seconds between attempts to reach the store while it can't be reached */
	UPROPERTY(Config)
	float RetryInterval = 30.f;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "Streak Saver", meta = (WorldContext = "WorldContextObject"))
	static UStreakSaverStoreSubsystem* Get(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Streak Saver")
	static int32 GetTierDays(EStreakSaverTier Tier);

	/** The store can take a purchase now */
	UFUNCTION(BlueprintPure, Category = "Streak Saver")
	bool CanBuy() const;

	/** A purchase flow is open */
	UFUNCTION(BlueprintPure, Category = "Streak Saver")
	bool IsPurchasing() const { return bPurchasing; }

	/** A purchase is waiting for its payment to clear */
	UFUNCTION(BlueprintPure, Category = "Streak Saver")
	bool IsPending() const { return bPending; }

	/** The store's localized price, once it has been reached; empty until then */
	UFUNCTION(BlueprintPure, Category = "Streak Saver")
	FText GetPriceText(EStreakSaverTier Tier) const;

	UFUNCTION(BlueprintCallable, Category = "Streak Saver")
	void Buy(EStreakSaverTier Tier);

	/** Something above changed */
	UPROPERTY(BlueprintAssignable, Category = "Streak Saver")
	FOnStreakSaverStoreChanged OnChanged;

private:
	const FString& GetProductId(EStreakSaverTier Tier) const;
	/** The tier a store offer id is for; false if it isn't a streak saver */
	bool FindTier(const FString& OfferId, EStreakSaverTier& OutTier) const;

	bool Tick(float DeltaTime);
	void CheckStore();
	void QueryPrices();
	/** Turn the store's paid, unconsumed streak saver purchases into saver days, and consume them */
	void HandleReceipts();
	/** Grant one purchase's days (once per transaction), then consume it */
	void Deliver(const FString& TransactionId, EStreakSaverTier Tier, int32 Quantity);
	void Consume(const FString& TransactionId);

	class IOnlineSubsystem* GetStore() const;
	TSharedPtr<const class FUniqueNetId> GetUserId() const;

	bool bStoreReached = false;
	bool bChecking = false;
	bool bPurchasing = false;
	bool bPending = false;
	TMap<EStreakSaverTier, FText> Prices;
	double NextCheck = 0.0;
	FTSTicker::FDelegateHandle TickHandle;
};
