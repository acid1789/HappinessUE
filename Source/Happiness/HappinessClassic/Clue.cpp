#include "Clue.h"
#include "Puzzle.h"
#include "PuzzleRow.h"
#include "PuzzleCell.h"

Clue::Clue(Puzzle& P, FRandomStream& Rand)
{
	GenerateClue(P, Rand);
}

void Clue::GenerateClue(Puzzle& P, FRandomStream& Rand)
{
	PickClueType(P, Rand);

	switch (m_Type)
	{
	case eClueType::Given:
		GenerateGiven(P, Rand);
		break;

	case eClueType::Vertical:
		GenerateVertical(P, Rand);
		break;

	case eClueType::Horizontal:
		GenerateHorizontal(P, Rand);
		break;
	}
}

void Clue::PickClueType(Puzzle& P, FRandomStream& Rand)
{
	float Val = Rand.FRand();

	if (Val < 0.1f && P.GetNumGivenClues() < 5)
		m_Type = eClueType::Given;
	else if (Val < 0.35f)
		m_Type = eClueType::Vertical;
	else
		m_Type = eClueType::Horizontal;
}

void Clue::GenerateGiven(Puzzle& P, FRandomStream& Rand)
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

void Clue::GenerateVertical(Puzzle& P, FRandomStream& Rand)
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
	int Type = Rand.RandRange(0, 6);
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

					if (P.m_Solution[m_iNotCell][m_iCol] != m_iHorizontal1)
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
				bGoodCell = (m_iNotCell != P.m_Solution[m_iRow2][m_iCol]);
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
				bGoodCell = (m_iNotCell != P.m_Solution[m_iRow][m_iCol]);
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
				bGoodCell = (m_iNotCell != P.m_Solution[m_iRow2][m_iCol]);
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
				bGoodCell = (m_iNotCell != P.m_Solution[m_iRow3][m_iCol]);
			}
			break;
		}
	}
}

void Clue::GenerateTwoRowColumn(Puzzle& P, FRandomStream& Rand)
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

void Clue::GenerateThreeRowColumn(Puzzle& P, FRandomStream& Rand)
{
	bool bGoodRows = false;
	int iSize = P.m_iSize;
	while (!bGoodRows)
	{
		// Pick 3 random rows
		m_iRow = Rand.RandRange(0, iSize);
		m_iRow2 = Rand.RandRange(0, iSize);
		m_iRow3 = Rand.RandRange(0, iSize);
		while (m_iRow2 == m_iRow)
		{
			m_iRow2 = Rand.RandRange(0, iSize);
		}
		while (m_iRow3 == m_iRow2 || m_iRow3 == m_iRow)
		{
			m_iRow3 = Rand.RandRange(0, iSize);
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

void Clue::GenerateHorizontal(Puzzle& P, FRandomStream& Rand)
{
	int iSize = P.m_iSize;
	int Type = Rand.RandRange(0, 7);

	m_HorizontalType = (eHorizontalType)Type;
	switch (m_HorizontalType) 
	{
		case eHorizontalType::NextTo:
		{
			while (true)
			{
				// Pick first icon randomly
				m_iRow = Rand.RandRange(0, iSize);
				m_iCol = Rand.RandRange(0, iSize);
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
				m_iRow2 = Rand.RandRange(0, iSize);
				// Make sure this clue is useful
				if (P.m_Rows[m_iRow].m_Cells[m_iCol].m_iFinalIcon < 0 || P.m_Rows[m_iRow2].m_Cells[m_iCol2].m_iFinalIcon < 0)
					break;
			}
			m_iRow3 = m_iRow;
			break;
		}
		case eHorizontalType::NotNextTo:
		{
			while (true)
			{
				// Pick first icon randomly
				m_iRow = Rand.RandRange(0, iSize);
				m_iCol = Rand.RandRange(0, iSize);
				// Pick a neighboring row
				m_iRow2 = Rand.RandRange(0, iSize);
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
					m_iHorizontal1 = Rand.RandRange(0, iSize);
					// Make sure its not the same as the first icon
					if (m_iRow2 == m_iRow && P.m_Solution[m_iRow][m_iCol] == m_iHorizontal1)
						continue;
					// Make sure its not on the left of the first column
					if (m_iCol > 0 && P.m_Solution[m_iRow2][m_iCol - 1] == m_iHorizontal1)
						continue;
					// Make sure its not on the right of the first column
					if (m_iCol < (P.m_iSize - 1) && P.m_Solution[m_iRow2][m_iCol + 1] == m_iHorizontal1)
						continue;
					// This one looks fine
					break;
				}
			}
			m_iRow3 = m_iRow;
			break;
		}
		case eHorizontalType::LeftOf:
		{
			while (true)
			{
				// Pick first icon randomly
				m_iCol = Rand.RandRange(0, iSize - 1);
				m_iRow = Rand.RandRange(0, iSize);
				// Pick second icon
				m_iCol2 = Rand.RandRange(m_iCol + 1, iSize);
				m_iRow2 = Rand.RandRange(0, iSize);
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
				m_iCol = Rand.RandRange(1, iSize);
				m_iRow = Rand.RandRange(0, iSize);
				// Pick second icon
				m_iCol2 = Rand.RandRange(0, m_iCol);
				m_iRow2 = Rand.RandRange(0, iSize);
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
				m_iHorizontal1 = Rand.RandRange(0, iSize);
				if (P.m_Solution[m_iRow][m_iCol] == m_iHorizontal1)
					continue;
				if (P.m_Solution[m_iRow2][m_iCol2] == m_iHorizontal1)
					continue;
				if (P.m_Solution[m_iRow3][m_iCol3] == m_iHorizontal1)
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
				m_iHorizontal1 = Rand.RandRange(0, iSize);
				if (P.m_Solution[m_iRow][m_iCol] == m_iHorizontal1)
					continue;
				if (P.m_Solution[m_iRow2][m_iCol2] == m_iHorizontal1)
					continue;
				if (P.m_Solution[m_iRow3][m_iCol3] == m_iHorizontal1)
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
				m_iHorizontal1 = Rand.RandRange(0, iSize);
				if (P.m_Solution[m_iRow][m_iCol] == m_iHorizontal1)
					continue;
				if (P.m_Solution[m_iRow2][m_iCol2] == m_iHorizontal1)
					continue;
				if (P.m_Solution[m_iRow3][m_iCol3] == m_iHorizontal1)
					continue;
				break;
			}
			break;
		}
	}
}

void Clue::GenerateHorizontalSpan(Puzzle& P, FRandomStream& Rand)
{
	int iSize = P.m_iSize;
	while (true)
	{
		// Pick the first icon
		m_iCol = Rand.RandRange(0, iSize);

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
		m_iRow = Rand.RandRange(0, iSize);
		m_iRow2 = Rand.RandRange(0, iSize);
		m_iRow3 = Rand.RandRange(0, iSize);

		// Make sure the clue is useful
		if (P.m_Rows[m_iRow].m_Cells[m_iCol].m_iFinalIcon < 0 ||
			P.m_Rows[m_iRow2].m_Cells[m_iCol2].m_iFinalIcon < 0 ||
			P.m_Rows[m_iRow3].m_Cells[m_iCol3].m_iFinalIcon < 0)
			break;
	}
}

void Clue::Analyze(Puzzle& P)
{
	switch (m_Type)
	{
	case eClueType::Given:
		AnalyzeGiven(P);
		break;

	case eClueType::Vertical:
		AnalyzeVertical(P);
		break;

	case eClueType::Horizontal:
		AnalyzeHorizontal(P);
		break;
	}
}

void Clue::AnalyzeGiven(Puzzle& P)
{
	int Icon = P.m_Solution[m_iRow][m_iCol];

	P.SetFinalIcon(this, m_iRow, m_iCol, Icon);

	m_iUseCount += 25;
}

void Clue::AnalyzeVertical(Puzzle& P)
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

void Clue::AnalyzeHorizontal(Puzzle& P)
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
	}
}

void Clue::AnalyzeVerticalTwo(Puzzle& P)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = P.m_Solution[m_iRow2][m_iCol];
	for (int i = 0; i < P.m_iSize; i++)
	{
		if (!P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
		{
			P.EliminateIcon(this, m_iRow2, i, iIcon2);
		}
		else if (!P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
		{
			P.EliminateIcon(this, m_iRow, i, iIcon1);
		}

		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			P.SetFinalIcon(this, m_iRow2, i, iIcon2);
			return;
		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			P.SetFinalIcon(this, m_iRow, i, iIcon1);
			return;
		}
	}
}

void Clue::AnalyzeVerticalThree(Puzzle& P)
{
	int iIcon1 = P.m_Solution[m_iRow][ m_iCol];
	int iIcon2 = P.m_Solution[m_iRow2][m_iCol];
	int iIcon3 = P.m_Solution[m_iRow3][m_iCol];
	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			P.SetFinalIcon(this, m_iRow2, i, iIcon2);
			P.SetFinalIcon(this, m_iRow3, i, iIcon3);
			return;
		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			P.SetFinalIcon(this, m_iRow, i, iIcon1);
			P.SetFinalIcon(this, m_iRow3, i, iIcon3);
			return;
		}
		else if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon == iIcon3)
		{
			P.SetFinalIcon(this, m_iRow, i, iIcon1);
			P.SetFinalIcon(this, m_iRow2, i, iIcon2);
			return;
		}

		if (!P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
		{
			P.EliminateIcon(this, m_iRow2, i, iIcon2);
			P.EliminateIcon(this, m_iRow3, i, iIcon3);
		}
		else if (!P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
		{
			P.EliminateIcon(this, m_iRow, i, iIcon1);
			P.EliminateIcon(this, m_iRow3, i, iIcon3);
		}
		else if (!P.m_Rows[m_iRow3].m_Cells[i].m_bValues[iIcon3])
		{
			P.EliminateIcon(this, m_iRow, i, iIcon1);
			P.EliminateIcon(this, m_iRow2, i, iIcon2);
		}
	}
}

void Clue::AnalyzeVerticalEitherOr(Puzzle& P)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = (m_iRow2 == m_iNotCell) ? m_iHorizontal1 : P.m_Solution[m_iRow2][m_iCol];
	int iIcon3 = (m_iRow3 == m_iNotCell) ? m_iHorizontal1 : P.m_Solution[m_iRow3][m_iCol];

	int iSize = P.m_iSize;
	int iIcon2Col = -1;
	int iIcon3Col = -1;
	for (int i = 0; i < iSize; i++)
	{
		if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			P.EliminateIcon(this, m_iRow3, i, iIcon3);
			iIcon2Col = i;
		}
		if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon == iIcon3)
		{
			P.EliminateIcon(this, m_iRow2, i, iIcon2);
			iIcon3Col = i;
		}
		if (!P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2] &&
			!P.m_Rows[m_iRow3].m_Cells[i].m_bValues[iIcon3])
		{
			// If neither icon is in this column, icon1 cant be here either
			P.EliminateIcon(this, m_iRow, i, iIcon1);
		}

		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			for (int j = 0; j < iSize; j++)
			{
				if (j == i)
					continue;

				if (P.m_Rows[m_iRow2].m_Cells[j].m_iFinalIcon == iIcon2)
				{
					P.SetFinalIcon(this, m_iRow3, i, iIcon3);
				}
				else if (P.m_Rows[m_iRow3].m_Cells[j].m_iFinalIcon == iIcon3)
				{
					P.SetFinalIcon(this, m_iRow2, i, iIcon2);
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
				P.EliminateIcon(this, m_iRow, i, iIcon1);
			}
		}
	}
}

void Clue::AnalyzeVerticalTwoNot(Puzzle& P)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = m_iNotCell;

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			P.EliminateIcon(this, m_iRow2, i, iIcon2);

		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			P.EliminateIcon(this, m_iRow, i, iIcon1);
		}
	}
}

void Clue::AnalyzeVerticalThreeTopNot(Puzzle& P)
{
	int iIcon1 = m_iNotCell;
	int iIcon2 = P.m_Solution[m_iRow2][m_iCol];
	int iIcon3 = P.m_Solution[m_iRow3][m_iCol];

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			P.EliminateIcon(this, m_iRow2, i, iIcon2);
			P.EliminateIcon(this, m_iRow3, i, iIcon3);
		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			P.EliminateIcon(this, m_iRow, i, iIcon1);
			P.SetFinalIcon(this, m_iRow3, i, iIcon3);
			return;
		}
		else if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon == iIcon3)
		{
			P.EliminateIcon(this, m_iRow, i, iIcon1);
			P.SetFinalIcon(this, m_iRow2, i, iIcon2);
			return;
		}
		else if (!P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
		{
			P.EliminateIcon(this, m_iRow3, i, iIcon3);
		}
		else if (!P.m_Rows[m_iRow3].m_Cells[i].m_bValues[iIcon3])
		{
			P.EliminateIcon(this, m_iRow2, i, iIcon2);
		}
	}
}

void Clue::AnalyzeVerticalThreeMidNot(Puzzle& P)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = m_iNotCell;
	int iIcon3 = P.m_Solution[m_iRow3][m_iCol];

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			P.EliminateIcon(this, m_iRow, i, iIcon1);
			P.EliminateIcon(this, m_iRow3, i, iIcon3);

		}
		else if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			P.EliminateIcon(this, m_iRow2, i, iIcon2);
			P.SetFinalIcon(this, m_iRow3, i, iIcon3);
			return;
		}
		else if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon == iIcon3)
		{
			P.EliminateIcon(this, m_iRow2, i, iIcon2);
			P.SetFinalIcon(this, m_iRow, i, iIcon1);
			return;
		}
		else if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon >= 0)
		{
			P.EliminateIcon(this, m_iRow3, i, iIcon3);
		}
		else if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon >= 0)
		{
			P.EliminateIcon(this, m_iRow, i, iIcon1);
		}
	}
}

void Clue::AnalyzeVerticalThreeBotNot(Puzzle& P)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = P.m_Solution[m_iRow2][m_iCol];
	int iIcon3 = m_iNotCell;

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow3].m_Cells[i].m_iFinalIcon == iIcon3)
		{
			P.EliminateIcon(this, m_iRow2, i, iIcon2);
			P.EliminateIcon(this, m_iRow, i, iIcon1);

		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			P.EliminateIcon(this, m_iRow3, i, iIcon3);
			P.SetFinalIcon(this, m_iRow, i, iIcon1);
			return;
		}
		else if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			P.EliminateIcon(this, m_iRow3, i, iIcon3);
			P.SetFinalIcon(this, m_iRow2, i, iIcon2);
			return;
		}
		else if (!P.m_Rows[m_iRow2].m_Cells[i].m_bValues[iIcon2])
		{
			P.EliminateIcon(this, m_iRow, i, iIcon1);
		}
		else if (!P.m_Rows[m_iRow].m_Cells[i].m_bValues[iIcon1])
		{
			P.EliminateIcon(this, m_iRow2, i, iIcon2);
		}
	}
}

void Clue::AnalyzeHorizontalNextTo(Puzzle& P)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = P.m_Solution[m_iRow2][m_iCol2];

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			if (i == 0)
			{
				P.SetFinalIcon(this, m_iRow2, 1, iIcon2);

			}
			else if (i == P.m_iSize - 1)
			{
				P.SetFinalIcon(this, m_iRow2, i - 1, iIcon2);

			}
			else
			{
				for (int j = 0; j < P.m_iSize; j++)
				{
					if (j == (i - 1) || j == (i + 1))
						continue;
					P.EliminateIcon(this, m_iRow2, j, iIcon2);

				}
			}
			break;
		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			if (i == 0)
			{
				P.SetFinalIcon(this, m_iRow, 1, iIcon1);

			}
			else if (i == P.m_iSize - 1)
			{
				P.SetFinalIcon(this, m_iRow, i - 1, iIcon1);

			}
			else
			{
				for (int j = 0; j < P.m_iSize; j++)
				{
					if (j == (i - 1) || j == (i + 1))
						continue;
					P.EliminateIcon(this, m_iRow, j, iIcon1);

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
					P.EliminateIcon(this, m_iRow, i, iIcon1);

				}
				if (!P.m_Rows[m_iRow].m_Cells[i + 1].m_bValues[iIcon1])
				{
					P.EliminateIcon(this, m_iRow2, i, iIcon2);

				}
			}
			else if (i == P.m_iSize - 1)
			{
				if (!P.m_Rows[m_iRow2].m_Cells[i - 1].m_bValues[iIcon2])
				{
					P.EliminateIcon(this, m_iRow, i, iIcon1);

				}
				if (!P.m_Rows[m_iRow].m_Cells[i - 1].m_bValues[iIcon1])
				{
					P.EliminateIcon(this, m_iRow2, i, iIcon2);

				}
			}
			else
			{
				if (!P.m_Rows[m_iRow2].m_Cells[i + 1].m_bValues[iIcon2] &&
					!P.m_Rows[m_iRow2].m_Cells[i - 1].m_bValues[iIcon2])
				{
					P.EliminateIcon(this, m_iRow, i, iIcon1);

				}
				if (!P.m_Rows[m_iRow].m_Cells[i + 1].m_bValues[iIcon1] &&
					!P.m_Rows[m_iRow].m_Cells[i - 1].m_bValues[iIcon1])
				{
					P.EliminateIcon(this, m_iRow2, i, iIcon2);

				}
			}
		}
	}
}

void Clue::AnalyzeHorizontalNotNextTo(Puzzle& P)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = m_iHorizontal1;

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			if (i == 0)
			{
				P.EliminateIcon(this, m_iRow2, i + 1, iIcon2);

			}
			else if (i == P.m_iSize - 1)
			{
				P.EliminateIcon(this, m_iRow2, i - 1, iIcon2);

			}
			else
			{
				P.EliminateIcon(this, m_iRow2, i - 1, iIcon2);
				P.EliminateIcon(this, m_iRow2, i + 1, iIcon2);

			}
			break;
		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			if (i == 0)
			{
				P.EliminateIcon(this, m_iRow, i + 1, iIcon1);

			}
			else if (i == P.m_iSize - 1)
			{
				P.EliminateIcon(this, m_iRow, i - 1, iIcon1);

			}
			else
			{
				P.EliminateIcon(this, m_iRow, i - 1, iIcon1);
				P.EliminateIcon(this, m_iRow, i + 1, iIcon1);

			}
			break;
		}
	}
}

void Clue::AnalyzeHorizontalLeftOf(Puzzle& P)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = P.m_Solution[m_iRow2][m_iCol2];

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
		P.SetFinalIcon(this, m_iRow, iFirstPossibleLeft, iIcon1);
		P.SetFinalIcon(this, m_iRow2, iFirstPossibleRight, iIcon2);
	}
	else
	{
		// Remove all icon2's from the left side of the first possible left
		for (int i = 0; i <= iFirstPossibleLeft; i++)
		{
			P.EliminateIcon(this, m_iRow2, i, iIcon2);
		}

		// Remove all the icon1's from the right side of the first possible right
		for (int i = iFirstPossibleRight; i < P.m_iSize; i++)
		{
			P.EliminateIcon(this, m_iRow, i, iIcon1);
		}
	}
}

void Clue::AnalyzeHorizontalNotLeftOf(Puzzle& P)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = P.m_Solution[m_iRow2][m_iCol2];

	// If both icons are on the same row
	if (m_iRow == m_iRow2)
	{
		// Icon1 cant be in the zero column, because that would ensure that it is always left of Icon2
		P.EliminateIcon(this, m_iRow, 0, iIcon1);

		// Icon2 cant be in the last column, because that would ensure that it is always right of Icon1
		P.EliminateIcon(this, m_iRow2, P.m_iSize - 1, iIcon2);

	}

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (P.m_Rows[m_iRow].m_Cells[i].m_iFinalIcon == iIcon1)
		{
			// Icon1 is known, remove all instances of icon2 to the right
			for (int j = i + 1; j < P.m_iSize; j++)
			{
				P.EliminateIcon(this, m_iRow2, j, iIcon2);

			}
			break;
		}
		else if (P.m_Rows[m_iRow2].m_Cells[i].m_iFinalIcon == iIcon2)
		{
			// Icon2 is known, remove all instances of icon1 to the left
			for (int j = 0; j < i; j++)
			{
				P.EliminateIcon(this, m_iRow, j, iIcon1);

			}
			break;
		}
	}
}

bool Clue::SolveSpan(int iCol, int iRow1, int iIcon1, bool bNot1, int iRow2, int iIcon2, bool bNot2, int iRow3, int iIcon3, bool bNot3, Puzzle& P)
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
					P.SetFinalIcon(this, iRow3, iCol, iIcon3);

				}
			}
			else
			{
				if (bNot2)
				{
					P.EliminateIcon(this, iRow2, iCol + 1, iIcon2);

				}
				else
				{
					P.SetFinalIcon(this, iRow2, iCol + 1, iIcon2);

				}

				if (bNot3)
				{
					P.EliminateIcon(this, iRow3, iCol + 2, iIcon3);

				}
				else
				{
					P.SetFinalIcon(this, iRow3, iCol + 2, iIcon3);

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
					P.SetFinalIcon(this, iRow3, iCol, iIcon3);

				}
			}
			else
			{
				if (bNot2)
					P.EliminateIcon(this, iRow2, iCol - 1, iIcon2);
				else
					P.SetFinalIcon(this, iRow2, iCol - 1, iIcon2);

				if (bNot3)
					P.EliminateIcon(this, iRow3, iCol - 2, iIcon3);
				else
					P.SetFinalIcon(this, iRow3, iCol - 2, iIcon3);

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
					P.SetFinalIcon(this, iRow3, iCol, iIcon3);

				}
				else if (bNot2)
				{
					P.SetFinalIcon(this, iRow3, iCol + 2, iIcon3);

				}
				else if (bNot3)
				{
					P.EliminateIcon(this, iRow3, iCol - 2, iIcon3);

				}
				else
				{
					P.SetFinalIcon(this, iRow3, iCol - 2, iIcon3);

				}
				return true;
			}
			else if (iFinal2Right == iIcon2)
			{
				if (bNot1)
				{
					P.SetFinalIcon(this, iRow3, iCol, iIcon3);

				}
				else if (bNot2)
				{
					P.SetFinalIcon(this, iRow3, iCol - 2, iIcon3);

				}
				else if (bNot3)
				{
					P.EliminateIcon(this, iRow3, iCol + 2, iIcon3);

				}
				else
				{
					P.SetFinalIcon(this, iRow3, iCol + 2, iIcon3);

				}
				return true;
			}
			else if (!P.m_Rows[iRow2].m_Cells[iCol - 1].m_bValues[iIcon2] && !bNot1 && !bNot2)
			{
				P.SetFinalIcon(this, iRow2, iCol + 1, iIcon2);
				if (bNot3)
					P.EliminateIcon(this, iRow3, iCol + 2, iIcon3);
				else
					P.SetFinalIcon(this, iRow3, iCol + 2, iIcon3);

				return true;
			}
			else if (!P.m_Rows[iRow2].m_Cells[iCol + 1].m_bValues[iIcon2] && !bNot1 && !bNot2)
			{
				P.SetFinalIcon(this, iRow2, iCol - 1, iIcon2);
				if (bNot3)
					P.EliminateIcon(this, iRow3, iCol - 2, iIcon3);
				else
					P.SetFinalIcon(this, iRow3, iCol - 2, iIcon3);

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
						P.SetFinalIcon(this, iRow2, iCol - 3, iIcon2);

						return true;
					}
					else if (bNot2)
					{
						P.EliminateIcon(this, iRow2, iCol - 1, iIcon2);

					}
					else
					{
						P.SetFinalIcon(this, iRow2, iCol - 1, iIcon2);

						return true;
					}
				}
				else if (iFinal3Right == iIcon3)
				{
					if (bNot1)
					{
						P.SetFinalIcon(this, iRow2, iCol + 3, iIcon2);

						return true;
					}
					else if (bNot2)
					{
						P.EliminateIcon(this, iRow2, iCol + 1, iIcon2);

					}
					else
					{
						P.SetFinalIcon(this, iRow2, iCol + 1, iIcon2);

						return true;
					}
				}
				else if (!P.m_Rows[iRow3].m_Cells[iCol - 2].m_bValues[iIcon3] && !bNot1 && !bNot2)
				{
					P.SetFinalIcon(this, iRow2, iCol + 1, iIcon2);
					P.SetFinalIcon(this, iRow3, iCol + 2, iIcon3);

					return true;
				}
				else if (!P.m_Rows[iRow3].m_Cells[iCol + 2].m_bValues[iIcon3] && !bNot1 && !bNot2)
				{
					P.SetFinalIcon(this, iRow2, iCol - 1, iIcon2);
					P.SetFinalIcon(this, iRow3, iCol - 2, iIcon3);

					return true;
				}
				else if (!bNot1)
				{
					for (int j = 0; j < P.m_iSize; j++)
					{
						if (!bNot2 && j != (iCol - 1) && j != (iCol + 1))
						{
							P.EliminateIcon(this, iRow2, j, iIcon2);

						}
						if (j != (iCol - 2) && j != (iCol + 2))
						{
							P.EliminateIcon(this, iRow3, j, iIcon3);

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
				P.EliminateIcon(this, iRow2, iCol + 1, iIcon2);

			}
			else if (iCol == P.m_iSize - 1)
			{
				P.EliminateIcon(this, iRow2, iCol - 1, iIcon2);

			}
			else if (iCol == P.m_iSize - 3)
			{
				P.EliminateIcon(this, iRow1, iCol + 2, iIcon1);
				P.EliminateIcon(this, iRow2, iCol + 1, iIcon2);
				P.EliminateIcon(this, iRow3, iCol + 2, iIcon3);

			}
			else if (iCol == 2)
			{
				P.EliminateIcon(this, iRow1, iCol - 2, iIcon1);
				P.EliminateIcon(this, iRow2, iCol - 1, iIcon2);
				P.EliminateIcon(this, iRow3, iCol - 2, iIcon3);

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
					P.EliminateIcon(this, iRow3, iCol + 2, iIcon3);
				}
			}
			else if (iCol + 2 < P.m_iSize)
			{
				P.EliminateIcon(this, iRow3, iCol + 2, iIcon3);
			}
			if (iCol - 4 >= 0)
			{
				if (!P.m_Rows[iRow1].m_Cells[iCol - 4].m_bValues[iIcon1])
				{
					P.EliminateIcon(this, iRow3, iCol - 2, iIcon3);
				}
			}
			else if (iCol - 2 >= 0)
			{
				P.EliminateIcon(this, iRow3, iCol - 2, iIcon3);
			}
		}
		if (!bNot2)
		{
			if (iCol + 2 < P.m_iSize)
			{
				if (!P.m_Rows[iRow1].m_Cells[iCol + 2].m_bValues[iIcon1])
				{
					P.EliminateIcon(this, iRow2, iCol + 1, iIcon2);
				}
			}
			if (iCol - 2 >= 0)
			{
				if (!P.m_Rows[iRow1].m_Cells[iCol - 2].m_bValues[iIcon1])
				{
					P.EliminateIcon(this, iRow2, iCol - 1, iIcon2);
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
					P.EliminateIcon(this, iRow1, i, iIcon1);
				if (!bNot3)
					P.EliminateIcon(this, iRow3, i, iIcon3);
			}
		}
	}

	return false;
}

void Clue::AnalyzeHorizontalSpan(Puzzle& P)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = P.m_Solution[m_iRow2][m_iCol2];
	int iIcon3 = P.m_Solution[m_iRow3][m_iCol3];

	// Icon2 cant be on either end
	P.EliminateIcon(this, m_iRow2, 0, iIcon2);
	P.EliminateIcon(this, m_iRow2, P.m_iSize - 1, iIcon2);


	for (int i = 0; i < P.m_iSize; i++)
	{
		if (SolveSpan(i, m_iRow, iIcon1, false, m_iRow2, iIcon2, false, m_iRow3, iIcon3, false, P))
			return;
		if (SolveSpan(i, m_iRow3, iIcon3, false, m_iRow2, iIcon2, false, m_iRow, iIcon1, false, P))
			return;
	}
}

void Clue::AnalyzeHorizontalSpanNotLeft(Puzzle& P)
{
	int iIcon1 = m_iHorizontal1;
	int iIcon2 = P.m_Solution[m_iRow2][m_iCol2];
	int iIcon3 = P.m_Solution[m_iRow3][m_iCol3];

	// Icon2 cant be on either end
	P.EliminateIcon(this, m_iRow2, 0, iIcon2);
	P.EliminateIcon(this, m_iRow2, P.m_iSize - 1, iIcon2);


	for (int i = 0; i < P.m_iSize; i++)
	{
		if (SolveSpan(i, m_iRow, iIcon1, true, m_iRow2, iIcon2, false, m_iRow3, iIcon3, false, P))
			return;
		if (SolveSpan(i, m_iRow3, iIcon3, false, m_iRow2, iIcon2, false, m_iRow, iIcon1, true, P))
			return;
	}
}

void Clue::AnalyzeHorizontalSpanNotMid(Puzzle& P)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = m_iHorizontal1;
	int iIcon3 = P.m_Solution[m_iRow3][m_iCol3];

	for (int i = 0; i < P.m_iSize; i++)
	{
		if (SolveSpan(i, m_iRow, iIcon1, false, m_iRow2, iIcon2, true, m_iRow3, iIcon3, false, P))
			return;
		if (SolveSpan(i, m_iRow3, iIcon3, false, m_iRow2, iIcon2, true, m_iRow, iIcon1, false, P))
			return;
	}
}

void Clue::AnalyzeHorizontalSpanNotRight(Puzzle& P)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = P.m_Solution[m_iRow2][m_iCol2];
	int iIcon3 = m_iHorizontal1;

	// Icon2 cant be on either end
	P.EliminateIcon(this, m_iRow2, 0, iIcon2);
	P.EliminateIcon(this, m_iRow2, P.m_iSize - 1, iIcon2);


	for (int i = 0; i < P.m_iSize; i++)
	{
		if (SolveSpan(i, m_iRow, iIcon1, false, m_iRow2, iIcon2, false, m_iRow3, iIcon3, true, P))
			return;
		if (SolveSpan(i, m_iRow3, iIcon3, true, m_iRow2, iIcon2, false, m_iRow, iIcon1, false, P))
			return;
	}
}

void Clue::Dump(int32 iIndex, Puzzle& P)
{
	FString Output = FString::Printf(TEXT("Clue(%d): "), iIndex);

	switch (m_Type)
	{
		case eClueType::Given:
			Output += FString::Printf(TEXT("Type: Given (%d, %d, %d)"), m_iRow, m_iCol, P.m_Solution[m_iRow][m_iCol]);
			break;

		case eClueType::Vertical:
			Output += TEXT("Type: Vertical  VType: ");
			switch (m_VerticalType)
			{
				case eVerticalType::Two: Output += FString::Printf(TEXT("Two ([%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[m_iRow][m_iCol],	m_iRow2, P.m_Solution[m_iRow2][m_iCol]); break;
				case eVerticalType::Three: Output += FString::Printf(TEXT("Three ([%d]:%d, [%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[m_iRow][m_iCol], m_iRow2, P.m_Solution[m_iRow2][m_iCol], m_iRow3, P.m_Solution[m_iRow3][m_iCol]); break;
				case eVerticalType::EitherOr: Output += FString::Printf(TEXT("EitherOr ([%d]:%d, [%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[m_iRow][m_iCol], m_iRow2, (m_iRow2 == m_iNotCell) ? m_iHorizontal1 : P.m_Solution[m_iRow2][m_iCol], m_iRow3, (m_iRow3 == m_iNotCell) ? m_iHorizontal1 : P.m_Solution[m_iRow3][m_iCol]); break;
				case eVerticalType::TwoNot: Output += FString::Printf(TEXT("TwoNot ([%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[m_iRow][m_iCol], m_iRow2, m_iNotCell); break;
				case eVerticalType::ThreeTopNot: Output += FString::Printf(TEXT("ThreeTopNot ([%d]:%d, [%d]:%d, [%d]:%d)"), m_iRow, m_iNotCell, m_iRow2, P.m_Solution[m_iRow2][m_iCol], m_iRow3, P.m_Solution[m_iRow3][m_iCol]); break;
				case eVerticalType::ThreeMidNot: Output += FString::Printf(TEXT("ThreeMidNot ([%d]:%d, [%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[m_iRow][m_iCol], m_iRow2, m_iNotCell, m_iRow3, P.m_Solution[m_iRow3][m_iCol]);	break;
				case eVerticalType::ThreeBotNot: Output += FString::Printf(TEXT("ThreeBotNot ([%d]:%d, [%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[m_iRow][m_iCol], m_iRow2, P.m_Solution[m_iRow2][m_iCol], m_iRow3, m_iNotCell); break;
			}
			break;

		case eClueType::Horizontal:
			Output += TEXT("Type: Horizontal  HType: ");
			switch (m_HorizontalType)
			{
				case eHorizontalType::NextTo: Output += FString::Printf(TEXT("NextTo ([%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[m_iRow][m_iCol], m_iRow2, P.m_Solution[m_iRow2][m_iCol2]); break;
				case eHorizontalType::NotNextTo: Output += FString::Printf(TEXT("NotNextTo ([%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[m_iRow][m_iCol], m_iRow2, m_iHorizontal1); break;
				case eHorizontalType::LeftOf: Output += FString::Printf(TEXT("LeftOf ([%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[m_iRow][m_iCol], m_iRow2, P.m_Solution[m_iRow2][m_iCol2]); break;
				case eHorizontalType::NotLeftOf: Output += FString::Printf(TEXT("NotLeftOf ([%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[m_iRow][m_iCol], m_iRow2, P.m_Solution[m_iRow2][m_iCol2]); break;
				case eHorizontalType::Span: Output += FString::Printf(TEXT("Span ([%d]:%d, [%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[m_iRow][m_iCol], m_iRow2, P.m_Solution[m_iRow2][m_iCol2], m_iRow3, P.m_Solution[m_iRow3][m_iCol3]); break;
				case eHorizontalType::SpanNotLeft: Output += FString::Printf(TEXT("SpanNotLeft ([%d]:%d, [%d]:%d, [%d]:%d)"), m_iRow, m_iHorizontal1, m_iRow2, P.m_Solution[m_iRow2][m_iCol2], m_iRow3, P.m_Solution[m_iRow3][m_iCol3]); break;
				case eHorizontalType::SpanNotMid: Output += FString::Printf(TEXT("SpanNotMid ([%d]:%d, [%d]:%d, [%d]:%d)"), m_iRow, P.m_Solution[m_iRow][m_iCol], m_iRow2, m_iHorizontal1, m_iRow3, P.m_Solution[m_iRow3][m_iCol3]); break;
				case eHorizontalType::SpanNotRight:	Output += FString::Printf(TEXT("SpanNotRight ([%d]:%d, [%d]:%d, [%d]:%d)"),	m_iRow, P.m_Solution[m_iRow][m_iCol], m_iRow2, P.m_Solution[m_iRow2][m_iCol2], m_iRow3, m_iHorizontal1); break;
			}
			break;
	}

	UE_LOG(LogTemp, Log, TEXT("%s"), *Output);
}

bool Clue::GetHintAction(Puzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
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

bool Clue::GetHintActionVerticalTwo(Puzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = P.m_Solution[m_iRow2][m_iCol];
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

bool Clue::GetHintActionVerticalThree(Puzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = P.m_Solution[m_iRow2][m_iCol];
	int iIcon3 = P.m_Solution[m_iRow3][m_iCol];
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

bool Clue::GetHintActionVerticalEitherOr(Puzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = (m_iRow2 == m_iNotCell) ? m_iHorizontal1 : P.m_Solution[m_iRow2][m_iCol];
	int iIcon3 = (m_iRow3 == m_iNotCell) ? m_iHorizontal1 : P.m_Solution[m_iRow3][m_iCol];

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

bool Clue::GetHintActionVerticalTwoNot(Puzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
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

bool Clue::GetHintActionVerticalThreeTopNot(Puzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = m_iNotCell;
	int iIcon2 = P.m_Solution[m_iRow2][m_iCol];
	int iIcon3 = P.m_Solution[m_iRow3][m_iCol];

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

bool Clue::GetHintActionVerticalThreeMidNot(Puzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = m_iNotCell;
	int iIcon3 = P.m_Solution[m_iRow3][m_iCol];

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

bool Clue::GetHintActionVerticalThreeBotNot(Puzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = P.m_Solution[m_iRow2][m_iCol];
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

bool Clue::GetHintActionHorizontalNextTo(Puzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = P.m_Solution[m_iRow2][m_iCol2];

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

bool Clue::GetHintActionHorizontalNotNextTo(Puzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
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

bool Clue::GetHintActionHorizontalLeftOf(Puzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = P.m_Solution[m_iRow2][m_iCol2];

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

bool Clue::GetHintActionHorizontalNotLeftOf(Puzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = P.m_Solution[m_iRow2][m_iCol2];

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

bool Clue::GetHintActionHorizontalSpan(Puzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = P.m_Solution[m_iRow2][m_iCol2];
	int iIcon3 = P.m_Solution[m_iRow3][m_iCol3];

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

bool Clue::GetHintActionSpan(int iCol, int iRow1, int iIcon1, bool bNot1, int iRow2, int iIcon2, bool bNot2, int iRow3, int iIcon3, bool bNot3, Puzzle P, bool bSetFinalIcon, int iOutRow, int iOutCol, int iOutIcon)
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

bool Clue::GetHintActionHorizontalSpanNotLeft(Puzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = m_iHorizontal1;
	int iIcon2 = P.m_Solution[m_iRow2][m_iCol2];
	int iIcon3 = P.m_Solution[m_iRow3][m_iCol3];

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

bool Clue::GetHintActionHorizontalSpanNotMid(Puzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = m_iHorizontal1;
	int iIcon3 = P.m_Solution[m_iRow3][m_iCol3];

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

bool Clue::GetHintActionHorizontalSpanNotRight(Puzzle& P, bool& bSetFinalIcon, int& iRow, int& iCol, int& iIcon)
{
	int iIcon1 = P.m_Solution[m_iRow][m_iCol];
	int iIcon2 = P.m_Solution[m_iRow2][m_iCol2];
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
