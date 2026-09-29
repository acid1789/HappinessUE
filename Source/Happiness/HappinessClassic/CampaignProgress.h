#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CampaignLesson.h"
#include "CampaignProgress.generated.h"

class UPuzzle;

/** One lesson's saved progress */
USTRUCT(BlueprintType)
struct FLessonProgress
{
	GENERATED_BODY()

	/** 0 to UCampaignSubsystem::MaxPoints */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Campaign")
	int32 Points = 0;

	/** Next seed to play for each stage. Every stage starts at seed 0 and advances when a puzzle is completed. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Campaign")
	TArray<int32> NextSeed;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Campaign")
	bool bFinalCompleted = false;
};

/** What finishing a lesson puzzle did */
USTRUCT(BlueprintType)
struct FLessonPuzzleResult
{
	GENERATED_BODY()

	/** The puzzle's score, 1 to 3 (3 = no hints, 2 = hints but none from the lesson clue, 1 = a lesson-clue hint) */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	int32 Score = 0;

	/** Points added to the lesson; 0 when replaying an earlier stage or the final */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	int32 PointsEarned = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	int32 TotalPoints = 0;

	/** True if this puzzle's points unlocked the next stage (or the final) */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	bool bStageUnlocked = false;

	/** True the first time the final is completed; the next lessons in the tree unlock */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	bool bLessonCompleted = false;
};

/** Campaign progress, stored in the normal game save (UHappinessSaveGame::Campaign) */
USTRUCT()
struct FCampaignSaveData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	TMap<ECampaignLesson, FLessonProgress> Lessons;

	// The lesson session and its open puzzle, so a reload resumes it
	UPROPERTY(SaveGame)
	bool bInSession = false;

	UPROPERTY(SaveGame)
	ECampaignLesson SessionLesson = ECampaignLesson::Given;

	UPROPERTY(SaveGame)
	int32 SessionStage = 0;

	/** A session puzzle has been started and not finished */
	UPROPERTY(SaveGame)
	bool bPuzzleOpen = false;

	UPROPERTY(SaveGame)
	int32 OpenSeed = 0;

	UPROPERTY(SaveGame)
	int32 OpenHintsUsed = 0;

	UPROPERTY(SaveGame)
	int32 OpenLessonHintsUsed = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCampaignProgressChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLessonPuzzleRequested, ECampaignLesson, Lesson, int32, Stage);

/**
 * Campaign progress: points, stages and seeds per lesson. Kept in the normal game save (SG_Happiness, whose
 * parent is UHappinessSaveGame); every SG_Happiness written to disk includes the current progress.
 *
 * Each lesson has stages 0-3 (3x3 easy, 3x3 normal, 4x4 easy, 4x4 normal), unlocked at 0/9/18/27 points,
 * and a final stage (4x4 hard) unlocked at 36 points. Only the current stage earns points; earlier stages
 * and the final can be replayed freely. Completing the final completes the lesson.
 *
 * Game flow: the lesson popup calls RequestLessonPuzzle. The game creates puzzles with InitPuzzleForPlay (classic or
 * lesson, depending on the session) and reports finished ones with HandlePuzzleFinished.
 */
UCLASS()
class HAPPINESS_API UCampaignSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static constexpr int32 NumStages = 5;
	static constexpr int32 FinalStage = 4;
	static constexpr int32 PointsPerStage = 9;
	static constexpr int32 MaxPoints = PointsPerStage * FinalStage;
	static constexpr int32 MaxPuzzleScore = 3;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ---- Stage definitions ----

	UFUNCTION(BlueprintPure, Category = "Campaign")
	static int32 GetNumStages() { return NumStages; }

	UFUNCTION(BlueprintPure, Category = "Campaign")
	static int32 GetFinalStage() { return FinalStage; }

	UFUNCTION(BlueprintPure, Category = "Campaign")
	static int32 GetMaxPoints() { return MaxPoints; }

	UFUNCTION(BlueprintPure, Category = "Campaign")
	static int32 GetStageSize(int32 Stage);

	UFUNCTION(BlueprintPure, Category = "Campaign")
	static int32 GetStageDifficulty(int32 Stage);

	/** Points needed to unlock the stage */
	UFUNCTION(BlueprintPure, Category = "Campaign")
	static int32 GetStagePointsRequired(int32 Stage);

	UFUNCTION(BlueprintPure, Category = "Campaign")
	static FText GetStageDisplayName(int32 Stage);

	/** Generator seed for a stage's Seed-th puzzle; distinct for every lesson and stage */
	static int32 GetGenerationSeed(ECampaignLesson Lesson, int32 Stage, int32 Seed);

	// ---- Progress ----

	UFUNCTION(BlueprintPure, Category = "Campaign")
	FLessonProgress GetLessonProgress(ECampaignLesson Lesson) const;

	UFUNCTION(BlueprintPure, Category = "Campaign")
	int32 GetLessonPoints(ECampaignLesson Lesson) const;

	/** The stage that currently earns points, or the final once all points are earned */
	UFUNCTION(BlueprintPure, Category = "Campaign")
	int32 GetCurrentStage(ECampaignLesson Lesson) const;

	UFUNCTION(BlueprintPure, Category = "Campaign")
	bool IsStageUnlocked(ECampaignLesson Lesson, int32 Stage) const;

	/** True once the lesson's final has been completed */
	UFUNCTION(BlueprintPure, Category = "Campaign")
	bool IsLessonCompleted(ECampaignLesson Lesson) const;

	/** True once every lesson in the earlier tree columns is completed */
	UFUNCTION(BlueprintPure, Category = "Campaign")
	bool IsLessonUnlocked(ECampaignLesson Lesson) const;

	UFUNCTION(BlueprintPure, Category = "Campaign")
	TArray<ECampaignLesson> GetCompletedLessons() const;

	UPROPERTY(BlueprintAssignable, Category = "Campaign")
	FOnCampaignProgressChanged OnProgressChanged;

	// ---- Lesson session ----
	// A lesson session is active while the player is playing a lesson stage: from choosing a stage until they
	// start a classic game. It is saved with the campaign, together with the open puzzle's seed and hint counts,
	// so reloading the game resumes the same lesson puzzle.

	/** Called by the lesson popup when the player picks a stage: starts a session for it and broadcasts OnLessonPuzzleRequested */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void RequestLessonPuzzle(ECampaignLesson Lesson, int32 Stage);

	UPROPERTY(BlueprintAssignable, Category = "Campaign")
	FOnLessonPuzzleRequested OnLessonPuzzleRequested;

	/** Sets Puzzle up as the stage's next (or still open) lesson puzzle */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	bool StartLessonPuzzle(ECampaignLesson Lesson, int32 Stage, UPuzzle* Puzzle);

	UFUNCTION(BlueprintPure, Category = "Campaign")
	bool IsInLessonSession() const;

	UFUNCTION(BlueprintPure, Category = "Campaign")
	ECampaignLesson GetSessionLesson() const;

	UFUNCTION(BlueprintPure, Category = "Campaign")
	int32 GetSessionStage() const;

	/** Records a finished session puzzle: if solved, advances the stage's seed and adds points. Saves. */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	FLessonPuzzleResult FinishLessonPuzzle(UPuzzle* Puzzle);

	/** Leave the lesson session (e.g. a classic game is starting) */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void EndLessonSession();

	/** Result of the last FinishLessonPuzzle, for end-of-puzzle UI */
	UFUNCTION(BlueprintPure, Category = "Campaign")
	FLessonPuzzleResult GetLastResult() const { return LastResult; }

	/** Debug: wipe all campaign progress */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void ResetAllProgress();

	// ---- Hooks for the game flow (static so a Blueprint needs one node) ----

	/** Use in place of UPuzzle::Init: sets up the lesson puzzle during a lesson session, otherwise a classic puzzle */
	UFUNCTION(BlueprintCallable, Category = "Campaign", meta = (WorldContext = "WorldContextObject"))
	static void InitPuzzleForPlay(const UObject* WorldContextObject, UPuzzle* Puzzle, int32 Number, int32 Size, int32 Difficulty);

	/** Call when a puzzle is finished. During a lesson session records the result and returns true (skip classic bookkeeping). */
	UFUNCTION(BlueprintCallable, Category = "Campaign", meta = (WorldContext = "WorldContextObject"))
	static bool HandlePuzzleFinished(const UObject* WorldContextObject, UPuzzle* Puzzle);

	UFUNCTION(BlueprintPure, Category = "Campaign", meta = (WorldContext = "WorldContextObject"))
	static bool IsPlayingLesson(const UObject* WorldContextObject);

	/** Ends any lesson session; call when a classic game is started */
	UFUNCTION(BlueprintCallable, Category = "Campaign", meta = (WorldContext = "WorldContextObject"))
	static void StopPlayingLesson(const UObject* WorldContextObject);

	static UCampaignSubsystem* Get(const UObject* WorldContextObject);

	/** The running game's campaign, for code without a world context (the save game); null outside a game */
	static UCampaignSubsystem* GetInstance() { return Instance.Get(); }

	const FCampaignSaveData& GetSaveData() const { return Data; }

private:
	FLessonProgress& GetMutableProgress(ECampaignLesson Lesson);
	void HandleHintUsed(UPuzzle* Puzzle);
	void Save();

	FCampaignSaveData Data;

	FLessonPuzzleResult LastResult;
	static TWeakObjectPtr<UCampaignSubsystem> Instance;
	FDelegateHandle HintUsedHandle;
};
