#pragma once

#include "CoreMinimal.h"
#include "CampaignLesson.generated.h"

/**
 * Campaign mode teaches one clue concept per lesson, in this order (easiest first).
 * Puzzles for a lesson should require that lesson's clue type and use no clue types from later lessons.
 *
 * Ordering rationale: positive before negative, two icons before three, exact/adjacent positions
 * before ranges, direct statements before either/or case analysis. Spans come early because they are
 * two NextTo relationships sharing a middle icon; LeftOf/NotLeftOf come late because "anywhere left of"
 * requires reasoning about ranges across the whole row.
 */
UENUM(BlueprintType)
enum class ECampaignLesson : uint8
{
	Given,			// Given: the board itself - one of each icon per row, a cell with one icon left is solved
					//   also NotHere: icon A is not in this cell - a negative Given, applied up front like Given
	Edge,			// Horizontal Edge / NotEdge: A is in an end column / A is not in an end column
	VerticalTwo,	// Vertical Two: A is in the same column as B
	VerticalThree,	// Vertical Three: A, B and C are in the same column
	NextTo,			// Horizontal NextTo: A is directly beside B (either side, edges force placement)
	DirectlyLeftOf,	// Horizontal DirectlyLeftOf: A is immediately left of B - NextTo with a fixed direction
	Span,			// Horizontal Span: B is in the middle, A on one side, C on the other
	TwoNot,			// Vertical TwoNot: A is not in the same column as B - first negative
	ThreeNot,		// Vertical ThreeTopNot / ThreeMidNot / ThreeBotNot: two share a column, the third does not
	AllApart,		// Horizontal AllApart: A, B and C are all in different columns (weak clue - may be cut)
	NotNextTo,		// Horizontal NotNextTo: A is not directly beside B
	SpanNotSide,	// Horizontal SpanNotLeft / SpanNotRight: B has A beside it on one side and not C on the other
	Gap,			// Horizontal Gap: A and C have exactly one column between them (middle unspecified)
	SpanNotMid,		// Horizontal SpanNotMid: A and C have one column between them that does not contain B
	LeftOf,			// Horizontal LeftOf: A is somewhere left of B - range reasoning
	Between,		// Horizontal Between: A is somewhere between B and C - range version of Span
	NotLeftOf,		// Horizontal NotLeftOf: A is not anywhere left of B - negated range
	Chain,			// Horizontal Chain: A is left of B, and B is left of C
	EitherOr,		// Vertical EitherOr: A is in the column with B or the column with C - case analysis
	NextToEitherOr,	// Horizontal NextToEitherOr: A is next to B or next to C, but not both
};
