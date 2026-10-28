#include "IconSets.h"
#include "CampaignProgress.h"
#include "HappinessSaveGame.h"

#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "IconSets"

namespace
{
	const TCHAR* UnlocksPath = TEXT("/Game/Happiness/IconSets/DA_IconSetUnlocks.DA_IconSetUnlocks");
}

TWeakObjectPtr<UIconSetSubsystem> UIconSetSubsystem::Instance;

void UIconSetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Instance = this;
	if (const UHappinessSaveGame* SaveGame = UHappinessSaveGame::LoadFromSlot())
	{
		Data = SaveGame->IconSetChoices;
	}
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
	if (Instance == this)
	{
		Instance = nullptr;
	}
	Super::Deinitialize();
}

UIconSetSubsystem* UIconSetSubsystem::Get(const UObject* WorldContextObject)
{
	const UGameInstance* Game = UGameplayStatics::GetGameInstance(WorldContextObject);
	return Game ? Game->GetSubsystem<UIconSetSubsystem>() : nullptr;
}

UPrimaryDataAsset* UIconSetSubsystem::GetPlayerCollection(const UObject* WorldContextObject, UPrimaryDataAsset* DefaultCollection)
{
	UIconSetSubsystem* IconSets = Get(WorldContextObject);
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

bool UIconSetSubsystem::IsIconSetEnabled(const UObject* WorldContextObject, const UObject* IconSet)
{
	const UIconSetSubsystem* IconSets = Get(WorldContextObject);
	if (!IconSets || IconSets->GetNumEnabled() < MinEnabledSets)
	{
		return true;
	}
	return !IconSets->Data.DisabledSets.Contains(GetSetId(IconSet));
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
	case EIconSetUnlock::CampaignCompleted:
	{
		const UCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UCampaignSubsystem>();
		return Campaign && Campaign->IsCampaignCompleted();
	}
	case EIconSetUnlock::MasterCompleted:
	{
		const UCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UCampaignSubsystem>();
		return Campaign && Campaign->IsAllMasterCompleted();
	}
	}
	return false;
}

TArray<FIconSetEntry> UIconSetSubsystem::GetEntries() const
{
	TArray<FIconSetEntry> Entries;
	auto Add = [&](UObject* IconSet, bool bUnlocked, const FText& LockedReason)
	{
		UPrimaryDataAsset* Set = Cast<UPrimaryDataAsset>(IconSet);
		if (!Set || Entries.ContainsByPredicate([Set](const FIconSetEntry& Entry) { return Entry.IconSet == Set; }))
		{
			return;
		}
		FIconSetEntry& Entry = Entries.AddDefaulted_GetRef();
		Entry.IconSet = Set;
		Entry.Id = GetSetId(Set);
		Entry.Name = GetSetName(Set);
		Entry.bUnlocked = bUnlocked;
		Entry.bEnabled = bUnlocked && !Data.DisabledSets.Contains(Entry.Id);
		Entry.LockedReason = LockedReason;
	};
	for (UObject* Set : GetSets(DefaultCollection))
	{
		Add(Set, true, FText::GetEmpty());
	}
	if (const UIconSetUnlocks* AllUnlocks = GetUnlocks())
	{
		for (const FIconSetUnlock& Unlock : AllUnlocks->IconSets)
		{
			const bool bUnlocked = IsUnlocked(Unlock);
			if (!bUnlocked && Unlock.bHiddenUntilUnlocked)
			{
				continue;
			}
			FText Reason = Unlock.LockedReason;
			if (Reason.IsEmpty())
			{
				switch (Unlock.Unlock)
				{
				case EIconSetUnlock::LessonsCompleted: Reason = LOCTEXT("CompleteLessons", "Complete Lessons"); break;
				case EIconSetUnlock::CampaignCompleted: Reason = LOCTEXT("CompleteCampaign", "Complete Campaign"); break;
				case EIconSetUnlock::MasterCompleted: Reason = LOCTEXT("CompleteMaster", "Complete Every Master Challenge"); break;
				}
			}
			Add(Unlock.IconSet, bUnlocked, bUnlocked ? FText::GetEmpty() : Reason);
		}
	}
	return Entries;
}

int32 UIconSetSubsystem::GetNumEnabled() const
{
	int32 Count = 0;
	for (const UObject* Set : GetSets(PlayerCollection))
	{
		Count += Data.DisabledSets.Contains(GetSetId(Set)) ? 0 : 1;
	}
	return Count;
}

bool UIconSetSubsystem::SetEnabled(const UObject* IconSet, bool bEnabled)
{
	const FName Id = GetSetId(IconSet);
	const bool bDisabled = Data.DisabledSets.Contains(Id);
	if (Id.IsNone() || bDisabled != bEnabled)
	{
		// Already so
		return !Id.IsNone();
	}
	if (!bEnabled && GetNumEnabled() <= MinEnabledSets)
	{
		return false;
	}
	if (bEnabled)
	{
		Data.DisabledSets.Remove(Id);
	}
	else
	{
		Data.DisabledSets.AddUnique(Id);
	}
	// Written into the game save now; it would also go with the game's next save
	UHappinessSaveGame::SaveCurrentSettings();
	OnEnabledChanged.Broadcast();
	return true;
}

FName UIconSetSubsystem::GetSetId(const UObject* IconSet)
{
	const FNameProperty* IdProperty = IconSet ? FindFProperty<FNameProperty>(IconSet->GetClass(), TEXT("Id")) : nullptr;
	return IdProperty ? IdProperty->GetPropertyValue_InContainer(IconSet) : NAME_None;
}

TArray<UTexture2D*> UIconSetSubsystem::GetIconTextures(const UObject* IconSet)
{
	// DA_IconSet's Icons: structs of the texture and its Id
	TArray<UTexture2D*> Textures;
	const FArrayProperty* Icons = IconSet ? FindFProperty<FArrayProperty>(IconSet->GetClass(), TEXT("Icons")) : nullptr;
	if (!Icons)
	{
		return Textures;
	}
	FScriptArrayHelper Array(Icons, Icons->ContainerPtrToValuePtr<void>(IconSet));
	for (int32 Index = 0; Index < Array.Num(); Index++)
	{
		UTexture2D* Texture = nullptr;
		const uint8* Element = Array.GetRawPtr(Index);
		if (const FObjectPropertyBase* ObjectInner = CastField<FObjectPropertyBase>(Icons->Inner))
		{
			Texture = Cast<UTexture2D>(ObjectInner->GetObjectPropertyValue(Element));
		}
		else if (const FStructProperty* StructInner = CastField<FStructProperty>(Icons->Inner))
		{
			for (TFieldIterator<FObjectPropertyBase> It(StructInner->Struct); It && !Texture; ++It)
			{
				Texture = Cast<UTexture2D>(It->GetObjectPropertyValue_InContainer(Element));
			}
		}
		Textures.Add(Texture);
	}
	return Textures;
}

UIconSetUnlocks* UIconSetSubsystem::GetUnlocks() const
{
	if (!Unlocks)
	{
		// Loaded the first time it's needed
		const_cast<UIconSetSubsystem*>(this)->Unlocks = LoadObject<UIconSetUnlocks>(nullptr, UnlocksPath);
	}
	return Unlocks;
}

TArray<UObject*> UIconSetSubsystem::GetSets(const UObject* Collection) const
{
	TArray<UObject*> Result;
	const FArrayProperty* Sets = Collection ? FindFProperty<FArrayProperty>(Collection->GetClass(), TEXT("AllSets")) : nullptr;
	const FObjectPropertyBase* SetElement = Sets ? CastField<FObjectPropertyBase>(Sets->Inner) : nullptr;
	if (!SetElement)
	{
		return Result;
	}
	FScriptArrayHelper Array(Sets, Sets->ContainerPtrToValuePtr<void>(Collection));
	for (int32 Index = 0; Index < Array.Num(); Index++)
	{
		if (UObject* Set = SetElement->GetObjectPropertyValue(Array.GetRawPtr(Index)))
		{
			Result.Add(Set);
		}
	}
	return Result;
}

FText UIconSetSubsystem::GetSetName(const UObject* IconSet) const
{
	const FName Id = GetSetId(IconSet);
	if (const UIconSetUnlocks* AllUnlocks = GetUnlocks())
	{
		if (const FText* Name = AllUnlocks->SetNames.Find(Id))
		{
			return *Name;
		}
	}
	// "GardenVisitors" -> "Garden Visitors"
	const FString Source = Id.IsNone() ? (IconSet ? IconSet->GetName() : FString()) : Id.ToString();
	FString Spaced;
	for (int32 Index = 0; Index < Source.Len(); Index++)
	{
		if (Index > 0 && FChar::IsUpper(Source[Index]) && !FChar::IsUpper(Source[Index - 1]))
		{
			Spaced.AppendChar(TEXT(' '));
		}
		Spaced.AppendChar(Source[Index]);
	}
	return FText::FromString(Spaced);
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
	const UIconSetUnlocks* AllUnlocks = GetUnlocks();
	if (!AllUnlocks)
	{
		return;
	}
	FScriptArrayHelper Array(Sets, Sets->ContainerPtrToValuePtr<void>(PlayerCollection));
	for (const FIconSetUnlock& Unlock : AllUnlocks->IconSets)
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

#undef LOCTEXT_NAMESPACE
