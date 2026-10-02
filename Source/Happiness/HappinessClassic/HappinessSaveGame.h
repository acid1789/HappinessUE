#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CampaignProgress.h"
#include "FreePlaySettings.h"
#include "DailyPuzzle.h"
#include "HappinessSaveGame.generated.h"

/**
 * Parent of SG_Happiness, the game's save. Adds the campaign progress and the free play settings to it.
 *
 * Whenever a save game is written to disk it takes the running game's current campaign progress and free play
 * settings, so saving the game from Blueprint (which creates a fresh SG_Happiness each time) never loses them.
 */
UCLASS(Blueprintable)
class HAPPINESS_API UHappinessSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Must match PC_Happiness's HappinessSaveSlotName */
	static constexpr const TCHAR* SaveSlotName = TEXT("HappinessSave");
	static constexpr int32 SaveUserIndex = 0;

	virtual void Serialize(FArchive& Ar) override;

	/** The game save on disk, or null if there isn't one (or it isn't a UHappinessSaveGame) */
	static UHappinessSaveGame* LoadFromSlot();

	/**
	 * Rewrite the game save with the current campaign progress and free play settings, keeping the rest of it (the
	 * classic puzzle, experience, icons) as it is on disk. With no game save yet they go to disk with the game's first save.
	 */
	static void SaveCurrentSettings();

	/** True if the game save holds a puzzle in progress (SG_Happiness's ActivePuzzle): the player left it with Save and Quit */
	UFUNCTION(BlueprintPure, Category = "Happiness")
	static bool HasActivePuzzle();

	UPROPERTY(SaveGame)
	FCampaignSaveData Campaign;

	UPROPERTY(SaveGame)
	FFreePlaySaveData FreePlay;

	UPROPERTY(SaveGame)
	FDailySaveData Daily;
};
