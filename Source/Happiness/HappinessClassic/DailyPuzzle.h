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

	/** Streak saver days bought and not used yet: each one covers a day the streak missed */
	UPROPERTY(SaveGame)
	int32 SaverDays = 0;

	/** The latest date (YYYYMMDD) streak savers covered: the streak carries on from it as if solved; 0 = none */
	UPROPERTY(SaveGame)
	int32 CoveredThrough = 0;

	/** Store purchases already turned into saver days, so one the store reports again isn't counted twice */
	UPROPERTY(SaveGame)
	TArray<FString> SaverTransactions;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDailyChanged);

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
 *
 * Streak savers (bought through UStreakSaverStoreSubsystem) are days that cover missed days. Days missed since the
 * streak's last day use them up first: a broken streak comes back, without counting the missed days. What's left
 * covers later misses. Days are only used when there are enough to cover every missed day; otherwise the streak
 * stays broken and they wait (more saver days can still bring it back, until a new streak starts).
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

	/** The current streak: 0 once a day has been missed that streak savers can't cover */
	UFUNCTION(BlueprintPure, Category = "Daily")
	int32 GetStreak() const;

	UFUNCTION(BlueprintPure, Category = "Daily")
	int32 GetBestStreak() const { return Data.BestStreak; }

	UFUNCTION(BlueprintPure, Category = "Daily")
	bool IsTodaySolved() const { return Data.LastSolvedDate >= GetToday(); }

	/** Streak saver days left, after covering any days the streak has missed */
	UFUNCTION(BlueprintPure, Category = "Daily|Streak Saver")
	int32 GetSaverDays() const;

	/** A streak that ended and could come back with more saver days: its length, the days it missed and how many more
	 *  saver days would cover them. False if the streak is going (or there is none to bring back). */
	UFUNCTION(BlueprintPure, Category = "Daily|Streak Saver")
	bool GetBrokenStreak(int32& OutStreak, int32& OutMissedDays, int32& OutSaverDaysNeeded) const;

	/** Bought streak saver days (TransactionId: the store purchase, or empty): they cover missed days first. Saves. */
	void AddSaverDays(int32 Days, const FString& TransactionId);

	/** The store purchase has already been turned into saver days */
	bool HasSaverTransaction(const FString& TransactionId) const { return Data.SaverTransactions.Contains(TransactionId); }

	/** The streak or streak savers changed */
	UPROPERTY(BlueprintAssignable, Category = "Daily")
	FOnDailyChanged OnChanged;

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

	/** Testing (console Happiness.StreakSaver.Set <days>): the unused saver days, without a purchase */
	void DebugSetSaverDays(int32 Days);

	/** Testing (console Happiness.ResetDaily [all]): undo today's solve so today's puzzle can be played again, the
	 *  streak back to before it; with bAll, forget every solve, the streaks and the streak savers. Ends any daily
	 *  being played. */
	void DebugReset(bool bAll);

	static UDailySubsystem* Get(const UObject* WorldContextObject);

	/** The running game's daily data, for code without a world context (the save game); null outside a game */
	static UDailySubsystem* GetInstance() { return Instance.Get(); }

	const FDailySaveData& GetSaveData() const { return Data; }

private:
	void Save();

	/** The streak as of a date: its last day (solved, or covered by savers), the saver days left and the days missed
	 *  since. Saver days only cover missed days when there are enough for all of them. */
	struct FStreakState
	{
		int32 LastDay = 0;
		int32 SaverDays = 0;
		int32 MissedDays = 0;
	};
	FStreakState GetStreakState(int32 Date) const;
	/** Keep what savers covered by a date (GetStreakState), before anything else changes the streak */
	void ApplySavers(int32 Date);

	FDailySaveData Data;
	static TWeakObjectPtr<UDailySubsystem> Instance;
};
