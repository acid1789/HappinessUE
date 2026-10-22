#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "IconSets.generated.h"

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
};

/**
 * The player's own icon set collection: the default collection's sets (PC_Happiness's SoftIconSetCollection), then
 * each unlocked set from DA_IconSetUnlocks. Nothing is saved: what's unlocked follows from the player's progress.
 */
UCLASS()
class HAPPINESS_API UIconSetSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * A DA_IconSetCollection like DefaultCollection, with the unlocked sets added after its own. It stays up to date:
	 * sets unlocked later are added to it. PC_Happiness uses it in place of the default collection (BeginPlay).
	 */
	UFUNCTION(BlueprintCallable, Category = "Icon Sets", meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "DefaultCollection"))
	static UPrimaryDataAsset* GetPlayerCollection(const UObject* WorldContextObject, UPrimaryDataAsset* DefaultCollection);

	bool IsUnlocked(const FIconSetUnlock& Unlock) const;

private:
	/** Rebuild the player's collection's sets: the default ones, then the unlocked ones */
	UFUNCTION()
	void Refresh();

	UPROPERTY()
	TObjectPtr<UPrimaryDataAsset> DefaultCollection;

	UPROPERTY()
	TObjectPtr<UPrimaryDataAsset> PlayerCollection;

	UPROPERTY()
	TObjectPtr<UIconSetUnlocks> Unlocks;
};
