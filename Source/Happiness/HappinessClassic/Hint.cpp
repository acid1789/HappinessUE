#include "Hint.h"
#include "Puzzle.h"
#include "PuzzleRow.h"
#include "Clue.h"

bool Hint::Init(Puzzle& P, Clue& C)
{
	TheClue = &C;
	if (!C.GetHintAction(P, bSetFinalIcon, Row, Col, Icon))
	{
		return false;
	}

	return true;
}

bool Hint::ShouldHide(Puzzle& P) const
{
	const auto& Cell = P.m_Rows[Row].m_Cells[Col];

	if ((Cell.m_iFinalIcon == Icon && bSetFinalIcon) ||
		(!Cell.m_bValues[Icon] && !bSetFinalIcon))
	{
		return true;
	}

	return false;
}

bool Hint::ShouldDraw(int InRow, int InCol, int InIcon) const
{
	return (InRow == Row && InCol == Col && InIcon == Icon);
}

bool Hint::ShouldDraw(const Clue& C) const
{
	const Clue* Ptr = &C;
	return (Ptr == TheClue);
}