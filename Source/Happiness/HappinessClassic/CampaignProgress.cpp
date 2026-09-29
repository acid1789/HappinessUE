#include "CampaignProgress.h"
#include "CampaignTree.h"
#include "Puzzle.h"
#include "HappinessSaveGame.h"
#include "FreePlaySettings.h"

#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "CampaignProgress"

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

int32 UCampaignSubsystem::GetStageSize(int32 Stage)
{
	return Stage <= 1 ? 3 : 4;
}

int32 UCampaignSubsystem::GetStageDifficulty(int32 Stage)
{
	switch (Stage)
	{
		case 0: case 2:	return 0;
		case 1: case 3:	return 1;
		default:		return 2;	// Final
	}
}

int32 UCampaignSubsystem::GetStagePointsRequired(int32 Stage)
{
	return FMath::Clamp(Stage, 0, FinalStage) * PointsPerStage;
}

FText UCampaignSubsystem::GetStageDisplayName(int32 Stage)
{
	switch (Stage)
	{
		case 0:		return LOCTEXT("Stage0", "3x3 Easy");
		case 1:		return LOCTEXT("Stage1", "3x3 Normal");
		case 2:		return LOCTEXT("Stage2", "4x4 Easy");
		case 3:		return LOCTEXT("Stage3", "4x4 Normal");
		default:	return LOCTEXT("StageFinal", "Final");
	}
}

FLessonProgress UCampaignSubsystem::GetLessonProgress(ECampaignLesson Lesson) const
{
	const FLessonProgress* Progress = Data.Lessons.Find(Lesson);
	return Progress ? *Progress : FLessonProgress();
}

int32 UCampaignSubsystem::GetLessonPoints(ECampaignLesson Lesson) const
{
	return GetLessonProgress(Lesson).Points;
}

int32 UCampaignSubsystem::GetCurrentStage(ECampaignLesson Lesson) const
{
	return FMath::Min(GetLessonPoints(Lesson) / PointsPerStage, FinalStage);
}

bool UCampaignSubsystem::IsStageUnlocked(ECampaignLesson Lesson, int32 Stage) const
{
	return Stage >= 0 && Stage < NumStages && GetLessonPoints(Lesson) >= GetStagePointsRequired(Stage);
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
	for (const TPair<ECampaignLesson, FLessonProgress>& Pair : Data.Lessons)
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

int32 UCampaignSubsystem::GetGenerationSeed(ECampaignLesson Lesson, int32 Stage, int32 Seed)
{
	// Each stage's puzzle counter starts at 0, but every lesson and stage gets its own puzzles:
	// a unique generator seed per (lesson, stage, counter)
	return ((Seed * NumStages + Stage) << 6) | (int32(Lesson) & 63);
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
	for (int32 Attempt = 0; Attempt < 20; Attempt++, Seed++)
	{
		if (!Puzzle->InitCampaign(GetGenerationSeed(Lesson, Stage, Seed), GetStageSize(Stage), GetStageDifficulty(Stage), Lesson))
		{
			continue;
		}

		Progress.NextSeed[Stage] = Seed;

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
	Result.Score = Puzzle->GetCampaignScore();
	Result.bUsedHints = Puzzle->m_HintsUsed > 0;
	Result.bUsedLessonHint = Puzzle->m_LessonHintsUsed > 0;
	Result.bFirstFinish = Data.bPuzzleOpen;

	FLessonProgress& Progress = GetMutableProgress(Result.Lesson);
	Result.PreviousPoints = Progress.Points;

	// Scored once, when first solved. A board with mistakes earns nothing and the puzzle stays open, so the
	// player can restart it and still earn its points.
	if (Data.bPuzzleOpen && Result.bSolved)
	{
		Data.bPuzzleOpen = false;
		Progress.NextSeed[Result.Stage] = Data.OpenSeed + 1;

		if (Result.Stage == FinalStage)
		{
			// The final completes the lesson whatever hints were used
			Result.bLessonCompleted = !Progress.bFinalCompleted;
			Progress.bFinalCompleted = true;
		}
		else if (Result.Stage == GetCurrentStage(Result.Lesson) && Progress.Points < MaxPoints)
		{
			// Only the stage currently being worked on earns points; replays of earlier stages don't
			Progress.Points = FMath::Min(MaxPoints, Result.PreviousPoints + Result.Score);
			Result.PointsEarned = Progress.Points - Result.PreviousPoints;
			Result.bStageUnlocked = Progress.Points / PointsPerStage > Result.PreviousPoints / PointsPerStage;
		}
	}

	Result.TotalPoints = Progress.Points;
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
	// points are earned), whether the last puzzle unlocked it or was a replay of an earlier stage
	FCampaignSaveData& SaveData = Campaign->Data;
	const int32 CurrentStage = Campaign->GetCurrentStage(SaveData.SessionLesson);
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
	FLessonProgress& Progress = Data.Lessons.FindOrAdd(Lesson);
	if (Progress.NextSeed.Num() < NumStages)
	{
		Progress.NextSeed.SetNumZeroed(NumStages);
	}
	return Progress;
}

void UCampaignSubsystem::Save()
{
	UHappinessSaveGame::SaveCurrentSettings();
}

#undef LOCTEXT_NAMESPACE
