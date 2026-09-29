#pragma once

#include <CoreMinimal.h>
#include <UObject/Object.h>
#include "Clue.h"
#include "PuzzleRow.h"
#include "Hint.h"
#include "Puzzle.generated.h"

class UPuzzle;
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPuzzleHintUsed, UPuzzle*);

UCLASS(BlueprintType)
class HAPPINESS_API UPuzzle : public UObject
{
	GENERATED_BODY()

public:
	// Fired by GenerateHint whenever it hands out a hint (the campaign saves hint counts for resume)
	static FOnPuzzleHintUsed OnHintUsed;


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

	// Hints handed out by GenerateHint since Init/InitCampaign, and how many of those came from the campaign lesson's clue type
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
	bool InitCampaign(int Seed, int Size, int Difficulty, ECampaignLesson Lesson);

	// False for lessons that can't be played at this size (Given is never playable)
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool IsLessonAvailable(ECampaignLesson Lesson, int Size);

	// Campaign score for this puzzle: 3 with no hints, 2 if hints were used but none from the lesson clue, 1 otherwise
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

	UFUNCTION(BlueprintCallable)
	UHint* GenerateHint(const TArray<UClue*>& VisibleClues);

	const TArray<UClue*>& HorizontalClues() const { return m_HorizontalClues; }
	const TArray<UClue*>& VerticalClues() const { return m_VeritcalClues; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FPuzzleCell GetCell(int Row, int Col) { return m_Rows[Row].m_Cells[Col]; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FString FormatTimeString(float Seconds) const;

	UFUNCTION(BlueprintCallable)
	void FixPuzzle();

private:

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