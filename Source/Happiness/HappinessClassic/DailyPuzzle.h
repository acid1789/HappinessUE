#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CampaignLesson.h"
#include "DailyPuzzle.generated.h"

class UPuzzle;

/** The daily puzzle's streak and session, stored in the normal game save (UHappinessSaveGame::Daily) */
USTRUCT()
struct FDailySaveData
{
	GENERATED_BODY()

	/** Date (YYYYMMDD) of the latest daily puzzle solved; 0 = none yet */
	UPROPERTY(SaveGame)
	int32 LastSolvedDate = 0;

	/** Daily puzzles solved on consecutive dates, up to LastSolvedDate */
	UPROPERTY(SaveGame)
	int32 Streak = 0;

	UPROPERTY(SaveGame)
	int32 BestStreak = 0;

	/** A daily puzzle is being played (so a reload resumes it) and the date it is for */
	UPROPERTY(SaveGame)
	bool bActive = false;

	UPROPERTY(SaveGame)
	int32 ActiveDate = 0;
};

/** One day's daily puzzle, worked out from the date alone: the same for everyone on that date */
USTRUCT(BlueprintType)
struct FDailyPuzzleSpec
{
	GENERATED_BODY()

	/** YYYYMMDD */
	UPROPERTY(BlueprintReadOnly, Category = "Daily")
	int32 Date = 0;

	/** UPuzzle::Init's seed */
	UPROPERTY(BlueprintReadOnly, Category = "Daily")
	int32 Seed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Daily")
	int32 Size = 5;

	UPROPERTY(BlueprintReadOnly, Category = "Daily")
	int32 Difficulty = 1;

	/** The clue types it may use */
	UPROPERTY(BlueprintReadOnly, Category = "Daily")
	TArray<ECampaignLesson> Clues;

	/** For UPuzzle::m_ExcludedClues: every other clue type */
	int32 ExcludedClueMask = 0;
};

/**
 * The daily puzzle: one puzzle per calendar date (local time), chosen at random from the date: its clue types,
 * size, difficulty and seed. It plays like a Free Play puzzle; the first solve of each date's puzzle is worth
 * double experience and counts toward the streak of consecutive days.
 *
 * Game flow: the Daily button on the mode screen ends any lesson session and fires OnDailyChosen; WBP_GameSelect
 * then calls StartDaily and plays the puzzle it returns. UCampaignSubsystem::InitPuzzleForPlay builds the daily
 * while one is active, and UFreePlaySubsystem::FinishFreePlayPuzzle scores it and ends the session once solved.
 */
UCLASS()
class HAPPINESS_API UDailySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Experience multiplier for the first solve of a date's daily puzzle */
	static constexpr int32 ExpMultiplier = 2;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Today's date as YYYYMMDD (local time; the console variable Happiness.DailyDate overrides it for testing) */
	UFUNCTION(BlueprintPure, Category = "Daily")
	static int32 GetToday();

	/** The daily puzzle for a date (YYYYMMDD) */
	UFUNCTION(BlueprintPure, Category = "Daily")
	static FDailyPuzzleSpec GetSpec(int32 Date);

	UFUNCTION(BlueprintPure, Category = "Daily")
	static FDailyPuzzleSpec GetTodaySpec() { return GetSpec(GetToday()); }

	/** The current streak: 0 once a day has been missed */
	UFUNCTION(BlueprintPure, Category = "Daily")
	int32 GetStreak() const;

	UFUNCTION(BlueprintPure, Category = "Daily")
	int32 GetBestStreak() const { return Data.BestStreak; }

	UFUNCTION(BlueprintPure, Category = "Daily")
	bool IsTodaySolved() const { return Data.LastSolvedDate >= GetToday(); }

	/** Start today's daily puzzle: returns what to pass to the game's PlayHappiness (number = seed, size, difficulty).
	 *  Returns false, starting nothing, once today's puzzle is solved: one a day. */
	UFUNCTION(BlueprintCallable, Category = "Daily", meta = (WorldContext = "WorldContextObject"))
	static bool StartDaily(const UObject* WorldContextObject, int32& Number, int32& Size, int32& Difficulty);

	/** Leave the daily puzzle (another mode is starting) */
	UFUNCTION(BlueprintCallable, Category = "Daily", meta = (WorldContext = "WorldContextObject"))
	static void StopDaily(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Daily", meta = (WorldContext = "WorldContextObject"))
	static bool IsPlayingDaily(const UObject* WorldContextObject);

	/** The daily puzzle being played, if any */
	bool GetActiveSpec(FDailyPuzzleSpec& OutSpec) const;

	/** True if Puzzle is the daily puzzle being played */
	bool IsActiveDailyPuzzle(const UPuzzle* Puzzle) const;

	/**
	 * Daily touches on the Free Play end screen (WBP_EndScreen, after FinishFreePlayPuzzle): for a daily puzzle,
	 * hides Restart and Next Puzzle (Button_RestartPuzzle, Button_NextPuzzle), shows DailyStreakText, and on the first
	 * solve arms the doubling step (UFreePlayEndScreenWidget: Total EXP doubles, then DailyBonusText). For any other
	 * puzzle, shows the buttons and hides both lines.
	 */
	UFUNCTION(BlueprintCallable, Category = "Daily")
	static void ShowDailyEndScreen(UUserWidget* EndScreen);

	/**
	 * A daily puzzle was solved: ends the session and, the first time that date's puzzle is solved, counts it toward
	 * the streak. Returns true for that first solve (worth ExpMultiplier times the experience). Saves.
	 */
	bool RecordSolve(UPuzzle* Puzzle);

	/** Testing (console Happiness.ResetDaily [all]): undo today's solve so today's puzzle can be played again, the
	 *  streak back to before it; with bAll, forget every solve and the streaks. Ends any daily being played. */
	void DebugReset(bool bAll);

	static UDailySubsystem* Get(const UObject* WorldContextObject);

	/** The running game's daily data, for code without a world context (the save game); null outside a game */
	static UDailySubsystem* GetInstance() { return Instance.Get(); }

	const FDailySaveData& GetSaveData() const { return Data; }

private:
	void Save();

	FDailySaveData Data;
	static TWeakObjectPtr<UDailySubsystem> Instance;
};
