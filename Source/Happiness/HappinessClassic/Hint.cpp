#include "Hint.h"
#include "Puzzle.h"
#include "PuzzleRow.h"
#include "Clue.h"

bool UHint::Init(UPuzzle& P, UClue& C)
{
	TheClue = &C;
	if (!C.GetHintAction(P, bSetFinalIcon, Row, Col, Icon))
	{
		return false;
	}

	return true;
}

bool UHint::ShouldHide(UPuzzle& P) const
{
	const auto& Cell = P.m_Rows[Row].m_Cells[Col];

	if ((Cell.m_iFinalIcon == Icon && bSetFinalIcon) ||
		(!Cell.m_bValues[Icon] && !bSetFinalIcon))
	{
		return true;
	}

	return false;
}

bool UHint::ShouldDraw(int InRow, int InCol, int InIcon) const
{
	return (InRow == Row && InCol == Col && InIcon == Icon);
}

bool UHint::ShouldDraw(const UClue& C) const
{
	const UClue* Ptr = &C;
	return (Ptr == TheClue);
}