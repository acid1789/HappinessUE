#include "HappinessSaveGame.h"

#include "Ads/AdsSubsystem.h"
#include "Kismet/GameplayStatics.h"

bool UHappinessSaveGame::bOldSaveChecked = false;
bool UHappinessSaveGame::bOldSaveReset = false;

void UHappinessSaveGame::Serialize(FArchive& Ar)
{
	// Writing a save game: take the current campaign progress, free play and daily settings, the ads switch and the icon set choices. Not for the class default object (saved with the
	// Blueprint asset) or for reference collection and the like.
	const bool bWritingSave = Ar.IsSaving() && !Ar.IsObjectReferenceCollector() && !Ar.IsCountingMemory() && !Ar.IsTransacting() &&
		!HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) && !UPuzzleSlotSubsystem::IsSlotIO();
	if (bWritingSave)
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
		if (const UIconSetSubsystem* IconSetSubsystem = UIconSetSubsystem::GetInstance())
		{
			IconSetChoices = IconSetSubsystem->GetSaveData();
		}
		DataVersion = CurrentDataVersion;
	}

	Super::Serialize(Ar);

	if (bWritingSave && !bLoadedFromDisk)
	{
		// A new save of the game (PC_Happiness's SaveHappiness): its puzzle is the playing mode's
		if (UPuzzleSlotSubsystem* Slots = UPuzzleSlotSubsystem::GetInstance())
		{
			Slots->WriteSlot(*this);
		}
	}
	// Read from a save slot (UGameplayStatics::LoadGameFromSlot)
	if (Ar.IsLoading() && !Ar.IsObjectReferenceCollector() && !Ar.IsTransacting() &&
		!HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) && !UPuzzleSlotSubsystem::IsSlotIO())
	{
		bLoadedFromDisk = true;
		if (UPuzzleSlotSubsystem* Slots = UPuzzleSlotSubsystem::GetInstance())
		{
			Slots->ApplyResume(*this);
		}
	}
}

void UHappinessSaveGame::ResetOldSave()
{
	if (bOldSaveChecked)
	{
		return;
	}
	bOldSaveChecked = true;

	TArray<uint8> Bytes;
	if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex) || !UGameplayStatics::LoadDataFromSlot(Bytes, SaveSlotName, SaveUserIndex))
	{
		return;
	}
	// Saved properties are tagged with their names, as strings (length with the terminator, then the characters), and
	// a property at its default isn't written. So a save with campaign progress has this tag, and one written by this
	// version or later has a DataVersion; the old game's save has neither.
	static const uint8 CampaignTag[] = {9, 0, 0, 0, 'C', 'a', 'm', 'p', 'a', 'i', 'g', 'n', 0};
	USaveGame* Old = UGameplayStatics::LoadGameFromMemory(Bytes);
	const UHappinessSaveGame* OldSave = Cast<UHappinessSaveGame>(Old);
	bool bHasCampaign = false;
	for (int32 At = 0; At + int32(sizeof(CampaignTag)) <= Bytes.Num() && !bHasCampaign; At++)
	{
		bHasCampaign = FMemory::Memcmp(&Bytes[At], CampaignTag, sizeof(CampaignTag)) == 0;
	}
	if (!Old || (OldSave && OldSave->DataVersion > 0) || bHasCampaign)
	{
		return;
	}

	// A fresh save with the level and experience (SG_Happiness's Level and Experience, read by PC_Happiness)
	USaveGame* Fresh = UGameplayStatics::CreateSaveGameObject(Old->GetClass());
	for (const TCHAR* Name : {TEXT("Level"), TEXT("Experience"), TEXT("SaveVersion")})
	{
		const FIntProperty* Property = FindFProperty<FIntProperty>(Old->GetClass(), Name);
		if (Property && Fresh)
		{
			Property->SetPropertyValue_InContainer(Fresh, Property->GetPropertyValue_InContainer(Old));
		}
	}
	UPuzzleSlotSubsystem::DeleteAllSlots();
	UGameplayStatics::DeleteGameInSlot(SaveSlotName, SaveUserIndex);
	if (Fresh)
	{
		UGameplayStatics::SaveGameToSlot(Fresh, SaveSlotName, SaveUserIndex);
	}
	bOldSaveReset = true;
	UE_LOG(LogTemp, Log, TEXT("Old game save replaced: the game starts over, keeping the player's level"));
}

UHappinessSaveGame* UHappinessSaveGame::LoadFromSlot()
{
	ResetOldSave();
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
	return false;
}
