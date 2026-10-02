#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CampaignLesson.h"
#include "CampaignProgress.generated.h"

class UPuzzle;

/** Which track the lesson tree is being played in. Each has its own stages and progress. */
UENUM(BlueprintType)
enum class ECampaignMode : uint8
{
	/** Teaches each clue type: small boards, points per puzzle with hint penalties */
	Lessons,
	/** After every lesson is completed: each clue type again on bigger boards, one step per solved puzzle */
	Campaign,
};

/** One lesson's saved progress */
USTRUCT(BlueprintType)
struct FLessonProgress
{
	GENERATED_BODY()

	/** 0 to UCampaignSubsystem::GetMaxPoints(): points in Lessons, solved puzzles in Campaign */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Campaign")
	int32 Points = 0;

	/** Next seed to play for each stage. Every stage starts at seed 0 and advances when a puzzle is completed. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Campaign")
	TArray<int32> NextSeed;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Campaign")
	bool bFinalCompleted = false;

	/** Campaign mode only: Master Mode puzzles solved, 0 to UCampaignSubsystem::GetMasterMaxPoints().
	 *  Master Mode opens when the final is completed and doesn't count toward the campaign. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Campaign")
	int32 MasterPoints = 0;

	/** Master Mode has been shown to the player: the first time they open the clue from the tree after its final.
	 *  Until then nothing mentions it, not the end screen nor the popup that reopens after the puzzle. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Campaign")
	bool bMasterRevealed = false;
};

/** What finishing a lesson puzzle did */
USTRUCT(BlueprintType)
struct FLessonPuzzleResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	ECampaignLesson Lesson = ECampaignLesson::Given;

	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	int32 Stage = 0;

	/** False if the finished board has mistakes: nothing is scored and the puzzle stays open to try again */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	bool bSolved = false;

	/** False when the puzzle had already been scored (finished again after a restart) */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	bool bFirstFinish = false;

	/** Any hint was used (-1) */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	bool bUsedHints = false;

	/** A hint was used on the lesson's clue (another -1) */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	bool bUsedLessonHint = false;

	/** The puzzle's score. Lessons: 1 to 3 (3 for completing it, -1 for using hints, another -1 for a lesson-clue
	 *  hint). Campaign: 1 for solving it. */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	int32 Score = 0;

	/** Points added to the lesson; 0 when replaying an earlier stage or the final */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	int32 PointsEarned = 0;

	/** Lesson points before this puzzle */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	int32 PreviousPoints = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	int32 TotalPoints = 0;

	/** True if this puzzle's points unlocked the next stage (or the final) */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	bool bStageUnlocked = false;

	/** True the first time the final is completed; the next lessons in the tree unlock */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	bool bLessonCompleted = false;

	/** A Master Mode puzzle: PreviousPoints/TotalPoints/PointsEarned count Master Mode puzzles, not the campaign */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	bool bMaster = false;

	/** True the first time every Master Mode puzzle is solved */
	UPROPERTY(BlueprintReadOnly, Category = "Campaign")
	bool bMasterCompleted = false;
};

/** Campaign progress, stored in the normal game save (UHappinessSaveGame::Campaign) */
USTRUCT()
struct FCampaignSaveData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	TMap<ECampaignLesson, FLessonProgress> Lessons;

	/** Campaign mode's progress, kept apart from the lessons' */
	UPROPERTY(SaveGame)
	TMap<ECampaignLesson, FLessonProgress> CampaignLessons;

	/** The track being played; the session below belongs to it */
	UPROPERTY(SaveGame)
	ECampaignMode Mode = ECampaignMode::Lessons;

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
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCampaignProgressChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLessonPuzzleRequested, ECampaignLesson, Lesson, int32, Stage);

/**
 * Campaign progress: points, stages and seeds per lesson. Kept in the normal game save (SG_Happiness, whose
 * parent is UHappinessSaveGame); every SG_Happiness written to disk includes the current progress.
 *
 * Two tracks over the same lesson tree (ECampaignMode), each with its own progress:
 * - Lessons: stages 0-3 (3x3 easy, 3x3 normal, 4x4 easy, 4x4 normal), unlocked at 0/9/18/27 points, and a final
 *   (4x4 hard) at 36. A puzzle scores 3, less 1 for any hint and 1 more for a hint on the lesson's clue.
 * - Campaign, unlocked by completing every lesson: 4 puzzles of 5x5 normal, 1 of 5x5 hard, 4 of 6x6 normal,
 *   1 of 6x6 hard, 4 of 7x7 normal, 1 of 7x7 hard, then the final, 8x8 easy. Each solved puzzle is one step.
 * Only the current stage earns progress; earlier stages and the final can be replayed freely. Completing the
 * final completes that clue. The stage functions below describe the current mode's track.
 *
 * Master Mode (Campaign only) opens for a clue once its final is completed: 4 puzzles of 8x8 normal, then 4 of
 * 8x8 hard (unlocked by the first 4). They come after the final, numbered from GetFirstMasterStage(), and have
 * their own progress (FLessonProgress::MasterPoints) that doesn't count toward the campaign.
 *
 * Game flow: the lesson popup calls RequestLessonPuzzle. The game creates puzzles with InitPuzzleForPlay (classic or
 * lesson, depending on the session) and reports finished ones with HandlePuzzleFinished.
 */
UCLASS()
class HAPPINESS_API UCampaignSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ---- Mode ----

	/** The track being played (Lessons outside a game) */
	UFUNCTION(BlueprintPure, Category = "Campaign")
	static ECampaignMode GetMode();

	/** Switch track. A session in the other track ends. Broadcasts OnProgressChanged. */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void SetMode(ECampaignMode NewMode);

	/** Campaign mode opens once every lesson is completed */
	UFUNCTION(BlueprintPure, Category = "Campaign")
	bool IsCampaignModeUnlocked() const;

	// ---- Stage definitions (current mode) ----

	UFUNCTION(BlueprintPure, Category = "Campaign")
	static int32 GetNumStages();

	UFUNCTION(BlueprintPure, Category = "Campaign")
	static int32 GetFinalStage() { return GetNumStages() - 1; }

	/** Progress that completes the track up to the final */
	UFUNCTION(BlueprintPure, Category = "Campaign")
	static int32 GetMaxPoints();

	/** Best score of one puzzle: 3 in Lessons, 1 in Campaign */
	UFUNCTION(BlueprintPure, Category = "Campaign")
	static int32 GetMaxPuzzleScore();

	// ---- Master Mode (current mode; none in Lessons) ----

	UFUNCTION(BlueprintPure, Category = "Campaign")
	static int32 GetNumMasterStages();

	/** Stage number of the first Master Mode stage: right after the final */
	UFUNCTION(BlueprintPure, Category = "Campaign")
	static int32 GetFirstMasterStage() { return GetNumStages(); }

	UFUNCTION(BlueprintPure, Category = "Campaign")
	static bool IsMasterStage(int32 Stage);

	/** Master Mode puzzles that complete it */
	UFUNCTION(BlueprintPure, Category = "Campaign")
	static int32 GetMasterMaxPoints();

	/** Master Mode puzzles solved to unlock a Master Mode stage */
	UFUNCTION(BlueprintPure, Category = "Campaign")
	static int32 GetMasterStagePointsRequired(int32 Stage);

	UFUNCTION(BlueprintPure, Category = "Campaign")
	int32 GetMasterPoints(ECampaignLesson Lesson) const;

	/** Master Mode is open once the clue's final is completed (Campaign mode) */
	UFUNCTION(BlueprintPure, Category = "Campaign")
	bool IsMasterUnlocked(ECampaignLesson Lesson) const;

	UFUNCTION(BlueprintPure, Category = "Campaign")
	bool IsMasterCompleted(ECampaignLesson Lesson) const;

	/** Master Mode is unlocked and the player has been shown it (FLessonProgress::bMasterRevealed) */
	UFUNCTION(BlueprintPure, Category = "Campaign")
	bool IsMasterRevealed(ECampaignLesson Lesson) const;

	/** The player opened the clue from the tree: from now on its Master Mode is shown, if unlocked. Saves. */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void RevealMaster(ECampaignLesson Lesson);

	/** The Master Mode stage that currently counts */
	UFUNCTION(BlueprintPure, Category = "Campaign")
	int32 GetCurrentMasterStage(ECampaignLesson Lesson) const;

	/** Stage size, difficulty and the progress needed to unlock it, for a given mode */
	static int32 GetStageSizeFor(ECampaignMode Mode, int32 Stage);
	static int32 GetStageDifficultyFor(ECampaignMode Mode, int32 Stage);
	static int32 GetNumStagesFor(ECampaignMode Mode);

	UFUNCTION(BlueprintPure, Category = "Campaign")
	static int32 GetStageSize(int32 Stage);

	UFUNCTION(BlueprintPure, Category = "Campaign")
	static int32 GetStageDifficulty(int32 Stage);

	/** Points needed to unlock the stage */
	UFUNCTION(BlueprintPure, Category = "Campaign")
	static int32 GetStagePointsRequired(int32 Stage);

	UFUNCTION(BlueprintPure, Category = "Campaign")
	static FText GetStageDisplayName(int32 Stage);

	/** Generator seed for a stage's Seed-th puzzle; distinct for every mode, lesson and stage */
	static int32 GetGenerationSeed(ECampaignMode Mode, ECampaignLesson Lesson, int32 Stage, int32 Seed);

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

	/** True once every lesson in the earlier tree columns is completed (in the current mode) */
	UFUNCTION(BlueprintPure, Category = "Campaign")
	bool IsLessonUnlocked(ECampaignLesson Lesson) const;

	UFUNCTION(BlueprintPure, Category = "Campaign")
	TArray<ECampaignLesson> GetCompletedLessons() const;

	UPROPERTY(BlueprintAssignable, Category = "Campaign")
	FOnCampaignProgressChanged OnProgressChanged;

	// ---- Lesson session ----
	// A lesson session is active while the player is playing a lesson stage: from choosing a stage until they
	// start a classic game. It is saved with the campaign, together with the open puzzle's seed, so reloading the
	// game resumes the same lesson puzzle. Hint penalties belong to one play of a puzzle and aren't saved.

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

	/**
	 * Before playing the session's next puzzle: the session moves on to the stage that currently earns points (the
	 * final once all points are earned). Returns the size and difficulty of the stage to play.
	 */
	UFUNCTION(BlueprintCallable, Category = "Campaign", meta = (WorldContext = "WorldContextObject"))
	static void PrepareNextLessonPuzzle(const UObject* WorldContextObject, int32& Size, int32& Difficulty);

	/** Debug: complete every lesson (unlocks Campaign mode). Console: Happiness.CompleteAllLessons */
	void DebugCompleteAllLessons();

	/** Debug: wipe all campaign progress */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void ResetAllProgress();

	// ---- Hooks for the game flow (static so a Blueprint needs one node) ----

	/** Use in place of UPuzzle::Init: sets up the lesson puzzle during a lesson session, otherwise a classic puzzle */
	UFUNCTION(BlueprintCallable, Category = "Campaign", meta = (WorldContext = "WorldContextObject"))
	static void InitPuzzleForPlay(const UObject* WorldContextObject, UPuzzle* Puzzle, int32 Number, int32 Size, int32 Difficulty);

	/**
	 * Call when a puzzle is finished. During a lesson session returns true (skip classic bookkeeping) and records the
	 * result if the lesson end screen hasn't already (FinishLessonPuzzle).
	 */
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
	struct FStage
	{
		int32 Size;
		int32 Difficulty;
		int32 Required;
	};
	static const TArray<FStage>& GetStages(ECampaignMode Mode);
	static const TArray<FStage>& GetMasterStages(ECampaignMode Mode);
	static const FStage& GetStage(ECampaignMode Mode, int32 Stage);

	/** The current mode's progress */
	const TMap<ECampaignLesson, FLessonProgress>& GetProgressMap() const;
	FLessonProgress& GetMutableProgress(ECampaignLesson Lesson);
	void Save();

	FCampaignSaveData Data;

	FLessonPuzzleResult LastResult;
	static TWeakObjectPtr<UCampaignSubsystem> Instance;
};
