#include "HappinessSaveGame.h"

#include "Ads/AdsSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UHappinessSaveGame::Serialize(FArchive& Ar)
{
	// Writing a save game: take the current campaign progress, free play and daily settings, and the ads switch. Not for the class default object (saved with the
	// Blueprint asset) or for reference collection and the like.
	if (Ar.IsSaving() && !Ar.IsObjectReferenceCollector() && !Ar.IsCountingMemory() && !Ar.IsTransacting() &&
		!HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		if (const UCampaignSubsystem* CampaignSubsystem = UCampaignSubsystem::GetInstance())
		{
			Campaign = CampaignSubsystem->GetSaveData();
		}
		if (const UFreePlaySubsystem* FreePlaySubsystem = UFreePlaySubsystem::GetInstance())
		{
			FreePlay = FreePlaySubsystem->GetSaveData();
		}
		if (const UDailySubsystem* DailySubsystem = UDailySubsystem::GetInstance())
		{
			Daily = DailySubsystem->GetSaveData();
		}
		if (const UAdsSubsystem* AdsSubsystem = UAdsSubsystem::GetInstance())
		{
			bAdsDisabled = AdsSubsystem->GetAdsDisabledSaveData();
		}
	}

	Super::Serialize(Ar);
}

UHappinessSaveGame* UHappinessSaveGame::LoadFromSlot()
{
	if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex))
	{
		return nullptr;
	}
	return Cast<UHappinessSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex));
}

void UHappinessSaveGame::SaveCurrentSettings()
{
	// Serialize fills in the current settings as the save is written
	if (UHappinessSaveGame* SaveGame = LoadFromSlot())
	{
		UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, SaveUserIndex);
	}
}

bool UHappinessSaveGame::HasActivePuzzle()
{
	const UHappinessSaveGame* SaveGame = LoadFromSlot();
	if (!SaveGame)
	{
		return false;
	}

	// ActivePuzzle is a Blueprint variable of SG_Happiness
	const FBoolProperty* ActivePuzzle = FindFProperty<FBoolProperty>(SaveGame->GetClass(), TEXT("ActivePuzzle"));
	return ActivePuzzle && ActivePuzzle->GetPropertyValue_InContainer(SaveGame);
}
