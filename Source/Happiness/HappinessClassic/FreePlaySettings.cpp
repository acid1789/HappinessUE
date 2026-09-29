#include "FreePlaySettings.h"
#include "CampaignTree.h"
#include "HappinessSaveGame.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"

#define LOCTEXT_NAMESPACE "FreePlay"

TWeakObjectPtr<UFreePlaySubsystem> UFreePlaySubsystem::Instance;

void UFreePlaySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Instance = this;
	if (const UHappinessSaveGame* SaveGame = UHappinessSaveGame::LoadFromSlot())
	{
		Data = SaveGame->FreePlay;
	}
}

void UFreePlaySubsystem::Deinitialize()
{
	if (Instance == this)
	{
		Instance = nullptr;
	}
	Super::Deinitialize();
}

TArray<ECampaignLesson> UFreePlaySubsystem::GetSelectableClues()
{
	TArray<ECampaignLesson> Clues;
	for (const TArray<ECampaignLesson>& Column : UCampaignTree::GetColumns())
	{
		Clues.Append(Column);
	}
	return Clues;
}

FText UFreePlaySubsystem::GetClueName(ECampaignLesson Clue)
{
	// The lesson names, except the first lesson's, which is named for the game rather than its clue
	if (Clue == ECampaignLesson::VerticalTwo)
	{
		return LOCTEXT("SameColumn", "Same Column");
	}
	return UCampaignTree::GetLessonDisplayName(Clue);
}

bool UFreePlaySubsystem::IsClueIncluded(ECampaignLesson Clue) const
{
	return !Data.ExcludedClues.Contains(Clue);
}

void UFreePlaySubsystem::SetClueIncluded(ECampaignLesson Clue, bool bIncluded)
{
	if (Clue == ECampaignLesson::Given || IsClueIncluded(Clue) == bIncluded)
	{
		return;
	}

	if (bIncluded)
	{
		Data.ExcludedClues.Remove(Clue);
	}
	else
	{
		Data.ExcludedClues.Add(Clue);
	}
	Changed();
}

void UFreePlaySubsystem::SetAllCluesIncluded(bool bIncluded)
{
	Data.ExcludedClues.Reset();
	if (!bIncluded)
	{
		Data.ExcludedClues = GetSelectableClues();
	}
	Changed();
}

int32 UFreePlaySubsystem::GetIncludedClueCount() const
{
	int32 Count = 0;
	for (ECampaignLesson Clue : GetSelectableClues())
	{
		Count += IsClueIncluded(Clue) ? 1 : 0;
	}
	return Count;
}

FText UFreePlaySubsystem::GetSelectionSummary() const
{
	const int32 Total = GetSelectableClues().Num();
	const int32 Included = GetIncludedClueCount();
	if (Included == Total)
	{
		return LOCTEXT("AllClues", "All");
	}
	if (Included == 0)
	{
		return LOCTEXT("GivensOnly", "Givens only");
	}
	return FText::Format(LOCTEXT("SomeClues", "{0} of {1}"), Included, Total);
}

int32 UFreePlaySubsystem::GetExcludedClueMask() const
{
	int32 Mask = 0;
	for (ECampaignLesson Clue : Data.ExcludedClues)
	{
		if (Clue != ECampaignLesson::Given)
		{
			Mask |= 1 << int32(Clue);
		}
	}
	return Mask;
}

UFreePlaySubsystem* UFreePlaySubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UFreePlaySubsystem>() : nullptr;
}

void UFreePlaySubsystem::Changed()
{
	// Written into the game save now; it would also go with the game's next save
	UHappinessSaveGame::SaveCurrentSettings();
	OnCluesChanged.Broadcast();
}

#undef LOCTEXT_NAMESPACE
