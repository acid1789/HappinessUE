#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CampaignLesson.h"
#include "FreePlaySettings.generated.h"

class UPuzzle;

/** Free play settings, stored in the normal game save (UHappinessSaveGame::FreePlay) */
USTRUCT()
struct FFreePlaySaveData
{
	GENERATED_BODY()

	/** Clue types the player turned off; everything else is included, so new clue types start on */
	UPROPERTY(SaveGame)
	TArray<ECampaignLesson> ExcludedClues;

	/** The player's pace: a rolling average of seconds per rating point over their solved puzzles; 0 = none yet */
	UPROPERTY(SaveGame)
	float SecondsPerPoint = 0.f;

	UPROPERTY(SaveGame)
	int32 SolvedPuzzles = 0;

	/** UFreePlaySubsystem::RatingVersion the pace was learned with; a different one resets the pace */
	UPROPERTY(SaveGame)
	int32 RatingVersion = 0;
};

/** Score for a finished free play puzzle (UFreePlaySubsystem::FinishFreePlayPuzzle) */
USTRUCT(BlueprintType)
struct FFreePlayScore
{
	GENERATED_BODY()

	/** The puzzle's difficulty rating (UPuzzle::GetRating) */
	UPROPERTY(BlueprintReadOnly, Category = "Free Play")
	float Rating = 0.f;

	/** Time to beat: the rating at the player's pace, plus ParAllowance */
	UPROPERTY(BlueprintReadOnly, Category = "Free Play")
	float ParSeconds = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Free Play")
	float PuzzleSeconds = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Free Play")
	bool bSolved = false;

	/** Completion experience: the rating x ExpPerRatingPoint; 0 when the board has mistakes */
	UPROPERTY(BlueprintReadOnly, Category = "Free Play")
	int32 BaseExp = 0;

	/** Time bonus: up to MaxTimeBonus of the base, reached at half the par time */
	UPROPERTY(BlueprintReadOnly, Category = "Free Play")
	int32 BonusExp = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFreePlayCluesChanged);

/**
 * Free play settings: which clue types free play puzzles may use (all by default). Givens and NotHeres are always
 * included, so every puzzle can be solved. UCampaignSubsystem::InitPuzzleForPlay applies the selection to free play
 * puzzles.
 *
 * Free play scoring: a puzzle is worth its difficulty rating (UPuzzle::GetRating), so harder clues, fewer givens and
 * bigger boards all pay more. The par time is the rating at the player's own pace (seconds per rating point, a
 * rolling average of their solves), and beating it adds a time bonus. Hints are paid for in time (the game adds
 * time for each one).
 */
UCLASS()
class HAPPINESS_API UFreePlaySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** The clue types the player can turn on and off, in campaign order */
	UFUNCTION(BlueprintPure, Category = "Free Play")
	static TArray<ECampaignLesson> GetSelectableClues();

	/** Name of a clue type for the clue list */
	UFUNCTION(BlueprintPure, Category = "Free Play")
	static FText GetClueName(ECampaignLesson Clue);

	UFUNCTION(BlueprintPure, Category = "Free Play")
	bool IsClueIncluded(ECampaignLesson Clue) const;

	UFUNCTION(BlueprintCallable, Category = "Free Play")
	void SetClueIncluded(ECampaignLesson Clue, bool bIncluded);

	UFUNCTION(BlueprintCallable, Category = "Free Play")
	void SetAllCluesIncluded(bool bIncluded);

	UFUNCTION(BlueprintPure, Category = "Free Play")
	int32 GetIncludedClueCount() const;

	/** "All", "12 of 19" or "Givens only" */
	UFUNCTION(BlueprintPure, Category = "Free Play")
	FText GetSelectionSummary() const;

	/** For UPuzzle::m_ExcludedClues */
	int32 GetExcludedClueMask() const;

	// ---- Scoring ----

	/**
	 * Bump when the rating changes (UPuzzle::GetClueRatingWeight): a pace learned from the old ratings would be off,
	 * so it starts over
	 */
	static constexpr int32 RatingVersion = 2;
	/** Pace before the player has solved anything */
	static constexpr float DefaultSecondsPerPoint = 2.0f;
	/** Completion experience per rating point */
	static constexpr int32 ExpPerRatingPoint = 8;
	/** Par is this much longer than the player's pace would take */
	static constexpr float ParAllowance = 1.25f;
	/** Largest time bonus, as a fraction of the completion experience */
	static constexpr float MaxTimeBonus = 0.5f;
	/** Weight of each new solve in the rolling pace */
	static constexpr float PaceSmoothing = 0.2f;

	/** The player's pace in seconds per rating point */
	UFUNCTION(BlueprintPure, Category = "Free Play")
	float GetSecondsPerPoint() const;

	/** Par time for a puzzle: the player's pace, plus ParAllowance */
	UFUNCTION(BlueprintPure, Category = "Free Play", meta = (WorldContext = "WorldContextObject"))
	static float GetParSeconds(const UObject* WorldContextObject, UPuzzle* Puzzle);

	/**
	 * Score a finished free play puzzle, then fold its time into the player's pace (solved puzzles only). Calling it
	 * again for the same finish returns the same score without counting it twice.
	 */
	UFUNCTION(BlueprintCallable, Category = "Free Play", meta = (WorldContext = "WorldContextObject"))
	static FFreePlayScore FinishFreePlayPuzzle(const UObject* WorldContextObject, UPuzzle* Puzzle, float PuzzleSeconds);

	/** The last FinishFreePlayPuzzle result, for the end screen */
	UFUNCTION(BlueprintPure, Category = "Free Play", meta = (WorldContext = "WorldContextObject"))
	static FFreePlayScore GetLastFreePlayScore(const UObject* WorldContextObject);

	UPROPERTY(BlueprintAssignable, Category = "Free Play")
	FOnFreePlayCluesChanged OnCluesChanged;

	static UFreePlaySubsystem* Get(const UObject* WorldContextObject);

	/** The running game's settings, for code without a world context (the save game); null outside a game */
	static UFreePlaySubsystem* GetInstance() { return Instance.Get(); }

	const FFreePlaySaveData& GetSaveData() const { return Data; }

private:
	void Changed();

	FFreePlaySaveData Data;
	FFreePlayScore LastScore;
	TWeakObjectPtr<UPuzzle> LastScoredPuzzle;
	static TWeakObjectPtr<UFreePlaySubsystem> Instance;
};
