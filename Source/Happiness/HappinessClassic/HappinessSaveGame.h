#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CampaignProgress.h"
#include "FreePlaySettings.h"
#include "IconSets.h"
#include "DailyPuzzle.h"
#include "PuzzleSlots.h"
#include "HappinessSaveGame.generated.h"

/**
 * Parent of SG_Happiness, the game's save. Adds the campaign progress, the free play settings and the icon set choices to it.
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
	 * A save from the game before lessons and the campaign (the one on Play before this version) starts the game
	 * over, as for a new player: the first time the game save is read (LoadFromSlot, or the campaign's start), it is
	 * replaced by a fresh one that keeps just the player's level and experience, and the puzzle slots are deleted.
	 * Such a save has no DataVersion and no campaign progress; saves since have one or both, and are kept.
	 */
	static void ResetOldSave();

	/** ResetOldSave replaced an old save this run (the game starts as for a new player) */
	static bool WasOldSaveReset() { return bOldSaveReset; }

	/** The run is over (the game instance is going): the next one checks again (in the editor, the next PIE) */
	static void ForgetOldSaveCheck() { bOldSaveChecked = bOldSaveReset = false; }

	/** Written into every save from this version on; older saves read as 0 */
	static constexpr int32 CurrentDataVersion = 1;

	UPROPERTY(SaveGame)
	int32 DataVersion = 0;

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

	/** The icon sets the player turned off (UIconSetSubsystem) */
	UPROPERTY(SaveGame)
	FIconSetSaveData IconSetChoices;

	/** Ads turned off (UAdsSubsystem) */
	UPROPERTY(SaveGame)
	bool bAdsDisabled = false;

	/** A mode's puzzle slot: which mode, and what tells whether its puzzle can still be resumed */
	UPROPERTY(SaveGame)
	FPuzzleSlotInfo PuzzleSlot;

private:
	/** Read from disk, so not a new save of the current game: writing it again doesn't touch the puzzle slots */
	bool bLoadedFromDisk = false;

	static bool bOldSaveChecked;
	static bool bOldSaveReset;
};
