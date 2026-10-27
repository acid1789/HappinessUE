#include "PuzzleSlots.h"

#include "CampaignProgress.h"
#include "DailyPuzzle.h"
#include "HappinessSaveGame.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

bool UPuzzleSlotSubsystem::bSlotIO = false;
TWeakObjectPtr<UPuzzleSlotSubsystem> UPuzzleSlotSubsystem::Instance;

namespace
{
	const EPuzzleSlot AllSlots[] = { EPuzzleSlot::Daily, EPuzzleSlot::FreePlay, EPuzzleSlot::Lessons, EPuzzleSlot::Campaign };

	/** The puzzle in a game save: every SG_Happiness (Blueprint) variable except the player's level and experience */
	void CopyPuzzle(const UHappinessSaveGame& From, UHappinessSaveGame& To)
	{
		if (From.GetClass() != To.GetClass())
		{
			return;
		}
		for (TFieldIterator<FProperty> It(From.GetClass()); It; ++It)
		{
			const UClass* Owner = It->GetOwnerClass();
			if (!Owner || !Owner->HasAnyClassFlags(CLASS_CompiledFromBlueprint) ||
				It->GetFName() == TEXT("Experience") || It->GetFName() == TEXT("Level"))
			{
				continue;
			}
			It->CopyCompleteValue_InContainer(&To, &From);
		}
	}

	/** SG_Happiness's ActivePuzzle: it holds a puzzle in progress */
	const FBoolProperty* FindActivePuzzle(const UHappinessSaveGame& GameSave)
	{
		return FindFProperty<FBoolProperty>(GameSave.GetClass(), TEXT("ActivePuzzle"));
	}

	bool HasPuzzle(const UHappinessSaveGame& GameSave)
	{
		const FBoolProperty* ActivePuzzle = FindActivePuzzle(GameSave);
		return ActivePuzzle && ActivePuzzle->GetPropertyValue_InContainer(&GameSave);
	}
}

void UPuzzleSlotSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Instance = this;
	MigrateOldSave();
}

void UPuzzleSlotSubsystem::Deinitialize()
{
	if (Instance == this)
	{
		Instance = nullptr;
	}
	Super::Deinitialize();
}

UPuzzleSlotSubsystem* UPuzzleSlotSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UPuzzleSlotSubsystem>() : nullptr;
}

FString UPuzzleSlotSubsystem::GetSlotName(EPuzzleSlot Slot)
{
	switch (Slot)
	{
	case EPuzzleSlot::Daily: return TEXT("HappinessPuzzle_Daily");
	case EPuzzleSlot::FreePlay: return TEXT("HappinessPuzzle_FreePlay");
	case EPuzzleSlot::Lessons: return TEXT("HappinessPuzzle_Lessons");
	case EPuzzleSlot::Campaign: return TEXT("HappinessPuzzle_Campaign");
	default: return FString();
	}
}

void UPuzzleSlotSubsystem::DeleteAllSlots()
{
	for (EPuzzleSlot Slot : {EPuzzleSlot::Daily, EPuzzleSlot::FreePlay, EPuzzleSlot::Lessons, EPuzzleSlot::Campaign})
	{
		UGameplayStatics::DeleteGameInSlot(GetSlotName(Slot), UHappinessSaveGame::SaveUserIndex);
	}
}

UHappinessSaveGame* UPuzzleSlotSubsystem::LoadSlot(EPuzzleSlot Slot) const
{
	const FString Name = GetSlotName(Slot);
	if (Name.IsEmpty() || !UGameplayStatics::DoesSaveGameExist(Name, UHappinessSaveGame::SaveUserIndex))
	{
		return nullptr;
	}
	TGuardValue<bool> Guard(bSlotIO, true);
	return Cast<UHappinessSaveGame>(UGameplayStatics::LoadGameFromSlot(Name, UHappinessSaveGame::SaveUserIndex));
}

void UPuzzleSlotSubsystem::NotePuzzleStarted(const UObject* WorldContextObject)
{
	UPuzzleSlotSubsystem* Slots = Get(WorldContextObject);
	if (!Slots)
	{
		return;
	}
	const UCampaignSubsystem* Campaign = UCampaignSubsystem::Get(WorldContextObject);
	const UDailySubsystem* Daily = UDailySubsystem::Get(WorldContextObject);
	FDailyPuzzleSpec Spec;
	if (Campaign && Campaign->IsInLessonSession())
	{
		Slots->PlayingSlot = UCampaignSubsystem::GetMode() == ECampaignMode::Campaign ? EPuzzleSlot::Campaign : EPuzzleSlot::Lessons;
	}
	else
	{
		Slots->PlayingSlot = Daily && Daily->GetActiveSpec(Spec) ? EPuzzleSlot::Daily : EPuzzleSlot::FreePlay;
	}
}

bool UPuzzleSlotSubsystem::CanResume(EPuzzleSlot Slot, FPuzzleSlotInfo* OutInfo) const
{
	const UHappinessSaveGame* SlotSave = LoadSlot(Slot);
	if (!SlotSave || !HasPuzzle(*SlotSave) || SlotSave->PuzzleSlot.Slot != Slot)
	{
		return false;
	}
	const FPuzzleSlotInfo& Info = SlotSave->PuzzleSlot;
	if (OutInfo)
	{
		*OutInfo = Info;
	}
	switch (Slot)
	{
	case EPuzzleSlot::Daily:
	{
		// Only today's: another day has another puzzle
		const UDailySubsystem* Daily = UDailySubsystem::GetInstance();
		return Daily && !Daily->IsTodaySolved() && Info.Date == UDailySubsystem::GetToday();
	}
	case EPuzzleSlot::Lessons:
	case EPuzzleSlot::Campaign:
	{
		// Until it's solved (the stage moves on to its next puzzle)
		const UCampaignSubsystem* Campaign = UCampaignSubsystem::GetInstance();
		return Campaign && Campaign->CanResumeLessonPuzzle(Slot == EPuzzleSlot::Campaign ? ECampaignMode::Campaign : ECampaignMode::Lessons,
			Info.Lesson, Info.Stage, Info.Seed);
	}
	default:
		return true;
	}
}

void UPuzzleSlotSubsystem::AbandonPuzzle(EPuzzleSlot Slot)
{
	UHappinessSaveGame* SlotSave = LoadSlot(Slot);
	const FBoolProperty* ActivePuzzle = SlotSave ? FindActivePuzzle(*SlotSave) : nullptr;
	if (!ActivePuzzle)
	{
		return;
	}
	ActivePuzzle->SetPropertyValue_InContainer(SlotSave, false);
	TGuardValue<bool> Guard(bSlotIO, true);
	UGameplayStatics::SaveGameToSlot(SlotSave, GetSlotName(Slot), UHappinessSaveGame::SaveUserIndex);
}

void UPuzzleSlotSubsystem::WriteSlot(const UHappinessSaveGame& GameSave)
{
	if (PlayingSlot == EPuzzleSlot::None || bSlotIO)
	{
		return;
	}
	UHappinessSaveGame* SlotSave = NewObject<UHappinessSaveGame>(GetTransientPackage(), GameSave.GetClass());
	CopyPuzzle(GameSave, *SlotSave);
	FPuzzleSlotInfo& Info = SlotSave->PuzzleSlot;
	Info.Slot = PlayingSlot;
	if (PlayingSlot == EPuzzleSlot::Lessons || PlayingSlot == EPuzzleSlot::Campaign)
	{
		if (const UCampaignSubsystem* Campaign = UCampaignSubsystem::GetInstance())
		{
			Info.Lesson = Campaign->GetSessionLesson();
			Info.Stage = Campaign->GetSessionStage();
			Info.Seed = Campaign->GetStageSeed(Info.Lesson, Info.Stage);
		}
	}
	else if (PlayingSlot == EPuzzleSlot::Daily)
	{
		// None once it's solved
		FDailyPuzzleSpec Spec;
		const UDailySubsystem* Daily = UDailySubsystem::GetInstance();
		Info.Date = Daily && Daily->GetActiveSpec(Spec) ? Spec.Date : 0;
	}
	TGuardValue<bool> Guard(bSlotIO, true);
	UGameplayStatics::SaveGameToSlot(SlotSave, GetSlotName(PlayingSlot), UHappinessSaveGame::SaveUserIndex);
}

void UPuzzleSlotSubsystem::ApplyResume(UHappinessSaveGame& GameSave)
{
	if (ResumeSlot == EPuzzleSlot::None || bSlotIO)
	{
		return;
	}
	const UHappinessSaveGame* SlotSave = LoadSlot(ResumeSlot);
	if (SlotSave && SlotSave->GetClass() == GameSave.GetClass())
	{
		CopyPuzzle(*SlotSave, GameSave);
	}
	else if (const FBoolProperty* ActivePuzzle = FindActivePuzzle(GameSave))
	{
		// The game save's own puzzle may be another mode's
		ActivePuzzle->SetPropertyValue_InContainer(&GameSave, false);
	}
}

void UPuzzleSlotSubsystem::MigrateOldSave()
{
	for (EPuzzleSlot Slot : AllSlots)
	{
		if (UGameplayStatics::DoesSaveGameExist(GetSlotName(Slot), UHappinessSaveGame::SaveUserIndex))
		{
			return;
		}
	}
	const UHappinessSaveGame* GameSave = UHappinessSaveGame::LoadFromSlot();
	if (!GameSave || !HasPuzzle(*GameSave))
	{
		return;
	}

	// The puzzle belongs to the mode it was being played in
	UHappinessSaveGame* SlotSave = NewObject<UHappinessSaveGame>(GetTransientPackage(), GameSave->GetClass());
	CopyPuzzle(*GameSave, *SlotSave);
	FPuzzleSlotInfo& Info = SlotSave->PuzzleSlot;
	const FCampaignSaveData& Campaign = GameSave->Campaign;
	if (Campaign.bInSession && Campaign.bPuzzleOpen)
	{
		Info.Slot = Campaign.Mode == ECampaignMode::Campaign ? EPuzzleSlot::Campaign : EPuzzleSlot::Lessons;
		Info.Lesson = Campaign.SessionLesson;
		Info.Stage = Campaign.SessionStage;
		Info.Seed = Campaign.OpenSeed;
	}
	else if (GameSave->Daily.bActive)
	{
		Info.Slot = EPuzzleSlot::Daily;
		Info.Date = GameSave->Daily.ActiveDate;
	}
	else
	{
		Info.Slot = EPuzzleSlot::FreePlay;
	}
	TGuardValue<bool> Guard(bSlotIO, true);
	UGameplayStatics::SaveGameToSlot(SlotSave, GetSlotName(Info.Slot), UHappinessSaveGame::SaveUserIndex);
}
