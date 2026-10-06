#include "ClueExample.h"
#include "Clue.h"
#include "Puzzle.h"

void FClueExample::Add(const UPuzzle& Puzzle, int32 Row, int32 Icon, bool bCrossed)
{
	FClueExamplePiece& Piece = Pieces.AddDefaulted_GetRef();
	Piece.Row = Row;
	Piece.Icon = Icon;
	Piece.bCrossed = bCrossed;
	for (int32 Column = 0; Column < Puzzle.m_iSize; Column++)
	{
		if (Puzzle.m_Solution[Row * Puzzle.m_iSize + Column] == Icon)
		{
			Piece.SolutionColumn = Column;
		}
	}
}

void FClueExample::AddSolved(const UPuzzle& Puzzle, int32 Row, int32 Column)
{
	Add(Puzzle, Row, Puzzle.m_Solution[Row * Puzzle.m_iSize + Column]);
}

bool FClueExample::Build(const UClue& Clue, const UPuzzle& Puzzle)
{
	Pieces.Reset();
	Size = Puzzle.m_iSize;
	const UPuzzle& P = Puzzle;

	if (Clue.m_Type == eClueType::Vertical)
	{
		// The clue's column m_iCol holds every icon it shows, except the crossed-out one (m_iNotCell)
		switch (Clue.m_VerticalType)
		{
			case eVerticalType::Two:
				Kind = EKind::SameColumn;
				AddSolved(P, Clue.m_iRow, Clue.m_iCol);
				AddSolved(P, Clue.m_iRow2, Clue.m_iCol);
				return true;
			case eVerticalType::Three:
				Kind = EKind::SameColumn;
				AddSolved(P, Clue.m_iRow, Clue.m_iCol);
				AddSolved(P, Clue.m_iRow2, Clue.m_iCol);
				AddSolved(P, Clue.m_iRow3, Clue.m_iCol);
				return true;
			case eVerticalType::TwoNot:
				Kind = EKind::NotSameColumn;
				AddSolved(P, Clue.m_iRow, Clue.m_iCol);
				Add(P, Clue.m_iRow2, Clue.m_iNotCell, true);
				return true;
			case eVerticalType::ThreeTopNot:
				Kind = EKind::TwoTogetherOneNot;
				Odd = 0;
				Add(P, Clue.m_iRow, Clue.m_iNotCell, true);
				AddSolved(P, Clue.m_iRow2, Clue.m_iCol);
				AddSolved(P, Clue.m_iRow3, Clue.m_iCol);
				return true;
			case eVerticalType::ThreeMidNot:
				Kind = EKind::TwoTogetherOneNot;
				Odd = 1;
				AddSolved(P, Clue.m_iRow, Clue.m_iCol);
				Add(P, Clue.m_iRow2, Clue.m_iNotCell, true);
				AddSolved(P, Clue.m_iRow3, Clue.m_iCol);
				return true;
			case eVerticalType::ThreeBotNot:
				Kind = EKind::TwoTogetherOneNot;
				Odd = 2;
				AddSolved(P, Clue.m_iRow, Clue.m_iCol);
				AddSolved(P, Clue.m_iRow2, Clue.m_iCol);
				Add(P, Clue.m_iRow3, Clue.m_iNotCell, true);
				return true;
			case eVerticalType::EitherOr:
				// The decoy is in row m_iNotCell, shown as icon m_iHorizontal1
				Kind = EKind::EitherOr;
				AddSolved(P, Clue.m_iRow, Clue.m_iCol);
				for (const int32 Row : { Clue.m_iRow2, Clue.m_iRow3 })
				{
					if (Row == Clue.m_iNotCell)
					{
						Add(P, Row, Clue.m_iHorizontal1, true);
					}
					else
					{
						AddSolved(P, Row, Clue.m_iCol);
					}
				}
				return true;
			default:
				return false;
		}
	}

	if (Clue.m_Type != eClueType::Horizontal)
	{
		return false;
	}

	switch (Clue.m_HorizontalType)
	{
		case eHorizontalType::NextTo:
			Kind = EKind::NextTo;
			AddSolved(P, Clue.m_iRow, Clue.m_iCol);
			AddSolved(P, Clue.m_iRow2, Clue.m_iCol2);
			return true;
		case eHorizontalType::NotNextTo:
			Kind = EKind::NotNextTo;
			AddSolved(P, Clue.m_iRow, Clue.m_iCol);
			Add(P, Clue.m_iRow2, Clue.m_iHorizontal1, true);
			return true;
		case eHorizontalType::LeftOf:
		case eHorizontalType::NotLeftOf:
			Kind = Clue.m_HorizontalType == eHorizontalType::LeftOf ? EKind::LeftOf : EKind::NotLeftOf;
			AddSolved(P, Clue.m_iRow, Clue.m_iCol);
			AddSolved(P, Clue.m_iRow2, Clue.m_iCol2);
			return true;
		case eHorizontalType::Span:
			Kind = EKind::Span;
			AddSolved(P, Clue.m_iRow, Clue.m_iCol);
			AddSolved(P, Clue.m_iRow2, Clue.m_iCol2);
			AddSolved(P, Clue.m_iRow3, Clue.m_iCol3);
			return true;
		case eHorizontalType::SpanNotLeft:
			// The middle icon (slot 1) has the right icon beside it, and the crossed-out left icon not on its other side
			Kind = EKind::SpanNotSide;
			Odd = 0;
			Middle = 1;
			Beside = 2;
			Add(P, Clue.m_iRow, Clue.m_iHorizontal1, true);
			AddSolved(P, Clue.m_iRow2, Clue.m_iCol2);
			AddSolved(P, Clue.m_iRow3, Clue.m_iCol3);
			return true;
		case eHorizontalType::SpanNotRight:
			Kind = EKind::SpanNotSide;
			Odd = 2;
			Middle = 1;
			Beside = 0;
			AddSolved(P, Clue.m_iRow, Clue.m_iCol);
			AddSolved(P, Clue.m_iRow2, Clue.m_iCol2);
			Add(P, Clue.m_iRow3, Clue.m_iHorizontal1, true);
			return true;
		case eHorizontalType::SpanNotMid:
			Kind = EKind::SpanNotMid;
			AddSolved(P, Clue.m_iRow, Clue.m_iCol);
			Add(P, Clue.m_iRow2, Clue.m_iHorizontal1, true);
			AddSolved(P, Clue.m_iRow3, Clue.m_iCol3);
			return true;
		default:
			break;
	}

	// Extended clues: display slots 0..2 (unused slots have row -1); NextToEitherOr's decoy is slot m_iNotCell
	switch (Clue.m_HorizontalType)
	{
		case eHorizontalType::Edge:				Kind = EKind::Edge; break;
		case eHorizontalType::NotEdge:			Kind = EKind::NotEdge; break;
		case eHorizontalType::DirectlyLeftOf:	Kind = EKind::DirectlyLeftOf; break;
		case eHorizontalType::Gap:				Kind = EKind::Gap; break;
		case eHorizontalType::Between:			Kind = EKind::Between; break;
		case eHorizontalType::Chain:			Kind = EKind::Chain; break;
		case eHorizontalType::NextToEitherOr:	Kind = EKind::NextToEitherOr; break;
		case eHorizontalType::AllApart:			Kind = EKind::AllApart; break;
		default:								return false;
	}
	int Rows[3];
	int Icons[3];
	Clue.GetSlots(P, Rows, Icons);
	for (int32 Slot = 0; Slot < 3 && Rows[Slot] >= 0; Slot++)
	{
		const bool bDecoy = Kind == EKind::NextToEitherOr && Slot == Clue.m_iNotCell;
		Add(P, Rows[Slot], Icons[Slot], bDecoy);
	}
	return Pieces.Num() > 0;
}

TArray<int32> FClueExample::SolutionColumns() const
{
	TArray<int32> Columns;
	for (const FClueExamplePiece& Piece : Pieces)
	{
		Columns.Add(Piece.SolutionColumn);
	}
	return Columns;
}

bool FClueExample::Holds(const TArray<int32>& C) const
{
	if (C.Num() != Pieces.Num())
	{
		return false;
	}
	auto Adjacent = [](int32 A, int32 B) { return FMath::Abs(A - B) == 1; };
	switch (Kind)
	{
		case EKind::SameColumn:
			for (int32 Column : C)
			{
				if (Column != C[0])
				{
					return false;
				}
			}
			return true;
		case EKind::NotSameColumn:		return C[0] != C[1];
		case EKind::TwoTogetherOneNot:
		{
			const int32 A = Odd == 0 ? 1 : 0;
			const int32 B = Odd == 2 ? 1 : 2;
			return C[A] == C[B] && C[Odd] != C[A];
		}
		case EKind::EitherOr:			return (C[1] == C[0]) != (C[2] == C[0]);
		case EKind::NextTo:				return Adjacent(C[0], C[1]);
		case EKind::NotNextTo:			return !Adjacent(C[0], C[1]);
		case EKind::LeftOf:				return C[0] < C[1];
		case EKind::NotLeftOf:			return C[0] >= C[1];
		case EKind::Span:				return Adjacent(C[0], C[1]) && Adjacent(C[2], C[1]) && C[0] != C[2];
		case EKind::SpanNotSide:		return Adjacent(C[Beside], C[Middle]) && C[Odd] != 2 * C[Middle] - C[Beside];
		case EKind::SpanNotMid:			return FMath::Abs(C[0] - C[2]) == 2 && C[1] != (C[0] + C[2]) / 2;
		case EKind::Edge:				return C[0] == 0 || C[0] == Size - 1;
		case EKind::NotEdge:			return C[0] > 0 && C[0] < Size - 1;
		case EKind::DirectlyLeftOf:		return C[1] == C[0] + 1;
		case EKind::Gap:				return FMath::Abs(C[0] - C[1]) == 2;
		case EKind::Between:			return (C[0] < C[1] && C[1] < C[2]) || (C[2] < C[1] && C[1] < C[0]);
		case EKind::Chain:				return C[0] < C[1] && C[1] < C[2];
		case EKind::NextToEitherOr:		return Adjacent(C[1], C[0]) != Adjacent(C[1], C[2]);
		case EKind::AllApart:			return C[0] != C[1] && C[1] != C[2] && C[0] != C[2];
		default:						return true;
	}
}

bool FClueExample::FindBroken(TArray<int32>& OutColumns, int32& OutMovedPiece) const
{
	const TArray<int32> Solved = SolutionColumns();
	// The last pieces first, the nearest columns first
	for (int32 Piece = Pieces.Num() - 1; Piece >= 0; Piece--)
	{
		for (int32 Distance = 1; Distance < Size; Distance++)
		{
			for (const int32 Column : { Solved[Piece] - Distance, Solved[Piece] + Distance })
			{
				if (Column < 0 || Column >= Size)
				{
					continue;
				}
				// Two icons of one row can't share a cell
				bool bFree = true;
				for (int32 Other = 0; Other < Pieces.Num(); Other++)
				{
					bFree &= Other == Piece || Pieces[Other].Row != Pieces[Piece].Row || Solved[Other] != Column;
				}
				TArray<int32> Columns = Solved;
				Columns[Piece] = Column;
				if (bFree && !Holds(Columns))
				{
					OutColumns = Columns;
					OutMovedPiece = Piece;
					return true;
				}
			}
		}
	}

	// Some need two moves (Not Left Of with the first icon in the last column and the second in the first):
	// any arrangement of the pieces, one row's icons in different columns
	TArray<int32> Columns;
	Columns.SetNumZeroed(Pieces.Num());
	TFunction<bool(int32)> Try = [&](int32 Piece) -> bool
	{
		if (Piece == Pieces.Num())
		{
			return !Holds(Columns);
		}
		for (int32 Column = 0; Column < Size; Column++)
		{
			bool bFree = true;
			for (int32 Other = 0; Other < Piece; Other++)
			{
				bFree &= Pieces[Other].Row != Pieces[Piece].Row || Columns[Other] != Column;
			}
			Columns[Piece] = Column;
			if (bFree && Try(Piece + 1))
			{
				return true;
			}
		}
		return false;
	};
	if (Try(0))
	{
		OutColumns = Columns;
		OutMovedPiece = INDEX_NONE;
		for (int32 Piece = Pieces.Num() - 1; Piece >= 0 && OutMovedPiece == INDEX_NONE; Piece--)
		{
			OutMovedPiece = Columns[Piece] != Solved[Piece] ? Piece : INDEX_NONE;
		}
		return true;
	}
	return false;
}
