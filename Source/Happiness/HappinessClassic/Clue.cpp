#include "Clue.h"
#include "CampaignTree.h"
#include "Puzzle.h"
#include "PuzzleRow.h"
#include "PuzzleCell.h"
#include "Algo/Sort.h"

#pragma optimize("", off)

void UClue::Init(UPuzzle& P, FRandomStream& Rand)
{
	if (!P.m_bCampaign)
	{
		GenerateClue(P, Rand);
		return;
	}

	// Campaign: only clue types from the current lesson or earlier. Half the time insist on the current
	// lesson itself so it shows up plenty; if that type can't be generated here, fall back to any allowed type.
	const bool bWantLesson = Rand.FRand() < 0.5f;
	for (int iTries = 0; iTries < 2000; iTries++)
	{
		GenerateClue(P, Rand);

		const ECampaignLesson Lesson = GetCampaignLesson();
		if (Lesson == P.m_CampaignLesson || (UCampaignTree::IsClueLessonAllowed(Lesson, P.m_CampaignLesson) && (!bWantLesson || iTries >= 1000)))
			return;
	}

	// Nothing allowed could be generated; a NotHere is always within any lesson
	m_Type = eClueType::NotHere;
	GenerateNotHere(P, Rand);
	GenerateClueHelp(P);
}

void UClue::GenerateClue(UPuzzle& P, FRandomStream& Rand)
{
	PickClueType(P, Rand);

	switch (m_Type)
	{
	case eClueType::Given:
		GenerateGiven(P, Rand);
		break;

	case eClueType::NotHere:
		GenerateNotHere(P, Rand);
		break;

	case eClueType::Vertical:
		GenerateVertical(P, Rand);
		break;

	case eClueType::Horizontal:
		GenerateHorizontal(P, Rand);
		break;
	}

	GenerateClueHelp(P);
}

void UClue::PickClueType(UPuzzle& P, FRandomStream& Rand)
{
	float Val = Rand.FRand();

	// Givens are placed up front by UPuzzle::GenerateClues, never picked here
	if (Val < 0.03f)
		m_Type = eClueType::NotHere;
	else if (Val < 0.35f)
		m_Type = eClueType::Vertical;
	else
		m_Type = eClueType::Horizontal;
}

void UClue::GenerateGiven(UPuzzle& P, FRandomStream& Rand)
{
	bool bGood = false;

	while (!bGood)
	{
		m_iRow = Rand.RandRange(0, P.m_iSize - 1);
		m_iCol = Rand.RandRange(0, P.m_iSize - 1);

		if (P.m_Rows[m_iRow].m_Cells[m_iCol].m_iFinalIcon < 0)
			bGood = true;
	}
}

void UClue::GenerateVertical(UPuzzle& P, FRandomStream& Rand)
{	
	// Pick a random column that isnt complete
	bool bGoodColumn = false;

	while (!bGoodColumn)
	{
		m_iCol = Rand.RandRange(0, P.m_iSize - 1);

		for (int i = 0; i < P.m_iSize; i++)
		{
			if (P.m_Rows[i].m_Cells[m_iCol].m_iFinalIcon < 0)
			{
				// This has a cell that isnt completed yet
				bGoodColumn = true;
				break;
			}
		}
	}

	int iSize = P.m_iSize;
	int Type = Rand.RandRange(0, (int)eVerticalType::ThreeBotNot);
	m_VerticalType = (eVerticalType)Type;
	switch (m_VerticalType)
	{
		case eVerticalType::Two: GenerateTwoRowColumn(P, Rand); break;
		case eVerticalType::Three: GenerateThreeRowColumn(P, Rand); break;
		case eVerticalType::EitherOr:
		{
			int iTries = 0;
			bool bGoodRows = false;

			while (!bGoodRows)
			{
				iTries++;

				// Pick 3 random rows
				m_iRow3 = m_iRow2 = m_iRow = Rand.RandRange(0, iSize - 1);

				while (m_iRow2 == m_iRow)
					m_iRow2 = Rand.RandRange(0, iSize - 1);

				while (m_iRow3 == m_iRow2 || m_iRow3 == m_iRow)
					m_iRow3 = Rand.RandRange(0, iSize - 1);

				int iTemp1 = m_iRow;
				int iTemp2 = m_iRow2;
				int iTemp3 = m_iRow3;

				m_iRow = FMath::Min(iTemp1, FMath::Min(iTemp2, iTemp3));
				m_iRow3 = FMath::Max(iTemp1, FMath::Max(iTemp2, iTemp3));

				m_iRow2 =
					(iTemp1 != m_iRow && iTemp1 != m_iRow3) ? iTemp1 :
					(iTemp2 != m_iRow && iTemp2 != m_iRow3) ? iTemp2 :
					iTemp3;

				// Pick one to be the not
				float dfVal = Rand.FRand();
				m_iNotCell = (dfVal <= 0.5) ? m_iRow2 : m_iRow3;

				// Pick an icon to show as the not
				while (true)
				{
					m_iHorizontal1 = Rand.RandRange(0, iSize - 1);

					if (P.m_Solution[(m_iNotCell * iSize) + m_iCol] != m_iHorizontal1)
						break;
				}

				// Make sure the either or isnt both already
				bGoodRows = (P.m_Rows[m_iRow2].m_Cells[m_iCol].m_iFinalIcon < 0 ||	P.m_Rows[m_iRow3].m_Cells[m_iCol].m_iFinalIcon < 0);

				if (iTries > 5)
				{
					GenerateClue(P, Rand);
					return;
				}
			}
			break;
		}
		case eVerticalType::TwoNot:
		{
			GenerateTwoRowColumn(P, Rand);
			bool bGoodCell = false;
			while (!bGoodCell)
			{
				m_iNotCell = Rand.RandRange(0, iSize - 1);
				bGoodCell = (m_iNotCell != P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol]);
			}
			break;
		}
		case eVerticalType::ThreeTopNot:
		{
			GenerateThreeRowColumn(P, Rand);
			bool bGoodCell = false;
			while (!bGoodCell)
			{
				m_iNotCell = Rand.RandRange(0, iSize - 1);
				bGoodCell = (m_iNotCell != P.m_Solution[(m_iRow * P.m_iSize) + m_iCol]);
			}
			break;
		}
		case eVerticalType::ThreeMidNot:
		{
			GenerateThreeRowColumn(P, Rand);
			bool bGoodCell = false;
			while (!bGoodCell)
			{
				m_iNotCell = Rand.RandRange(0, iSize - 1);
				bGoodCell = (m_iNotCell != P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol]);
			}
			break;
		}
		case eVerticalType::ThreeBotNot:
		{
			GenerateThreeRowColumn(P, Rand);
			bool bGoodCell = false;
			while (!bGoodCell)
			{
				m_iNotCell = Rand.RandRange(0, iSize - 1);
				bGoodCell = (m_iNotCell != P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol]);
			}
			break;
		}
	}
}

void UClue::GenerateTwoRowColumn(UPuzzle& P, FRandomStream& Rand)
{
	bool bGoodRows = false;

	int iSize = P.m_iSize;
	while (!bGoodRows)
	{
		// Pick 2 random rows
		m_iRow2 = m_iRow = Rand.RandRange(0, iSize - 1);

		while (m_iRow2 == m_iRow)
		{
			m_iRow2 = Rand.RandRange(0, iSize - 1);
		}

		// Make sure one of the rows is at least useful
		bGoodRows = (P.m_Rows[m_iRow].m_Cells[m_iCol].m_iFinalIcon < 0 ||
					P.m_Rows[m_iRow2].m_Cells[m_iCol].m_iFinalIcon < 0);
	}

	int iTemp1 = m_iRow;
	int iTemp2 = m_iRow2;

	m_iRow = FMath::Min(iTemp1, iTemp2);
	m_iRow2 = FMath::Max(iTemp1, iTemp2);
}

void UClue::GenerateThreeRowColumn(UPuzzle& P, FRandomStream& Rand)
{
	bool bGoodRows = false;
	int iSize = P.m_iSize;
	while (!bGoodRows)
	{
		// Pick 3 random rows
		m_iRow = Rand.RandRange(0, iSize - 1);
		m_iRow2 = Rand.RandRange(0, iSize - 1);
		m_iRow3 = Rand.RandRange(0, iSize - 1);
		while (m_iRow2 == m_iRow)
		{
			m_iRow2 = Rand.RandRange(0, iSize - 1);
		}
		while (m_iRow3 == m_iRow2 || m_iRow3 == m_iRow)
		{
			m_iRow3 = Rand.RandRange(0, iSize - 1);
		}

		// Make sure one of the rows is at least useful
		bGoodRows = (P.m_Rows[m_iRow].m_Cells[m_iCol].m_iFinalIcon < 0 ||
					P.m_Rows[m_iRow2].m_Cells[m_iCol].m_iFinalIcon < 0 ||
					P.m_Rows[m_iRow3].m_Cells[m_iCol].m_iFinalIcon < 0);
	}

	int iTemp1 = m_iRow;
	int iTemp2 = m_iRow2;
	int iTemp3 = m_iRow3;
	m_iRow = FMath::Min(iTemp1, FMath::Min(iTemp2, iTemp3));
	m_iRow3 = FMath::Max(iTemp1, FMath::Max(iTemp2, iTemp3));
	m_iRow2 = (iTemp1 != m_iRow && iTemp1 != m_iRow3) ? iTemp1 : (iTemp2 != m_iRow && iTemp2 != m_iRow3) ? iTemp2 : iTemp3;
}

void UClue::GenerateHorizontal(UPuzzle& P, FRandomStream& Rand)
{
	int iSize = P.m_iSize;
	int Type = Rand.RandRange(0, (int)eHorizontalType::AllApart);

	m_HorizontalType = (eHorizontalType)Type;
	switch (m_HorizontalType)
	{
		case eHorizontalType::Edge:
		case eHorizontalType::NotEdge:
		case eHorizontalType::DirectlyLeftOf:
		case eHorizontalType::Gap:
		case eHorizontalType::Between:
		case eHorizontalType::Chain:
		case eHorizontalType::NextToEitherOr:
		case eHorizontalType::AllApart:
		{
			if (!GenerateHorizontalExtended(P, Rand))
				GenerateClue(P, Rand);
			return;
		}

		case eHorizontalType::NextTo:
		{
			while (true)
			{
				// Pick first icon randomly
				m_iRow = Rand.RandRange(0, iSize - 1);
				m_iCol = Rand.RandRange(0, iSize - 1);
				// Pick neighboring column
				if (m_iCol == 0)
					m_iCol2 = 1;
				else if (m_iCol == (P.m_iSize - 1))
					m_iCol2 = m_iCol - 1;
				else
				{
					float dfVal = Rand.FRand();
					if (dfVal <= 0.5f)
						m_iCol2 = m_iCol - 1;
					else
						m_iCol2 = m_iCol + 1;
				}
				// Pick a neighboring row
				m_iRow2 = Rand.RandRange(0, iSize - 1);
				// Make sure this clue is useful
				if (P.m_Rows[m_iRow].m_Cells[m_iCol].m_iFinalIcon < 0 || P.m_Rows[m_iRow2].m_Cells[m_iCol2].m_iFinalIcon < 0)
					break;
			}
			m_iRow3 = m_iRow;
			break;
		}
		case eHorizontalType::NotNextTo:
		{
			// Pick first icon randomly
			m_iRow = Rand.RandRange(0, iSize - 1);
			m_iCol = Rand.RandRange(0, iSize - 1);
			// Pick a neighboring row
			m_iRow2 = Rand.RandRange(0, iSize - 1);
			// Pick an icon that is not in m_iRow2 on either side of m_iCol
			int iTries = 0;
			while (true)
			{
				if (iTries++ > 25)
				{
					GenerateHorizontal(P, Rand);
					return;
				}
				// Pick one randomly
				m_iHorizontal1 = Rand.RandRange(0, iSize - 1);
				// Make sure its not the same as the first icon
				if (m_iRow2 == m_iRow && P.m_Solution[(m_iRow * P.m_iSize) + m_iCol] == m_iHorizontal1)
					continue;
				// Make sure its not on the left of the first column
				if (m_iCol > 0 && P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol - 1] == m_iHorizontal1)
					continue;
				// Make sure its not on the right of the first column
				if (m_iCol < (P.m_iSize - 1) && P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol + 1] == m_iHorizontal1)
					continue;
				// This one looks fine
				break;			
			}
			m_iRow3 = m_iRow;
			break;
		}
		case eHorizontalType::LeftOf:
		{
			while (true)
			{
				// Pick first icon randomly
				m_iCol = Rand.RandRange(0, iSize - 2);
				m_iRow = Rand.RandRange(0, iSize - 1);
				// Pick second icon
				m_iCol2 = Rand.RandRange(m_iCol + 1, iSize - 1);
				m_iRow2 = Rand.RandRange(0, iSize - 1);
				// Make sure this is useful
				if (P.m_Rows[m_iRow].m_Cells[m_iCol].m_iFinalIcon < 0 || P.m_Rows[m_iRow2].m_Cells[m_iCol2].m_iFinalIcon < 0)
					break;
			}
			break;
		}
		case eHorizontalType::NotLeftOf:
		{
			int iTries = 0;
			while (true)
			{
				// Pick first icon randomly
				m_iCol = Rand.RandRange(1, iSize - 1);
				m_iRow = Rand.RandRange(0, iSize - 1);
				// Pick second icon
				m_iCol2 = Rand.RandRange(0, m_iCol - 1);
				m_iRow2 = Rand.RandRange(0, iSize - 1);
				// Make sure this is useful
				if (P.m_Rows[m_iRow].m_Cells[m_iCol].m_iFinalIcon < 0 || P.m_Rows[m_iRow2].m_Cells[m_iCol2].m_iFinalIcon < 0)
					break;
				iTries++;
				if (iTries > 5)
				{
					GenerateClue(P, Rand);
					return;
				}
			}
			break;
		}
		case eHorizontalType::Span:
		{
			GenerateHorizontalSpan(P, Rand);
			break;
		}
		case eHorizontalType::SpanNotLeft:
		{
			GenerateHorizontalSpan(P, Rand);
			// Find an icon that is not the correct icon for m_iCol
			int iTries = 0;
			while (true)
			{
				if (iTries++ > 25)
				{
					GenerateHorizontal(P, Rand);
					return;
				}
				m_iHorizontal1 = Rand.RandRange(0, iSize - 1);
				if (P.m_Solution[(m_iRow * P.m_iSize) + m_iCol] == m_iHorizontal1)
					continue;
				if (P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol2] == m_iHorizontal1)
					continue;
				if (P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol3] == m_iHorizontal1)
					continue;
				break;
			}
			break;
		}
		case eHorizontalType::SpanNotMid:
		{
			GenerateHorizontalSpan(P, Rand);
			// Find an icon that is not the correct icon for m_iCol2
			int iTries = 0;
			while (true)
			{
				if (iTries++ > 25)
				{
					GenerateHorizontal(P, Rand);
					return;
				}
				m_iHorizontal1 = Rand.RandRange(0, iSize - 1);
				if (P.m_Solution[(m_iRow * P.m_iSize) + m_iCol] == m_iHorizontal1)
					continue;
				if (P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol2] == m_iHorizontal1)
					continue;
				if (P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol3] == m_iHorizontal1)
					continue;
				break;
			}
			break;
		}
		case eHorizontalType::SpanNotRight:
		{
			GenerateHorizontalSpan(P, Rand);
			// Find an icon that is not the correct icon for m_iCol3
			int iTries = 0;
			while (true)
			{
				if (iTries++ > 25)
				{
					GenerateHorizontal(P, Rand);
					return;
				}
				m_iHorizontal1 = Rand.RandRange(0, iSize - 1);
				if (P.m_Solution[(m_iRow * P.m_iSize) + m_iCol] == m_iHorizontal1)
					continue;
				if (P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol2] == m_iHorizontal1)
					continue;
				if (P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol3] == m_iHorizontal1)
					continue;
				break;
			}
			break;
		}
	}
}

void UClue::GenerateHorizontalSpan(UPuzzle& P, FRandomStream& Rand)
{
	int iSize = P.m_iSize;
	while (true)
	{
		// Pick the first icon
		m_iCol = Rand.RandRange(0, iSize - 1);

		// Pick the second & third columns
		if (m_iCol + 2 < iSize)
		{
			m_iCol2 = m_iCol + 1;
			m_iCol3 = m_iCol2 + 1;
		}
		else if (m_iCol - 2 >= 0)
		{
			m_iCol2 = m_iCol - 1;
			m_iCol3 = m_iCol2 - 1;
		}
		else
			continue;

		// Generate the rows randomly
		m_iRow = Rand.RandRange(0, iSize - 1);
		m_iRow2 = Rand.RandRange(0, iSize - 1);
		m_iRow3 = Rand.RandRange(0, iSize - 1);

		// Make sure the clue is useful
		if (P.m_Rows[m_iRow].m_Cells[m_iCol].m_iFinalIcon < 0 ||
			P.m_Rows[m_iRow2].m_Cells[m_iCol2].m_iFinalIcon < 0 ||
			P.m_Rows[m_iRow3].m_Cells[m_iCol3].m_iFinalIcon < 0)
			break;
	}
}

void UClue::Analyze(UPuzzle& P)
{
	switch (m_Type)
	{
	case eClueType::Given:
		AnalyzeGiven(P);
		break;

	case eClueType::NotHere:
		AnalyzeNotHere(P);
		break;

	case eClueType::Vertical:
		AnalyzeVertical(P);
		break;

	case eClueType::Horizontal:
		AnalyzeHorizontal(P);
		break;
	}
}

void UClue::AnalyzeGiven(UPuzzle& P)
{
	int Icon = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];

	P.SetFinalIconWithClue(this, m_iRow, m_iCol, Icon);

	m_iUseCount += 25;
}

void UClue::AnalyzeVertical(UPuzzle& P)
{
	switch (m_VerticalType)
	{
	case eVerticalType::Two:
		AnalyzeVerticalTwo(P);
		break;

	case eVerticalType::Three:
		AnalyzeVerticalThree(P);
		break;

	case eVerticalType::EitherOr:
		AnalyzeVerticalEitherOr(P);
		break;

	case eVerticalType::TwoNot:
		AnalyzeVerticalTwoNot(P);
		break;

	case eVerticalType::ThreeTopNot:
		AnalyzeVerticalThreeTopNot(P);
		break;

	case eVerticalType::ThreeMidNot:
		AnalyzeVerticalThreeMidNot(P);
		break;

	case eVerticalType::ThreeBotNot:
		AnalyzeVerticalThreeBotNot(P);
		break;
	}
}

void UClue::AnalyzeHorizontal(UPuzzle& P)
{
	switch (m_HorizontalType)
	{
	case eHorizontalType::NextTo:
		AnalyzeHorizontalNextTo(P);
		break;

	case eHorizontalType::NotNextTo:
		AnalyzeHorizontalNotNextTo(P);
		break;

	case eHorizontalType::LeftOf:
		AnalyzeHorizontalLeftOf(P);
		break;

	case eHorizontalType::NotLeftOf:
		AnalyzeHorizontalNotLeftOf(P);
		break;

	case eHorizontalType::Span:
		AnalyzeHorizontalSpan(P);
		break;

	case eHorizontalType::SpanNotLeft:
		AnalyzeHorizontalSpanNotLeft(P);
		break;

	case eHorizontalType::SpanNotMid:
		AnalyzeHorizontalSpanNotMid(P);
		break;

	case eHorizontalType::SpanNotRight:
		AnalyzeHorizontalSpanNotRight(P);
		break;

	case eHorizontalType::Edge:
	case eHorizontalType::NotEdge:
	case eHorizontalType::DirectlyLeftOf:
	case eHorizontalType::Gap:
	case eHorizontalType::Between:
	case eHorizontalType::Chain:
	case eHorizontalType::NextToEitherOr:
	case eHorizontalType::AllApart:
		AnalyzeConstraint(P);
		break;
	}
}

void UClue::AnalyzeVerticalTwo(UPuzzle& P)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol];
	for (int i = 0; i < P.m_iSize; i++)
	{
		if (!P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
		{
			P.EliminateIconWithClue(this, m_iRow2, i, iIcon2);
		}
		else if (!P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
		{
			P.EliminateIconWithClue(this, m_iRow, i, iIcon1);
		}

		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			P.SetFinalIconWithClue(this, m_iRow2, i, iIcon2);
			return;
		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			P.SetFinalIconWithClue(this, m_iRow, i, iIcon1);
			return;
		}
	}
}

void UClue::AnalyzeVerticalThree(UPuzzle& P)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol];
	int iIcon3 = P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol];
	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			P.SetFinalIconWithClue(this, m_iRow2, i, iIcon2);
			P.SetFinalIconWithClue(this, m_iRow3, i, iIcon3);
			return;
		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			P.SetFinalIconWithClue(this, m_iRow, i, iIcon1);
			P.SetFinalIconWithClue(this, m_iRow3, i, iIcon3);
			return;
		}
		else if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon == iIcon3)
		{
			P.SetFinalIconWithClue(this, m_iRow, i, iIcon1);
			P.SetFinalIconWithClue(this, m_iRow2, i, iIcon2);
			return;
		}

		if (!P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
		{
			P.EliminateIconWithClue(this, m_iRow2, i, iIcon2);
			P.EliminateIconWithClue(this, m_iRow3, i, iIcon3);
		}
		else if (!P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
		{
			P.EliminateIconWithClue(this, m_iRow, i, iIcon1);
			P.EliminateIconWithClue(this, m_iRow3, i, iIcon3);
		}
		else if (!P.m_Rows[m_iRow3].m_Cells[i].m_bValues[iIcon3])
		{
			P.EliminateIconWithClue(this, m_iRow, i, iIcon1);
			P.EliminateIconWithClue(this, m_iRow2, i, iIcon2);
		}
	}
}

void UClue::AnalyzeVerticalEitherOr(UPuzzle& P)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = (m_iRow2 == m_iNotCell) ? m_iHorizontal1 : P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol];
	int iIcon3 = (m_iRow3 == m_iNotCell) ? m_iHorizontal1 : P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol];

	int iSize = P.m_iSize;
	int iIcon2Col = -1;
	int iIcon3Col = -1;
	for (int i = 0; i < iSize; i++)
	{
		if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			P.EliminateIconWithClue(this, m_iRow3, i, iIcon3);
			iIcon2Col = i;
		}
		if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon == iIcon3)
		{
			P.EliminateIconWithClue(this, m_iRow2, i, iIcon2);
			iIcon3Col = i;
		}
		if (!P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2] &&
			!P.m_Rows[m_iRow3].m_Cells[i].m_bValues[iIcon3])
		{
			// If neither icon is in this column, icon1 cant be here either
			P.EliminateIconWithClue(this, m_iRow, i, iIcon1);
		}

		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			for (int j = 0; j < iSize; j++)
			{
				if (j == i)
					continue;

				if (P.m_Rows[m_iRow2].m_Cells[j].m_iFinalIcon == iIcon2)
				{
					P.SetFinalIconWithClue(this, m_iRow3, i, iIcon3);
				}
				else if (P.m_Rows[m_iRow3].m_Cells[j].m_iFinalIcon == iIcon3)
				{
					P.SetFinalIconWithClue(this, m_iRow2, i, iIcon2);
				}
			}
		}
	}

	if (iIcon2Col >= 0 && iIcon3Col >= 0)
	{
		for (int i = 0; i < iSize; i++)
		{
			if (i != iIcon2Col && i != iIcon3Col)
			{
				P.EliminateIconWithClue(this, m_iRow, i, iIcon1);
			}
		}
	}
}

void UClue::AnalyzeVerticalTwoNot(UPuzzle& P)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = m_iNotCell;

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			P.EliminateIconWithClue(this, m_iRow2, i, iIcon2);

		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			P.EliminateIconWithClue(this, m_iRow, i, iIcon1);
		}
	}
}

void UClue::AnalyzeVerticalThreeTopNot(UPuzzle& P)
{
	int iIcon1 = m_iNotCell;
	int iIcon2 = P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol];
	int iIcon3 = P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol];

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			P.EliminateIconWithClue(this, m_iRow2, i, iIcon2);
			P.EliminateIconWithClue(this, m_iRow3, i, iIcon3);
		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			P.EliminateIconWithClue(this, m_iRow, i, iIcon1);
			P.SetFinalIconWithClue(this, m_iRow3, i, iIcon3);
			return;
		}
		else if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon == iIcon3)
		{
			P.EliminateIconWithClue(this, m_iRow, i, iIcon1);
			P.SetFinalIconWithClue(this, m_iRow2, i, iIcon2);
			return;
		}
		else if (!P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
		{
			P.EliminateIconWithClue(this, m_iRow3, i, iIcon3);
		}
		else if (!P.m_Rows[m_iRow3].m_Cells[i].m_bValues[iIcon3])
		{
			P.EliminateIconWithClue(this, m_iRow2, i, iIcon2);
		}
	}
}

void UClue::AnalyzeVerticalThreeMidNot(UPuzzle& P)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = m_iNotCell;
	int iIcon3 = P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol];

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			P.EliminateIconWithClue(this, m_iRow, i, iIcon1);
			P.EliminateIconWithClue(this, m_iRow3, i, iIcon3);

		}
		else if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			P.EliminateIconWithClue(this, m_iRow2, i, iIcon2);
			P.SetFinalIconWithClue(this, m_iRow3, i, iIcon3);
			return;
		}
		else if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon == iIcon3)
		{
			P.EliminateIconWithClue(this, m_iRow2, i, iIcon2);
			P.SetFinalIconWithClue(this, m_iRow, i, iIcon1);
			return;
		}
		else if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon >= 0)
		{
			P.EliminateIconWithClue(this, m_iRow3, i, iIcon3);
		}
		else if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon >= 0)
		{
			P.EliminateIconWithClue(this, m_iRow, i, iIcon1);
		}
	}
}

void UClue::AnalyzeVerticalThreeBotNot(UPuzzle& P)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol];
	int iIcon3 = m_iNotCell;

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon == iIcon3)
		{
			P.EliminateIconWithClue(this, m_iRow2, i, iIcon2);
			P.EliminateIconWithClue(this, m_iRow, i, iIcon1);

		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			P.EliminateIconWithClue(this, m_iRow3, i, iIcon3);
			P.SetFinalIconWithClue(this, m_iRow, i, iIcon1);
			return;
		}
		else if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			P.EliminateIconWithClue(this, m_iRow3, i, iIcon3);
			P.SetFinalIconWithClue(this, m_iRow2, i, iIcon2);
			return;
		}
		else if (!P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
		{
			P.EliminateIconWithClue(this, m_iRow, i, iIcon1);
		}
		else if (!P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
		{
			P.EliminateIconWithClue(this, m_iRow2, i, iIcon2);
		}
	}
}

void UClue::AnalyzeHorizontalNextTo(UPuzzle& P)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol2];

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			if (i == 0)
			{
				P.SetFinalIconWithClue(this, m_iRow2, 1, iIcon2);

			}
			else if (i == P.m_iSize - 1)
			{
				P.SetFinalIconWithClue(this, m_iRow2, i - 1, iIcon2);

			}
			else
			{
				for (int j = 0; j < P.m_iSize; j++)
				{
					if (j == (i - 1) || j == (i + 1))
						continue;
					P.EliminateIconWithClue(this, m_iRow2, j, iIcon2);

				}
			}
			break;
		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			if (i == 0)
			{
				P.SetFinalIconWithClue(this, m_iRow, 1, iIcon1);

			}
			else if (i == P.m_iSize - 1)
			{
				P.SetFinalIconWithClue(this, m_iRow, i - 1, iIcon1);

			}
			else
			{
				for (int j = 0; j < P.m_iSize; j++)
				{
					if (j == (i - 1) || j == (i + 1))
						continue;
					P.EliminateIconWithClue(this, m_iRow, j, iIcon1);

				}
			}
			break;
		}
		else
		{
			if (i == 0)
			{
				if (!P.m_Rows[m_iRow2].m_Cells[i + 1].m_bValues[iIcon2])
				{
					P.EliminateIconWithClue(this, m_iRow, i, iIcon1);

				}
				if (!P.m_Rows[m_iRow].m_Cells[i + 1].m_bValues[iIcon1])
				{
					P.EliminateIconWithClue(this, m_iRow2, i, iIcon2);

				}
			}
			else if (i == P.m_iSize - 1)
			{
				if (!P.m_Rows[m_iRow2].m_Cells[i - 1].m_bValues[iIcon2])
				{
					P.EliminateIconWithClue(this, m_iRow, i, iIcon1);

				}
				if (!P.m_Rows[m_iRow].m_Cells[i - 1].m_bValues[iIcon1])
				{
					P.EliminateIconWithClue(this, m_iRow2, i, iIcon2);

				}
			}
			else
			{
				if (!P.m_Rows[m_iRow2].m_Cells[i + 1].m_bValues[iIcon2] &&
					!P.m_Rows[m_iRow2].m_Cells[i - 1].m_bValues[iIcon2])
				{
					P.EliminateIconWithClue(this, m_iRow, i, iIcon1);

				}
				if (!P.m_Rows[m_iRow].m_Cells[i + 1].m_bValues[iIcon1] &&
					!P.m_Rows[m_iRow].m_Cells[i - 1].m_bValues[iIcon1])
				{
					P.EliminateIconWithClue(this, m_iRow2, i, iIcon2);

				}
			}
		}
	}
}

void UClue::AnalyzeHorizontalNotNextTo(UPuzzle& P)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = m_iHorizontal1;

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			if (i == 0)
			{
				P.EliminateIconWithClue(this, m_iRow2, i + 1, iIcon2);

			}
			else if (i == P.m_iSize - 1)
			{
				P.EliminateIconWithClue(this, m_iRow2, i - 1, iIcon2);

			}
			else
			{
				P.EliminateIconWithClue(this, m_iRow2, i - 1, iIcon2);
				P.EliminateIconWithClue(this, m_iRow2, i + 1, iIcon2);

			}
			break;
		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			if (i == 0)
			{
				P.EliminateIconWithClue(this, m_iRow, i + 1, iIcon1);

			}
			else if (i == P.m_iSize - 1)
			{
				P.EliminateIconWithClue(this, m_iRow, i - 1, iIcon1);

			}
			else
			{
				P.EliminateIconWithClue(this, m_iRow, i - 1, iIcon1);
				P.EliminateIconWithClue(this, m_iRow, i + 1, iIcon1);

			}
			break;
		}
	}
}

void UClue::AnalyzeHorizontalLeftOf(UPuzzle& P)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol2];

	int iFirstPossibleLeft = 0;
	for (iFirstPossibleLeft = 0; iFirstPossibleLeft < P.m_iSize; iFirstPossibleLeft++)
	{
		if (P.m_Rows[m_iRow].m_Cells[iFirstPossibleLeft].m_bValues[iIcon1])
			break;
	}

	int iFirstPossibleRight = P.m_iSize - 1;
	for (iFirstPossibleRight = P.m_iSize - 1; iFirstPossibleRight >= 0; iFirstPossibleRight--)
	{
		if (P.m_Rows[m_iRow2].m_Cells[iFirstPossibleRight].m_bValues[iIcon2])
			break;
	}

	if (iFirstPossibleLeft + 1 == iFirstPossibleRight)
	{
		// we have a solution for this clue
		P.SetFinalIconWithClue(this, m_iRow, iFirstPossibleLeft, iIcon1);
		P.SetFinalIconWithClue(this, m_iRow2, iFirstPossibleRight, iIcon2);
	}
	else
	{
		// Remove all icon2's from the left side of the first possible left
		for (int i = 0; i <= iFirstPossibleLeft; i++)
		{
			P.EliminateIconWithClue(this, m_iRow2, i, iIcon2);
		}

		// Remove all the icon1's from the right side of the first possible right
		for (int i = iFirstPossibleRight; i < P.m_iSize; i++)
		{
			P.EliminateIconWithClue(this, m_iRow, i, iIcon1);
		}
	}
}

void UClue::AnalyzeHorizontalNotLeftOf(UPuzzle& P)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol2];

	// If both icons are on the same row
	if (m_iRow == m_iRow2)
	{
		// Icon1 cant be in the zero column, because that would ensure that it is always left of Icon2
		P.EliminateIconWithClue(this, m_iRow, 0, iIcon1);

		// Icon2 cant be in the last column, because that would ensure that it is always right of Icon1
		P.EliminateIconWithClue(this, m_iRow2, P.m_iSize - 1, iIcon2);

	}

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			// Icon1 is known, remove all instances of icon2 to the right
			for (int j = i + 1; j < P.m_iSize; j++)
			{
				P.EliminateIconWithClue(this, m_iRow2, j, iIcon2);

			}
			break;
		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			// Icon2 is known, remove all instances of icon1 to the left
			for (int j = 0; j < i; j++)
			{
				P.EliminateIconWithClue(this, m_iRow, j, iIcon1);

			}
			break;
		}
	}
}

bool UClue::SolveSpan(int iCol, int iRow1, int iIcon1, bool bNot1, int iRow2, int iIcon2, bool bNot2, int iRow3, int iIcon3, bool bNot3, UPuzzle& P)
{
	int iFinal1 = P.m_Rows[iRow1].m_Cells[iCol].m_iFinalIcon;
	if (iFinal1 == iIcon1)
	{
		if (iCol < 2)
		{
			if (bNot1)
			{
				int iFinal2Right = P.m_Rows[iRow2].m_Cells[iCol + 1].m_iFinalIcon;
				if (iFinal2Right == iIcon2)
				{
					P.SetFinalIconWithClue(this, iRow3, iCol, iIcon3);

				}
			}
			else
			{
				if (bNot2)
				{
					P.EliminateIconWithClue(this, iRow2, iCol + 1, iIcon2);

				}
				else
				{
					P.SetFinalIconWithClue(this, iRow2, iCol + 1, iIcon2);

				}

				if (bNot3)
				{
					P.EliminateIconWithClue(this, iRow3, iCol + 2, iIcon3);

				}
				else
				{
					P.SetFinalIconWithClue(this, iRow3, iCol + 2, iIcon3);

				}
				return true;
			}
		}
		else if (iCol > P.m_iSize - 3)
		{
			if (bNot1)
			{
				int iFinal2Left = P.m_Rows[iRow2].m_Cells[iCol - 1].m_iFinalIcon;
				if (iFinal2Left == iIcon2)
				{
					P.SetFinalIconWithClue(this, iRow3, iCol, iIcon3);

				}
			}
			else
			{
				if (bNot2)
					P.EliminateIconWithClue(this, iRow2, iCol - 1, iIcon2);
				else
					P.SetFinalIconWithClue(this, iRow2, iCol - 1, iIcon2);

				if (bNot3)
					P.EliminateIconWithClue(this, iRow3, iCol - 2, iIcon3);
				else
					P.SetFinalIconWithClue(this, iRow3, iCol - 2, iIcon3);

				return true;
			}
		}
		else
		{
			int iFinal2Left = P.m_Rows[iRow2].m_Cells[iCol - 1].m_iFinalIcon;
			int iFinal2Right = P.m_Rows[iRow2].m_Cells[iCol + 1].m_iFinalIcon;
			if (iFinal2Left == iIcon2)
			{
				if (bNot1)
				{
					P.SetFinalIconWithClue(this, iRow3, iCol, iIcon3);

				}
				else if (bNot2)
				{
					P.SetFinalIconWithClue(this, iRow3, iCol + 2, iIcon3);

				}
				else if (bNot3)
				{
					P.EliminateIconWithClue(this, iRow3, iCol - 2, iIcon3);

				}
				else
				{
					P.SetFinalIconWithClue(this, iRow3, iCol - 2, iIcon3);

				}
				return true;
			}
			else if (iFinal2Right == iIcon2)
			{
				if (bNot1)
				{
					P.SetFinalIconWithClue(this, iRow3, iCol, iIcon3);

				}
				else if (bNot2)
				{
					P.SetFinalIconWithClue(this, iRow3, iCol - 2, iIcon3);

				}
				else if (bNot3)
				{
					P.EliminateIconWithClue(this, iRow3, iCol + 2, iIcon3);

				}
				else
				{
					P.SetFinalIconWithClue(this, iRow3, iCol + 2, iIcon3);

				}
				return true;
			}
			else if (!P.m_Rows[iRow2].m_Cells[iCol - 1].m_bValues[iIcon2] && !bNot1 && !bNot2)
			{
				P.SetFinalIconWithClue(this, iRow2, iCol + 1, iIcon2);
				if (bNot3)
					P.EliminateIconWithClue(this, iRow3, iCol + 2, iIcon3);
				else
					P.SetFinalIconWithClue(this, iRow3, iCol + 2, iIcon3);

				return true;
			}
			else if (!P.m_Rows[iRow2].m_Cells[iCol + 1].m_bValues[iIcon2] && !bNot1 && !bNot2)
			{
				P.SetFinalIconWithClue(this, iRow2, iCol - 1, iIcon2);
				if (bNot3)
					P.EliminateIconWithClue(this, iRow3, iCol - 2, iIcon3);
				else
					P.SetFinalIconWithClue(this, iRow3, iCol - 2, iIcon3);

				return true;
			}
			else if (!bNot3)
			{
				int iFinal3Left = P.m_Rows[iRow3].m_Cells[iCol - 2].m_iFinalIcon;
				int iFinal3Right = P.m_Rows[iRow3].m_Cells[iCol + 2].m_iFinalIcon;
				if (iFinal3Left == iIcon3)
				{
					if (bNot1)
					{
						P.SetFinalIconWithClue(this, iRow2, iCol - 3, iIcon2);

						return true;
					}
					else if (bNot2)
					{
						P.EliminateIconWithClue(this, iRow2, iCol - 1, iIcon2);

					}
					else
					{
						P.SetFinalIconWithClue(this, iRow2, iCol - 1, iIcon2);

						return true;
					}
				}
				else if (iFinal3Right == iIcon3)
				{
					if (bNot1)
					{
						P.SetFinalIconWithClue(this, iRow2, iCol + 3, iIcon2);

						return true;
					}
					else if (bNot2)
					{
						P.EliminateIconWithClue(this, iRow2, iCol + 1, iIcon2);

					}
					else
					{
						P.SetFinalIconWithClue(this, iRow2, iCol + 1, iIcon2);

						return true;
					}
				}
				else if (!P.m_Rows[iRow3].m_Cells[iCol - 2].m_bValues[iIcon3] && !bNot1 && !bNot2)
				{
					P.SetFinalIconWithClue(this, iRow2, iCol + 1, iIcon2);
					P.SetFinalIconWithClue(this, iRow3, iCol + 2, iIcon3);

					return true;
				}
				else if (!P.m_Rows[iRow3].m_Cells[iCol + 2].m_bValues[iIcon3] && !bNot1 && !bNot2)
				{
					P.SetFinalIconWithClue(this, iRow2, iCol - 1, iIcon2);
					P.SetFinalIconWithClue(this, iRow3, iCol - 2, iIcon3);

					return true;
				}
				else if (!bNot1)
				{
					for (int j = 0; j < P.m_iSize; j++)
					{
						if (!bNot2 && j != (iCol - 1) && j != (iCol + 1))
						{
							P.EliminateIconWithClue(this, iRow2, j, iIcon2);

						}
						if (j != (iCol - 2) && j != (iCol + 2))
						{
							P.EliminateIconWithClue(this, iRow3, j, iIcon3);

						}
					}
				}
			}
		}
	}
	else if (iFinal1 >= 0 && !bNot1 && !bNot2 && !bNot3)
	{
		int iFinal3 = P.m_Rows[iRow3].m_Cells[iCol].m_iFinalIcon;
		if (iFinal3 != iIcon3 && iFinal3 >= 0)
		{
			if (iCol == 0)
			{
				P.EliminateIconWithClue(this, iRow2, iCol + 1, iIcon2);

			}
			else if (iCol == P.m_iSize - 1)
			{
				P.EliminateIconWithClue(this, iRow2, iCol - 1, iIcon2);

			}
			else if (iCol == P.m_iSize - 3)
			{
				P.EliminateIconWithClue(this, iRow1, iCol + 2, iIcon1);
				P.EliminateIconWithClue(this, iRow2, iCol + 1, iIcon2);
				P.EliminateIconWithClue(this, iRow3, iCol + 2, iIcon3);

			}
			else if (iCol == 2)
			{
				P.EliminateIconWithClue(this, iRow1, iCol - 2, iIcon1);
				P.EliminateIconWithClue(this, iRow2, iCol - 1, iIcon2);
				P.EliminateIconWithClue(this, iRow3, iCol - 2, iIcon3);

			}
		}
	}
	if (!P.m_Rows[iRow1].m_Cells[iCol].m_bValues[iIcon1] && !bNot1)
	{
		if (!bNot3)
		{
			if (iCol + 4 < P.m_iSize)
			{
				if (!P.m_Rows[iRow1].m_Cells[iCol + 4].m_bValues[iIcon1])
				{
					P.EliminateIconWithClue(this, iRow3, iCol + 2, iIcon3);
				}
			}
			else if (iCol + 2 < P.m_iSize)
			{
				P.EliminateIconWithClue(this, iRow3, iCol + 2, iIcon3);
			}
			if (iCol - 4 >= 0)
			{
				if (!P.m_Rows[iRow1].m_Cells[iCol - 4].m_bValues[iIcon1])
				{
					P.EliminateIconWithClue(this, iRow3, iCol - 2, iIcon3);
				}
			}
			else if (iCol - 2 >= 0)
			{
				P.EliminateIconWithClue(this, iRow3, iCol - 2, iIcon3);
			}
		}
		if (!bNot2)
		{
			if (iCol + 2 < P.m_iSize)
			{
				if (!P.m_Rows[iRow1].m_Cells[iCol + 2].m_bValues[iIcon1])
				{
					P.EliminateIconWithClue(this, iRow2, iCol + 1, iIcon2);
				}
			}
			if (iCol - 2 >= 0)
			{
				if (!P.m_Rows[iRow1].m_Cells[iCol - 2].m_bValues[iIcon1])
				{
					P.EliminateIconWithClue(this, iRow2, iCol - 1, iIcon2);
				}
			}
		}
	}
	if (P.m_Rows[iRow2].m_Cells[iCol].m_iFinalIcon == iIcon2 && !bNot2)
	{
		// Middle icon is known, eliminate impossible end icons
		for (int i = 0; i < P.m_iSize; i++)
		{
			if (i != iCol - 1 && i != iCol + 1)
			{
				if (!bNot1)
					P.EliminateIconWithClue(this, iRow1, i, iIcon1);
				if (!bNot3)
					P.EliminateIconWithClue(this, iRow3, i, iIcon3);
			}
		}
	}

	return false;
}

void UClue::AnalyzeHorizontalSpan(UPuzzle& P)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol2];
	int iIcon3 = P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol3];

	// Icon2 cant be on either end
	P.EliminateIconWithClue(this, m_iRow2, 0, iIcon2);
	P.EliminateIconWithClue(this, m_iRow2, P.m_iSize - 1, iIcon2);


	for (int i = 0; i < P.m_iSize; i++)
	{
		if (SolveSpan(i, m_iRow, iIcon1, false, m_iRow2, iIcon2, false, m_iRow3, iIcon3, false, P))
			return;
		if (SolveSpan(i, m_iRow3, iIcon3, false, m_iRow2, iIcon2, false, m_iRow, iIcon1, false, P))
			return;
	}
}

void UClue::AnalyzeHorizontalSpanNotLeft(UPuzzle& P)
{
	int iIcon1 = m_iHorizontal1;
	int iIcon2 = P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol2];
	int iIcon3 = P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol3];

	// Icon2 cant be on either end
	P.EliminateIconWithClue(this, m_iRow2, 0, iIcon2);
	P.EliminateIconWithClue(this, m_iRow2, P.m_iSize - 1, iIcon2);


	for (int i = 0; i < P.m_iSize; i++)
	{
		if (SolveSpan(i, m_iRow, iIcon1, true, m_iRow2, iIcon2, false, m_iRow3, iIcon3, false, P))
			return;
		if (SolveSpan(i, m_iRow3, iIcon3, false, m_iRow2, iIcon2, false, m_iRow, iIcon1, true, P))
			return;
	}
}

void UClue::AnalyzeHorizontalSpanNotMid(UPuzzle& P)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = m_iHorizontal1;
	int iIcon3 = P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol3];

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (SolveSpan(i, m_iRow, iIcon1, false, m_iRow2, iIcon2, true, m_iRow3, iIcon3, false, P))
			return;
		if (SolveSpan(i, m_iRow3, iIcon3, false, m_iRow2, iIcon2, true, m_iRow, iIcon1, false, P))
			return;
	}
}

void UClue::AnalyzeHorizontalSpanNotRight(UPuzzle& P)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol2];
	int iIcon3 = m_iHorizontal1;

	// Icon2 cant be on either end
	P.EliminateIconWithClue(this, m_iRow2, 0, iIcon2);
	P.EliminateIconWithClue(this, m_iRow2, P.m_iSize - 1, iIcon2);


	for (int i = 0; i < P.m_iSize; i++)
	{
		if (SolveSpan(i, m_iRow, iIcon1, false, m_iRow2, iIcon2, false, m_iRow3, iIcon3, true, P))
			return;
		if (SolveSpan(i, m_iRow3, iIcon3, true, m_iRow2, iIcon2, false, m_iRow, iIcon1, false, P))
			return;
	}
}

void UClue::Dump(int32 iIndex, UPuzzle& P)
{
	FString Output = FString::Printf(TEXT("Clue(%d): "), iIndex);

	switch (m_Type)
	{
		case eClueType::Given:
			Output += FString::Printf(TEXT("Type: Given (%d, %d, %d)"), m_iRow, m_iCol, P.m_Solution[(m_iRow * P.m_iSize) + m_iCol]);
			break;

		case eClueType::NotHere:
			Output += FString::Printf(TEXT("Type: NotHere (%d, %d, not %d)"), m_iRow, m_iCol, m_iHorizontal1);
			break;

		case eClueType::Vertical:
			Output += TEXT("Type: Vertical  VType: ");
			switch (m_VerticalType)
			{
				case eVerticalType::Two: Output += FString::Printf(TEXT("Two ([%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[(m_iRow * P.m_iSize) + m_iCol],	m_iRow2, P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol]); break;
				case eVerticalType::Three: Output += FString::Printf(TEXT("Three ([%d]:%d, [%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[(m_iRow * P.m_iSize) + m_iCol], m_iRow2, P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol], m_iRow3, P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol]); break;
				case eVerticalType::EitherOr: Output += FString::Printf(TEXT("EitherOr ([%d]:%d, [%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[(m_iRow * P.m_iSize) + m_iCol], m_iRow2, (m_iRow2 == m_iNotCell) ? m_iHorizontal1 : P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol], m_iRow3, (m_iRow3 == m_iNotCell) ? m_iHorizontal1 : P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol]); break;
				case eVerticalType::TwoNot: Output += FString::Printf(TEXT("TwoNot ([%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[(m_iRow * P.m_iSize) + m_iCol], m_iRow2, m_iNotCell); break;
				case eVerticalType::ThreeTopNot: Output += FString::Printf(TEXT("ThreeTopNot ([%d]:%d, [%d]:%d, [%d]:%d)"), m_iRow, m_iNotCell, m_iRow2, P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol], m_iRow3, P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol]); break;
				case eVerticalType::ThreeMidNot: Output += FString::Printf(TEXT("ThreeMidNot ([%d]:%d, [%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[(m_iRow * P.m_iSize) + m_iCol], m_iRow2, m_iNotCell, m_iRow3, P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol]);	break;
				case eVerticalType::ThreeBotNot: Output += FString::Printf(TEXT("ThreeBotNot ([%d]:%d, [%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[(m_iRow * P.m_iSize) + m_iCol], m_iRow2, P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol], m_iRow3, m_iNotCell); break;
			}
			break;

		case eClueType::Horizontal:
			Output += TEXT("Type: Horizontal  HType: ");
			switch (m_HorizontalType)
			{
				case eHorizontalType::NextTo: Output += FString::Printf(TEXT("NextTo ([%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[(m_iRow * P.m_iSize) + m_iCol], m_iRow2, P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol2]); break;
				case eHorizontalType::NotNextTo: Output += FString::Printf(TEXT("NotNextTo ([%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[(m_iRow * P.m_iSize) + m_iCol], m_iRow2, m_iHorizontal1); break;
				case eHorizontalType::LeftOf: Output += FString::Printf(TEXT("LeftOf ([%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[(m_iRow * P.m_iSize) + m_iCol], m_iRow2, P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol2]); break;
				case eHorizontalType::NotLeftOf: Output += FString::Printf(TEXT("NotLeftOf ([%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[(m_iRow * P.m_iSize) + m_iCol], m_iRow2, P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol2]); break;
				case eHorizontalType::Span: Output += FString::Printf(TEXT("Span ([%d]:%d, [%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[(m_iRow * P.m_iSize) + m_iCol], m_iRow2, P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol2], m_iRow3, P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol3]); break;
				case eHorizontalType::SpanNotLeft: Output += FString::Printf(TEXT("SpanNotLeft ([%d]:%d, [%d]:%d, [%d]:%d)"), m_iRow, m_iHorizontal1, m_iRow2, P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol2], m_iRow3, P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol3]); break;
				case eHorizontalType::SpanNotMid: Output += FString::Printf(TEXT("SpanNotMid ([%d]:%d, [%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[(m_iRow * P.m_iSize) + m_iCol], m_iRow2, m_iHorizontal1, m_iRow3, P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol3]); break;
				case eHorizontalType::SpanNotRight:	Output += FString::Printf(TEXT("SpanNotRight ([%d]:%d, [%d]:%d, [%d]:%d)"),	m_iRow, P.m_Solution[(m_iRow * P.m_iSize) + m_iCol], m_iRow2, P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol2], m_iRow3, m_iHorizontal1); break;
				case eHorizontalType::Edge:
				case eHorizontalType::NotEdge:
				case eHorizontalType::DirectlyLeftOf:
				case eHorizontalType::Gap:
				case eHorizontalType::Between:
				case eHorizontalType::Chain:
				case eHorizontalType::NextToEitherOr:
				case eHorizontalType::AllApart:
				{
					// Slot icons, -1 for unused slots
					int SlotRows[3], SlotIcons[3];
					GetSlots(P, SlotRows, SlotIcons);
					Output += FString::Printf(TEXT("%s ([%d]:%d, [%d]:%d, [%d]:%d)"), *StaticEnum<eHorizontalType>()->GetNameStringByValue((int64)m_HorizontalType),
						SlotRows[0], SlotIcons[0], SlotRows[1], SlotIcons[1], SlotRows[2], SlotIcons[2]);
					break;
				}
			}
			break;
	}

	UE_LOG(LogTemp, Log, TEXT("%s"), *Output);
}

bool UClue::GetHintAction(UPuzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	bSetFinalIcon = false;
	iRow = -1;
	iCol = -1;
	iIcon = -1;

	switch (m_Type)
	{
		case eClueType::Horizontal:
			switch (m_HorizontalType)
			{
				case eHorizontalType::NextTo:
					return GetHintActionHorizontalNextTo(P, bSetFinalIcon, iRow, iCol, iIcon);
				case eHorizontalType::NotNextTo:
					return GetHintActionHorizontalNotNextTo(P, bSetFinalIcon, iRow, iCol, iIcon);
				case eHorizontalType::LeftOf:
					return GetHintActionHorizontalLeftOf(P, bSetFinalIcon, iRow, iCol, iIcon);
				case eHorizontalType::NotLeftOf:
					return GetHintActionHorizontalNotLeftOf(P, bSetFinalIcon, iRow, iCol, iIcon);
				case eHorizontalType::Span:
					return GetHintActionHorizontalSpan(P, bSetFinalIcon, iRow, iCol, iIcon);
				case eHorizontalType::SpanNotLeft:
					return GetHintActionHorizontalSpanNotLeft(P, bSetFinalIcon, iRow, iCol, iIcon);
				case eHorizontalType::SpanNotMid:
					return GetHintActionHorizontalSpanNotMid(P, bSetFinalIcon, iRow, iCol, iIcon);
				case eHorizontalType::SpanNotRight:
					return GetHintActionHorizontalSpanNotRight(P, bSetFinalIcon, iRow, iCol, iIcon);
				case eHorizontalType::Edge:
				case eHorizontalType::NotEdge:
				case eHorizontalType::DirectlyLeftOf:
				case eHorizontalType::Gap:
				case eHorizontalType::Between:
				case eHorizontalType::Chain:
				case eHorizontalType::NextToEitherOr:
				case eHorizontalType::AllApart:
					return GetHintActionConstraint(P, bSetFinalIcon, iRow, iCol, iIcon);
				default:
					return false;
			}
		case eClueType::Vertical:
			switch (m_VerticalType)
			{
				case eVerticalType::Two:
					return GetHintActionVerticalTwo(P, bSetFinalIcon, iRow, iCol, iIcon);
				case eVerticalType::Three:
					return GetHintActionVerticalThree(P, bSetFinalIcon, iRow, iCol, iIcon);
				case eVerticalType::EitherOr:
					return GetHintActionVerticalEitherOr(P, bSetFinalIcon, iRow, iCol, iIcon);
				case eVerticalType::TwoNot:
					return GetHintActionVerticalTwoNot(P, bSetFinalIcon, iRow, iCol, iIcon);
				case eVerticalType::ThreeTopNot:
					return GetHintActionVerticalThreeTopNot(P, bSetFinalIcon, iRow, iCol, iIcon);
				case eVerticalType::ThreeMidNot:
					return GetHintActionVerticalThreeMidNot(P, bSetFinalIcon, iRow, iCol, iIcon);
				case eVerticalType::ThreeBotNot:
					return GetHintActionVerticalThreeBotNot(P, bSetFinalIcon, iRow, iCol, iIcon);
				default:
					return false;
			}
		default:
			return false;
	}
}

bool UClue::GetHintActionVerticalTwo(UPuzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol];
	for (int i = 0; i < P.m_iSize; i++)
	{
		if (!P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
		{
			if (P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
			{
				bSetFinalIcon = false;
				iRow = m_iRow2;
				iCol = i;
				iIcon = iIcon2;
				return true;
			}
		}
		else if (!P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
		{
			if (P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
			{
				bSetFinalIcon = false;
				iRow = m_iRow;
				iCol = i;
				iIcon = iIcon1;
				return true;
			}
		}

		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon != iIcon2)
			{
				bSetFinalIcon = true;
				iRow = m_iRow2;
				iCol = i;
				iIcon = iIcon2;
				return true;
			}
		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon != iIcon1)
			{
				bSetFinalIcon = true;
				iRow = m_iRow;
				iCol = i;
				iIcon = iIcon1;
				return true;
			}
		}
	}

	bSetFinalIcon = false;
	iRow = -1;
	iCol = -1;
	iIcon = -1;
	return false;
}

bool UClue::GetHintActionVerticalThree(UPuzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol];
	int iIcon3 = P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol];
	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon != iIcon2)
			{
				bSetFinalIcon = true;
				iRow = m_iRow2;
				iCol = i;
				iIcon = iIcon2;
				return true;
			}
			else if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon != iIcon3)
			{
				bSetFinalIcon = true;
				iRow = m_iRow3;
				iCol = i;
				iIcon = iIcon3;
				return true;
			}
		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon != iIcon1)
			{
				bSetFinalIcon = true;
				iRow = m_iRow;
				iCol = i;
				iIcon = iIcon1;
				return true;
			}
			else if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon != iIcon3)
			{
				bSetFinalIcon = true;
				iRow = m_iRow3;
				iCol = i;
				iIcon = iIcon3;
				return true;
			}
		}
		else if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon == iIcon3)
		{
			if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon != iIcon1)
			{
				bSetFinalIcon = true;
				iRow = m_iRow;
				iCol = i;
				iIcon = iIcon1;
				return true;
			}
			else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon != iIcon2)
			{
				bSetFinalIcon = true;
				iRow = m_iRow2;
				iCol = i;
				iIcon = iIcon2;
				return true;
			}
		}

		if (!P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
		{
			if (P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
			{
				bSetFinalIcon = false;
				iRow = m_iRow2;
				iCol = i;
				iIcon = iIcon2;
				return true;
			}
			else if (P.m_Rows[m_iRow3].m_Cells[i].m_bValues[iIcon3])
			{
				bSetFinalIcon = false;
				iRow = m_iRow3;
				iCol = i;
				iIcon = iIcon3;
				return true;
			}
		}
		else if (!P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
		{
			if (P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
			{
				bSetFinalIcon = false;
				iRow = m_iRow;
				iCol = i;
				iIcon = iIcon1;
				return true;
			}
			else if (P.m_Rows[m_iRow3].m_Cells[i].m_bValues[iIcon3])
			{
				bSetFinalIcon = false;
				iRow = m_iRow3;
				iCol = i;
				iIcon = iIcon3;
				return true;
			}
		}
		else if (!P.m_Rows[m_iRow3].m_Cells[i].m_bValues[iIcon3])
		{
			if (P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
			{
				bSetFinalIcon = false;
				iRow = m_iRow;
				iCol = i;
				iIcon = iIcon1;
				return true;
			}
			else if (P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
			{
				bSetFinalIcon = false;
				iRow = m_iRow2;
				iCol = i;
				iIcon = iIcon2;
				return true;
			}
		}
	}

	bSetFinalIcon = false;
	iRow = -1;
	iCol = -1;
	iIcon = -1;
	return false;
}

bool UClue::GetHintActionVerticalEitherOr(UPuzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = (m_iRow2 == m_iNotCell) ? m_iHorizontal1 : P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol];
	int iIcon3 = (m_iRow3 == m_iNotCell) ? m_iHorizontal1 : P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol];

	int iIcon2Col = -1;
	int iIcon3Col = -1;
	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			if (P.m_Rows[m_iRow3].m_Cells[i].m_bValues[iIcon3])
			{
				bSetFinalIcon = false;
				iRow = m_iRow3;
				iCol = i;
				iIcon = iIcon3;
				return true;
			}
			iIcon2Col = i;
		}
		if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon == iIcon3)
		{
			if (P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
			{
				bSetFinalIcon = false;
				iRow = m_iRow2;
				iCol = i;
				iIcon = iIcon2;
				return true;
			}
			iIcon3Col = i;
		}

		if (!P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2] &&
			!P.m_Rows[m_iRow3].m_Cells[i].m_bValues[iIcon3])
		{
			if (P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
			{
				bSetFinalIcon = false;
				iRow = m_iRow;
				iCol = i;
				iIcon = iIcon1;
				return true;
			}
		}

		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			for (int j = 0; j < P.m_iSize; j++)
			{
				if (j == i)
					continue;

				if (P.m_Rows[m_iRow2].m_Cells[j].m_iFinalIcon == iIcon2)
				{
					if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon != iIcon3)
					{
						bSetFinalIcon = true;
						iRow = m_iRow3;
						iCol = i;
						iIcon = iIcon3;
						return true;
					}
				}
				else if (P.m_Rows[m_iRow3].m_Cells[j].m_iFinalIcon == iIcon3)
				{
					if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon != iIcon2)
					{
						bSetFinalIcon = true;
						iRow = m_iRow2;
						iCol = i;
						iIcon = iIcon2;
						return true;
					}
				}
			}
		}
	}

	if (iIcon2Col >= 0 && iIcon3Col >= 0)
	{
		for (int i = 0; i < P.m_iSize; i++)
		{
			if (i != iIcon2Col && i != iIcon3Col)
			{
				if (P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
				{
					bSetFinalIcon = false;
					iRow = m_iRow;
					iCol = i;
					iIcon = iIcon1;
					return true;
				}
			}
		}
	}

	bSetFinalIcon = false;
	iRow = -1;
	iCol = -1;
	iIcon = -1;
	return false;
}

bool UClue::GetHintActionVerticalTwoNot(UPuzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = m_iNotCell;

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			if (P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
			{
				bSetFinalIcon = false;
				iRow = m_iRow2;
				iCol = i;
				iIcon = iIcon2;
				return true;
			}
		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			if (P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
			{
				bSetFinalIcon = false;
				iRow = m_iRow;
				iCol = i;
				iIcon = iIcon1;
				return true;
			}
		}
	}

	bSetFinalIcon = false;
	iRow = -1;
	iCol = -1;
	iIcon = -1;
	return false;
}

bool UClue::GetHintActionVerticalThreeTopNot(UPuzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = m_iNotCell;
	int iIcon2 = P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol];
	int iIcon3 = P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol];

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			if (P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
			{
				bSetFinalIcon = false;
				iRow = m_iRow2;
				iCol = i;
				iIcon = iIcon2;
				return true;
			}
			else if (P.m_Rows[m_iRow3].m_Cells[i].m_bValues[iIcon3])
			{
				bSetFinalIcon = false;
				iRow = m_iRow3;
				iCol = i;
				iIcon = iIcon3;
				return true;
			}
		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			if (P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
			{
				bSetFinalIcon = false;
				iRow = m_iRow;
				iCol = i;
				iIcon = iIcon1;
				return true;
			}

			if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon != iIcon3)
			{
				bSetFinalIcon = true;
				iRow = m_iRow3;
				iCol = i;
				iIcon = iIcon3;
				return true;
			}
		}
		else if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon == iIcon3)
		{
			if (P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
			{
				bSetFinalIcon = false;
				iRow = m_iRow;
				iCol = i;
				iIcon = iIcon1;
				return true;
			}
			if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon != iIcon2)
			{
				bSetFinalIcon = true;
				iRow = m_iRow2;
				iCol = i;
				iIcon = iIcon2;
				return true;
			}
		}
		else if (!P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
		{
			if (P.m_Rows[m_iRow3].m_Cells[i].m_bValues[iIcon3])
			{
				bSetFinalIcon = false;
				iRow = m_iRow3;
				iCol = i;
				iIcon = iIcon3;
				return true;
			}
		}
		else if (!P.m_Rows[m_iRow3].m_Cells[i].m_bValues[iIcon3])
		{
			if (P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
			{
				bSetFinalIcon = false;
				iRow = m_iRow2;
				iCol = i;
				iIcon = iIcon2;
				return true;
			}
		}
	}

	bSetFinalIcon = false;
	iRow = -1;
	iCol = -1;
	iIcon = -1;
	return false;
}

bool UClue::GetHintActionVerticalThreeMidNot(UPuzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = m_iNotCell;
	int iIcon3 = P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol];

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			if (P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
			{
				bSetFinalIcon = false;
				iRow = m_iRow;
				iCol = i;
				iIcon = iIcon1;
				return true;
			}
			else if (P.m_Rows[m_iRow3].m_Cells[i].m_bValues[iIcon3])
			{
				bSetFinalIcon = false;
				iRow = m_iRow3;
				iCol = i;
				iIcon = iIcon3;
				return true;
			}
		}
		else if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			if (P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
			{
				bSetFinalIcon = false;
				iRow = m_iRow2;
				iCol = i;
				iIcon = iIcon2;
				return true;
			}
			if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon != iIcon3)
			{
				bSetFinalIcon = true;
				iRow = m_iRow3;
				iCol = i;
				iIcon = iIcon3;
				return true;
			}
		}
		else if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon == iIcon3)
		{
			if (P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
			{
				bSetFinalIcon = false;
				iRow = m_iRow2;
				iCol = i;
				iIcon = iIcon2;
				return true;
			}
			if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon != iIcon1)
			{
				bSetFinalIcon = true;
				iRow = m_iRow;
				iCol = i;
				iIcon = iIcon1;
				return true;
			}
		}
		else if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon >= 0)
		{
			if (P.m_Rows[m_iRow3].m_Cells[i].m_bValues[iIcon3])
			{
				bSetFinalIcon = false;
				iRow = m_iRow3;
				iCol = i;
				iIcon = iIcon3;
				return true;
			}
		}
		else if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon >= 0)
		{
			if (P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
			{
				bSetFinalIcon = false;
				iRow = m_iRow;
				iCol = i;
				iIcon = iIcon1;
				return true;
			}
		}
	}

	bSetFinalIcon = false;
	iRow = -1;
	iCol = -1;
	iIcon = -1;
	return false;
}

bool UClue::GetHintActionVerticalThreeBotNot(UPuzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol];
	int iIcon3 = m_iNotCell;

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon == iIcon3)
		{
			if (P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
			{
				bSetFinalIcon = false;
				iRow = m_iRow2;
				iCol = i;
				iIcon = iIcon2;
				return true;
			}
			else if (P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
			{
				bSetFinalIcon = false;
				iRow = m_iRow;
				iCol = i;
				iIcon = iIcon1;
				return true;
			}
		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			if (P.m_Rows[m_iRow3].m_Cells[i].m_bValues[iIcon3])
			{
				bSetFinalIcon = false;
				iRow = m_iRow3;
				iCol = i;
				iIcon = iIcon3;
				return true;
			}
			if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon != iIcon1)
			{
				bSetFinalIcon = true;
				iRow = m_iRow;
				iCol = i;
				iIcon = iIcon1;
				return true;
			}
		}
		else if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			if (P.m_Rows[m_iRow3].m_Cells[i].m_bValues[iIcon3])
			{
				bSetFinalIcon = false;
				iRow = m_iRow3;
				iCol = i;
				iIcon = iIcon3;
				return true;
			}
			if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon != iIcon2)
			{
				bSetFinalIcon = true;
				iRow = m_iRow2;
				iCol = i;
				iIcon = iIcon2;
				return true;
			}
		}
		else if (!P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
		{
			if (P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
			{
				bSetFinalIcon = false;
				iRow = m_iRow;
				iCol = i;
				iIcon = iIcon1;
				return true;
			}
		}
		else if (!P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
		{
			if (P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
			{
				bSetFinalIcon = false;
				iRow = m_iRow2;
				iCol = i;
				iIcon = iIcon2;
				return true;
			}
		}
	}

	bSetFinalIcon = false;
	iRow = -1;
	iCol = -1;
	iIcon = -1;
	return false;
}

bool UClue::GetHintActionHorizontalNextTo(UPuzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol2];

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			if (i == 0)
			{
				if (P.m_Rows[m_iRow2].m_Cells[1].m_iFinalIcon != iIcon2)
				{
					bSetFinalIcon = true;
					iRow = m_iRow2;
					iCol = 1;
					iIcon = iIcon2;
					return true;
				}
			}
			else if (i == P.m_iSize - 1)
			{
				if (P.m_Rows[m_iRow2].m_Cells[i - 1].m_iFinalIcon != iIcon2)
				{
					bSetFinalIcon = true;
					iRow = m_iRow2;
					iCol = i - 1;
					iIcon = iIcon2;
					return true;
				}
			}
			else
			{
				for (int j = 0; j < P.m_iSize; j++)
				{
					if (j == (i - 1) || j == (i + 1))
						continue;
					if (P.m_Rows[m_iRow2].m_Cells[j].m_bValues[iIcon2])
					{
						bSetFinalIcon = false;
						iRow = m_iRow2;
						iCol = j;
						iIcon = iIcon2;
						return true;
					}

				}
			}
			break;
		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			if (i == 0)
			{
				if (P.m_Rows[m_iRow].m_Cells[1].m_iFinalIcon != iIcon1)
				{
					bSetFinalIcon = true;
					iRow = m_iRow;
					iCol = 1;
					iIcon = iIcon1;
					return true;
				}
			}
			else if (i == P.m_iSize - 1)
			{
				if (P.m_Rows[m_iRow].m_Cells[i - 1].m_iFinalIcon != iIcon1)
				{
					bSetFinalIcon = true;
					iRow = m_iRow;
					iCol = i - 1;
					iIcon = iIcon1;
					return true;
				}
			}
			else
			{
				for (int j = 0; j < P.m_iSize; j++)
				{
					if (j == (i - 1) || j == (i + 1))
						continue;
					if (P.m_Rows[m_iRow].m_Cells[j].m_bValues[iIcon1])
					{
						bSetFinalIcon = false;
						iRow = m_iRow;
						iCol = j;
						iIcon = iIcon1;
						return true;
					}
				}
			}
			break;
		}
		else
		{
			if (i == 0)
			{
				if (!P.m_Rows[m_iRow2].m_Cells[i + 1].m_bValues[iIcon2])
				{
					if (P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
					{
						bSetFinalIcon = false;
						iRow = m_iRow;
						iCol = i;
						iIcon = iIcon1;
						return true;
					}
				}
				if (!P.m_Rows[m_iRow].m_Cells[i + 1].m_bValues[iIcon1])
				{
					if (P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
					{
						bSetFinalIcon = false;
						iRow = m_iRow2;
						iCol = i;
						iIcon = iIcon2;
						return true;
					}
				}
			}
			else if (i == P.m_iSize - 1)
			{
				if (!P.m_Rows[m_iRow2].m_Cells[i - 1].m_bValues[iIcon2])
				{
					if (P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
					{
						bSetFinalIcon = false;
						iRow = m_iRow;
						iCol = i;
						iIcon = iIcon1;
						return true;
					}
				}
				if (!P.m_Rows[m_iRow].m_Cells[i - 1].m_bValues[iIcon1])
				{
					if (P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
					{
						bSetFinalIcon = false;
						iRow = m_iRow2;
						iCol = i;
						iIcon = iIcon2;
						return true;
					}
				}
			}
			else
			{
				if (!P.m_Rows[m_iRow2].m_Cells[i + 1].m_bValues[iIcon2] &&
					!P.m_Rows[m_iRow2].m_Cells[i - 1].m_bValues[iIcon2])
				{
					if (P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
					{
						bSetFinalIcon = false;
						iRow = m_iRow;
						iCol = i;
						iIcon = iIcon1;
						return true;
					}
				}
				if (!P.m_Rows[m_iRow].m_Cells[i + 1].m_bValues[iIcon1] &&
					!P.m_Rows[m_iRow].m_Cells[i - 1].m_bValues[iIcon1])
				{
					if (P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
					{
						bSetFinalIcon = false;
						iRow = m_iRow2;
						iCol = i;
						iIcon = iIcon2;
						return true;
					}
				}
			}
		}
	}

	bSetFinalIcon = false;
	iRow = -1;
	iCol = -1;
	iIcon = -1;
	return false;
}

bool UClue::GetHintActionHorizontalNotNextTo(UPuzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = m_iHorizontal1;

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			if (i == 0)
			{
				if (P.m_Rows[m_iRow2].m_Cells[i + 1].m_bValues[iIcon2])
				{
					bSetFinalIcon = false;
					iRow = m_iRow2;
					iCol = i + 1;
					iIcon = iIcon2;
					return true;
				}
			}
			else if (i == P.m_iSize - 1)
			{
				if (P.m_Rows[m_iRow2].m_Cells[i - 1].m_bValues[iIcon2])
				{
					bSetFinalIcon = false;
					iRow = m_iRow2;
					iCol = i - 1;
					iIcon = iIcon2;
					return true;
				}
			}
			else
			{
				if (P.m_Rows[m_iRow2].m_Cells[i - 1].m_bValues[iIcon2])
				{
					bSetFinalIcon = false;
					iRow = m_iRow2;
					iCol = i - 1;
					iIcon = iIcon2;
					return true;
				}
				else if (P.m_Rows[m_iRow2].m_Cells[i + 1].m_bValues[iIcon2])
				{
					bSetFinalIcon = false;
					iRow = m_iRow2;
					iCol = i + 1;
					iIcon = iIcon2;
					return true;
				}
			}
			break;
		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			if (i == 0)
			{
				if (P.m_Rows[m_iRow].m_Cells[i + 1].m_bValues[iIcon1])
				{
					bSetFinalIcon = false;
					iRow = m_iRow;
					iCol = i + 1;
					iIcon = iIcon1;
					return true;
				}
			}
			else if (i == P.m_iSize - 1)
			{
				if (P.m_Rows[m_iRow].m_Cells[i - 1].m_bValues[iIcon1])
				{
					bSetFinalIcon = false;
					iRow = m_iRow;
					iCol = i - 1;
					iIcon = iIcon1;
					return true;
				}
			}
			else
			{
				if (P.m_Rows[m_iRow].m_Cells[i - 1].m_bValues[iIcon1])
				{
					bSetFinalIcon = false;
					iRow = m_iRow;
					iCol = i - 1;
					iIcon = iIcon1;
					return true;
				}
				else if (P.m_Rows[m_iRow].m_Cells[i + 1].m_bValues[iIcon1])
				{
					bSetFinalIcon = false;
					iRow = m_iRow;
					iCol = i + 1;
					iIcon = iIcon1;
					return true;
				}
			}
			break;
		}
	}

	bSetFinalIcon = false;
	iRow = -1;
	iCol = -1;
	iIcon = -1;
	return false;
}

bool UClue::GetHintActionHorizontalLeftOf(UPuzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol2];

	int iFirstPossibleLeft = 0;
	for (iFirstPossibleLeft = 0; iFirstPossibleLeft < P.m_iSize; iFirstPossibleLeft++)
	{
		if (P.m_Rows[m_iRow].m_Cells[iFirstPossibleLeft].m_bValues[iIcon1])
			break;
	}

	int iFirstPossibleRight = P.m_iSize - 1;
	for (iFirstPossibleRight = P.m_iSize - 1; iFirstPossibleRight >= 0; iFirstPossibleRight--)
	{
		if (P.m_Rows[m_iRow2].m_Cells[iFirstPossibleRight].m_bValues[iIcon2])
			break;
	}

	if (iFirstPossibleLeft + 1 == iFirstPossibleRight)
	{
		// we have a solution for this clue
		if (P.m_Rows[m_iRow].m_Cells[iFirstPossibleLeft].m_iFinalIcon != iIcon1)
		{
			bSetFinalIcon = true;
			iRow = m_iRow;
			iCol = iFirstPossibleLeft;
			iIcon = iIcon1;
			return true;
		}
		else if (P.m_Rows[m_iRow2].m_Cells[iFirstPossibleRight].m_iFinalIcon != iIcon2)
		{
			bSetFinalIcon = true;
			iRow = m_iRow2;
			iCol = iFirstPossibleRight;
			iIcon = iIcon2;
			return true;
		}
	}
	else
	{
		// Remove all icon2's from the left side of the first possible left
		for (int i = 0; i <= iFirstPossibleLeft; i++)
		{
			if (P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
			{
				bSetFinalIcon = false;
				iRow = m_iRow2;
				iCol = i;
				iIcon = iIcon2;
				return true;
			}
		}

		// Remove all the icon1's from the right side of the first possible right
		for (int i = iFirstPossibleRight; i < P.m_iSize; i++)
		{
			if (P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
			{
				bSetFinalIcon = false;
				iRow = m_iRow;
				iCol = i;
				iIcon = iIcon1;
				return true;
			}
		}
	}

	bSetFinalIcon = false;
	iRow = -1;
	iCol = -1;
	iIcon = -1;
	return false;
}

bool UClue::GetHintActionHorizontalNotLeftOf(UPuzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol2];

	// If both icons are on the same row
	if (m_iRow == m_iRow2)
	{
		if (P.m_Rows[m_iRow].m_Cells[0].m_bValues[iIcon1])
		{
			bSetFinalIcon = false;
			iRow = m_iRow;
			iCol = 0;
			iIcon = iIcon1;
			return true;
		}

		if (P.m_Rows[m_iRow2].m_Cells[P.m_iSize - 1].m_bValues[iIcon2])
		{
			bSetFinalIcon = false;
			iRow = m_iRow2;
			iCol = P.m_iSize - 1;
			iIcon = iIcon2;
			return true;
		}
	}

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			// Icon1 is known, remove all instances of icon2 to the right
			for (int j = i + 1; j < P.m_iSize; j++)
			{
				if (P.m_Rows[m_iRow2].m_Cells[j].m_bValues[iIcon2])
				{
					bSetFinalIcon = false;
					iRow = m_iRow2;
					iCol = j;
					iIcon = iIcon2;
					return true;
				}
			}
			break;
		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			// Icon2 is known, remove all instances of icon1 to the left
			for (int j = 0; j < i; j++)
			{
				if (P.m_Rows[m_iRow].m_Cells[j].m_bValues[iIcon1])
				{
					bSetFinalIcon = false;
					iRow = m_iRow;
					iCol = j;
					iIcon = iIcon1;
					return true;
				}
			}
			break;
		}
	}

	bSetFinalIcon = false;
	iRow = -1;
	iCol = -1;
	iIcon = -1;
	return false;
}

bool UClue::GetHintActionHorizontalSpan(UPuzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol2];
	int iIcon3 = P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol3];

	// Icon2 cant be on either end
	if (P.m_Rows[m_iRow2].m_Cells[0].m_bValues[iIcon2])
	{
		bSetFinalIcon = false;
		iRow = m_iRow2;
		iCol = 0;
		iIcon = iIcon2;
		return true;
	}
	else if (P.m_Rows[m_iRow2].m_Cells[P.m_iSize - 1].m_bValues[iIcon2])
	{
		bSetFinalIcon = false;
		iRow = m_iRow2;
		iCol = P.m_iSize - 1;
		iIcon = iIcon2;
		return true;
	}


	for (int i = 0; i < P.m_iSize; i++)
	{
		if (GetHintActionSpan(i, m_iRow, iIcon1, false, m_iRow2, iIcon2, false, m_iRow3, iIcon3, false, P, bSetFinalIcon, iRow, iCol, iIcon))
			return true;
		if (GetHintActionSpan(i, m_iRow3, iIcon3, false, m_iRow2, iIcon2, false, m_iRow, iIcon1, false, P, bSetFinalIcon, iRow, iCol, iIcon))
			return true;
	}

	bSetFinalIcon = false;
	iRow = -1;
	iCol = -1;
	iIcon = -1;
	return false;
}

bool UClue::GetHintActionSpan(int iCol, int iRow1, int iIcon1, bool bNot1, int iRow2, int iIcon2, bool bNot2, int iRow3, int iIcon3, bool bNot3, UPuzzle& P, bool& bSetFinalIcon, int& iOutRow, int& iOutCol, int& iOutIcon)
{
	int iFinal1 = P.m_Rows[iRow1].m_Cells[iCol].m_iFinalIcon;
	if (iFinal1 == iIcon1)
	{
		if (iCol < 2)
		{
			if (bNot1)
			{
				int iFinal2Right = P.m_Rows[iRow2].m_Cells[iCol + 1].m_iFinalIcon;
				if (iFinal2Right == iIcon2)
				{
					if (P.m_Rows[iRow3].m_Cells[iCol].m_iFinalIcon != iIcon3)
					{
						bSetFinalIcon = true;
						iOutRow = iRow3;
						iOutCol = iCol;
						iOutIcon = iIcon3;
						return true;
					}
				}
			}
			else
			{
				if (bNot2)
				{
					if (P.m_Rows[iRow2].m_Cells[iCol + 1].m_bValues[iIcon2])
					{
						bSetFinalIcon = false;
						iOutRow = iRow2;
						iOutCol = iCol + 1;
						iOutIcon = iIcon2;
						return true;
					}
				}
				else
				{
					if (P.m_Rows[iRow2].m_Cells[iCol + 1].m_iFinalIcon != iIcon2)
					{
						bSetFinalIcon = true;
						iOutRow = iRow2;
						iOutCol = iCol + 1;
						iOutIcon = iIcon2;
						return true;
					}
				}

				if (bNot3)
				{
					if (P.m_Rows[iRow3].m_Cells[iCol + 2].m_bValues[iIcon3])
					{
						bSetFinalIcon = false;
						iOutRow = iRow3;
						iOutCol = iCol + 2;
						iOutIcon = iIcon3;
						return true;
					}
				}
				else
				{
					if (P.m_Rows[iRow3].m_Cells[iCol + 2].m_iFinalIcon != iIcon3)
					{
						bSetFinalIcon = true;
						iOutRow = iRow3;
						iOutCol = iCol + 2;
						iOutIcon = iIcon3;
						return true;
					}
				}
			}
		}
		else if (iCol > P.m_iSize - 3)
		{
			if (bNot1)
			{
				int iFinal2Left = P.m_Rows[iRow2].m_Cells[iCol - 1].m_iFinalIcon;
				if (iFinal2Left == iIcon2)
				{
					if (P.m_Rows[iRow3].m_Cells[iCol].m_iFinalIcon != iIcon3)
					{
						bSetFinalIcon = true;
						iOutRow = iRow3;
						iOutCol = iCol;
						iOutIcon = iIcon3;
						return true;
					}
				}
			}
			else
			{
				if (bNot2)
				{
					if (P.m_Rows[iRow2].m_Cells[iCol - 1].m_bValues[iIcon2])
					{
						bSetFinalIcon = false;
						iOutRow = iRow2;
						iOutCol = iCol - 1;
						iOutIcon = iIcon2;
						return true;
					}
				}
				else
				{
					if (P.m_Rows[iRow2].m_Cells[iCol - 1].m_iFinalIcon != iIcon2)
					{
						bSetFinalIcon = true;
						iOutRow = iRow2;
						iOutCol = iCol - 1;
						iOutIcon = iIcon2;
						return true;
					}
				}

				if (bNot3)
				{
					if (P.m_Rows[iRow3].m_Cells[iCol - 2].m_bValues[iIcon3])
					{
						bSetFinalIcon = false;
						iOutRow = iRow3;
						iOutCol = iCol - 2;
						iOutIcon = iIcon3;
						return true;
					}
				}
				else
				{
					if (P.m_Rows[iRow3].m_Cells[iCol - 2].m_iFinalIcon != iIcon3)
					{
						bSetFinalIcon = true;
						iOutRow = iRow3;
						iOutCol = iCol - 2;
						iOutIcon = iIcon3;
						return true;
					}
				}
			}
		}
		else
		{
			int iFinal2Left = P.m_Rows[iRow2].m_Cells[iCol - 1].m_iFinalIcon;
			int iFinal2Right = P.m_Rows[iRow2].m_Cells[iCol + 1].m_iFinalIcon;
			if (iFinal2Left == iIcon2)
			{
				if (bNot1)
				{
					if (P.m_Rows[iRow3].m_Cells[iCol].m_iFinalIcon != iIcon3)
					{
						bSetFinalIcon = true;
						iOutRow = iRow3;
						iOutCol = iCol;
						iOutIcon = iIcon3;
						return true;
					}
				}
				else if (bNot2)
				{
					if (P.m_Rows[iRow3].m_Cells[iCol + 2].m_iFinalIcon != iIcon3)
					{
						bSetFinalIcon = true;
						iOutRow = iRow3;
						iOutCol = iCol + 2;
						iOutIcon = iIcon3;
						return true;
					}
				}
				else if (bNot3)
				{
					if (P.m_Rows[iRow3].m_Cells[iCol - 2].m_bValues[iIcon3])
					{
						bSetFinalIcon = false;
						iOutRow = iRow3;
						iOutCol = iCol - 2;
						iOutIcon = iIcon3;
						return true;
					}
				}
				else
				{
					if (P.m_Rows[iRow3].m_Cells[iCol - 2].m_iFinalIcon != iIcon3)
					{
						bSetFinalIcon = true;
						iOutRow = iRow3;
						iOutCol = iCol - 2;
						iOutIcon = iIcon3;
						return true;
					}
				}
			}
			else if (iFinal2Right == iIcon2)
			{
				if (bNot1)
				{
					if (P.m_Rows[iRow3].m_Cells[iCol].m_iFinalIcon != iIcon3)
					{
						bSetFinalIcon = true;
						iOutRow = iRow3;
						iOutCol = iCol;
						iOutIcon = iIcon3;
						return true;
					}
				}
				else if (bNot2)
				{
					if (P.m_Rows[iRow3].m_Cells[iCol - 2].m_iFinalIcon != iIcon3)
					{
						bSetFinalIcon = true;
						iOutRow = iRow3;
						iOutCol = iCol - 2;
						iOutIcon = iIcon3;
						return true;
					}
				}
				else if (bNot3)
				{
					if (P.m_Rows[iRow3].m_Cells[iCol + 2].m_bValues[iIcon3])
					{
						bSetFinalIcon = false;
						iOutRow = iRow3;
						iOutCol = iCol + 2;
						iOutIcon = iIcon3;
						return true;
					}
				}
				else
				{
					if (P.m_Rows[iRow3].m_Cells[iCol + 2].m_iFinalIcon != iIcon3)
					{
						bSetFinalIcon = true;
						iOutRow = iRow3;
						iOutCol = iCol + 2;
						iOutIcon = iIcon3;
						return true;
					}
				}
			}
			else if (!P.m_Rows[iRow2].m_Cells[iCol - 1].m_bValues[iIcon2] && !bNot1 && !bNot2)
			{
				if (P.m_Rows[iRow2].m_Cells[iCol + 1].m_iFinalIcon != iIcon2)
				{
					bSetFinalIcon = true;
					iOutRow = iRow2;
					iOutCol = iCol + 1;
					iOutIcon = iIcon2;
					return true;
				}

				if (bNot3)
				{
					if (P.m_Rows[iRow3].m_Cells[iCol + 2].m_bValues[iIcon3])
					{
						bSetFinalIcon = false;
						iOutRow = iRow3;
						iOutCol = iCol + 2;
						iOutIcon = iIcon3;
						return true;
					}
				}
				else
				{
					if (P.m_Rows[iRow3].m_Cells[iCol + 2].m_iFinalIcon != iIcon3)
					{
						bSetFinalIcon = true;
						iOutRow = iRow3;
						iOutCol = iCol + 2;
						iOutIcon = iIcon3;
						return true;
					}
				}
			}
			else if (!P.m_Rows[iRow2].m_Cells[iCol + 1].m_bValues[iIcon2] && !bNot1 && !bNot2)
			{
				if (P.m_Rows[iRow2].m_Cells[iCol - 1].m_iFinalIcon != iIcon2)
				{
					bSetFinalIcon = true;
					iOutRow = iRow2;
					iOutCol = iCol - 1;
					iOutIcon = iIcon2;
					return true;
				}

				if (bNot3)
				{
					if (P.m_Rows[iRow3].m_Cells[iCol - 2].m_bValues[iIcon3])
					{
						bSetFinalIcon = false;
						iOutRow = iRow3;
						iOutCol = iCol - 2;
						iOutIcon = iIcon3;
						return true;
					}
				}
				else
				{
					if (P.m_Rows[iRow3].m_Cells[iCol - 2].m_iFinalIcon != iIcon3)
					{
						bSetFinalIcon = true;
						iOutRow = iRow3;
						iOutCol = iCol - 2;
						iOutIcon = iIcon3;
						return true;
					}
				}
			}
			else if (!bNot3)
			{
				int iFinal3Left = P.m_Rows[iRow3].m_Cells[iCol - 2].m_iFinalIcon;
				int iFinal3Right = P.m_Rows[iRow3].m_Cells[iCol + 2].m_iFinalIcon;
				if (iFinal3Left == iIcon3)
				{
					if (bNot1)
					{
						if (P.m_Rows[iRow2].m_Cells[iCol - 3].m_iFinalIcon != iIcon2)
						{
							bSetFinalIcon = true;
							iOutRow = iRow2;
							iOutCol = iCol - 3;
							iOutIcon = iIcon2;
							return true;
						}
					}
					else if (bNot2)
					{
						if (P.m_Rows[iRow2].m_Cells[iCol - 1].m_bValues[iIcon2])
						{
							bSetFinalIcon = false;
							iOutRow = iRow2;
							iOutCol = iCol - 1;
							iOutIcon = iIcon2;
							return true;
						}
					}
					else
					{
						if (P.m_Rows[iRow2].m_Cells[iCol - 1].m_iFinalIcon != iIcon2)
						{
							bSetFinalIcon = true;
							iOutRow = iRow2;
							iOutCol = iCol - 1;
							iOutIcon = iIcon2;
							return true;
						}
					}
				}
				else if (iFinal3Right == iIcon3)
				{
					if (bNot1)
					{
						if (P.m_Rows[iRow2].m_Cells[iCol + 3].m_iFinalIcon != iIcon2)
						{
							bSetFinalIcon = true;
							iOutRow = iRow2;
							iOutCol = iCol + 3;
							iOutIcon = iIcon2;
							return true;
						}
					}
					else if (bNot2)
					{
						if (P.m_Rows[iRow2].m_Cells[iCol + 1].m_bValues[iIcon2])
						{
							bSetFinalIcon = false;
							iOutRow = iRow2;
							iOutCol = iCol + 1;
							iOutIcon = iIcon2;
							return true;
						}
					}
					else
					{
						if (P.m_Rows[iRow2].m_Cells[iCol + 1].m_iFinalIcon != iIcon2)
						{
							bSetFinalIcon = true;
							iOutRow = iRow2;
							iOutCol = iCol + 1;
							iOutIcon = iIcon2;
							return true;
						}
					}
				}
				else if (!P.m_Rows[iRow3].m_Cells[iCol - 2].m_bValues[iIcon3] && !bNot1 && !bNot2)
				{
					if (P.m_Rows[iRow2].m_Cells[iCol + 1].m_iFinalIcon != iIcon2)
					{
						bSetFinalIcon = true;
						iOutRow = iRow2;
						iOutCol = iCol + 1;
						iOutIcon = iIcon2;
						return true;
					}
					else if (P.m_Rows[iRow3].m_Cells[iCol + 2].m_iFinalIcon != iIcon3)
					{
						bSetFinalIcon = true;
						iOutRow = iRow3;
						iOutCol = iCol + 2;
						iOutIcon = iIcon3;
						return true;
					}
				}
				else if (!P.m_Rows[iRow3].m_Cells[iCol + 2].m_bValues[iIcon3] && !bNot1 && !bNot2)
				{
					if (P.m_Rows[iRow2].m_Cells[iCol - 1].m_iFinalIcon != iIcon2)
					{
						bSetFinalIcon = true;
						iOutRow = iRow2;
						iOutCol = iCol - 1;
						iOutIcon = iIcon2;
						return true;
					}
					else if (P.m_Rows[iRow3].m_Cells[iCol - 2].m_iFinalIcon != iIcon3)
					{
						bSetFinalIcon = true;
						iOutRow = iRow3;
						iOutCol = iCol - 2;
						iOutIcon = iIcon3;
						return true;
					}
				}
				else if (!bNot1)
				{
					for (int j = 0; j < P.m_iSize; j++)
					{
						if (!bNot2 && j != (iCol - 1) && j != (iCol + 1))
						{
							if (P.m_Rows[iRow2].m_Cells[j].m_bValues[iIcon2])
							{
								bSetFinalIcon = false;
								iOutRow = iRow2;
								iOutCol = j;
								iOutIcon = iIcon2;
								return true;
							}
						}
						if (j != (iCol - 2) && j != (iCol + 2))
						{
							if (P.m_Rows[iRow3].m_Cells[j].m_bValues[iIcon3])
							{
								bSetFinalIcon = false;
								iOutRow = iRow3;
								iOutCol = j;
								iOutIcon = iIcon3;
								return true;
							}
						}
					}
				}
			}
		}
	}
	else if (iFinal1 >= 0 && !bNot1 && !bNot2 && !bNot3)
	{
		int iFinal3 = P.m_Rows[iRow3].m_Cells[iCol].m_iFinalIcon;
		if (iFinal3 != iIcon3 && iFinal3 >= 0)
		{
			if (iCol == 0)
			{
				if (P.m_Rows[iRow2].m_Cells[iCol + 1].m_bValues[iIcon2])
				{
					bSetFinalIcon = false;
					iOutRow = iRow2;
					iOutCol = iCol + 1;
					iOutIcon = iIcon2;
					return true;
				}
			}
			else if (iCol == P.m_iSize - 1)
			{
				if (P.m_Rows[iRow2].m_Cells[iCol - 1].m_bValues[iIcon2])
				{
					bSetFinalIcon = false;
					iOutRow = iRow2;
					iOutCol = iCol - 1;
					iOutIcon = iIcon2;
					return true;
				}
			}
			else if (iCol == P.m_iSize - 3)
			{
				if (P.m_Rows[iRow1].m_Cells[iCol + 2].m_bValues[iIcon1])
				{
					bSetFinalIcon = false;
					iOutRow = iRow1;
					iOutCol = iCol + 2;
					iOutIcon = iIcon1;
					return true;
				}
				if (P.m_Rows[iRow2].m_Cells[iCol + 1].m_bValues[iIcon2])
				{
					bSetFinalIcon = false;
					iOutRow = iRow2;
					iOutCol = iCol + 1;
					iOutIcon = iIcon2;
					return true;
				}
				if (P.m_Rows[iRow3].m_Cells[iCol + 2].m_bValues[iIcon3])
				{
					bSetFinalIcon = false;
					iOutRow = iRow3;
					iOutCol = iCol + 2;
					iOutIcon = iIcon3;
					return true;
				}
			}
			else if (iCol == 2)
			{
				if (P.m_Rows[iRow1].m_Cells[iCol - 2].m_bValues[iIcon1])
				{
					bSetFinalIcon = false;
					iOutRow = iRow1;
					iOutCol = iCol - 2;
					iOutIcon = iIcon1;
					return true;
				}
				if (P.m_Rows[iRow2].m_Cells[iCol - 1].m_bValues[iIcon2])
				{
					bSetFinalIcon = false;
					iOutRow = iRow2;
					iOutCol = iCol - 1;
					iOutIcon = iIcon2;
					return true;
				}
				if (P.m_Rows[iRow3].m_Cells[iCol - 2].m_bValues[iIcon3])
				{
					bSetFinalIcon = false;
					iOutRow = iRow3;
					iOutCol = iCol - 2;
					iOutIcon = iIcon3;
					return true;
				}
			}
		}
	}
	if (!P.m_Rows[iRow1].m_Cells[iCol].m_bValues[iIcon1] && !bNot1)
	{
		if (!bNot3)
		{
			if (iCol + 4 < P.m_iSize)
			{
				if (!P.m_Rows[iRow1].m_Cells[iCol + 4].m_bValues[iIcon1])
				{
					if (P.m_Rows[iRow3].m_Cells[iCol + 2].m_bValues[iIcon3])
					{
						bSetFinalIcon = false;
						iOutRow = iRow3;
						iOutCol = iCol + 2;
						iOutIcon = iIcon3;
						return true;
					}
				}
			}
			else if (iCol + 2 < P.m_iSize)
			{
				if (P.m_Rows[iRow3].m_Cells[iCol + 2].m_bValues[iIcon3])
				{
					bSetFinalIcon = false;
					iOutRow = iRow3;
					iOutCol = iCol + 2;
					iOutIcon = iIcon3;
					return true;
				}
			}
			if (iCol - 4 >= 0)
			{
				if (!P.m_Rows[iRow1].m_Cells[iCol - 4].m_bValues[iIcon1])
				{
					if (P.m_Rows[iRow3].m_Cells[iCol - 2].m_bValues[iIcon3])
					{
						bSetFinalIcon = false;
						iOutRow = iRow3;
						iOutCol = iCol - 2;
						iOutIcon = iIcon3;
						return true;
					}
				}
			}
			else if (iCol - 2 >= 0)
			{
				if (P.m_Rows[iRow3].m_Cells[iCol - 2].m_bValues[iIcon3])
				{
					bSetFinalIcon = false;
					iOutRow = iRow3;
					iOutCol = iCol - 2;
					iOutIcon = iIcon3;
					return true;
				}
			}
		}
		if (!bNot2)
		{
			if (iCol + 2 < P.m_iSize)
			{
				if (!P.m_Rows[iRow1].m_Cells[iCol + 2].m_bValues[iIcon1])
				{
					if (P.m_Rows[iRow2].m_Cells[iCol + 1].m_bValues[iIcon2])
					{
						bSetFinalIcon = false;
						iOutRow = iRow2;
						iOutCol = iCol + 1;
						iOutIcon = iIcon2;
						return true;
					}
				}
			}
			if (iCol - 2 >= 0)
			{
				if (!P.m_Rows[iRow1].m_Cells[iCol - 2].m_bValues[iIcon1])
				{
					if (P.m_Rows[iRow2].m_Cells[iCol - 1].m_bValues[iIcon2])
					{
						bSetFinalIcon = false;
						iOutRow = iRow2;
						iOutCol = iCol - 1;
						iOutIcon = iIcon2;
						return true;
					}
				}
			}
		}
	}
	if (P.m_Rows[iRow2].m_Cells[iCol].m_iFinalIcon == iIcon2 && !bNot2)
	{
		// Middle icon is known, eliminate impossible end icons
		for (int i = 0; i < P.m_iSize; i++)
		{
			if (i != iCol - 1 && i != iCol + 1)
			{
				if (!bNot1)
				{
					if (P.m_Rows[iRow1].m_Cells[i].m_bValues[iIcon1])
					{
						bSetFinalIcon = false;
						iOutRow = iRow1;
						iOutCol = i;
						iOutIcon = iIcon1;
						return true;
					}
				}
				if (!bNot3)
				{
					if (P.m_Rows[iRow3].m_Cells[i].m_bValues[iIcon3])
					{
						bSetFinalIcon = false;
						iOutRow = iRow3;
						iOutCol = i;
						iOutIcon = iIcon3;
						return true;
					}
				}
			}
		}
	}

	bSetFinalIcon = false;
	iOutRow = -1;
	iOutCol = -1;
	iOutIcon = -1;
	return false;
}

bool UClue::GetHintActionHorizontalSpanNotLeft(UPuzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = m_iHorizontal1;
	int iIcon2 = P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol2];
	int iIcon3 = P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol3];

	// Icon2 cant be on either end
	if (P.m_Rows[m_iRow2].m_Cells[0].m_bValues[iIcon2])
	{
		bSetFinalIcon = false;
		iRow = m_iRow2;
		iCol = 0;
		iIcon = iIcon2;
		return true;
	}
	else if (P.m_Rows[m_iRow2].m_Cells[P.m_iSize - 1].m_bValues[iIcon2])
	{
		bSetFinalIcon = false;
		iRow = m_iRow2;
		iCol = P.m_iSize - 1;
		iIcon = iIcon2;
		return true;
	}


	for (int i = 0; i < P.m_iSize; i++)
	{
		if (GetHintActionSpan(i, m_iRow, iIcon1, true, m_iRow2, iIcon2, false, m_iRow3, iIcon3, false, P, bSetFinalIcon, iRow, iCol, iIcon))
			return true;
		if (GetHintActionSpan(i, m_iRow3, iIcon3, false, m_iRow2, iIcon2, false, m_iRow, iIcon1, true, P, bSetFinalIcon, iRow, iCol, iIcon))
			return true;
	}

	bSetFinalIcon = false;
	iRow = -1;
	iCol = -1;
	iIcon = -1;
	return false;
}

bool UClue::GetHintActionHorizontalSpanNotMid(UPuzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = m_iHorizontal1;
	int iIcon3 = P.m_Solution[(m_iRow3 * P.m_iSize) + m_iCol3];

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (GetHintActionSpan(i, m_iRow, iIcon1, false, m_iRow2, iIcon2, true, m_iRow3, iIcon3, false, P, bSetFinalIcon, iRow, iCol, iIcon))
			return true;
		if (GetHintActionSpan(i, m_iRow3, iIcon3, false, m_iRow2, iIcon2, true, m_iRow, iIcon1, false, P, bSetFinalIcon, iRow, iCol, iIcon))
			return true;
	}

	bSetFinalIcon = false;
	iRow = -1;
	iCol = -1;
	iIcon = -1;
	return false;
}

bool UClue::GetHintActionHorizontalSpanNotRight(UPuzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[(m_iRow * P.m_iSize) + m_iCol];
	int iIcon2 = P.m_Solution[(m_iRow2 * P.m_iSize) + m_iCol2];
	int iIcon3 = m_iHorizontal1;

	// Icon2 cant be on either end
	if (P.m_Rows[m_iRow2].m_Cells[0].m_bValues[iIcon2])
	{
		bSetFinalIcon = false;
		iRow = m_iRow2;
		iCol = 0;
		iIcon = iIcon2;
		return true;
	}
	else if (P.m_Rows[m_iRow2].m_Cells[P.m_iSize - 1].m_bValues[iIcon2])
	{
		bSetFinalIcon = false;
		iRow = m_iRow2;
		iCol = P.m_iSize - 1;
		iIcon = iIcon2;
		return true;
	}

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (GetHintActionSpan(i, m_iRow, iIcon1, false, m_iRow2, iIcon2, false, m_iRow3, iIcon3, true, P, bSetFinalIcon, iRow, iCol, iIcon))
			return true;
		if (GetHintActionSpan(i, m_iRow3, iIcon3, true, m_iRow2, iIcon2, false, m_iRow, iIcon1, false, P, bSetFinalIcon, iRow, iCol, iIcon))
			return true;
	}

	bSetFinalIcon = false;
	iRow = -1;
	iCol = -1;
	iIcon = -1;
	return false;
}

FString UClue::ToString() const
{
	FString ClueString;

	switch (m_Type) 
	{
		case eClueType::Given:		return FString::Printf(TEXT("Given: (%d, %d)"), m_iRow, m_iCol);
		case eClueType::NotHere:	return FString::Printf(TEXT("NotHere: (%d, %d) not %d"), m_iRow, m_iCol, m_iHorizontal1);
		case eClueType::Horizontal: return HorizontalToString();
		case eClueType::Vertical:	return VerticalToString();
	}

	return ClueString;
}

FString UClue::HorizontalToString() const
{
	FString ClueString = "Horizontal: ";

	switch (m_HorizontalType)
	{
		case eHorizontalType::NextTo:		ClueString += FString::Printf(TEXT("Next To: (%d, %d) <-> (%d, %d)"), m_iRow, m_iCol, m_iRow2, m_iCol2); break;
		case eHorizontalType::LeftOf:		ClueString += FString::Printf(TEXT("LeftOf: (%d, %d) <-- (%d, %d)"), m_iRow, m_iCol, m_iRow2, m_iCol2); break;
		case eHorizontalType::NotLeftOf:	ClueString += FString::Printf(TEXT("NotLeftOf: (%d, %d) !<-- (%d, %d)"), m_iRow, m_iCol, m_iRow2, m_iCol2); break;
		case eHorizontalType::NotNextTo:	ClueString += FString::Printf(TEXT("NotNtextTo: (%d, %d) !<-> (%d, %d)"), m_iRow, m_iCol, m_iRow2, m_iHorizontal1); break;
		case eHorizontalType::Span:			ClueString += FString::Printf(TEXT("Span: (%d, %d) < (%d, %d) > (%d, %d)"), m_iRow, m_iCol, m_iRow2, m_iCol2, m_iRow3, m_iCol3); break;
		case eHorizontalType::SpanNotLeft:	ClueString += FString::Printf(TEXT("SpanNotLeft: !(%d, %d) < (%d, %d) > (%d, %d) %d"), m_iRow, m_iCol, m_iRow2, m_iCol2, m_iRow3, m_iRow3, m_iHorizontal1); break;
		case eHorizontalType::SpanNotMid:	ClueString += FString::Printf(TEXT("SpanNotMid: (%d, %d) < !(%d, %d) > (%d, %d) %d"), m_iRow, m_iCol, m_iRow2, m_iCol2, m_iRow3, m_iRow3, m_iHorizontal1); break;
		case eHorizontalType::SpanNotRight: ClueString += FString::Printf(TEXT("SpanNotRight: (%d, %d) < (%d, %d) > !(%d, %d) %d)"), m_iRow, m_iCol, m_iRow2, m_iCol2, m_iRow3, m_iRow3, m_iHorizontal1); break;
		case eHorizontalType::Edge:
		case eHorizontalType::NotEdge:
		case eHorizontalType::DirectlyLeftOf:
		case eHorizontalType::Gap:
		case eHorizontalType::Between:
		case eHorizontalType::Chain:
		case eHorizontalType::NextToEitherOr:
		case eHorizontalType::AllApart:
			return ExtendedToString();
	}
	return ClueString;
}

FString UClue::VerticalToString() const
{
	FString ClueString = "Vertical: ";

	switch (m_VerticalType)
	{
		case eVerticalType::Two:			ClueString += FString::Printf(TEXT("Two: (%d) (%d)"), m_iRow, m_iRow2); break;
		case eVerticalType::Three:			ClueString += FString::Printf(TEXT("Three: (%d) (%d) (%d)"), m_iRow, m_iRow2, m_iRow3); break;
		case eVerticalType::TwoNot:			ClueString += FString::Printf(TEXT("TwoNot: (%d) (%d) Not: (%d)"), m_iRow, m_iRow2, m_iNotCell); break;
		case eVerticalType::EitherOr:		ClueString += FString::Printf(TEXT("EitherOr: (%d) (%d) (%d) (%d)"), m_iRow, m_iRow2, m_iRow3, m_iNotCell); break;
		case eVerticalType::ThreeTopNot:	ClueString += FString::Printf(TEXT("ThreeTopNot: (%d) (%d) (%d) (%d)"), m_iRow, m_iRow2, m_iRow3, m_iNotCell); break;
		case eVerticalType::ThreeMidNot:	ClueString += FString::Printf(TEXT("ThreeMidNot: (%d) (%d) (%d) (%d)"), m_iRow, m_iRow2, m_iRow3, m_iNotCell); break;
		case eVerticalType::ThreeBotNot:	ClueString += FString::Printf(TEXT("ThreeBotNot: (%d) (%d) (%d) (%d)"), m_iRow, m_iRow2, m_iRow3, m_iNotCell); break;
	}
	return ClueString;
}

void UClue::GetRows(TArray<int>& Rows)
{
	Rows.Add(m_iRow);
	Rows.Add(m_iRow2);
	Rows.Add(m_iRow3);
}

void UClue::GetIcons(UPuzzle* P, TArray<int32>& Icons)
{
	Icons.Reset();

	switch (m_Type)
	{
		case eClueType::Given:
			break;

		case eClueType::NotHere:
			Icons.Add(m_iHorizontal1);
			break;

		case eClueType::Vertical:
		{
			switch (m_VerticalType)
			{
				case eVerticalType::Two:
					Icons.Add(P->m_Solution[(m_iRow * P->m_iSize) + m_iCol]);
					Icons.Add(P->m_Solution[(m_iRow2 * P->m_iSize) + m_iCol]);
					break;

				case eVerticalType::Three:
					Icons.Add(P->m_Solution[(m_iRow * P->m_iSize) + m_iCol]);
					Icons.Add(P->m_Solution[(m_iRow2 * P->m_iSize) + m_iCol]);
					Icons.Add(P->m_Solution[(m_iRow3 * P->m_iSize) + m_iCol]);
					break;

				case eVerticalType::EitherOr:
					Icons.Add(P->m_Solution[(m_iRow * P->m_iSize) + m_iCol]);
					Icons.Add((m_iRow2 == m_iNotCell) ? m_iHorizontal1 : P->m_Solution[(m_iRow2 * P->m_iSize) + m_iCol]);
					Icons.Add((m_iRow3 == m_iNotCell) ? m_iHorizontal1 : P->m_Solution[(m_iRow3 * P->m_iSize) + m_iCol]);
					break;

				case eVerticalType::TwoNot:
					Icons.Add(P->m_Solution[(m_iRow * P->m_iSize) + m_iCol]);
					Icons.Add(m_iNotCell);
					break;

				case eVerticalType::ThreeTopNot:
					Icons.Add(m_iNotCell);
					Icons.Add(P->m_Solution[(m_iRow2 * P->m_iSize) + m_iCol]);
					Icons.Add(P->m_Solution[(m_iRow3 * P->m_iSize) + m_iCol]);
					break;

				case eVerticalType::ThreeMidNot:
					Icons.Add(P->m_Solution[(m_iRow * P->m_iSize) + m_iCol]);
					Icons.Add(m_iNotCell);
					Icons.Add(P->m_Solution[(m_iRow3 * P->m_iSize) + m_iCol]);
					break;

				case eVerticalType::ThreeBotNot:
					Icons.Add(P->m_Solution[(m_iRow * P->m_iSize) + m_iCol]);
					Icons.Add(P->m_Solution[(m_iRow2 * P->m_iSize) + m_iCol]);
					Icons.Add(m_iNotCell);
					break;
			}
		}
		break;

		case eClueType::Horizontal:
		{
			switch (m_HorizontalType)
			{
			case eHorizontalType::NextTo:
				Icons.Add(P->m_Solution[(m_iRow * P->m_iSize) + m_iCol]);
				Icons.Add(P->m_Solution[(m_iRow2 * P->m_iSize) + m_iCol2]);
				Icons.Add(P->m_Solution[(m_iRow * P->m_iSize) + m_iCol]);
				break;

			case eHorizontalType::NotNextTo:
				Icons.Add(P->m_Solution[(m_iRow * P->m_iSize) + m_iCol]);
				Icons.Add(m_iHorizontal1);
				Icons.Add(P->m_Solution[(m_iRow * P->m_iSize) + m_iCol]);
				break;

			case eHorizontalType::LeftOf:
			case eHorizontalType::NotLeftOf:
				Icons.Add(P->m_Solution[(m_iRow * P->m_iSize) + m_iCol]);
				Icons.Add(P->m_Solution[(m_iRow2 * P->m_iSize) + m_iCol2]);
				break;

			case eHorizontalType::Span:
				Icons.Add(P->m_Solution[(m_iRow * P->m_iSize) + m_iCol]);
				Icons.Add(P->m_Solution[(m_iRow2 * P->m_iSize) + m_iCol2]);
				Icons.Add(P->m_Solution[(m_iRow3 * P->m_iSize) + m_iCol3]);
				break;

			case eHorizontalType::SpanNotLeft:
				Icons.Add(m_iHorizontal1);
				Icons.Add(P->m_Solution[(m_iRow2 * P->m_iSize) + m_iCol2]);
				Icons.Add(P->m_Solution[(m_iRow3 * P->m_iSize) + m_iCol3]);
				break;

			case eHorizontalType::SpanNotMid:
				Icons.Add(P->m_Solution[(m_iRow * P->m_iSize) + m_iCol]);
				Icons.Add(m_iHorizontal1);
				Icons.Add(P->m_Solution[(m_iRow3 * P->m_iSize) + m_iCol3]);
				break;

			case eHorizontalType::SpanNotRight:
				Icons.Add(P->m_Solution[(m_iRow * P->m_iSize) + m_iCol]);
				Icons.Add(P->m_Solution[(m_iRow2 * P->m_iSize) + m_iCol2]);
				Icons.Add(m_iHorizontal1);
				break;


			case eHorizontalType::Edge:
			case eHorizontalType::NotEdge:
				Icons.Add(P->m_Solution[(m_iRow * P->m_iSize) + m_iCol]);
				break;

			case eHorizontalType::DirectlyLeftOf:
				Icons.Add(P->m_Solution[(m_iRow * P->m_iSize) + m_iCol]);
				Icons.Add(P->m_Solution[(m_iRow2 * P->m_iSize) + m_iCol2]);
				break;

			case eHorizontalType::Gap:
				Icons.Add(P->m_Solution[(m_iRow * P->m_iSize) + m_iCol]);
				Icons.Add(P->m_Solution[(m_iRow2 * P->m_iSize) + m_iCol2]);
				break;

			case eHorizontalType::Between:
			case eHorizontalType::Chain:
			case eHorizontalType::AllApart:
				Icons.Add(P->m_Solution[(m_iRow * P->m_iSize) + m_iCol]);
				Icons.Add(P->m_Solution[(m_iRow2 * P->m_iSize) + m_iCol2]);
				Icons.Add(P->m_Solution[(m_iRow3 * P->m_iSize) + m_iCol3]);
				break;

			case eHorizontalType::NextToEitherOr:
				// Slot 1 is the subject; slot m_iNotCell (0 or 2) is the false option, shown as icon m_iHorizontal1
				Icons.Add((m_iNotCell == 0) ? m_iHorizontal1 : P->m_Solution[(m_iRow * P->m_iSize) + m_iCol]);
				Icons.Add(P->m_Solution[(m_iRow2 * P->m_iSize) + m_iCol2]);
				Icons.Add((m_iNotCell == 2) ? m_iHorizontal1 : P->m_Solution[(m_iRow3 * P->m_iSize) + m_iCol3]);
				break;
			}
		}
		break;
	}
}

void UClue::GenerateClueHelp(UPuzzle& P)
{
	TArray<int> Rows;
	TArray<int32> Icons;
	GetRows(Rows);
	GetIcons(&P, Icons);
	ClueHelp.Segments.Empty();
	switch (m_Type)
	{
		case eClueType::NotHere:
			ClueHelp.AddIcon(m_iRow, m_iHorizontal1);
			ClueHelp.AddText(TEXT("is not in this cell"));
			break;
		case eClueType::Horizontal:
			switch (m_HorizontalType)
			{
				case eHorizontalType::Edge:
					ClueHelp.AddIcon(Rows[0], Icons[0]);
					ClueHelp.AddText(TEXT("is in the first or last column"));
					break;
				case eHorizontalType::NotEdge:
					ClueHelp.AddIcon(Rows[0], Icons[0]);
					ClueHelp.AddText(TEXT("is not in the first or last column"));
					break;
				case eHorizontalType::DirectlyLeftOf:
					ClueHelp.AddIcon(Rows[0], Icons[0]);
					ClueHelp.AddText(TEXT("is directly left of"));
					ClueHelp.AddIcon(Rows[1], Icons[1]);
					break;
				case eHorizontalType::Gap:
					ClueHelp.AddIcon(Rows[0], Icons[0]);
					ClueHelp.AddText(TEXT("and"));
					ClueHelp.AddIcon(Rows[1], Icons[1]);
					ClueHelp.AddText(TEXT("have exactly one column between them"));
					break;
				case eHorizontalType::Between:
					ClueHelp.AddIcon(Rows[1], Icons[1]);
					ClueHelp.AddText(TEXT("is somewhere between"));
					ClueHelp.AddIcon(Rows[0], Icons[0]);
					ClueHelp.AddText(TEXT("and"));
					ClueHelp.AddIcon(Rows[2], Icons[2]);
					break;
				case eHorizontalType::Chain:
					ClueHelp.AddIcon(Rows[0], Icons[0]);
					ClueHelp.AddText(TEXT("is left of"));
					ClueHelp.AddIcon(Rows[1], Icons[1]);
					ClueHelp.AddText(TEXT(", which is left of"));
					ClueHelp.AddIcon(Rows[2], Icons[2]);
					break;
				case eHorizontalType::NextToEitherOr:
					ClueHelp.AddIcon(Rows[1], Icons[1]);
					ClueHelp.AddText(TEXT("is next to either"));
					ClueHelp.AddIcon(Rows[0], Icons[0]);
					ClueHelp.AddText(TEXT("or"));
					ClueHelp.AddIcon(Rows[2], Icons[2]);
					ClueHelp.AddText(TEXT(", but not both"));
					break;
				case eHorizontalType::AllApart:
					ClueHelp.AddIcon(Rows[0], Icons[0]);
					ClueHelp.AddText(TEXT(","));
					ClueHelp.AddIcon(Rows[1], Icons[1]);
					ClueHelp.AddText(TEXT("and"));
					ClueHelp.AddIcon(Rows[2], Icons[2]);
					ClueHelp.AddText(TEXT("are all in different columns"));
					break;
				case eHorizontalType::NextTo:					
					ClueHelp.AddIcon(Rows[0], Icons[0]);
					ClueHelp.AddText(TEXT("is next to"));
					ClueHelp.AddIcon(Rows[1], Icons[1]);					
					break;
				case eHorizontalType::NotNextTo:
					ClueHelp.AddIcon(Rows[0], Icons[0]);
					ClueHelp.AddText(TEXT("is not next to"));
					ClueHelp.AddIcon(Rows[1], Icons[1]);
					break;
				case eHorizontalType::LeftOf:
					ClueHelp.AddIcon(Rows[0], Icons[0]);
					ClueHelp.AddText(TEXT("is left of"));
					ClueHelp.AddIcon(Rows[1], Icons[1]);
					break;
				case eHorizontalType::NotLeftOf:
					ClueHelp.AddIcon(Rows[0], Icons[0]);
					ClueHelp.AddText(TEXT("is not left of"));
					ClueHelp.AddIcon(Rows[1], Icons[1]);
					break;
				case eHorizontalType::Span:
					ClueHelp.AddIcon(Rows[1], Icons[1]);
					ClueHelp.AddText(TEXT("has"));
					ClueHelp.AddIcon(Rows[0], Icons[0]);
					ClueHelp.AddText(TEXT("next to it on one side, and"));
					ClueHelp.AddIcon(Rows[2], Icons[2]);
					ClueHelp.AddText(TEXT("next to it on the other"));
					break;
				case eHorizontalType::SpanNotLeft:
					ClueHelp.AddIcon(Rows[1], Icons[1]);
					ClueHelp.AddText(TEXT("has"));
					ClueHelp.AddIcon(Rows[2], Icons[2]);
					ClueHelp.AddText(TEXT("next to it on one side, and not"));
					ClueHelp.AddIcon(Rows[0], Icons[0]);
					ClueHelp.AddText(TEXT("next to it on the other"));
					break;
				case eHorizontalType::SpanNotMid:
					ClueHelp.AddIcon(Rows[0], Icons[0]);
					ClueHelp.AddText(TEXT("and"));
					ClueHelp.AddIcon(Rows[2], Icons[2]);
					ClueHelp.AddText(TEXT("have one column between them without"));
					ClueHelp.AddIcon(Rows[1], Icons[1]);
					break;
				case eHorizontalType::SpanNotRight:
					ClueHelp.AddIcon(Rows[1], Icons[1]);
					ClueHelp.AddText(TEXT("has"));
					ClueHelp.AddIcon(Rows[0], Icons[0]);
					ClueHelp.AddText(TEXT("on one side, and not"));
					ClueHelp.AddIcon(Rows[2], Icons[2]);
					ClueHelp.AddText(TEXT("on the other"));
					break;
			}
			break;
		case eClueType::Vertical:
			switch (m_VerticalType)
			{
			case eVerticalType::Two:
				ClueHelp.AddIcon(Rows[0], Icons[0]);
				ClueHelp.AddText(TEXT("is in the same column as"));
				ClueHelp.AddIcon(Rows[1], Icons[1]);
				break;
			case eVerticalType::Three:
				ClueHelp.AddIcon(Rows[0], Icons[0]);
				ClueHelp.AddText(TEXT("is in the same column as"));
				ClueHelp.AddIcon(Rows[1], Icons[1]);
				ClueHelp.AddText(TEXT("and"));
				ClueHelp.AddIcon(Rows[2], Icons[2]);
				break;
			case eVerticalType::EitherOr:
				ClueHelp.AddIcon(Rows[0], Icons[0]);
				ClueHelp.AddText(TEXT("is either in the column with"));
				ClueHelp.AddIcon(Rows[1], Icons[1]);
				ClueHelp.AddText(TEXT("or the column with"));
				ClueHelp.AddIcon(Rows[2], Icons[2]);
				break;
			case eVerticalType::TwoNot:
				ClueHelp.AddIcon(Rows[0], Icons[0]);
				ClueHelp.AddText(TEXT("is not in the same column as"));
				ClueHelp.AddIcon(Rows[1], Icons[1]);
				break;
			case eVerticalType::ThreeTopNot:
				ClueHelp.AddIcon(Rows[1], Icons[1]);
				ClueHelp.AddText(TEXT("and"));
				ClueHelp.AddIcon(Rows[2], Icons[2]);
				ClueHelp.AddText(TEXT("are in the same column but"));
				ClueHelp.AddIcon(Rows[0], Icons[0]);
				ClueHelp.AddText(TEXT("is not"));				
				break;
			case eVerticalType::ThreeMidNot:
				ClueHelp.AddIcon(Rows[0], Icons[0]);
				ClueHelp.AddText(TEXT("and"));
				ClueHelp.AddIcon(Rows[2], Icons[2]);
				ClueHelp.AddText(TEXT("are in the same column but"));
				ClueHelp.AddIcon(Rows[1], Icons[1]);
				ClueHelp.AddText(TEXT("is not"));				
				break;
			case eVerticalType::ThreeBotNot:
				ClueHelp.AddIcon(Rows[0], Icons[0]);
				ClueHelp.AddText(TEXT("and"));
				ClueHelp.AddIcon(Rows[1], Icons[1]);
				ClueHelp.AddText(TEXT("are in the same column but"));
				ClueHelp.AddIcon(Rows[2], Icons[2]);
				ClueHelp.AddText(TEXT("is not"));
				break;
			}
			break;
	}
}

int64 UClue::GetId() const
{
	uint64 id = 0;

	// Helper to pack value: 0-7 → 3 bits, -1 becomes 0b1000 (flag)
	auto PackValue = [](int32 val) -> uint64
		{
			if (val == -1)
				return 0b1000ULL;                    // flag = 1, value = 0
			else
				return static_cast<uint64>(val & 0b0111); // 3 bits value + flag 0
		};

	// Helper to pack enum (0-7) → 3 bits, no flag needed
	auto PackEnum = [](uint8 enumVal) -> uint64
		{
			return static_cast<uint64>(enumVal & 0b0111);
		};

	// Pack the 8 integer fields (4 bits each)
	id |= PackValue(m_iRow) << 0;
	id |= PackValue(m_iRow2) << 4;
	id |= PackValue(m_iRow3) << 8;
	id |= PackValue(m_iCol) << 12;
	id |= PackValue(m_iCol2) << 16;
	id |= PackValue(m_iCol3) << 20;
	id |= PackValue(m_iHorizontal1) << 24;
	id |= PackValue(m_iNotCell) << 28;

	// Pack the 3 enums (3 bits each) starting at bit 32
	id |= PackEnum(static_cast<uint8>(m_Type)) << 32;
	id |= PackEnum(static_cast<uint8>(m_VerticalType)) << 35;
	id |= PackEnum(static_cast<uint8>(m_HorizontalType)) << 38;

	// Horizontal types above 7 need a 4th bit; it lives at bit 41 so IDs saved before it existed decode unchanged
	id |= static_cast<uint64>((static_cast<uint8>(m_HorizontalType) >> 3) & 0b1) << 41;

	return static_cast<int64>(id);
}

void UClue::SetFromId(int64 sid)
{
	uint64 id = static_cast<uint64>(sid);
	// Helper to unpack the value fields (4 bits)
	auto UnpackValue = [](uint64 packed) -> int32
		{
			if (packed & 0b1000)           // flag bit set
				return -1;
			else
				return static_cast<int32>(packed & 0b0111);
		};

	// Helper to unpack enum (3 bits)
	auto UnpackEnum = [](uint64 packed) -> uint8
		{
			return static_cast<uint8>(packed & 0b0111);
		};

	// Unpack the 8 integer fields
	m_iRow = UnpackValue((id >> 0) & 0xF);
	m_iRow2 = UnpackValue((id >> 4) & 0xF);
	m_iRow3 = UnpackValue((id >> 8) & 0xF);
	m_iCol = UnpackValue((id >> 12) & 0xF);
	m_iCol2 = UnpackValue((id >> 16) & 0xF);
	m_iCol3 = UnpackValue((id >> 20) & 0xF);
	m_iHorizontal1 = UnpackValue((id >> 24) & 0xF);
	m_iNotCell = UnpackValue((id >> 28) & 0xF);

	// Unpack the 3 enums
	m_Type = static_cast<eClueType>(UnpackEnum((id >> 32) & 0x7));
	m_VerticalType = static_cast<eVerticalType>(UnpackEnum((id >> 35) & 0x7));
	m_HorizontalType = static_cast<eHorizontalType>(UnpackEnum((id >> 38) & 0x7) | (((id >> 41) & 0x1) << 3));
}
ECampaignLesson UClue::GetCampaignLesson() const
{
	switch (m_Type)
	{
		case eClueType::Vertical:
			switch (m_VerticalType)
			{
				case eVerticalType::Two:			return ECampaignLesson::VerticalTwo;
				case eVerticalType::Three:			return ECampaignLesson::VerticalThree;
				case eVerticalType::TwoNot:			return ECampaignLesson::TwoNot;
				case eVerticalType::ThreeTopNot:
				case eVerticalType::ThreeMidNot:
				case eVerticalType::ThreeBotNot:	return ECampaignLesson::ThreeNot;
				case eVerticalType::EitherOr:		return ECampaignLesson::EitherOr;
			}
			break;

		case eClueType::Horizontal:
			switch (m_HorizontalType)
			{
				case eHorizontalType::NextTo:		return ECampaignLesson::NextTo;
				case eHorizontalType::Span:			return ECampaignLesson::Span;
				case eHorizontalType::NotNextTo:	return ECampaignLesson::NotNextTo;
				case eHorizontalType::SpanNotLeft:
				case eHorizontalType::SpanNotRight:	return ECampaignLesson::SpanNotSide;
				case eHorizontalType::SpanNotMid:	return ECampaignLesson::SpanNotMid;
				case eHorizontalType::LeftOf:		return ECampaignLesson::LeftOf;
				case eHorizontalType::NotLeftOf:	return ECampaignLesson::NotLeftOf;
				case eHorizontalType::Edge:
				case eHorizontalType::NotEdge:		return ECampaignLesson::Edge;
				case eHorizontalType::DirectlyLeftOf:	return ECampaignLesson::DirectlyLeftOf;
				case eHorizontalType::Gap:			return ECampaignLesson::Gap;
				case eHorizontalType::Between:		return ECampaignLesson::Between;
				case eHorizontalType::Chain:		return ECampaignLesson::Chain;
				case eHorizontalType::NextToEitherOr:	return ECampaignLesson::NextToEitherOr;
				case eHorizontalType::AllApart:		return ECampaignLesson::AllApart;
			}
			break;
	}

	return ECampaignLesson::Given;
}

bool UClue::IsConstraintClue() const
{
	if (m_Type == eClueType::Horizontal)
		return m_HorizontalType >= eHorizontalType::Edge;

	return false;
}

void UClue::GetSlots(const UPuzzle& P, int Rows[3], int Icons[3]) const
{
	const int SlotRows[3] = { m_iRow, m_iRow2, m_iRow3 };
	const int SlotCols[3] = { m_iCol, m_iCol2, m_iCol3 };

	for (int i = 0; i < 3; i++)
	{
		Rows[i] = SlotRows[i];

		if (SlotRows[i] < 0)
			Icons[i] = -1;
		else if (m_Type == eClueType::Horizontal && m_HorizontalType == eHorizontalType::NextToEitherOr && m_iNotCell == i)
			Icons[i] = m_iHorizontal1;
		else
			Icons[i] = P.m_Solution[(SlotRows[i] * P.m_iSize) + SlotCols[i]];
	}
}

bool UClue::IsSameClue(const UClue& Other) const
{
	if (m_Type != Other.m_Type)
		return false;
	if (m_Type == eClueType::Vertical && m_VerticalType != Other.m_VerticalType)
		return false;
	if (m_Type == eClueType::Horizontal && m_HorizontalType != Other.m_HorizontalType)
		return false;

	if (!IsConstraintClue())
	{
		return m_iRow == Other.m_iRow && m_iRow2 == Other.m_iRow2 && m_iRow3 == Other.m_iRow3 &&
			m_iCol == Other.m_iCol && m_iCol2 == Other.m_iCol2 && m_iCol3 == Other.m_iCol3 &&
			m_iHorizontal1 == Other.m_iHorizontal1 && m_iNotCell == Other.m_iNotCell;
	}

	// Constraint clues: compare slots as (row, column, false-option icon), with the slots whose order
	// doesn't matter sorted, so e.g. Gap A..C and Gap C..A count as the same clue
	auto CanonicalSlots = [](const UClue& Clue)
	{
		const int Rows[3] = { Clue.m_iRow, Clue.m_iRow2, Clue.m_iRow3 };
		const int Cols[3] = { Clue.m_iCol, Clue.m_iCol2, Clue.m_iCol3 };
		TArray<FIntVector> Slots;
		for (int i = 0; i < 3; i++)
		{
			const bool bFalseOption = Clue.m_HorizontalType == eHorizontalType::NextToEitherOr && Clue.m_iNotCell == i;
			Slots.Add(FIntVector(Rows[i], bFalseOption ? -1 : Cols[i], bFalseOption ? Clue.m_iHorizontal1 : -1));
		}

		auto Less = [](const FIntVector& A, const FIntVector& B)
		{
			return A.X != B.X ? A.X < B.X : (A.Y != B.Y ? A.Y < B.Y : A.Z < B.Z);
		};
		auto SortPair = [&Slots, &Less](int A, int B)
		{
			if (Less(Slots[B], Slots[A]))
				Slots.Swap(A, B);
		};

		switch (Clue.m_HorizontalType)
		{
			case eHorizontalType::Gap:				SortPair(0, 1); break;	// A and C either way round
			case eHorizontalType::Between:
			case eHorizontalType::NextToEitherOr:	SortPair(0, 2); break;	// the two outer slots
			case eHorizontalType::AllApart:			Slots.Sort(Less); break;
			default:								break;					// Edge, NotEdge, DirectlyLeftOf, Chain: ordered
		}
		return Slots;
	};

	return CanonicalSlots(*this) == CanonicalSlots(Other);
}

void UClue::GenerateNotHere(UPuzzle& P, FRandomStream& Rand)
{
	int iSize = P.m_iSize;

	for (int iTries = 0; iTries < 50; iTries++)
	{
		m_iRow = Rand.RandRange(0, iSize - 1);
		m_iCol = Rand.RandRange(0, iSize - 1);
		m_iHorizontal1 = Rand.RandRange(0, iSize - 1);

		// Must be a wrong icon that is still possible in the cell, otherwise it tells the player nothing
		if (m_iHorizontal1 != P.m_Solution[(m_iRow * iSize) + m_iCol] && P.m_Rows[m_iRow].m_Cells[m_iCol].m_bValues[m_iHorizontal1])
			return;
	}

	// No useful NotHere found; pick a different clue instead
	GenerateClue(P, Rand);
}

void UClue::InitGiven(UPuzzle& P, FRandomStream& Rand)
{
	m_Type = eClueType::Given;
	GenerateGiven(P, Rand);
	GenerateClueHelp(P);
}

bool UClue::GenerateHorizontalExtended(UPuzzle& P, FRandomStream& Rand)
{
	const int iSize = P.m_iSize;
	if (iSize < 3 && m_HorizontalType != eHorizontalType::Edge && m_HorizontalType != eHorizontalType::DirectlyLeftOf)
		return false;

	// On a 3 wide board Between and Chain can only use columns 0, 1 and 2, so they are just a Span,
	// NotEdge can only mean "the middle column", and Edge is too strong. The campaign lesson that
	// teaches one of them still uses it on 3x3.
	const bool bTeachingThis = P.m_bCampaign && GetCampaignLesson() == P.m_CampaignLesson;
	if (iSize < 4 && !bTeachingThis && (m_HorizontalType == eHorizontalType::Between || m_HorizontalType == eHorizontalType::Chain ||
		m_HorizontalType == eHorizontalType::Edge || m_HorizontalType == eHorizontalType::NotEdge))
		return false;

	auto Sol = [&P, iSize](int Row, int Col) { return P.m_Solution[(Row * iSize) + Col]; };

	for (int iTries = 0; iTries < 25; iTries++)
	{
		int Rows[3] = { Rand.RandRange(0, iSize - 1), -1, -1 };
		int Cols[3] = { -1, -1, -1 };
		m_iHorizontal1 = -1;
		m_iNotCell = -1;

		switch (m_HorizontalType)
		{
			case eHorizontalType::Edge:
				Cols[0] = (Rand.FRand() < 0.5f) ? 0 : iSize - 1;
				break;

			case eHorizontalType::NotEdge:
				Cols[0] = Rand.RandRange(1, iSize - 2);
				break;

			case eHorizontalType::DirectlyLeftOf:
				Cols[0] = Rand.RandRange(0, iSize - 2);
				Cols[1] = Cols[0] + 1;
				Rows[1] = Rand.RandRange(0, iSize - 1);
				break;

			case eHorizontalType::Gap:
			{
				Cols[0] = Rand.RandRange(0, iSize - 1);
				bool bCanLeft = Cols[0] - 2 >= 0;
				bool bCanRight = Cols[0] + 2 < iSize;
				if (!bCanLeft && !bCanRight)
					continue;
				Cols[1] = (bCanLeft && (!bCanRight || Rand.FRand() < 0.5f)) ? Cols[0] - 2 : Cols[0] + 2;
				Rows[1] = Rand.RandRange(0, iSize - 1);
				break;
			}

			case eHorizontalType::Between:
			case eHorizontalType::Chain:
			{
				// 3 different columns, left to right
				int Sorted[3] = { Rand.RandRange(0, iSize - 1), 0, 0 };
				do { Sorted[1] = Rand.RandRange(0, iSize - 1); } while (Sorted[1] == Sorted[0]);
				do { Sorted[2] = Rand.RandRange(0, iSize - 1); } while (Sorted[2] == Sorted[0] || Sorted[2] == Sorted[1]);
				Algo::Sort(Sorted);

				Cols[1] = Sorted[1];
				bool bFlip = m_HorizontalType == eHorizontalType::Between && Rand.FRand() < 0.5f;
				Cols[0] = bFlip ? Sorted[2] : Sorted[0];
				Cols[2] = bFlip ? Sorted[0] : Sorted[2];
				Rows[1] = Rand.RandRange(0, iSize - 1);
				Rows[2] = Rand.RandRange(0, iSize - 1);
				break;
			}

			case eHorizontalType::NextToEitherOr:
			{
				// The subject icon is the middle slot, the options are slots 0 and 2
				Rows[1] = Rows[0];
				Cols[1] = Rand.RandRange(0, iSize - 1);
				int iNeighbor = (Cols[1] == 0) ? 1 : (Cols[1] == iSize - 1) ? Cols[1] - 1 : (Rand.FRand() < 0.5f ? Cols[1] - 1 : Cols[1] + 1);

				int iTrue = (Rand.FRand() < 0.5f) ? 0 : 2;
				int iFalse = 2 - iTrue;
				Rows[iTrue] = Rand.RandRange(0, iSize - 1);
				Cols[iTrue] = iNeighbor;

				// The false option is a real icon that is not next to the subject and isn't one of the other icons
				Rows[iFalse] = Rand.RandRange(0, iSize - 1);
				Cols[iFalse] = -1;
				int iIcon = Rand.RandRange(0, iSize - 1);
				int iIconCol = 0;
				while (Sol(Rows[iFalse], iIconCol) != iIcon)
					iIconCol++;

				if (FMath::Abs(iIconCol - Cols[1]) == 1 ||
					(Rows[iFalse] == Rows[1] && iIcon == Sol(Rows[1], Cols[1])) ||
					(Rows[iFalse] == Rows[iTrue] && iIcon == Sol(Rows[iTrue], Cols[iTrue])))
					continue;

				m_iNotCell = iFalse;
				m_iHorizontal1 = iIcon;
				break;
			}

			case eHorizontalType::AllApart:
			{
				// 3 different rows and 3 different columns
				do { Rows[1] = Rand.RandRange(0, iSize - 1); } while (Rows[1] == Rows[0]);
				do { Rows[2] = Rand.RandRange(0, iSize - 1); } while (Rows[2] == Rows[0] || Rows[2] == Rows[1]);

				Cols[0] = Rand.RandRange(0, iSize - 1);
				do { Cols[1] = Rand.RandRange(0, iSize - 1); } while (Cols[1] == Cols[0]);
				do { Cols[2] = Rand.RandRange(0, iSize - 1); } while (Cols[2] == Cols[0] || Cols[2] == Cols[1]);
				break;
			}

			default:
				return false;
		}

		m_iRow = Rows[0]; m_iRow2 = Rows[1]; m_iRow3 = Rows[2];
		m_iCol = Cols[0]; m_iCol2 = Cols[1]; m_iCol3 = Cols[2];

		// Make sure the clue is useful: at least one of its real icons isn't placed yet
		for (int i = 0; i < 3; i++)
		{
			if (Cols[i] >= 0 && P.m_Rows[Rows[i]].m_Cells[Cols[i]].m_iFinalIcon < 0)
				return true;
		}
	}

	return false;
}

void UClue::AnalyzeNotHere(UPuzzle& P)
{
	// Skip if the player has wrongly placed this icon here; eliminating a final icon is an error
	if (P.m_Rows[m_iRow].m_Cells[m_iCol].m_iFinalIcon != m_iHorizontal1)
		P.EliminateIconWithClue(this, m_iRow, m_iCol, m_iHorizontal1);
}

bool UClue::ConstraintHolds(const int Cols[3], int Size) const
{
	const int C0 = Cols[0], C1 = Cols[1], C2 = Cols[2];

	switch (m_HorizontalType)
	{
		case eHorizontalType::AllApart:			return C0 != C1 && C1 != C2 && C0 != C2;
		case eHorizontalType::Edge:				return C0 == 0 || C0 == Size - 1;
		case eHorizontalType::NotEdge:			return C0 > 0 && C0 < Size - 1;
		case eHorizontalType::DirectlyLeftOf:	return C1 == C0 + 1;
		case eHorizontalType::Gap:				return FMath::Abs(C0 - C1) == 2;
		case eHorizontalType::Between:			return (C0 < C1 && C1 < C2) || (C2 < C1 && C1 < C0);
		case eHorizontalType::Chain:			return C0 < C1 && C1 < C2;
		case eHorizontalType::NextToEitherOr:	return (FMath::Abs(C1 - C0) == 1) != (FMath::Abs(C1 - C2) == 1);
		default:								return true;
	}
}

bool UClue::HasSupport(UPuzzle& P, const int Rows[3], const int Icons[3], int Slot, int Col) const
{
	const int iSize = P.m_iSize;
	check(iSize <= 16);

	// Candidate columns for each slot: the fixed column for Slot, every still-possible column for the others,
	// and a single dummy entry for unused slots
	int Cand[3][16];
	int NumCand[3];
	for (int s = 0; s < 3; s++)
	{
		NumCand[s] = 0;
		if (Rows[s] < 0)
			Cand[s][NumCand[s]++] = -1;
		else if (s == Slot)
			Cand[s][NumCand[s]++] = Col;
		else
		{
			for (int c = 0; c < iSize; c++)
			{
				if (P.m_Rows[Rows[s]].m_Cells[c].m_bValues[Icons[s]])
					Cand[s][NumCand[s]++] = c;
			}
		}
	}

	// Two icons in the same row can't occupy the same cell
	auto SharesCell = [&Rows](int A, int B, const int Cols[3])
	{
		return Rows[A] >= 0 && Rows[A] == Rows[B] && Cols[A] == Cols[B];
	};

	int Cols[3];
	for (int a = 0; a < NumCand[0]; a++)
	{
		Cols[0] = Cand[0][a];
		for (int b = 0; b < NumCand[1]; b++)
		{
			Cols[1] = Cand[1][b];
			if (SharesCell(0, 1, Cols))
				continue;

			for (int c = 0; c < NumCand[2]; c++)
			{
				Cols[2] = Cand[2][c];
				if (SharesCell(0, 2, Cols) || SharesCell(1, 2, Cols))
					continue;

				if (ConstraintHolds(Cols, iSize))
					return true;
			}
		}
	}

	return false;
}

void UClue::AnalyzeConstraint(UPuzzle& P)
{
	int Rows[3], Icons[3];
	GetSlots(P, Rows, Icons);

	for (int s = 0; s < 3; s++)
	{
		if (Rows[s] < 0)
			continue;

		for (int c = 0; c < P.m_iSize; c++)
		{
			const FPuzzleCell& Cell = P.m_Rows[Rows[s]].m_Cells[c];
			if (Cell.m_bValues[Icons[s]] && Cell.m_iFinalIcon != Icons[s] && !HasSupport(P, Rows, Icons, s, c))
				P.EliminateIconWithClue(this, Rows[s], c, Icons[s]);
		}
	}
}

bool UClue::GetHintActionConstraint(UPuzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int Rows[3], Icons[3];
	GetSlots(P, Rows, Icons);

	// Prefer placing an icon when the clue leaves it only one column
	for (int s = 0; s < 3; s++)
	{
		if (Rows[s] < 0)
			continue;

		int iPossible = 0;
		int iSupported = 0;
		int iOnlyCol = -1;
		for (int c = 0; c < P.m_iSize; c++)
		{
			if (!P.m_Rows[Rows[s]].m_Cells[c].m_bValues[Icons[s]])
				continue;

			iPossible++;
			if (HasSupport(P, Rows, Icons, s, c))
			{
				iSupported++;
				iOnlyCol = c;
			}
		}

		if (iSupported == 1 && iPossible > 1 && P.m_Rows[Rows[s]].m_Cells[iOnlyCol].m_iFinalIcon != Icons[s])
		{
			bSetFinalIcon = true;
			iRow = Rows[s];
			iCol = iOnlyCol;
			iIcon = Icons[s];
			return true;
		}
	}

	// Otherwise eliminate the first position the clue rules out (same order as AnalyzeConstraint)
	for (int s = 0; s < 3; s++)
	{
		if (Rows[s] < 0)
			continue;

		for (int c = 0; c < P.m_iSize; c++)
		{
			const FPuzzleCell& Cell = P.m_Rows[Rows[s]].m_Cells[c];
			if (Cell.m_bValues[Icons[s]] && Cell.m_iFinalIcon != Icons[s] && !HasSupport(P, Rows, Icons, s, c))
			{
				bSetFinalIcon = false;
				iRow = Rows[s];
				iCol = c;
				iIcon = Icons[s];
				return true;
			}
		}
	}

	return false;
}

FString UClue::ExtendedToString() const
{
	FString ClueString = (m_Type == eClueType::Vertical)
		? TEXT("Vertical: ") + StaticEnum<eVerticalType>()->GetNameStringByValue((int64)m_VerticalType)
		: TEXT("Horizontal: ") + StaticEnum<eHorizontalType>()->GetNameStringByValue((int64)m_HorizontalType);
	ClueString += TEXT(":");

	const int Rows[3] = { m_iRow, m_iRow2, m_iRow3 };
	const int Cols[3] = { m_iCol, m_iCol2, m_iCol3 };
	for (int i = 0; i < 3; i++)
	{
		if (Rows[i] < 0)
			ClueString += TEXT(" (-)");
		else if (m_Type == eClueType::Horizontal && m_HorizontalType == eHorizontalType::NextToEitherOr && m_iNotCell == i)
			ClueString += FString::Printf(TEXT(" (%d, icon %d false)"), Rows[i], m_iHorizontal1);
		else
			ClueString += FString::Printf(TEXT(" (%d, %d)"), Rows[i], Cols[i]);
	}

	return ClueString;
}

void UClue::GetPositiveParts(UPuzzle& P, TArray<UClue*>& Out)
{
	auto Make = [&P](eClueType Type)
	{
		UClue* C = NewObject<UClue>(&P);
		C->m_Type = Type;
		C->m_iRow2 = C->m_iRow3 = -1;
		C->m_iCol2 = C->m_iCol3 = -1;
		C->m_iHorizontal1 = C->m_iNotCell = -1;
		return C;
	};

	auto MakeTwo = [&](int Row, int Row2)
	{
		UClue* C = Make(eClueType::Vertical);
		C->m_VerticalType = eVerticalType::Two;
		C->m_iCol = m_iCol;
		C->m_iRow = Row;
		C->m_iRow2 = Row2;
		Out.Add(C);
	};

	auto MakeNextTo = [&](int Row, int Col, int Row2, int Col2)
	{
		UClue* C = Make(eClueType::Horizontal);
		C->m_HorizontalType = eHorizontalType::NextTo;
		C->m_iRow = Row;
		C->m_iCol = Col;
		C->m_iRow2 = Row2;
		C->m_iCol2 = Col2;
		C->m_iRow3 = Row;
		Out.Add(C);
	};

	// The middle of a span can't be in an end column; that comes from the span's shape, not its "not"
	auto MakeNotEdge = [&](int Row, int Col)
	{
		UClue* C = Make(eClueType::Horizontal);
		C->m_HorizontalType = eHorizontalType::NotEdge;
		C->m_iRow = Row;
		C->m_iCol = Col;
		Out.Add(C);
	};

	if (m_Type == eClueType::Vertical)
	{
		switch (m_VerticalType)
		{
			case eVerticalType::TwoNot:			return;
			case eVerticalType::ThreeTopNot:	MakeTwo(m_iRow2, m_iRow3); return;
			case eVerticalType::ThreeMidNot:	MakeTwo(m_iRow, m_iRow3); return;
			case eVerticalType::ThreeBotNot:	MakeTwo(m_iRow, m_iRow2); return;
			default: break;
		}
	}
	else if (m_Type == eClueType::Horizontal)
	{
		switch (m_HorizontalType)
		{
			case eHorizontalType::NotNextTo:
			case eHorizontalType::NotLeftOf:
			case eHorizontalType::NotEdge:
				return;

			case eHorizontalType::SpanNotLeft:
				MakeNextTo(m_iRow2, m_iCol2, m_iRow3, m_iCol3);
				MakeNotEdge(m_iRow2, m_iCol2);
				return;

			case eHorizontalType::SpanNotRight:
				MakeNextTo(m_iRow, m_iCol, m_iRow2, m_iCol2);
				MakeNotEdge(m_iRow2, m_iCol2);
				return;

			case eHorizontalType::SpanNotMid:
			{
				UClue* C = Make(eClueType::Horizontal);
				C->m_HorizontalType = eHorizontalType::Gap;
				C->m_iRow = m_iRow;
				C->m_iCol = m_iCol;
				C->m_iRow2 = m_iRow3;
				C->m_iCol2 = m_iCol3;
				Out.Add(C);
				return;
			}

			default: break;
		}
	}

	// No "not" component: the clue itself
	Out.Add(this);
}
