#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CampaignProgress.h"
#include "HappinessSaveGame.generated.h"

/**
 * Parent of SG_Happiness, the game's save. Adds the campaign progress to it.
 *
 * Whenever a save game is written to disk it takes the running game's current campaign progress, so saving the
 * game from Blueprint (which creates a fresh SG_Happiness each time) never loses it.
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

	UPROPERTY(SaveGame)
	FCampaignSaveData Campaign;
};
