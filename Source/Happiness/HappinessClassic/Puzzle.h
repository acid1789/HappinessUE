#pragma once

#include <CoreMinimal.h>
#include <UObject/Object.h>
#include "Clue.h"
#include "PuzzleRow.h"
#include "Hint.h"
#include "Puzzle.generated.h"

/** How hard a puzzle is to solve, from UPuzzle::ComputeRating */
USTRUCT(BlueprintType)
struct FPuzzleRating
{
	GENERATED_BODY()

	/** Sum over the solve's steps of each step's clue weight; 0 if it couldn't be solved */
	UPROPERTY(BlueprintReadOnly)
	float Rating = 0.f;

	/** Deductions (single eliminations or placements) needed to solve it */
	UPROPERTY(BlueprintReadOnly)
	int32 Steps = 0;

	/** The hardest clue type the solve needed */
	UPROPERTY(BlueprintReadOnly)
	ECampaignLesson HardestClue = ECampaignLesson::Given;

	UPROPERTY(BlueprintReadOnly)
	bool bSolved = false;
};

UCLASS(BlueprintType)
class HAPPINESS_API UPuzzle : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	int32 m_iSeed;

	UPROPERTY(BlueprintReadOnly)
	int32 m_iSize;

	UPROPERTY(BlueprintReadOnly)
	int32 m_iDifficulty;

	UPROPERTY(BlueprintReadOnly)
	TArray<int> m_Solution;

	UPROPERTY(BlueprintReadOnly)
	TArray<FPuzzleRow> m_Rows;

	UPROPERTY(BlueprintReadOnly)
	TArray<FPuzzleRow> m_MarkerRows;

	UPROPERTY(BlueprintReadOnly)
	TArray<UClue*> m_Clues;

	UPROPERTY(BlueprintReadOnly)
	TArray<UClue*> m_GivenClues;

	UPROPERTY(BlueprintReadOnly)
	TArray<UClue*> m_HorizontalClues;

	UPROPERTY(BlueprintReadOnly)
	TArray<UClue*> m_VeritcalClues;

	UPROPERTY(BlueprintReadWrite)
	bool AutoSetIcons = true;

	// Set by InitCampaign: clue generation is limited to lessons up to m_CampaignLesson
	UPROPERTY(BlueprintReadOnly)
	bool m_bCampaign = false;

	UPROPERTY(BlueprintReadOnly)
	ECampaignLesson m_CampaignLesson = ECampaignLesson::Given;

	// Set by InitCampaign for Campaign mode: every clue type may appear, not just lessons up to m_CampaignLesson
	bool m_bCampaignAllClues = false;

	// Chance that a generated clue insists on m_CampaignLesson's type (the rest may be any allowed type).
	// Set by InitCampaign: LessonClueBias in Lessons, CampaignClueBias in Campaign mode.
	float m_LessonClueBias = 0.5f;
	static constexpr float LessonClueBias = 0.5f;
	// Higher in Campaign mode, where every clue type is allowed: about 3 in 4 of the final clues are the featured
	// type (half was about 56%), for 6-9% more clues per puzzle
	static constexpr float CampaignClueBias = 0.75f;

	// Testing: when >= 0, InitCampaign uses this bias instead (the Puzzle commandlet's -lessonbias=)
	float m_LessonClueBiasOverride = -1.f;

	// Free play: clue types (bit 1 << ECampaignLesson) that Init won't generate. Set before Init; Given (givens and
	// NotHeres) can't be excluded, so every puzzle can still be solved. Ignored by InitCampaign.
	UPROPERTY(BlueprintReadWrite)
	int32 m_ExcludedClues = 0;

	// Cached ComputeRating result for GetRating; negative until computed
	float m_Rating = -1.f;

	// Set while trying out hypothetical boards (UHint::GetExplanation): contradictions are expected there, so they
	// aren't logged as errors and don't call DebugError (which resets the board)
	bool m_bHypothetical = false;

	bool IsClueExcluded(ECampaignLesson Lesson) const
	{
		return Lesson != ECampaignLesson::Given && (m_ExcludedClues & (1 << int32(Lesson))) != 0;
	}

	// Hints handed out by GenerateHint since Init/InitCampaign or the last Reset, and how many of those came from the
	// campaign lesson's clue type
	UPROPERTY(BlueprintReadOnly)
	int32 m_HintsUsed = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 m_LessonHintsUsed = 0;

	FRandomStream m_Rand;

public:
	UPuzzle() {}

	UFUNCTION(BlueprintCallable)
	void Init(int Seed, int Size, int Difficulty);

	// Campaign puzzle: only clue types from Lesson and earlier lessons, and it can't be solved without Lesson's clues.
	// Difficulty sets the givens and extra clues as usual. Returns false if no such puzzle was found (e.g. Between on 3x3).
	UFUNCTION(BlueprintCallable)
	bool InitCampaign(int Seed, int Size, int Difficulty, ECampaignLesson Lesson, bool bAllClueTypes = false);

	// False for lessons that can't be played at this size (Given is never playable)
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool IsLessonAvailable(ECampaignLesson Lesson, int Size);

	// Difficulty rating: solves the puzzle one deduction at a time, each time using the easiest clue that can make
	// progress, and adds up those clues' weights (GetClueRatingWeight). Harder clue types, more deductions and fewer
	// givens all raise it. The board, hint counts and clue use counts are left as they were.
	UFUNCTION(BlueprintCallable)
	FPuzzleRating ComputeRating();

	// ComputeRating's Rating, computed the first time it's asked for (free play computes it when the puzzle starts)
	UFUNCTION(BlueprintCallable)
	float GetRating();

	// Rating weight of one deduction made with a clue of this type. Changing the weights changes every rating:
	// bump UFreePlaySubsystem::RatingVersion so players' learned pace starts over.
	UFUNCTION(BlueprintPure)
	static float GetClueRatingWeight(ECampaignLesson Clue);

	// Campaign score for this puzzle: 3 for completing it, -1 if any hint was used, another -1 if any was on the lesson clue (1 to 3)
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int32 GetCampaignScore() const;

	// True if the puzzle can't be solved once every clue of Lesson is removed. Clues with a "not" component
	// are reduced to their positive part instead, so the "not" itself must be needed.
	UFUNCTION(BlueprintCallable)
	bool RequiresLesson(ECampaignLesson Lesson);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetNumGivenClues();

	// Range of Given clues a puzzle gets: easy exactly size-2, normal 1 to size-3 (at least 1), hard 0
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static void GetGivenRangeForDifficulty(int Size, int Difficulty, int& Min, int& Max);

	static void RandomDistribution(FRandomStream& Rand, TArray<int>& Rands);

	UFUNCTION(BlueprintCallable)
	void Reset();

	UFUNCTION(BlueprintCallable)
	void ResetRow(int Row);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsSolved();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsCompleted();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsCorrect(int Row, int Col);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int SolutionIcon(int Row, int Col);

	UFUNCTION(BlueprintCallable)
	void SetFinalIcon(int Row, int Col, int Icon);

	UFUNCTION(BlueprintCallable)
	void SetFinalIconWithClue(UClue* clue, int Row, int Col, int Icon);

	UFUNCTION(BlueprintCallable)
	void EliminateIcon(int Row, int Col, int Icon);

	UFUNCTION(BlueprintCallable)
	void EliminateIconWithClue(UClue* clue, int Row, int Col, int Icon);

	// The best hint from all the clues, hidden or not; on a tie, one of the clues on screen (VisibleClues) wins.
	// Counts as a hint used.
	UFUNCTION(BlueprintCallable)
	UHint* GenerateHint(const TArray<UClue*>& VisibleClues);

	// The clue GenerateHint would pick, without handing out a hint. Lets the UI unhide it first.
	UFUNCTION(BlueprintCallable)
	UClue* GetHintClue(const TArray<UClue*>& VisibleClues);

	const TArray<UClue*>& HorizontalClues() const { return m_HorizontalClues; }
	const TArray<UClue*>& VerticalClues() const { return m_VeritcalClues; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FPuzzleCell GetCell(int Row, int Col) { return m_Rows[Row].m_Cells[Col]; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FString FormatTimeString(float Seconds) const;

	UFUNCTION(BlueprintCallable)
	void FixPuzzle();

private:
	UHint* FindBestHint(const TArray<UClue*>& VisibleClues, UClue*& OutClue);



	// Build a fresh solution and clue set from m_Rand (shared by Init and InitCampaign)
	void Generate();

	void GenerateSolution();
	void GenerateClues();

	bool ValidateClue(UClue& C);
	bool IsDuplicateClue(UClue& C);

	void AnalyzeAllClues();

	void OptimizeClues();

	// Sort clues easiest campaign lesson first
	static void SortCluesByLesson(TArray<UClue*>& Clues);

	// Reset the board and solve using only SortedClues, trying easier clues first each pass
	bool IsSolvableWith(const TArray<UClue*>& SortedClues);
	void BuildClueLists();
	void ScrambleClues();

	void ApplyAllGiven();

	void ReEnforceFinalIcons();

	void DumpPuzzle();
	void DumpSolution();
	void DumpClues();

	void DebugError();

	void SetMarker();
	void RestoreMarker();
};