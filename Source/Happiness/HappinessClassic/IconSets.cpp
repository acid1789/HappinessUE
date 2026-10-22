#include "IconSets.h"
#include "CampaignProgress.h"

#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const TCHAR* UnlocksPath = TEXT("/Game/Happiness/IconSets/DA_IconSetUnlocks.DA_IconSetUnlocks");
}

void UIconSetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UCampaignSubsystem* Campaign = Collection.InitializeDependency<UCampaignSubsystem>())
	{
		Campaign->OnProgressChanged.AddUniqueDynamic(this, &UIconSetSubsystem::Refresh);
	}
}

void UIconSetSubsystem::Deinitialize()
{
	if (UCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UCampaignSubsystem>())
	{
		Campaign->OnProgressChanged.RemoveDynamic(this, &UIconSetSubsystem::Refresh);
	}
	Super::Deinitialize();
}

UPrimaryDataAsset* UIconSetSubsystem::GetPlayerCollection(const UObject* WorldContextObject, UPrimaryDataAsset* DefaultCollection)
{
	const UGameInstance* Game = UGameplayStatics::GetGameInstance(WorldContextObject);
	UIconSetSubsystem* IconSets = Game ? Game->GetSubsystem<UIconSetSubsystem>() : nullptr;
	if (!IconSets || !DefaultCollection)
	{
		return DefaultCollection;
	}
	if (IconSets->DefaultCollection != DefaultCollection || !IconSets->PlayerCollection)
	{
		IconSets->DefaultCollection = DefaultCollection;
		IconSets->PlayerCollection = DuplicateObject(DefaultCollection, IconSets);
		IconSets->Refresh();
	}
	return IconSets->PlayerCollection;
}

bool UIconSetSubsystem::IsUnlocked(const FIconSetUnlock& Unlock) const
{
	switch (Unlock.Unlock)
	{
	case EIconSetUnlock::LessonsCompleted:
	{
		const UCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UCampaignSubsystem>();
		return Campaign && Campaign->IsCampaignModeUnlocked();
	}
	}
	return false;
}

void UIconSetSubsystem::Refresh()
{
	if (!PlayerCollection || !DefaultCollection)
	{
		return;
	}
	const FArrayProperty* Sets = FindFProperty<FArrayProperty>(PlayerCollection->GetClass(), TEXT("AllSets"));
	const FObjectPropertyBase* SetElement = Sets ? CastField<FObjectPropertyBase>(Sets->Inner) : nullptr;
	if (!SetElement)
	{
		return;
	}

	// The default sets, then the unlocked ones (in DA_IconSetUnlocks' order)
	Sets->CopyCompleteValue_InContainer(PlayerCollection, DefaultCollection);
	if (!Unlocks)
	{
		Unlocks = LoadObject<UIconSetUnlocks>(nullptr, UnlocksPath);
	}
	if (!Unlocks)
	{
		return;
	}
	FScriptArrayHelper Array(Sets, Sets->ContainerPtrToValuePtr<void>(PlayerCollection));
	for (const FIconSetUnlock& Unlock : Unlocks->IconSets)
	{
		if (!Unlock.IconSet || !Unlock.IconSet->IsA(SetElement->PropertyClass) || !IsUnlocked(Unlock))
		{
			continue;
		}
		bool bPresent = false;
		for (int32 Index = 0; Index < Array.Num() && !bPresent; Index++)
		{
			bPresent = SetElement->GetObjectPropertyValue(Array.GetRawPtr(Index)) == Unlock.IconSet;
		}
		if (!bPresent)
		{
			SetElement->SetObjectPropertyValue(Array.GetRawPtr(Array.AddValue()), Unlock.IconSet);
		}
	}
}
