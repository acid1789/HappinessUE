#pragma once

#include <CoreMinimal.h>
#include "Clue.h"
#include "PuzzleRow.h"
#include "Hint.h"

class Puzzle
{
public:

	int m_iSeed;
	int m_iSize;
	int m_iDifficulty;

	TArray<TArray<int>> m_Solution;

	TArray<PuzzleRow> m_Rows;
	TArray<PuzzleRow> m_MarkerRows;

	FRandomStream m_Rand;

	TArray<Clue*> m_Clues;
	TArray<Clue*> m_GivenClues;
	TArray<Clue*> m_VeritcalClues;
	TArray<Clue*> m_HorizontalClues;

public:

	Puzzle(int Seed, int Size, int Difficulty);

	int GetNumGivenClues();

	static void RandomDistribution(FRandomStream& Rand, TArray<int>& Rands);

	void Reset();
	void ResetRow(int Row);

	bool IsSolved();
	bool IsCompleted();

	bool IsCorrect(int Row, int Col);
	int SolutionIcon(int Row, int Col);

	void SetFinalIcon(int Row, int Col, int Icon);
	void SetFinalIcon(Clue* Clue, int Row, int Col, int Icon);

	void EliminateIcon(int Row, int Col, int Icon);
	void EliminateIcon(Clue* Clue, int Row, int Col, int Icon);

	Hint* GenerateHint(TArray<Clue*>& VisibleClues);

	const TArray<Clue*>& HorizontalClues() const { return m_HorizontalClues; }
	const TArray<Clue*>& VerticalClues() const { return m_VeritcalClues; }

private:

	void GenerateSolution();
	void GenerateClues();

	bool ValidateClue(Clue& C);
	bool IsDuplicateClue(Clue& C);

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