#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "IconSets.generated.h"

class UTexture2D;

/** When an unlockable icon set becomes available */
UENUM()
enum class EIconSetUnlock : uint8
{
	/** Every lesson is completed (Campaign mode unlocked) */
	LessonsCompleted,
};

USTRUCT()
struct FIconSetUnlock
{
	GENERATED_BODY()

	/** A DA_IconSet */
	UPROPERTY(EditAnywhere, Category = "Icon Sets")
	TObjectPtr<UPrimaryDataAsset> IconSet;

	UPROPERTY(EditAnywhere, Category = "Icon Sets")
	EIconSetUnlock Unlock = EIconSetUnlock::LessonsCompleted;

	/** Left out of the icon sets screen until it is unlocked (a surprise); otherwise shown locked, with the reason */
	UPROPERTY(EditAnywhere, Category = "Icon Sets")
	bool bHiddenUntilUnlocked = false;

	/** What the icon sets screen says it takes to unlock; empty for the unlock condition's own text */
	UPROPERTY(EditAnywhere, Category = "Icon Sets")
	FText LockedReason;
};

/**
 * DA_IconSetUnlocks (/Game/Happiness/IconSets): the icon sets the player can unlock beyond the default collection.
 * Always cooked (DefaultGame.ini's Asset Manager rules), and the sets it references with it.
 */
UCLASS()
class HAPPINESS_API UIconSetUnlocks : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Icon Sets")
	TArray<FIconSetUnlock> IconSets;

	/** The names the icon sets screen shows, by set Id (DA_IconSet's Id); a set without one shows its Id, spaced out */
	UPROPERTY(EditAnywhere, Category = "Icon Sets")
	TMap<FName, FText> SetNames;
};

/** The player's icon set choices, in the game save */
USTRUCT()
struct FIconSetSaveData
{
	GENERATED_BODY()

	/** Sets the player turned off (their Ids): new puzzles don't use them */
	UPROPERTY(SaveGame)
	TArray<FName> DisabledSets;
};

/** One set on the icon sets screen */
struct FIconSetEntry
{
	UPrimaryDataAsset* IconSet = nullptr;
	FName Id;
	FText Name;
	bool bUnlocked = false;
	/** Unlocked and not turned off */
	bool bEnabled = false;
	/** Why it is locked (when it is) */
	FText LockedReason;
};

/**
 * The player's own icon set collection: the default collection's sets (PC_Happiness's SoftIconSetCollection), then
 * each unlocked set from DA_IconSetUnlocks. What's unlocked follows from the player's progress; the sets the player
 * turned off are saved (FIconSetSaveData), and new puzzles pick their sets from the rest (IsIconSetEnabled).
 */
UCLASS()
class HAPPINESS_API UIconSetSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** A puzzle has up to 8 rows, one set each: the player can't turn off sets below this */
	static constexpr int32 MinEnabledSets = 8;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static UIconSetSubsystem* Get(const UObject* WorldContextObject);
	static UIconSetSubsystem* GetInstance() { return Instance.Get(); }

	/**
	 * A DA_IconSetCollection like DefaultCollection, with the unlocked sets added after its own. It stays up to date:
	 * sets unlocked later are added to it. PC_Happiness uses it in place of the default collection (BeginPlay).
	 * It holds every unlocked set, turned off or not, so a saved puzzle always finds its sets.
	 */
	UFUNCTION(BlueprintCallable, Category = "Icon Sets", meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "DefaultCollection"))
	static UPrimaryDataAsset* GetPlayerCollection(const UObject* WorldContextObject, UPrimaryDataAsset* DefaultCollection);

	/**
	 * Whether a new puzzle may use IconSet (PC_Happiness's SelectRandomIconSets): true unless the player turned it
	 * off. If somehow fewer than MinEnabledSets are left on, every set is.
	 */
	UFUNCTION(BlueprintPure, Category = "Icon Sets", meta = (WorldContext = "WorldContextObject"))
	static bool IsIconSetEnabled(const UObject* WorldContextObject, const UObject* IconSet);

	bool IsUnlocked(const FIconSetUnlock& Unlock) const;

	/** The sets the icon sets screen shows: the default ones, then each unlockable one that is unlocked or not hidden */
	TArray<FIconSetEntry> GetEntries() const;

	/** How many unlocked sets are on */
	int32 GetNumEnabled() const;

	/** Turn an unlocked set on or off (saved). False, and nothing changes, when it would leave too few on. */
	bool SetEnabled(const UObject* IconSet, bool bEnabled);

	/** An icon set's (DA_IconSet's) Id */
	static FName GetSetId(const UObject* IconSet);

	/** An icon set's icon textures */
	static TArray<UTexture2D*> GetIconTextures(const UObject* IconSet);

	const FIconSetSaveData& GetSaveData() const { return Data; }

	/** Fired when a set is turned on or off */
	FSimpleMulticastDelegate OnEnabledChanged;

private:
	/** Rebuild the player's collection's sets: the default ones, then the unlocked ones */
	UFUNCTION()
	void Refresh();

	UIconSetUnlocks* GetUnlocks() const;
	TArray<UObject*> GetSets(const UObject* Collection) const;
	FText GetSetName(const UObject* IconSet) const;

	UPROPERTY()
	TObjectPtr<UPrimaryDataAsset> DefaultCollection;

	UPROPERTY()
	TObjectPtr<UPrimaryDataAsset> PlayerCollection;

	UPROPERTY()
	TObjectPtr<UIconSetUnlocks> Unlocks;

	FIconSetSaveData Data;

	static TWeakObjectPtr<UIconSetSubsystem> Instance;
};
