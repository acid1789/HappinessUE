#include "CampaignProgress.h"
#include "CampaignTree.h"
#include "Puzzle.h"
#include "HappinessSaveGame.h"
#include "FreePlaySettings.h"

#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "CampaignProgress"

#if !UE_BUILD_SHIPPING
// Testing: mark every lesson complete (all points, final done), which unlocks Campaign mode.
// Type "Happiness.CompleteAllLessons" in the PIE console (~).
static FAutoConsoleCommandWithWorld GCompleteAllLessonsCommand(
	TEXT("Happiness.CompleteAllLessons"),
	TEXT("Marks every lesson complete, unlocking Campaign mode"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (UCampaignSubsystem* Campaign = UCampaignSubsystem::Get(World))
		{
			Campaign->DebugCompleteAllLessons();
		}
	}));
#endif

TWeakObjectPtr<UCampaignSubsystem> UCampaignSubsystem::Instance;

void UCampaignSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);


	Instance = this;

	if (const UHappinessSaveGame* SaveGame = UHappinessSaveGame::LoadFromSlot())
	{
		Data = SaveGame->Campaign;
	}

	// Progress used to be saved in its own slot; that slot is no longer used
	UGameplayStatics::DeleteGameInSlot(TEXT("Campaign"), UHappinessSaveGame::SaveUserIndex);
}

void UCampaignSubsystem::Deinitialize()
{
	if (Instance == this)
	{
		Instance = nullptr;
	}
	Super::Deinitialize();
}

const TArray<UCampaignSubsystem::FStage>& UCampaignSubsystem::GetStages(ECampaignMode Mode)
{
	// Size, difficulty (0 easy, 1 normal, 2 hard), progress needed to unlock. The last stage is the final.
	static const TArray<FStage> LessonStages =
	{
		{ 3, 0, 0 },
		{ 3, 1, 9 },
		{ 4, 0, 18 },
		{ 4, 1, 27 },
		{ 4, 2, 36 },	// Final
	};
	// One step per solved puzzle: 4 normal then 1 hard on each of 5x5, 6x6 and 7x7, then the final
	static const TArray<FStage> CampaignStages =
	{
		{ 5, 1, 0 },
		{ 5, 2, 4 },
		{ 6, 1, 5 },
		{ 6, 2, 9 },
		{ 7, 1, 10 },
		{ 7, 2, 14 },
		{ 8, 0, 15 },	// Final
	};
	return Mode == ECampaignMode::Campaign ? CampaignStages : LessonStages;
}

const TArray<UCampaignSubsystem::FStage>& UCampaignSubsystem::GetMasterStages(ECampaignMode Mode)
{
	// After the final, outside the campaign's progression: Required counts Master Mode puzzles solved
	static const TArray<FStage> None;
	static const TArray<FStage> CampaignMaster =
	{
		{ 8, 1, 0 },
		{ 8, 2, 4 },
	};
	return Mode == ECampaignMode::Campaign ? CampaignMaster : None;
}

const UCampaignSubsystem::FStage& UCampaignSubsystem::GetStage(ECampaignMode Mode, int32 Stage)
{
	const TArray<FStage>& Stages = GetStages(Mode);
	const TArray<FStage>& Master = GetMasterStages(Mode);
	if (Stage >= Stages.Num() && Master.Num() > 0)
	{
		return Master[FMath::Clamp(Stage - Stages.Num(), 0, Master.Num() - 1)];
	}
	return Stages[FMath::Clamp(Stage, 0, Stages.Num() - 1)];
}

int32 UCampaignSubsystem::GetNumMasterStages()
{
	return GetMasterStages(GetMode()).Num();
}

bool UCampaignSubsystem::IsMasterStage(int32 Stage)
{
	return Stage >= GetFirstMasterStage() && Stage < GetFirstMasterStage() + GetNumMasterStages();
}

int32 UCampaignSubsystem::GetMasterMaxPoints()
{
	// 4 puzzles at each Master Mode stage
	return GetNumMasterStages() > 0 ? GetMasterStages(GetMode()).Last().Required + 4 : 0;
}

int32 UCampaignSubsystem::GetMasterStagePointsRequired(int32 Stage)
{
	return IsMasterStage(Stage) ? GetStage(GetMode(), Stage).Required : 0;
}

int32 UCampaignSubsystem::GetMasterPoints(ECampaignLesson Lesson) const
{
	return GetLessonProgress(Lesson).MasterPoints;
}

bool UCampaignSubsystem::IsMasterUnlocked(ECampaignLesson Lesson) const
{
	return GetNumMasterStages() > 0 && IsLessonCompleted(Lesson);
}

bool UCampaignSubsystem::IsMasterCompleted(ECampaignLesson Lesson) const
{
	return IsMasterUnlocked(Lesson) && GetMasterPoints(Lesson) >= GetMasterMaxPoints();
}

bool UCampaignSubsystem::IsMasterRevealed(ECampaignLesson Lesson) const
{
	return IsMasterUnlocked(Lesson) && GetLessonProgress(Lesson).bMasterRevealed;
}

void UCampaignSubsystem::RevealMaster(ECampaignLesson Lesson)
{
	if (IsMasterUnlocked(Lesson) && !GetLessonProgress(Lesson).bMasterRevealed)
	{
		GetMutableProgress(Lesson).bMasterRevealed = true;
		Save();
	}
}

int32 UCampaignSubsystem::GetCurrentMasterStage(ECampaignLesson Lesson) const
{
	// The last Master Mode stage whose requirement is met
	const int32 Points = GetMasterPoints(Lesson);
	int32 Stage = GetFirstMasterStage();
	while (Stage + 1 < GetFirstMasterStage() + GetNumMasterStages() && Points >= GetMasterStagePointsRequired(Stage + 1))
	{
		Stage++;
	}
	return Stage;
}

ECampaignMode UCampaignSubsystem::GetMode()
{
	const UCampaignSubsystem* Campaign = Instance.Get();
	return Campaign ? Campaign->Data.Mode : ECampaignMode::Lessons;
}

void UCampaignSubsystem::SetMode(ECampaignMode NewMode)
{
	if (Data.Mode == NewMode)
	{
		return;
	}

	// The session belongs to the track it was started in
	Data.bInSession = false;
	Data.bPuzzleOpen = false;
	Data.Mode = NewMode;
	Save();
	OnProgressChanged.Broadcast();
}

bool UCampaignSubsystem::IsCampaignModeUnlocked() const
{
	for (const TArray<ECampaignLesson>& Column : UCampaignTree::GetColumns())
	{
		for (ECampaignLesson Lesson : Column)
		{
			const FLessonProgress* Progress = Data.Lessons.Find(Lesson);
			if (!Progress || !Progress->bFinalCompleted)
			{
				return false;
			}
		}
	}
	return true;
}

int32 UCampaignSubsystem::GetNumStagesFor(ECampaignMode Mode)
{
	return GetStages(Mode).Num();
}

int32 UCampaignSubsystem::GetStageSizeFor(ECampaignMode Mode, int32 Stage)
{
	return GetStage(Mode, Stage).Size;
}

int32 UCampaignSubsystem::GetStageDifficultyFor(ECampaignMode Mode, int32 Stage)
{
	return GetStage(Mode, Stage).Difficulty;
}

int32 UCampaignSubsystem::GetNumStages()
{
	return GetNumStagesFor(GetMode());
}

int32 UCampaignSubsystem::GetMaxPoints()
{
	return GetStages(GetMode()).Last().Required;
}

int32 UCampaignSubsystem::GetMaxPuzzleScore()
{
	return GetMode() == ECampaignMode::Campaign ? 1 : 3;
}

int32 UCampaignSubsystem::GetStageSize(int32 Stage)
{
	return GetStageSizeFor(GetMode(), Stage);
}

int32 UCampaignSubsystem::GetStageDifficulty(int32 Stage)
{
	return GetStageDifficultyFor(GetMode(), Stage);
}

int32 UCampaignSubsystem::GetStagePointsRequired(int32 Stage)
{
	return GetStage(GetMode(), Stage).Required;
}

FText UCampaignSubsystem::GetStageDisplayName(int32 Stage)
{
	if (Stage == GetFinalStage())
	{
		return LOCTEXT("StageFinal", "Final");
	}

	static const FText Difficulties[] = { LOCTEXT("Easy", "Easy"), LOCTEXT("Normal", "Normal"), LOCTEXT("Hard", "Hard") };
	const FStage& Def = GetStage(GetMode(), Stage);
	const FText Name = FText::Format(LOCTEXT("StageName", "{0}x{0} {1}"), Def.Size, Difficulties[FMath::Clamp(Def.Difficulty, 0, 2)]);
	return IsMasterStage(Stage) ? FText::Format(LOCTEXT("MasterStageName", "Master {0}"), Name) : Name;
}

const TMap<ECampaignLesson, FLessonProgress>& UCampaignSubsystem::GetProgressMap() const
{
	return Data.Mode == ECampaignMode::Campaign ? Data.CampaignLessons : Data.Lessons;
}

FLessonProgress UCampaignSubsystem::GetLessonProgress(ECampaignLesson Lesson) const
{
	const FLessonProgress* Progress = GetProgressMap().Find(Lesson);
	return Progress ? *Progress : FLessonProgress();
}

int32 UCampaignSubsystem::GetLessonPoints(ECampaignLesson Lesson) const
{
	return GetLessonProgress(Lesson).Points;
}

int32 UCampaignSubsystem::GetCurrentStage(ECampaignLesson Lesson) const
{
	// The last stage whose requirement is met
	const int32 Points = GetLessonPoints(Lesson);
	int32 Stage = 0;
	while (Stage + 1 < GetNumStages() && Points >= GetStagePointsRequired(Stage + 1))
	{
		Stage++;
	}
	return Stage;
}

bool UCampaignSubsystem::IsStageUnlocked(ECampaignLesson Lesson, int32 Stage) const
{
	if (IsMasterStage(Stage))
	{
		return IsMasterUnlocked(Lesson) && GetMasterPoints(Lesson) >= GetMasterStagePointsRequired(Stage);
	}
	return Stage >= 0 && Stage < GetNumStages() && GetLessonPoints(Lesson) >= GetStagePointsRequired(Stage);
}

bool UCampaignSubsystem::IsLessonCompleted(ECampaignLesson Lesson) const
{
	return GetLessonProgress(Lesson).bFinalCompleted;
}

bool UCampaignSubsystem::IsLessonUnlocked(ECampaignLesson Lesson) const
{
	const int32 LessonColumn = UCampaignTree::GetLessonColumn(Lesson);
	if (LessonColumn == INDEX_NONE)
	{
		return false;
	}

	for (int32 Column = 0; Column < LessonColumn; Column++)
	{
		for (ECampaignLesson Earlier : UCampaignTree::GetColumnLessons(Column))
		{
			if (!IsLessonCompleted(Earlier))
			{
				return false;
			}
		}
	}
	return true;
}

TArray<ECampaignLesson> UCampaignSubsystem::GetCompletedLessons() const
{
	TArray<ECampaignLesson> Completed;
	for (const TPair<ECampaignLesson, FLessonProgress>& Pair : GetProgressMap())
	{
		if (Pair.Value.bFinalCompleted)
		{
			Completed.Add(Pair.Key);
		}
	}
	return Completed;
}

void UCampaignSubsystem::RequestLessonPuzzle(ECampaignLesson Lesson, int32 Stage)
{
	if (!IsLessonUnlocked(Lesson) || !IsStageUnlocked(Lesson, Stage))
	{
		return;
	}

	// A different lesson or stage abandons the open puzzle; the same one resumes it
	if (!Data.bInSession || Data.SessionLesson != Lesson || Data.SessionStage != Stage)
	{
		Data.bPuzzleOpen = false;
	}

	Data.bInSession = true;
	Data.SessionLesson = Lesson;
	Data.SessionStage = Stage;
	Save();

	OnLessonPuzzleRequested.Broadcast(Lesson, Stage);
}

int32 UCampaignSubsystem::GetGenerationSeed(ECampaignMode Mode, ECampaignLesson Lesson, int32 Stage, int32 Seed)
{
	// Each stage's puzzle counter starts at 0, but every lesson and stage gets its own puzzles:
	// a unique generator seed per (lesson, stage, counter). Campaign puzzles are flagged in bit 28, Master Mode
	// puzzles also in bit 27 (their own range, so the campaign's seeds are unchanged).
	const int32 NumStages = GetNumStagesFor(Mode);
	const int32 NumMaster = GetMasterStages(Mode).Num();
	if (Stage >= NumStages && NumMaster > 0)
	{
		return (((Seed * NumMaster + (Stage - NumStages)) << 6) | (int32(Lesson) & 63)) | (1 << 28) | (1 << 27);
	}
	const int32 Seeds = ((Seed * NumStages + Stage) << 6) | (int32(Lesson) & 63);
	return Mode == ECampaignMode::Campaign ? (Seeds | (1 << 28)) : Seeds;
}

bool UCampaignSubsystem::StartLessonPuzzle(ECampaignLesson Lesson, int32 Stage, UPuzzle* Puzzle)
{
	if (!Puzzle || !IsStageUnlocked(Lesson, Stage) || !UPuzzle::IsLessonAvailable(Lesson, GetStageSize(Stage)))
	{
		return false;
	}

	FLessonProgress& Progress = GetMutableProgress(Lesson);
	int32 Seed = Progress.NextSeed[Stage];

	// A seed that can't produce a puzzle requiring the lesson is skipped for good, so the stage's
	// puzzle order stays the same every time
	// Campaign puzzles may use every clue type: all the lessons are done
	const bool bAllClueTypes = Data.Mode == ECampaignMode::Campaign;
	for (int32 Attempt = 0; Attempt < 20; Attempt++, Seed++)
	{
		if (!Puzzle->InitCampaign(GetGenerationSeed(Data.Mode, Lesson, Stage, Seed), GetStageSize(Stage), GetStageDifficulty(Stage), Lesson, bAllClueTypes))
		{
			continue;
		}

		Progress.NextSeed[Stage] = Seed;

		// Campaign puzzles are scored like Free Play, from the rating: rate it now, while the player is starting
		if (bAllClueTypes)
		{
			Puzzle->GetRating();
		}

		// Every start is a new play of the puzzle: InitCampaign cleared the hint counts
		Data.OpenSeed = Seed;
		Data.bInSession = true;
		Data.SessionLesson = Lesson;
		Data.SessionStage = Stage;
		Data.bPuzzleOpen = true;
		Save();
		return true;
	}

	return false;
}

bool UCampaignSubsystem::IsInLessonSession() const
{
	return Data.bInSession;
}

ECampaignLesson UCampaignSubsystem::GetSessionLesson() const
{
	return Data.SessionLesson;
}

int32 UCampaignSubsystem::GetSessionStage() const
{
	return Data.SessionStage;
}

FLessonPuzzleResult UCampaignSubsystem::FinishLessonPuzzle(UPuzzle* Puzzle)
{
	FLessonPuzzleResult Result;
	if (!Data.bInSession || !Puzzle)
	{
		return Result;
	}

	Result.Lesson = Data.SessionLesson;
	Result.Stage = Data.SessionStage;
	Result.bSolved = Puzzle->IsSolved();
	Result.Score = Data.Mode == ECampaignMode::Campaign ? 1 : Puzzle->GetCampaignScore();
	Result.bUsedHints = Puzzle->m_HintsUsed > 0;
	Result.bUsedLessonHint = Puzzle->m_LessonHintsUsed > 0;
	Result.bFirstFinish = Data.bPuzzleOpen;

	FLessonProgress& Progress = GetMutableProgress(Result.Lesson);
	Result.bMaster = IsMasterStage(Result.Stage);
	Result.PreviousPoints = Result.bMaster ? Progress.MasterPoints : Progress.Points;

	// Scored once, when first solved. A board with mistakes earns nothing and the puzzle stays open, so the
	// player can restart it and still earn its points.
	if (Data.bPuzzleOpen && Result.bSolved)
	{
		Data.bPuzzleOpen = false;
		Progress.NextSeed[Result.Stage] = Data.OpenSeed + 1;

		const int32 MaxPoints = GetMaxPoints();
		if (Result.bMaster)
		{
			// Master Mode keeps its own count: only its current stage counts, like the campaign
			const int32 MasterMax = GetMasterMaxPoints();
			const int32 StageBefore = GetCurrentMasterStage(Result.Lesson);
			if (Result.Stage == StageBefore && Progress.MasterPoints < MasterMax)
			{
				Progress.MasterPoints++;
				Result.PointsEarned = 1;
				Result.bStageUnlocked = GetCurrentMasterStage(Result.Lesson) > StageBefore;
				Result.bMasterCompleted = Progress.MasterPoints == MasterMax;
			}
		}
		else if (Result.Stage == GetFinalStage())
		{
			// The final completes the lesson whatever hints were used
			Result.bLessonCompleted = !Progress.bFinalCompleted;
			Progress.bFinalCompleted = true;
		}
		else if (Result.Stage == GetCurrentStage(Result.Lesson) && Progress.Points < MaxPoints)
		{
			// Only the stage currently being worked on earns points; replays of earlier stages don't
			const int32 StageBefore = GetCurrentStage(Result.Lesson);
			Progress.Points = FMath::Min(MaxPoints, Result.PreviousPoints + Result.Score);
			Result.PointsEarned = Progress.Points - Result.PreviousPoints;
			Result.bStageUnlocked = GetCurrentStage(Result.Lesson) > StageBefore;
		}
	}

	Result.TotalPoints = Result.bMaster ? Progress.MasterPoints : Progress.Points;
	LastResult = Result;

	Save();
	OnProgressChanged.Broadcast();
	return Result;
}

void UCampaignSubsystem::PrepareNextLessonPuzzle(const UObject* WorldContextObject, int32& Size, int32& Difficulty)
{
	Size = GetStageSize(0);
	Difficulty = GetStageDifficulty(0);

	UCampaignSubsystem* Campaign = Get(WorldContextObject);
	if (!Campaign)
	{
		return;
	}

	// Next puzzle always moves on through the lesson: the stage that currently earns points (the final once all
	// points are earned), whether the last puzzle unlocked it or was a replay of an earlier stage. In Master Mode,
	// its current stage. (Master Mode itself is only entered from the campaign tree, never by Next.)
	FCampaignSaveData& SaveData = Campaign->Data;
	const ECampaignLesson Lesson = SaveData.SessionLesson;
	const bool bToMaster = Campaign->IsMasterUnlocked(Lesson) && IsMasterStage(SaveData.SessionStage);
	const int32 CurrentStage = bToMaster ? Campaign->GetCurrentMasterStage(Lesson) : Campaign->GetCurrentStage(Lesson);
	if (SaveData.bInSession && SaveData.SessionStage != CurrentStage)
	{
		SaveData.SessionStage = CurrentStage;
		SaveData.bPuzzleOpen = false;
		Campaign->Save();
	}

	Size = GetStageSize(SaveData.SessionStage);
	Difficulty = GetStageDifficulty(SaveData.SessionStage);
}

void UCampaignSubsystem::EndLessonSession()
{
	if (Data.bInSession)
	{
		Data.bInSession = false;
		Data.bPuzzleOpen = false;
		Save();
	}
}

void UCampaignSubsystem::DebugCompleteAllLessons()
{
	for (const TArray<ECampaignLesson>& Column : UCampaignTree::GetColumns())
	{
		for (ECampaignLesson Lesson : Column)
		{
			FLessonProgress& Progress = Data.Lessons.FindOrAdd(Lesson);
			Progress.Points = GetStages(ECampaignMode::Lessons).Last().Required;
			Progress.bFinalCompleted = true;
		}
	}
	Save();
	OnProgressChanged.Broadcast();
}

void UCampaignSubsystem::ResetAllProgress()
{
	Data = FCampaignSaveData();
	Save();
	OnProgressChanged.Broadcast();
}

UCampaignSubsystem* UCampaignSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UCampaignSubsystem>() : nullptr;
}

void UCampaignSubsystem::InitPuzzleForPlay(const UObject* WorldContextObject, UPuzzle* Puzzle, int32 Number, int32 Size, int32 Difficulty)
{
	if (!Puzzle)
	{
		return;
	}

	UCampaignSubsystem* Campaign = Get(WorldContextObject);
	if (Campaign && Campaign->IsInLessonSession())
	{
		if (Campaign->StartLessonPuzzle(Campaign->GetSessionLesson(), Campaign->GetSessionStage(), Puzzle))
		{
			return;
		}
		// The lesson can't be played any more (shouldn't happen); fall back to a classic puzzle
		Campaign->EndLessonSession();
	}

	// Free play: only the clue types the player chose
	if (const UFreePlaySubsystem* FreePlay = UFreePlaySubsystem::Get(WorldContextObject))
	{
		Puzzle->m_ExcludedClues = FreePlay->GetExcludedClueMask();
	}
	Puzzle->Init(Number, Size, Difficulty);

	// Rate it now, while the player is starting, rather than on the end screen
	Puzzle->GetRating();
}

bool UCampaignSubsystem::HandlePuzzleFinished(const UObject* WorldContextObject, UPuzzle* Puzzle)
{
	UCampaignSubsystem* Campaign = Get(WorldContextObject);
	if (!Campaign || !Campaign->IsInLessonSession())
	{
		return false;
	}

	if (Campaign->Data.bPuzzleOpen)
	{
		Campaign->FinishLessonPuzzle(Puzzle);
	}
	return true;
}

bool UCampaignSubsystem::IsPlayingLesson(const UObject* WorldContextObject)
{
	const UCampaignSubsystem* Campaign = Get(WorldContextObject);
	return Campaign && Campaign->IsInLessonSession();
}

void UCampaignSubsystem::StopPlayingLesson(const UObject* WorldContextObject)
{
	if (UCampaignSubsystem* Campaign = Get(WorldContextObject))
	{
		Campaign->EndLessonSession();
	}
}

FLessonProgress& UCampaignSubsystem::GetMutableProgress(ECampaignLesson Lesson)
{
	TMap<ECampaignLesson, FLessonProgress>& Map = Data.Mode == ECampaignMode::Campaign ? Data.CampaignLessons : Data.Lessons;
	FLessonProgress& Progress = Map.FindOrAdd(Lesson);
	if (Progress.NextSeed.Num() < GetNumStages() + GetNumMasterStages())
	{
		Progress.NextSeed.SetNumZeroed(GetNumStages() + GetNumMasterStages());
	}
	return Progress;
}

void UCampaignSubsystem::Save()
{
	UHappinessSaveGame::SaveCurrentSettings();
}

#undef LOCTEXT_NAMESPACE
