#pragma once

#include <CoreMinimal.h>
#include <UObject/Object.h>
#include "Clue.h"
#include "PuzzleRow.h"
#include "Hint.h"
#include "Puzzle.generated.h"

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

	FRandomStream m_Rand;

public:
	UPuzzle() {}

	UFUNCTION(BlueprintCallable)
	void Init(int Seed, int Size, int Difficulty);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetNumGivenClues();

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

	void GenerateSolution();
	void GenerateClues();

	bool ValidateClue(UClue& C);
	bool IsDuplicateClue(UClue& C);

	void AnalyzeAllClues();

	void OptimizeClues();
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