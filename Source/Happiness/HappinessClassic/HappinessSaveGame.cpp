#include "HappinessSaveGame.h"

#include "Kismet/GameplayStatics.h"

void UHappinessSaveGame::Serialize(FArchive& Ar)
{
	// Writing a save game: take the current campaign progress. Not for the class default object (saved with the
	// Blueprint asset) or for reference collection and the like.
	if (Ar.IsSaving() && !Ar.IsObjectReferenceCollector() && !Ar.IsCountingMemory() && !Ar.IsTransacting() &&
		!HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		if (const UCampaignSubsystem* Subsystem = UCampaignSubsystem::GetInstance())
		{
			Campaign = Subsystem->GetSaveData();
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
