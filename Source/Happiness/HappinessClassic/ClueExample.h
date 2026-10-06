#pragma once

#include "CoreMinimal.h"

class UClue;
class UPuzzle;

/** One icon a clue talks about: its row, icon, and the column it has in the solution */
struct FClueExamplePiece
{
	int32 Row = 0;
	int32 Icon = 0;
	int32 SolutionColumn = 0;
	/** Shown crossed out (or as a decoy) in the clue */
	bool bCrossed = false;
};

/**
 * What a clue means, as pieces and a rule, for showing it on a small board (the game rules): the pieces in their
 * solution columns always satisfy the rule; moving one to a column that breaks the rule shows what the clue forbids.
 *
 * Tested over generated puzzles by the puzzle commandlet (-checkexamples).
 */
struct HAPPINESS_API FClueExample
{
	TArray<FClueExamplePiece> Pieces;
	int32 Size = 0;

	/** False for clues this can't show (Given, NotHere) */
	bool Build(const UClue& Clue, const UPuzzle& Puzzle);

	/** Do the pieces in these columns (one per piece) satisfy the clue? */
	bool Holds(const TArray<int32>& Columns) const;

	/**
	 * An arrangement the clue forbids: the solution columns with one piece moved (the nearest move that breaks the
	 * clue, preferring the last pieces), or failing that any arrangement that breaks it (OutMovedPiece: the last piece
	 * not in its solution column). False if there is none.
	 */
	bool FindBroken(TArray<int32>& OutColumns, int32& OutMovedPiece) const;

	TArray<int32> SolutionColumns() const;

private:
	enum class EKind : uint8
	{
		SameColumn,		// every piece in one column
		NotSameColumn,	// piece 1 not in piece 0's column
		TwoTogetherOneNot,	// pieces Together[] share a column, piece Odd is elsewhere
		EitherOr,		// piece 0 shares a column with exactly one of pieces 1 and 2
		NextTo,
		NotNextTo,
		LeftOf,
		NotLeftOf,
		Span,			// piece 1 directly between pieces 0 and 2
		SpanNotSide,	// pieces Middle and Beside adjacent; piece Odd not on Middle's other side
		SpanNotMid,		// pieces 0 and 2 one column apart; piece 1 not between them
		Edge,
		NotEdge,
		DirectlyLeftOf,
		Gap,
		Between,
		Chain,
		NextToEitherOr,	// piece 1 next to exactly one of pieces 0 and 2
		AllApart,
	};

	EKind Kind = EKind::SameColumn;
	int32 Odd = 0;
	int32 Middle = 0;
	int32 Beside = 0;

	void Add(const UPuzzle& Puzzle, int32 Row, int32 Icon, bool bCrossed = false);
	void AddSolved(const UPuzzle& Puzzle, int32 Row, int32 Column);
};
