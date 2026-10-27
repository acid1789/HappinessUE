#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CampaignProgress.h"
#include "FreePlaySettings.h"
#include "DailyPuzzle.h"
#include "PuzzleSlots.h"
#include "HappinessSaveGame.generated.h"

/**
 * Parent of SG_Happiness, the game's save. Adds the campaign progress and the free play settings to it.
 *
 * Whenever a save game is written to disk it takes the running game's current campaign progress and free play
 * settings, so saving the game from Blueprint (which creates a fresh SG_Happiness each time) never loses them.
 *
 * Each mode's puzzle in progress also has a save slot of its own, an SG_Happiness holding just that puzzle
 * (UPuzzleSlotSubsystem): writing the game save copies its puzzle there, and loading it to resume a mode's puzzle
 * copies that slot's puzzle in.
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

	/**
	 * Always false. WBP_GameSelect asks this to resume a puzzle in progress when the game starts, but puzzles are now
	 * resumed per mode, when the player chooses it on the mode screen (UPuzzleSlotSubsystem), so the game always
	 * starts on the mode screen.
	 */
	UFUNCTION(BlueprintPure, Category = "Happiness")
	static bool HasActivePuzzle();

	UPROPERTY(SaveGame)
	FCampaignSaveData Campaign;

	UPROPERTY(SaveGame)
	FFreePlaySaveData FreePlay;

	UPROPERTY(SaveGame)
	FDailySaveData Daily;

	/** Ads turned off (UAdsSubsystem) */
	UPROPERTY(SaveGame)
	bool bAdsDisabled = false;

	/** A mode's puzzle slot: which mode, and what tells whether its puzzle can still be resumed */
	UPROPERTY(SaveGame)
	FPuzzleSlotInfo PuzzleSlot;

private:
	/** Read from disk, so not a new save of the current game: writing it again doesn't touch the puzzle slots */
	bool bLoadedFromDisk = false;
};
