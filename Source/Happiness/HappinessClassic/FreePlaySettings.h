#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CampaignLesson.h"
#include "FreePlaySettings.generated.h"

/** Free play settings, stored in the normal game save (UHappinessSaveGame::FreePlay) */
USTRUCT()
struct FFreePlaySaveData
{
	GENERATED_BODY()

	/** Clue types the player turned off; everything else is included, so new clue types start on */
	UPROPERTY(SaveGame)
	TArray<ECampaignLesson> ExcludedClues;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFreePlayCluesChanged);

/**
 * Free play settings: which clue types free play puzzles may use (all by default). Givens and NotHeres are always
 * included, so every puzzle can be solved. UCampaignSubsystem::InitPuzzleForPlay applies the selection to free play
 * puzzles.
 */
UCLASS()
class HAPPINESS_API UFreePlaySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** The clue types the player can turn on and off, in campaign order */
	UFUNCTION(BlueprintPure, Category = "Free Play")
	static TArray<ECampaignLesson> GetSelectableClues();

	/** Name of a clue type for the clue list */
	UFUNCTION(BlueprintPure, Category = "Free Play")
	static FText GetClueName(ECampaignLesson Clue);

	UFUNCTION(BlueprintPure, Category = "Free Play")
	bool IsClueIncluded(ECampaignLesson Clue) const;

	UFUNCTION(BlueprintCallable, Category = "Free Play")
	void SetClueIncluded(ECampaignLesson Clue, bool bIncluded);

	UFUNCTION(BlueprintCallable, Category = "Free Play")
	void SetAllCluesIncluded(bool bIncluded);

	UFUNCTION(BlueprintPure, Category = "Free Play")
	int32 GetIncludedClueCount() const;

	/** "All", "12 of 19" or "Givens only" */
	UFUNCTION(BlueprintPure, Category = "Free Play")
	FText GetSelectionSummary() const;

	/** For UPuzzle::m_ExcludedClues */
	int32 GetExcludedClueMask() const;

	UPROPERTY(BlueprintAssignable, Category = "Free Play")
	FOnFreePlayCluesChanged OnCluesChanged;

	static UFreePlaySubsystem* Get(const UObject* WorldContextObject);

	/** The running game's settings, for code without a world context (the save game); null outside a game */
	static UFreePlaySubsystem* GetInstance() { return Instance.Get(); }

	const FFreePlaySaveData& GetSaveData() const { return Data; }

private:
	void Changed();

	FFreePlaySaveData Data;
	static TWeakObjectPtr<UFreePlaySubsystem> Instance;
};
