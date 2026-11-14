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
		To.UndoActions = From.UndoActions;
		To.UndoSolution = From.UndoSolution;
		To.HiddenClues = From.HiddenClues;
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

	bool IsFinishedSlotSession(const FCampaignSaveData& Campaign, const FPuzzleSlotInfo& Info)
	{
		const EPuzzleSlot ModeSlot = Campaign.Mode == ECampaignMode::Campaign ? EPuzzleSlot::Campaign : EPuzzleSlot::Lessons;
		const FLessonProgress* Progress = (Campaign.Mode == ECampaignMode::Campaign ? Campaign.CampaignLessons : Campaign.Lessons).Find(Info.Lesson);
		// Leaving an unfinished session also clears bPuzzleOpen; only a solved puzzle advances its seed.
		const bool bAdvanced = Progress && Progress->NextSeed.IsValidIndex(Info.Stage) &&
			int64(Progress->NextSeed[Info.Stage]) > int64(Campaign.OpenSeed);
		return Info.Slot == ModeSlot && Campaign.SessionLesson == Info.Lesson && Campaign.SessionStage == Info.Stage &&
			!Campaign.bPuzzleOpen && bAdvanced && (Info.Seed == Campaign.OpenSeed || int64(Info.Seed) == int64(Campaign.OpenSeed) + 1);
	}

	UHappinessSaveGame* MakePuzzleSlotSnapshot(const UHappinessSaveGame& GameSave, EPuzzleSlot Slot)
	{
		UHappinessSaveGame* SlotSave = NewObject<UHappinessSaveGame>(GetTransientPackage(), GameSave.GetClass());
		CopyPuzzle(GameSave, *SlotSave);
		FPuzzleSlotInfo& Info = SlotSave->PuzzleSlot;
		Info.Slot = Slot;
		if (Slot == EPuzzleSlot::Lessons || Slot == EPuzzleSlot::Campaign)
		{
			// The saved board belongs to OpenSeed. NextSeed has already advanced on the end screen.
			Info.Lesson = GameSave.Campaign.SessionLesson;
			Info.Stage = GameSave.Campaign.SessionStage;
			Info.Seed = GameSave.Campaign.OpenSeed;
			if (IsFinishedSlotSession(GameSave.Campaign, Info))
			{
				if (const FBoolProperty* ActivePuzzle = FindActivePuzzle(*SlotSave))
					ActivePuzzle->SetPropertyValue_InContainer(SlotSave, false);
			}
		}
		else if (Slot == EPuzzleSlot::Daily)
		{
			Info.Date = GameSave.Daily.bActive ? GameSave.Daily.ActiveDate : 0;
		}
		return SlotSave;
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
	// The tutorial puzzle (UCampaignSubsystem::StartTutorial) is kept in no slot
	if (UCampaignSubsystem::IsPlayingTutorial(WorldContextObject))
	{
		Slots->PlayingSlot = EPuzzleSlot::None;
	}
	else if (Campaign && Campaign->IsInLessonSession())
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
	// A solved board is over, even in a slot saved before that was marked on save
	if (!SlotSave || !HasPuzzle(*SlotSave) || SlotSave->PuzzleSlot.Slot != Slot || SlotSave->IsBoardSolved())
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
		// Older end-screen saves stamped the finished board with the next seed. Do not resume those either.
		return Campaign && !IsFinishedSlotSession(Campaign->GetSaveData(), Info) &&
			Campaign->CanResumeLessonPuzzle(Slot == EPuzzleSlot::Campaign ? ECampaignMode::Campaign : ECampaignMode::Lessons,
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
	UHappinessSaveGame* SlotSave = MakePuzzleSlotSnapshot(GameSave, PlayingSlot);
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

#if WITH_EDITOR
bool UPuzzleSlotSubsystem::TestSaveResume(FString& Report)
{
	UClass* SaveClass = LoadClass<UHappinessSaveGame>(nullptr, TEXT("/Game/General/Blueprints/SG_Happiness.SG_Happiness_C"));
	if (!SaveClass)
	{
		Report = TEXT("FAIL: SG_Happiness could not be loaded\n");
		return false;
	}
	UHappinessSaveGame* Save = NewObject<UHappinessSaveGame>(GetTransientPackage(), SaveClass);
	const FBoolProperty* Active = FindActivePuzzle(*Save);
	const FArrayProperty* RowsProperty = FindFProperty<FArrayProperty>(SaveClass, TEXT("Rows"));
	if (!Active || !RowsProperty)
	{
		Report = TEXT("FAIL: saved puzzle fields are missing\n");
		return false;
	}
	TArray<FPuzzleRow>& Rows = *RowsProperty->ContainerPtrToValuePtr<TArray<FPuzzleRow>>(Save);
	Rows.SetNum(3);
	for (int32 Row = 0; Row < 3; ++Row)
	{
		Rows[Row].m_Cells.SetNum(3);
		for (int32 Col = 0; Col < 3; ++Col)
		{
			FPuzzleCell& Cell = Rows[Row].m_Cells[Col];
			Cell.m_iFinalIcon = (Row + Col) % 3;
			Cell.m_bValues.Init(false, 3);
			Cell.m_bValues[Cell.m_iFinalIcon] = true;
		}
	}
	int32 Failures = 0;
	auto Check = [&](bool Passed, const TCHAR* Name)
	{
		Report += FString::Printf(TEXT("%s: %s\n"), Passed ? TEXT("PASS") : TEXT("FAIL"), Name);
		if (!Passed) ++Failures;
	};
	// No disk writes or live subsystems: exercise the real snapshot and serialization paths in memory.
	TGuardValue<bool> Guard(bSlotIO, true);
	for (EPuzzleSlot Mode : {EPuzzleSlot::Campaign, EPuzzleSlot::Lessons})
	{
		Report += Mode == EPuzzleSlot::Campaign ? TEXT("Campaign\n") : TEXT("Lessons\n");
		Save->Campaign.Mode = Mode == EPuzzleSlot::Campaign ? ECampaignMode::Campaign : ECampaignMode::Lessons;
		Save->Campaign.bInSession = true;
		Save->Campaign.SessionLesson = ECampaignLesson::Given;
		Save->Campaign.SessionStage = 0;
		Save->Campaign.OpenSeed = 7;
		Save->Campaign.bPuzzleOpen = true;
		FLessonProgress& Progress = (Mode == EPuzzleSlot::Campaign ? Save->Campaign.CampaignLessons : Save->Campaign.Lessons).FindOrAdd(ECampaignLesson::Given);
		Progress.NextSeed = {7};
		Rows[0].m_Cells[0].m_iFinalIcon = -1;
		Rows[0].m_Cells[0].m_bValues.Init(true, 3);
		Save->UndoSolution.Reset();
		for (int32 Row = 0; Row < 3; ++Row)
			for (int32 Col = 0; Col < 3; ++Col) Save->UndoSolution.Add((Row + Col) % 3);
		Active->SetPropertyValue_InContainer(Save, true);
		UHappinessSaveGame* Open = MakePuzzleSlotSnapshot(*Save, Mode);
		Check(HasPuzzle(*Open) && Open->PuzzleSlot.Seed == 7, TEXT("unfinished slot retains its board seed and active flag"));
		Check(!IsFinishedSlotSession(Save->Campaign, Open->PuzzleSlot), TEXT("unfinished/incorrect board is not retired"));

		TArray<uint8> OpenBytes;
		const bool OpenWritten = UGameplayStatics::SaveGameToMemory(Open, OpenBytes);
		UHappinessSaveGame* OpenReloaded = OpenWritten ? Cast<UHappinessSaveGame>(UGameplayStatics::LoadGameFromMemory(OpenBytes)) : nullptr;
		Check(OpenReloaded && HasPuzzle(*OpenReloaded) && OpenReloaded->PuzzleSlot.Seed == 7 &&
			OpenReloaded->UndoSolution == Save->UndoSolution && RowsProperty->Identical_InContainer(Open, OpenReloaded),
			TEXT("unfinished board and solution survive serialization unchanged"));

		Save->Campaign.bInSession = false;
		Save->Campaign.bPuzzleOpen = false;
		UHappinessSaveGame* Paused = MakePuzzleSlotSnapshot(*Save, Mode);
		Check(HasPuzzle(*Paused) && !IsFinishedSlotSession(Save->Campaign, Paused->PuzzleSlot),
			TEXT("leaving an unfinished session preserves its resumable slot"));
		Save->Campaign.bInSession = true;

		// Finishing advances progress before the end screen saves the old board again.
		Rows[0].m_Cells[0].m_iFinalIcon = 0;
		Rows[0].m_Cells[0].m_bValues.Init(false, 3);
		Rows[0].m_Cells[0].m_bValues[0] = true;
		Save->Campaign.bPuzzleOpen = false;
		Progress.NextSeed[0] = 8;
		Save->UndoSolution.Reset();
		UHappinessSaveGame* Finished = MakePuzzleSlotSnapshot(*Save, Mode);
		Check(Finished->PuzzleSlot.Seed == 7, TEXT("end-screen snapshot does not adopt the next puzzle seed"));
		Check(!HasPuzzle(*Finished), TEXT("finished slot is inactive even without saved undo/solution data"));
		TArray<uint8> Bytes;
		const bool Written = UGameplayStatics::SaveGameToMemory(Finished, Bytes);
		UHappinessSaveGame* Reloaded = Written ? Cast<UHappinessSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes)) : nullptr;
		Check(Reloaded && !HasPuzzle(*Reloaded) && Reloaded->PuzzleSlot.Seed == 7,
			TEXT("serialized end-screen save stays inactive after an app-restart round trip"));
		if (Reloaded)
		{
			const TArray<FPuzzleRow>& ReloadedRows = *RowsProperty->ContainerPtrToValuePtr<TArray<FPuzzleRow>>(Reloaded);
			Check(ReloadedRows.Num() == 3 && ReloadedRows[0].m_Cells.Num() == 3 && ReloadedRows[0].m_Cells[1].m_iFinalIcon == 1,
				TEXT("retiring the slot preserves its saved board"));
		}
		FPuzzleSlotInfo Legacy = Finished->PuzzleSlot;
		Legacy.Seed = 8;
		Check(IsFinishedSlotSession(Save->Campaign, Legacy), TEXT("legacy finished-board/next-seed mismatch is rejected"));
		Legacy.Stage = 1;
		Check(!IsFinishedSlotSession(Save->Campaign, Legacy), TEXT("another stage's unfinished slot is unaffected"));
		Legacy.Stage = 0;
		Legacy.Slot = Mode == EPuzzleSlot::Campaign ? EPuzzleSlot::Lessons : EPuzzleSlot::Campaign;
		Check(!IsFinishedSlotSession(Save->Campaign, Legacy), TEXT("another mode's unfinished slot is unaffected"));
	}
	Active->SetPropertyValue_InContainer(Save, true);
	Check(HasPuzzle(*MakePuzzleSlotSnapshot(*Save, EPuzzleSlot::FreePlay)), TEXT("Free Play active flag is unaffected by Campaign completion"));
	Report += FString::Printf(TEXT("Save/resume regression: %d failures\n"), Failures);
	return Failures == 0;
}
#endif
