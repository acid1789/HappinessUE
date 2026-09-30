#include "Hint.h"
#include "Puzzle.h"
#include "PuzzleRow.h"
#include "Clue.h"

namespace
{
	// All Apart: when two of its icons can only be in the same two columns, they take both of them between them.
	// Returns those two columns (sorted), or an empty array. Columns[Slot] are the columns each slot can be in.
	TArray<int> ApartPairColumns(const UClue& Clue, const TArray<TArray<int>>& Columns, int Other1, int Other2)
	{
		if (Clue.m_Type != eClueType::Horizontal || Clue.m_HorizontalType != eHorizontalType::AllApart)
			return {};
		TArray<int> Union = Columns[Other1];
		for (int c : Columns[Other2])
			Union.AddUnique(c);
		if (Union.Num() != 2 || Columns[Other1].Num() == 0 || Columns[Other2].Num() == 0)
			return {};
		Union.Sort();
		return Union;
	}

	FString ColumnPair(const TArray<int>& Pair)
	{
		return FString::Printf(TEXT("columns %d and %d"), Pair[0] + 1, Pair[1] + 1);
	}

	// The slots of a clue that must all be in the same column (Same Column, Three in a Column, and the pair of the
	// "Two Together, One Not" clues); empty for other clues
	TArray<int> GetTogetherSlots(const UClue& Clue)
	{
		if (Clue.m_Type != eClueType::Vertical)
			return {};
		switch (Clue.m_VerticalType)
		{
			case eVerticalType::Two:			return { 0, 1 };
			case eVerticalType::Three:			return { 0, 1, 2 };
			case eVerticalType::ThreeTopNot:	return { 1, 2 };
			case eVerticalType::ThreeMidNot:	return { 0, 2 };
			case eVerticalType::ThreeBotNot:	return { 0, 1 };
			default:							return {};
		}
	}
}

bool UHint::Init(UPuzzle& P, UClue& C, bool bPreferPlacement)
{
	TheClue = &C;
	if (!C.GetHintAction(P, bSetFinalIcon, Row, Col, Icon))
	{
		return false;
	}

	const bool bOwnSet = bSetFinalIcon;
	const int OwnRow = Row, OwnCol = Col, OwnIcon = Icon;

	if (bPreferPlacement)
	{
		// Icons that must share a column with only one column in common on the board: place one that isn't
		// placed yet there. One look at the board: "[Cosmos] and [Sundae] can only be together in this column"
		TArray<int> ClueSlotRows, ClueSlotIcons;
		C.GetRows(ClueSlotRows);
		C.GetIcons(&P, ClueSlotIcons);
		if (!bSetFinalIcon && C.m_Type == eClueType::Horizontal && C.m_HorizontalType == eHorizontalType::AllApart && ClueSlotIcons.Num() >= 3)
		{
			TArray<TArray<int>> Columns;
			for (int Slot = 0; Slot < 3; Slot++)
			{
				TArray<int> Can;
				for (int c = 0; c < P.m_iSize; c++)
					if (P.m_Rows[ClueSlotRows[Slot]].m_Cells[c].m_bValues[ClueSlotIcons[Slot]])
						Can.Add(c);
				Columns.Add(Can);
			}
			for (int Slot = 0; Slot < 3 && !bSetFinalIcon; Slot++)
			{
				const TArray<int> Pair = ApartPairColumns(C, Columns, (Slot + 1) % 3, (Slot + 2) % 3);
				if (Pair.Num() != 2 || Columns[Slot].Num() < 2)
					continue;
				TArray<int> Left = Columns[Slot];
				Left.Remove(Pair[0]);
				Left.Remove(Pair[1]);
				if (Left.Num() == 1)
				{
					bSetFinalIcon = true;
					Row = ClueSlotRows[Slot];
					Col = Left[0];
					Icon = ClueSlotIcons[Slot];
				}
			}
		}

		const TArray<int> Together = GetTogetherSlots(C);
		if (Together.Num() >= 2 && !bSetFinalIcon)
		{
			TArray<int> Common;
			for (int c = 0; c < P.m_iSize; c++)
			{
				bool bAll = true;
				for (int Slot : Together)
					bAll &= ClueSlotRows.IsValidIndex(Slot) && ClueSlotIcons.IsValidIndex(Slot) && P.m_Rows[ClueSlotRows[Slot]].m_Cells[c].m_bValues[ClueSlotIcons[Slot]];
				if (bAll)
					Common.Add(c);
			}
			if (Common.Num() == 1)
			{
				for (int Slot : Together)
				{
					int Columns = 0;
					for (int c = 0; c < P.m_iSize; c++)
						Columns += P.m_Rows[ClueSlotRows[Slot]].m_Cells[c].m_bValues[ClueSlotIcons[Slot]] ? 1 : 0;
					if (Columns > 1)
					{
						bSetFinalIcon = true;
						Row = ClueSlotRows[Slot];
						Col = Common[0];
						Icon = ClueSlotIcons[Slot];
						break;
					}
				}
			}
		}

		// Everything the clue works out from the board, applied until it stops changing anything
		TArray<int> ClueRows, ClueIcons;
		C.GetRows(ClueRows);
		C.GetIcons(&P, ClueIcons);
		const TArray<FPuzzleRow> Board = P.m_Rows;
		const int SavedUseCount = C.m_iUseCount;
		auto CountColumns = [&P](int SlotRow, int SlotIcon, int* OnlyColumn)
		{
			int Count = 0;
			for (int c = 0; c < P.m_iSize; c++)
			{
				if (P.m_Rows[SlotRow].m_Cells[c].m_bValues[SlotIcon])
				{
					Count++;
					*OnlyColumn = c;
				}
			}
			return Count;
		};

		auto CountAll = [&P]()
		{
			int Count = 0;
			for (const FPuzzleRow& BoardRow : P.m_Rows)
				for (const FPuzzleCell& Cell : BoardRow.m_Cells)
					for (int v = 0; v < Cell.m_bValues.Num(); v++)
						Count += Cell.m_bValues[v] ? 1 : 0;
			return Count;
		};
		const int CandidatesBefore = CountAll();

		TArray<int> Before;
		for (int i = 0; i < ClueIcons.Num() && i < ClueRows.Num(); i++)
		{
			int Unused;
			Before.Add(ClueRows[i] >= 0 ? CountColumns(ClueRows[i], ClueIcons[i], &Unused) : 0);
		}

		// The player's board may hold mistakes: a contradiction here mustn't count as an error (DebugError resets the board)
		const bool bWasHypothetical = P.m_bHypothetical;
		P.m_bHypothetical = true;
		for (int Pass = 0; Pass < 8; Pass++)
		{
			const int UseCount = C.m_iUseCount;
			C.Analyze(P);
			if (C.m_iUseCount == UseCount)
				break;
		}
		P.m_bHypothetical = bWasHypothetical;
		Impact = CandidatesBefore - CountAll();

		// What it removed, in case the hint has to be one of those single steps
		TArray<FIntVector> Removed;
		for (int r = 0; r < Board.Num(); r++)
			for (int c = 0; c < P.m_iSize; c++)
				for (int v = 0; v < P.m_iSize; v++)
					if (Board[r].m_Cells[c].m_bValues[v] && !P.m_Rows[r].m_Cells[c].m_bValues[v])
						Removed.Add(FIntVector(r, c, v));

		// An icon of the clue that's now down to one column: place it (unless the clue's own action already places one)
		for (int i = 0; i < Before.Num() && !bSetFinalIcon; i++)
		{
			int Only = -1;
			if (ClueRows[i] >= 0 && Before[i] > 1 && CountColumns(ClueRows[i], ClueIcons[i], &Only) == 1)
			{
				bSetFinalIcon = true;
				Row = ClueRows[i];
				Col = Only;
				Icon = ClueIcons[i];
				break;
			}
		}

		P.m_Rows = Board;
		C.m_iUseCount = SavedUseCount;

		// A hint is one step from the board as it is. A placement that needs other deductions first (clearing the
		// cell, or ruling the icon out of its other columns) isn't, whether it's promoted here or is the clue's own
		// action: give one of those deductions instead, one whose reason holds on the board as it is.
		auto IsReal = [](const FClueHelp& Why)
		{
			return Why.Segments.Num() > 0 && !(Why.Segments[0].Type == EClueHelpSegementType::Text && Why.Segments[0].Text == TEXT("So"));
		};
		{
			const FClueHelp Why = GetExplanation();
			if ((bSetFinalIcon && bIndirectPlacement) || (!bSetFinalIcon && !IsReal(Why)))
			{
				bool bFound = false;
				for (const FIntVector& Step : Removed)
				{
					bSetFinalIcon = false;
					Row = Step.X;
					Col = Step.Y;
					Icon = Step.Z;
					if (IsReal(GetExplanation()))
					{
						bFound = true;
						break;
					}
				}
				if (!bFound)
				{
					// No single step to give instead: keep the clue's own action
					bSetFinalIcon = bOwnSet;
					Row = OwnRow;
					Col = OwnCol;
					Icon = OwnIcon;
				}
			}
		}
	}

	return true;
}

FClueHelp UHint::GetActionHelp() const
{
	FClueHelp Help;
	Help.AddText(TEXT("So"));
	Help.AddIcon(Row, Icon);
	Help.AddText(bSetFinalIcon
		? FString::Printf(TEXT("goes in column %d"), Col + 1)
		: FString::Printf(TEXT("can't be in column %d"), Col + 1));
	return Help;
}

namespace
{
	FString ColumnList(const TArray<int>& Columns)
	{
		// "column 2", "column 2 or 4", "column 1, 3 or 5"
		FString List;
		for (int i = 0; i < Columns.Num(); i++)
		{
			if (i > 0)
				List += (i == Columns.Num() - 1) ? TEXT(" or ") : TEXT(", ");
			List += FString::FromInt(Columns[i] + 1);
		}
		return TEXT("column ") + List;
	}

	// How a clue relates its icons' columns, which decides the words that fit when describing where one must be
	enum class EPlaceWords : uint8
	{
		None,			// column numbers only
		Direction,		// left of / right of, any distance (Left Of, Not Left Of, Chain, Between)
		Adjacent,		// directly beside, next to, and two apart for the ends of a span
		TwoApart,		// exactly one column between (Gap, Gap Without)
	};

	EPlaceWords GetPlaceWords(const UClue& Clue)
	{
		if (Clue.m_Type != eClueType::Horizontal)
			return EPlaceWords::None;

		switch (Clue.m_HorizontalType)
		{
			case eHorizontalType::LeftOf:
			case eHorizontalType::NotLeftOf:
			case eHorizontalType::Chain:
			case eHorizontalType::Between:
				return EPlaceWords::Direction;
			case eHorizontalType::DirectlyLeftOf:
			case eHorizontalType::NextTo:
			case eHorizontalType::Span:
			case eHorizontalType::SpanNotLeft:
			case eHorizontalType::SpanNotRight:
			case eHorizontalType::NextToEitherOr:
				return EPlaceWords::Adjacent;
			case eHorizontalType::Gap:
			case eHorizontalType::SpanNotMid:
				return EPlaceWords::TwoApart;
			default:
				return EPlaceWords::None;
		}
	}

	// Clues three columns wide that can face either way (spans and gaps). When an end icon is in column Col, too
	// near an edge for the clue to go that way, and Needed (where a partner has to be) is all on the other side:
	// that side, +1 right or -1 left. 0 otherwise.
	int EdgeForcedSide(const UClue& Clue, const TArray<int>& Needed, int Col, int Size)
	{
		if (Clue.m_Type != eClueType::Horizontal || Needed.Num() == 0)
			return 0;
		switch (Clue.m_HorizontalType)
		{
			case eHorizontalType::Span:
			case eHorizontalType::SpanNotLeft:
			case eHorizontalType::SpanNotRight:
			case eHorizontalType::Gap:
			case eHorizontalType::SpanNotMid:
				break;
			default:
				return 0;
		}

		bool bAllRight = true, bAllLeft = true;
		for (int c : Needed)
		{
			bAllRight &= c > Col;
			bAllLeft &= c < Col;
		}
		if (bAllRight && Col < 2)
			return 1;
		if (bAllLeft && Col > Size - 3)
			return -1;
		return 0;
	}

	// Columns described relative to Col, in words that fit the clue: "right of this column", "next to this
	// column" and so on. Empty when they don't make such a place. Ever are the columns the clue ever allows that
	// icon (the middle of a chain is never in the last column): "right of this column" means anywhere right of
	// it that the clue allows.
	FString DescribePlace(const TArray<int>& Columns, int Col, int Size, EPlaceWords Words, const TArray<int>& Ever)
	{
		TArray<int> Sorted = Columns;
		Sorted.Sort();
		if (Sorted.Num() == 0 || Words == EPlaceWords::None)
			return FString();

		auto Where = [Size, &Sorted, &Ever](auto Test)
		{
			TArray<int> Wanted;
			for (int c = 0; c < Size; c++)
				if (Test(c) && Ever.Contains(c))
					Wanted.Add(c);
			return Wanted.Num() > 0 && Sorted == Wanted;
		};

		switch (Words)
		{
			case EPlaceWords::Direction:
				if (Where([Col](int c) { return c > Col; }))			return TEXT("right of this column");
				if (Where([Col](int c) { return c < Col; }))			return TEXT("left of this column");
				if (Where([Col](int c) { return c >= Col; }))			return TEXT("in or right of this column");
				if (Where([Col](int c) { return c <= Col; }))			return TEXT("in or left of this column");
				if (Where([Col](int c) { return c >= Col + 2; }))		return TEXT("two or more columns right of this one");
				if (Where([Col](int c) { return c <= Col - 2; }))		return TEXT("two or more columns left of this one");
				if (Where([Col](int c) { return FMath::Abs(c - Col) >= 2; }))	return TEXT("two or more columns away from this one");
				break;

			case EPlaceWords::Adjacent:
				if (Where([Col](int c) { return c == Col + 1; }))		return TEXT("directly right of this column");
				if (Where([Col](int c) { return c == Col - 1; }))		return TEXT("directly left of this column");
				if (Where([Col](int c) { return FMath::Abs(c - Col) == 1; }))	return TEXT("next to this column");
				if (Where([Col](int c) { return FMath::Abs(c - Col) == 2; }))	return TEXT("two columns from this one");
				break;

			case EPlaceWords::TwoApart:
				if (Where([Col](int c) { return FMath::Abs(c - Col) == 2; }))	return TEXT("two columns from this one");
				break;

			default:
				break;
		}
		return FString();
	}

	// One column relative to Col: "in this column", "directly right of this column", "two columns left of this one"
	FString DescribeColumn(int Column, int Col)
	{
		const int Offset = Column - Col;
		switch (Offset)
		{
			case 0:		return TEXT("in this column");
			case 1:		return TEXT("directly right of this column");
			case -1:	return TEXT("directly left of this column");
			case 2:		return TEXT("two columns right of this one");
			case -2:	return TEXT("two columns left of this one");
			default:	return FString::Printf(TEXT("in column %d"), Column + 1);
		}
	}

	// Whether columns Cols[0..2] for a three-icon clue's slots satisfy it, from the clue's meaning alone (the old
	// analyzers aren't made for arbitrary placements). False for clue types it doesn't know.
	bool ThreeSlotsHold(const UClue& Clue, const int Cols[3], int Size)
	{
		const int C0 = Cols[0], C1 = Cols[1], C2 = Cols[2];
		auto Beside = [](int X, int Y) { return FMath::Abs(X - Y) == 1; };

		if (Clue.m_Type == eClueType::Vertical)
		{
			switch (Clue.m_VerticalType)
			{
				case eVerticalType::Three:			return C0 == C1 && C1 == C2;
				case eVerticalType::ThreeTopNot:	return C1 == C2 && C0 != C1;
				case eVerticalType::ThreeMidNot:	return C0 == C2 && C1 != C0;
				case eVerticalType::ThreeBotNot:	return C0 == C1 && C2 != C0;
				case eVerticalType::EitherOr:		return (C0 == C1) != (C0 == C2);
				default:							return false;
			}
		}
		if (Clue.m_Type != eClueType::Horizontal)
			return false;

		switch (Clue.m_HorizontalType)
		{
			case eHorizontalType::Span:
				// Slot 1 in the middle, slots 0 and 2 beside it on opposite sides
				return Beside(C0, C1) && Beside(C2, C1) && C0 != C2;
			case eHorizontalType::SpanNotLeft:
			case eHorizontalType::SpanNotRight:
			{
				// Slot 1 in the middle with the real neighbor beside it; the other side is on the board and
				// doesn't hold the crossed-out icon
				const bool bLeft = Clue.m_HorizontalType == eHorizontalType::SpanNotLeft;
				const int Real = bLeft ? C2 : C0;
				const int Crossed = bLeft ? C0 : C2;
				const int Other = C1 - (Real - C1);
				return Beside(Real, C1) && Other >= 0 && Other < Size && Crossed != Other;
			}
			case eHorizontalType::SpanNotMid:
				// Slots 0 and 2 two apart, the crossed-out slot 1 not between them
				return FMath::Abs(C0 - C2) == 2 && C1 != (C0 + C2) / 2;
			case eHorizontalType::Between:			return (C0 < C1 && C1 < C2) || (C2 < C1 && C1 < C0);
			case eHorizontalType::Chain:			return C0 < C1 && C1 < C2;
			case eHorizontalType::AllApart:			return C0 != C1 && C1 != C2 && C0 != C2;
			case eHorizontalType::NextToEitherOr:	return Beside(C1, C0) != Beside(C1, C2);
			default:								return false;
		}
	}

	// Whether the clue relates slots X and Y positively (says where one is from the other: same column, next to,
	// left of, around) rather than negatively (only where it isn't: not the same column, not next to, apart, or
	// the crossed-out icon of a "not" clue). Only a positive relation can point an icon to a few columns.
	bool IsPositivePair(const UClue& Clue, int X, int Y)
	{
		int Crossed = -1;	// the crossed-out slot of a clue that has one
		if (Clue.m_Type == eClueType::Vertical)
		{
			switch (Clue.m_VerticalType)
			{
				case eVerticalType::TwoNot:			return false;
				case eVerticalType::ThreeTopNot:	Crossed = 0; break;
				case eVerticalType::ThreeMidNot:	Crossed = 1; break;
				case eVerticalType::ThreeBotNot:	Crossed = 2; break;
				default:							break;
			}
		}
		else if (Clue.m_Type == eClueType::Horizontal)
		{
			switch (Clue.m_HorizontalType)
			{
				case eHorizontalType::NotNextTo:
				case eHorizontalType::NotLeftOf:
				case eHorizontalType::AllApart:
				case eHorizontalType::NotEdge:		return false;
				case eHorizontalType::SpanNotLeft:	Crossed = 0; break;
				case eHorizontalType::SpanNotRight:	Crossed = 2; break;
				case eHorizontalType::SpanNotMid:	Crossed = 1; break;
				default:							break;
			}
		}
		return X != Crossed && Y != Crossed;
	}

	int CountCandidates(const TArray<FPuzzleRow>& Rows)
	{
		int Count = 0;
		for (const FPuzzleRow& Row : Rows)
			for (const FPuzzleCell& Cell : Row.m_Cells)
				for (int i = 0; i < Cell.m_bValues.Num(); i++)
					Count += Cell.m_bValues[i] ? 1 : 0;
		return Count;
	}
}

void UHint::ExplainNoRoom(FClueHelp& Help, int Subject, const TArray<int>& SlotRows, const TArray<int>& SlotIcons, int Size) const
{
	auto AddSlot = [&](int Slot) { Help.AddIcon(SlotRows[Slot], SlotIcons[Slot]); };
	auto AddSubject = [&]() { Help.AddIcon(Row, Icon); };
	const bool bFirst = Col == 0;
	const bool bLast = Col == Size - 1;
	const bool bNoRoomEitherWay = Col - 2 < 0 && Col + 2 >= Size;	// an icon two columns away doesn't fit on either side

	if (TheClue->m_Type == eClueType::Horizontal && SlotRows.Num() >= 2)
	{
		switch (TheClue->m_HorizontalType)
		{
			case eHorizontalType::Span:
				// Slot 1 is in the middle, with slots 0 and 2 on either side
				if (Subject == 1 && (bFirst || bLast))
				{
					Help.AddText(TEXT("Since"));
					AddSubject();
					Help.AddText(TEXT("has"));
					AddSlot(0);
					Help.AddText(TEXT("and"));
					AddSlot(2);
					Help.AddText(TEXT("on either side of it, it can't be in this column, which has only one neighboring column"));
					return;
				}
				if (Subject != 1 && bNoRoomEitherWay)
				{
					Help.AddText(TEXT("Since"));
					AddSlot(0);
					Help.AddText(TEXT("and"));
					AddSlot(2);
					Help.AddText(TEXT("are on either side of"));
					AddSlot(1);
					Help.AddText(TEXT(", there's no room for all three with"));
					AddSubject();
					Help.AddText(TEXT("in this column"));
					return;
				}
				break;

			case eHorizontalType::SpanNotLeft:
			case eHorizontalType::SpanNotRight:
			{
				// Slot 1 is in the middle: the real neighbor on one side, the crossed-out icon not on the other
				const int Real = TheClue->m_HorizontalType == eHorizontalType::SpanNotLeft ? 2 : 0;
				const int Crossed = 2 - Real;
				if (Subject == 1 && (bFirst || bLast))
				{
					Help.AddText(TEXT("Since"));
					AddSubject();
					Help.AddText(TEXT("has"));
					AddSlot(Real);
					Help.AddText(TEXT("on one side and something other than"));
					AddSlot(Crossed);
					Help.AddText(TEXT("on the other, it can't be in this column, which has only one neighboring column"));
					return;
				}
				if (Subject == Real && bNoRoomEitherWay)
				{
					Help.AddText(TEXT("Since"));
					AddSubject();
					Help.AddText(TEXT("is next to"));
					AddSlot(1);
					Help.AddText(TEXT(", which needs a neighbor on its other side too, there's no room for"));
					AddSubject();
					Help.AddText(TEXT("in this column"));
					return;
				}
				break;
			}

			case eHorizontalType::DirectlyLeftOf:
			case eHorizontalType::LeftOf:
			{
				// Slot 0 is left of slot 1: right next to it, or anywhere left of it
				const bool bDirectly = TheClue->m_HorizontalType == eHorizontalType::DirectlyLeftOf;
				if ((Subject == 0 && bLast) || (Subject == 1 && bFirst))
				{
					Help.AddText(TEXT("Since"));
					AddSubject();
					if (Subject == 0)
						Help.AddText(bDirectly ? TEXT("is directly left of") : TEXT("is somewhere left of"));
					else
						Help.AddText(bDirectly ? TEXT("is directly right of") : TEXT("is somewhere right of"));
					AddSlot(1 - Subject);
					Help.AddText(Subject == 0 ? TEXT(", it can't be in the last column") : TEXT(", it can't be in the first column"));
					return;
				}
				break;
			}

			case eHorizontalType::Chain:
				// Slot 0 is left of slot 1, which is left of slot 2
				if (Subject == 0 && Col >= Size - 2)	// the last two columns
				{
					Help.AddText(TEXT("Since"));
					AddSubject();
					Help.AddText(TEXT("has"));
					AddSlot(1);
					Help.AddText(TEXT("and"));
					AddSlot(2);
					Help.AddText(TEXT("to its right, it can't be in the last two columns"));
					return;
				}
				if (Subject == 2 && Col <= 1)	// the first two columns
				{
					Help.AddText(TEXT("Since"));
					AddSubject();
					Help.AddText(TEXT("has"));
					AddSlot(0);
					Help.AddText(TEXT("and"));
					AddSlot(1);
					Help.AddText(TEXT("to its left, it can't be in the first two columns"));
					return;
				}
				if (Subject == 1 && (bFirst || bLast))
				{
					// "Since [Elephant] has [Plum] to its right, it can't be in the last column"
					Help.AddText(TEXT("Since"));
					AddSubject();
					Help.AddText(TEXT("has"));
					AddSlot(bLast ? 2 : 0);
					Help.AddText(bLast ? TEXT("to its right, it can't be in the last column") : TEXT("to its left, it can't be in the first column"));
					return;
				}
				break;

			case eHorizontalType::Between:
				// Slot 1 is somewhere between slots 0 and 2
				if (Subject == 1 && (bFirst || bLast))
				{
					Help.AddText(TEXT("Since"));
					AddSubject();
					Help.AddText(TEXT("is between"));
					AddSlot(0);
					Help.AddText(TEXT("and"));
					AddSlot(2);
					Help.AddText(TEXT(", it can't be in the first or last column"));
					return;
				}
				break;

			default:
				break;
		}
	}

	// Anything else: the clue's own sentence as the reason. "Since [clue], [Top] can't be in this column"
	Help.AddText(TEXT("Since"));
	Help.Segments.Append(TheClue->ClueHelp.Segments);
	Help.AddText(TEXT(","));
	AddSubject();
	Help.AddText(TEXT("can't be in this column"));
}

bool UHint::ExplainPlacementByCell(FClueHelp& Help)
{
	UPuzzle* P = GetTypedOuter<UPuzzle>();
	if (!P || !TheClue || !bSetFinalIcon)
		return false;

	// What the clue works out from the board, applied fully: which icons it clears out of this cell
	const TArray<FPuzzleRow> Board = P->m_Rows;
	const int SavedUseCount = TheClue->m_iUseCount;
	const bool bWasHypothetical = P->m_bHypothetical;
	P->m_bHypothetical = true;
	for (int Pass = 0; Pass < 8; Pass++)
	{
		const int UseCount = TheClue->m_iUseCount;
		TheClue->Analyze(*P);
		if (TheClue->m_iUseCount == UseCount)
			break;
	}
	TArray<int> Cleared;
	bool bOnlySubjectLeft = P->m_Rows[Row].m_Cells[Col].m_bValues[Icon];
	for (int i = 0; i < P->m_iSize; i++)
	{
		const bool bBefore = Board[Row].m_Cells[Col].m_bValues[i];
		const bool bAfter = P->m_Rows[Row].m_Cells[Col].m_bValues[i];
		if (i != Icon && bAfter)
			bOnlySubjectLeft = false;
		if (i != Icon && bBefore && !bAfter)
			Cleared.Add(i);
	}
	P->m_Rows = Board;
	TheClue->m_iUseCount = SavedUseCount;
	P->m_bHypothetical = bWasHypothetical;

	// Only when the clue is what left the subject alone in the cell, and only for a short walk-through
	if (!bOnlySubjectLeft || Cleared.Num() == 0 || Cleared.Num() > 3)
		return false;

	// Each cleared icon with its own reason, from the elimination explanation for it
	TArray<FClueHelp> Steps;
	const int SubjectIcon = Icon;
	bSetFinalIcon = false;
	for (int Other : Cleared)
	{
		Icon = Other;
		FClueHelp Step = GetExplanation();
		const bool bReal = Step.Segments.Num() > 0 && !(Step.Segments[0].Type == EClueHelpSegementType::Text && Step.Segments[0].Text == TEXT("So"));
		if (!bReal)
			break;
		Steps.Add(Step);
	}
	Icon = SubjectIcon;
	bSetFinalIcon = true;
	if (Steps.Num() != Cleared.Num())
		return false;

	// "There's no [Orange] two columns from this one, so [Banana] can't be here. There's no [Banana] two columns
	// from this one, so [Orange] can't be here. That leaves [Plum], so it goes here"
	for (const FClueHelp& Step : Steps)
	{
		Help.Segments.Append(Step.Segments);
		Help.AddText(TEXT("."));
	}
	Help.AddText(TEXT("That leaves"));
	Help.AddIcon(Row, Icon);
	Help.AddText(TEXT(", so it goes here"));
	return true;
}

bool UHint::ExplainPlacementByRow(FClueHelp& Help)
{
	UPuzzle* P = GetTypedOuter<UPuzzle>();
	if (!P || !TheClue || !bSetFinalIcon)
		return false;

	// The subject's other columns on the board
	TArray<int> Others;
	for (int c = 0; c < P->m_iSize; c++)
		if (c != Col && P->m_Rows[Row].m_Cells[c].m_bValues[Icon])
			Others.Add(c);
	if (Others.Num() == 0 || Others.Num() > 3)
		return false;

	// Why it can't be in each, from the elimination explanation for that column. Those say "here" and "this
	// column" for the column they're about; name it instead, since the hint points at the placement.
	TArray<FClueHelp> Steps;
	const int PlaceCol = Col;
	bSetFinalIcon = false;
	for (int Other : Others)
	{
		Col = Other;
		FClueHelp Step = GetExplanation();
		const bool bReal = Step.Segments.Num() > 0 && !(Step.Segments[0].Type == EClueHelpSegementType::Text && Step.Segments[0].Text == TEXT("So"));
		if (!bReal)
			break;

		const FString Named = FString::Printf(TEXT("column %d"), Other + 1);
		for (FClueHelpSegment& Segment : Step.Segments)
		{
			if (Segment.Type != EClueHelpSegementType::Text)
				continue;
			Segment.Text.ReplaceInline(TEXT("can't be here"), *(TEXT("can't be in ") + Named));
			Segment.Text.ReplaceInline(TEXT("this column"), *Named);
			Segment.Text.ReplaceInline(TEXT("this one"), *Named);
		}
		Steps.Add(Step);
	}
	Col = PlaceCol;
	bSetFinalIcon = true;
	if (Steps.Num() != Others.Num())
		return false;

	// Steps giving the same reason merge: "[P] is in column 4, so [S] can't be in column 1 or 3". Each step ends
	// with the text "... can't be in column N"; the reason is everything before that ending.
	auto Reason = [](const FClueHelp& Step)
	{
		FString Key;
		for (int i = 0; i < Step.Segments.Num(); i++)
		{
			const FClueHelpSegment& Segment = Step.Segments[i];
			FString Text = Segment.Text;
			if (i == Step.Segments.Num() - 1)
			{
				const int At = Text.Find(TEXT("can't be in column"));
				Text = At >= 0 ? Text.Left(At) : Text;
			}
			Key += Segment.Type == EClueHelpSegementType::Icon ? FString::Printf(TEXT("[%d:%d]"), Segment.IconRow, Segment.IconColumn) : Text;
			Key += TEXT("|");
		}
		return Key;
	};

	RowWalkReasons = 0;
	for (int i = 0; i < Steps.Num(); i++)
	{
		RowWalkReasons++;
		TArray<int> Columns = { Others[i] };
		while (i + 1 < Steps.Num() && Reason(Steps[i + 1]) == Reason(Steps[i]) && !Reason(Steps[i]).Contains(FString::Printf(TEXT("column %d"), Others[i] + 1)))
		{
			i++;
			Columns.Add(Others[i]);
		}

		FClueHelp Step = Steps[i];
		FClueHelpSegment& Last = Step.Segments.Last();
		const int At = Last.Text.Find(TEXT("can't be in column"));
		if (Columns.Num() > 1 && Last.Type == EClueHelpSegementType::Text && At >= 0)
			Last.Text = Last.Text.Left(At) + TEXT("can't be in ") + ColumnList(Columns);

		// "There's no [Orange] two columns from column 2, so [Plum] can't be in column 2. ... That leaves only
		// this column for [Plum], so it goes here"
		Help.Segments.Append(Step.Segments);
		Help.AddText(TEXT("."));
	}
	Help.AddText(TEXT("That leaves only this column for"));
	Help.AddIcon(Row, Icon);
	Help.AddText(TEXT(", so it goes here"));
	return true;
}

FClueHelp UHint::GetExplanation()
{
	bIndirectPlacement = false;
	UPuzzle* P = GetTypedOuter<UPuzzle>();
	if (!P || !TheClue)
		return GetActionHelp();


	// The clue's icons: parallel rows and icons, as the clue help uses them
	TArray<int> ClueRows, ClueIcons;
	TheClue->GetRows(ClueRows);
	TheClue->GetIcons(P, ClueIcons);
	TArray<int> SlotRows, SlotIcons;
	int Subject = INDEX_NONE;
	for (int i = 0; i < ClueIcons.Num() && i < ClueRows.Num(); i++)
	{
		if (ClueRows[i] < 0)
			continue;
		if (ClueRows[i] == Row && ClueIcons[i] == Icon)
			Subject = SlotRows.Num();
		SlotRows.Add(ClueRows[i]);
		SlotIcons.Add(ClueIcons[i]);
	}
	if (Subject == INDEX_NONE)
		return GetActionHelp();

	const TArray<FPuzzleRow> Board = P->m_Rows;
	const int SavedUseCount = TheClue->m_iUseCount;
	P->m_bHypothetical = true;
	const int Size = P->m_iSize;

	// Columns each slot's icon can still be in on the board, and the column it's placed in (-1 if not yet known)
	auto Possible = [&Board, Size](int SlotRow, int SlotIcon)
	{
		TArray<int> Columns;
		for (int c = 0; c < Size; c++)
			if (Board[SlotRow].m_Cells[c].m_bValues[SlotIcon])
				Columns.Add(c);
		return Columns;
	};
	auto PlacedColumn = [&Possible](int SlotRow, int SlotIcon)
	{
		const TArray<int> Columns = Possible(SlotRow, SlotIcon);
		return Columns.Num() == 1 ? Columns[0] : -1;
	};

	// What the clue rules out by itself, on an empty board (edge columns and the like). The analyzers assume these
	// are gone, so every "what if" starts from here.
	auto Settle = [&]()
	{
		for (int Pass = 0; Pass < 8; Pass++)
		{
			const int Before = CountCandidates(P->m_Rows);
			TheClue->Analyze(*P);
			if (CountCandidates(P->m_Rows) == Before)
				break;
		}
	};
	TArray<FPuzzleRow> Base = Board;
	for (FPuzzleRow& BaseRow : Base)
		BaseRow.Reset();
	P->m_Rows = Base;
	Settle();
	Base = P->m_Rows;
	auto AllowedAlone = [&Base](int SlotRow, int SlotIcon, int Column) { return Base[SlotRow].m_Cells[Column].m_bValues[SlotIcon]; };
	auto EverAllowed = [&](int Slot)
	{
		TArray<int> Columns;
		for (int c = 0; c < Size; c++)
			if (AllowedAlone(SlotRows[Slot], SlotIcons[Slot], c))
				Columns.Add(c);
		return Columns;
	};

	// With only slot Fixed placed, in column FixedCol: the columns the clue leaves slot Other.
	// Empty if the clue alone rules out slot Fixed in that column.
	auto Allowed = [&](int Fixed, int FixedCol, int Other)
	{
		TArray<int> Columns;
		if (!AllowedAlone(SlotRows[Fixed], SlotIcons[Fixed], FixedCol))
			return Columns;

		P->m_Rows = Base;
		P->SetFinalIcon(SlotRows[Fixed], FixedCol, SlotIcons[Fixed]);
		Settle();
		for (int c = 0; c < Size; c++)
			if (P->m_Rows[SlotRows[Other]].m_Cells[c].m_bValues[SlotIcons[Other]])
				Columns.Add(c);
		return Columns;
	};

	FClueHelp Help;
	auto Where = [this](int Column) { return Column == Col ? FString(TEXT("is in this column, so")) : FString::Printf(TEXT("is in column %d, so"), Column + 1); };
	bool bExplained = false;

	if (!bSetFinalIcon)
	{
		// The subject can't be here: find a partner with nowhere to go if the subject were here
		for (int Other = 0; Other < SlotRows.Num() && !bExplained; Other++)
		{
			if (Other == Subject)
				continue;

			const TArray<int> Needed = Allowed(Subject, Col, Other);
			const TArray<int> Can = Possible(SlotRows[Other], SlotIcons[Other]);
			bool bOverlap = false;
			for (int c : Needed)
				bOverlap |= Can.Contains(c);
			if (bOverlap)
				continue;

			const int Placed = PlacedColumn(SlotRows[Other], SlotIcons[Other]);
			if (Needed.Num() == 0)
			{
				// The clue alone keeps the subject out of this column: say how. (Checked first: a partner being
				// placed somewhere isn't the reason then.)
				ExplainNoRoom(Help, Subject, SlotRows, SlotIcons, Size);
			}
			else if (Placed >= 0)
			{
				// "[Daisy] is in this column, so [Top] can't be here"
				Help.AddIcon(SlotRows[Other], SlotIcons[Other]);
				Help.AddText(Where(Placed));
				Help.AddIcon(Row, Icon);
				Help.AddText(TEXT("can't be here"));
			}
			else if (Needed.Num() == 1 && Needed[0] == Col)
			{
				// Same column: "[Daisy] isn't in this column, so [Pear] can't be either"
				Help.AddIcon(SlotRows[Other], SlotIcons[Other]);
				Help.AddText(TEXT("isn't in this column, so"));
				Help.AddIcon(Row, Icon);
				Help.AddText(TEXT("can't be either"));
			}
			else if (const int Side = EdgeForcedSide(*TheClue, Needed, Col, Size); Side != 0 && IsPositivePair(*TheClue, Subject, Other))
			{
				// Too near the edge for the clue to go the other way: "This clue needs three columns, so from
				// column 2 it has to go right. That puts [Fedora] in column 3, and there's no [Fedora] there, so
				// [Star] can't be here"
				Help.AddText(FString::Printf(TEXT("This clue needs three columns, so from column %d it has to go %s. That puts"),
					Col + 1, Side > 0 ? TEXT("right") : TEXT("left")));
				Help.AddIcon(SlotRows[Other], SlotIcons[Other]);
				Help.AddText(FString::Printf(TEXT("in %s, and there's no"), *ColumnList(Needed)));
				Help.AddIcon(SlotRows[Other], SlotIcons[Other]);
				Help.AddText(TEXT("there, so"));
				Help.AddIcon(Row, Icon);
				Help.AddText(TEXT("can't be here"));
			}
			else if (const FString Place = DescribePlace(Needed, Col, Size, GetPlaceWords(*TheClue), EverAllowed(Other)); !Place.IsEmpty())
			{
				// "There's no [Bowling Ball] right of this column, so [Duck] can't be here"
				Help.AddText(TEXT("There's no"));
				Help.AddIcon(SlotRows[Other], SlotIcons[Other]);
				Help.AddText(Place + TEXT(", so"));
				Help.AddIcon(Row, Icon);
				Help.AddText(TEXT("can't be here"));
			}
			else
			{
				// "[Bow] can't be in column 2 or 4, so [Top] can't be here"
				Help.AddIcon(SlotRows[Other], SlotIcons[Other]);
				Help.AddText(FString::Printf(TEXT("can't be in %s, so"), *ColumnList(Needed)));
				Help.AddIcon(Row, Icon);
				Help.AddText(TEXT("can't be here"));
			}
			bExplained = true;
		}

		// Either-or clues: the subject needs one of the two options in its column (vertical) or next to it
		// (NextToEitherOr), and neither can be there
		const bool bVerticalEitherOr = TheClue->m_Type == eClueType::Vertical && TheClue->m_VerticalType == eVerticalType::EitherOr;
		const bool bNextToEitherOr = TheClue->m_Type == eClueType::Horizontal && TheClue->m_HorizontalType == eHorizontalType::NextToEitherOr;
		const int Main = bVerticalEitherOr ? 0 : 1;
		if (!bExplained && (bVerticalEitherOr || bNextToEitherOr) && SlotRows.Num() == 3 && Subject == Main)
		{
			TArray<int> Spots;
			if (bVerticalEitherOr)
				Spots.Add(Col);
			else
			{
				if (Col > 0)
					Spots.Add(Col - 1);
				if (Col < Size - 1)
					Spots.Add(Col + 1);
			}

			bool bNeither = true;
			for (int Other = 0; Other < 3; Other++)
			{
				if (Other == Main)
					continue;
				const TArray<int> Can = Possible(SlotRows[Other], SlotIcons[Other]);
				for (int c : Spots)
					bNeither &= !Can.Contains(c);
			}

			if (bNeither)
			{
				// "Neither [A] nor [B] can be in this column, so [S] can't be here"
				const int A = Main == 0 ? 1 : 0;
				const int B = 2;
				Help.AddText(TEXT("Neither"));
				Help.AddIcon(SlotRows[A], SlotIcons[A]);
				Help.AddText(TEXT("nor"));
				Help.AddIcon(SlotRows[B], SlotIcons[B]);
				Help.AddText(bVerticalEitherOr ? TEXT("can be in this column, so") : TEXT("can be next to this column, so"));
				Help.AddIcon(Row, Icon);
				Help.AddText(TEXT("can't be here"));
				bExplained = true;
			}
		}

		// Three icons, where it's the two others together that don't fit
		if (!bExplained && SlotRows.Num() == 3)
		{
			const int A = Subject == 0 ? 1 : 0;
			const int B = Subject == 2 ? 1 : 2;

			// Every way the clue lets A and B go with the subject here, ignoring the board
			auto Valid = [&](int ColA, int ColB)
			{
				int Cols[3];
				Cols[Subject] = Col;
				Cols[A] = ColA;
				Cols[B] = ColB;
				for (int i = 0; i < 3; i++)
					for (int j = i + 1; j < 3; j++)
						if (SlotRows[i] == SlotRows[j] && Cols[i] == Cols[j])
							return false;	// two icons of one row can't share a cell
				return ThreeSlotsHold(*TheClue, Cols, Size);
			};
			TArray<TPair<int, int>> Arrangements;
			{
				for (int ColA = 0; ColA < Size; ColA++)
					for (int ColB = 0; ColB < Size; ColB++)
						if (Valid(ColA, ColB))
							Arrangements.Add({ ColA, ColB });
			}
			const TArray<int> CanA = Possible(SlotRows[A], SlotIcons[A]);
			const TArray<int> CanB = Possible(SlotRows[B], SlotIcons[B]);

			// Does the elimination follow from the board as it is? Only if no placement of the other two, among the
			// columns they can still be in, fits the clue with the subject here. Otherwise it takes more deductions
			// first, and none of the reasons below would be true.
			bool bFollowsFromBoard = true;
			for (int ColA : CanA)
			{
				for (int ColB : CanB)
				{
					int Cols[3];
					Cols[Subject] = Col;
					Cols[A] = ColA;
					Cols[B] = ColB;
					bool bShares = false;
					for (int i = 0; i < 3; i++)
						for (int j = i + 1; j < 3; j++)
							bShares |= SlotRows[i] == SlotRows[j] && Cols[i] == Cols[j];
					if (!bShares && ThreeSlotsHold(*TheClue, Cols, Size))
						bFollowsFromBoard = false;
				}
			}
			if (!bFollowsFromBoard)
			{
				// Not a single step: the plain action, which GetExplanation's callers treat as unexplained
				P->m_Rows = Board;
				TheClue->m_iUseCount = SavedUseCount;
				P->m_bHypothetical = false;
				return GetActionHelp();
			}

			// All Apart: the other two can only be in the same two columns, this being one of them
			// "[Cupcake] and [Cowboy Hat] can only be in columns 1 and 3, so [Bear] can't be here"
			if (!bExplained)
			{
				TArray<TArray<int>> Columns;
				for (int Slot = 0; Slot < 3; Slot++)
					Columns.Add(Possible(SlotRows[Slot], SlotIcons[Slot]));
				const TArray<int> Pair = ApartPairColumns(*TheClue, Columns, A, B);
				if (Pair.Num() == 2 && Pair.Contains(Col))
				{
					Help.AddIcon(SlotRows[A], SlotIcons[A]);
					Help.AddText(TEXT("and"));
					Help.AddIcon(SlotRows[B], SlotIcons[B]);
					Help.AddText(FString::Printf(TEXT("can only be in %s, so"), *ColumnPair(Pair)));
					Help.AddIcon(Row, Icon);
					Help.AddText(TEXT("can't be here"));
					bExplained = true;
				}
			}

			// Where the other two can go on the board as it is, fitting the clue with the subject somewhere. When
			// that's one way only, and it leaves no room for the subject here, say so: "[Cosmos] and [Sundae] can
			// only be together in this column, so [Peach] can't be here"
			if (!bExplained)
			{
				TArray<TPair<int, int>> BoardPairs;
				for (int ColA : CanA)
				{
					for (int ColB : CanB)
					{
						if (SlotRows[A] == SlotRows[B] && ColA == ColB)
							continue;
						bool bSomewhere = false;
						for (int c = 0; c < Size && !bSomewhere; c++)
						{
							int Cols[3];
							Cols[Subject] = c;
							Cols[A] = ColA;
							Cols[B] = ColB;
							const bool bShares = (SlotRows[Subject] == SlotRows[A] && c == ColA) || (SlotRows[Subject] == SlotRows[B] && c == ColB);
							bSomewhere = !bShares && ThreeSlotsHold(*TheClue, Cols, Size);
						}
						if (bSomewhere)
							BoardPairs.Add({ ColA, ColB });
					}
				}

				if (BoardPairs.Num() == 1)
				{
					const int PinA = BoardPairs[0].Key;
					const int PinB = BoardPairs[0].Value;
					int Cols[3];
					Cols[Subject] = Col;
					Cols[A] = PinA;
					Cols[B] = PinB;
					const bool bShares = (SlotRows[Subject] == SlotRows[A] && Col == PinA) || (SlotRows[Subject] == SlotRows[B] && Col == PinB);
					if (bShares || !ThreeSlotsHold(*TheClue, Cols, Size))
					{
						Help.AddIcon(SlotRows[A], SlotIcons[A]);
						if (PinA == PinB)
						{
							// "[Cosmos] and [Sundae] can only be together in this column, so [Peach] can't be here"
							Help.AddText(TEXT("and"));
							Help.AddIcon(SlotRows[B], SlotIcons[B]);
							Help.AddText(FString::Printf(TEXT("can only be together %s, so"), *DescribeColumn(PinA, Col)));
						}
						else
						{
							// "[A] can only be in column 2 and [B] directly right of this column, so [S] can't be here"
							Help.AddText(FString::Printf(TEXT("can only be %s and"), *DescribeColumn(PinA, Col)));
							Help.AddIcon(SlotRows[B], SlotIcons[B]);
							Help.AddText(DescribeColumn(PinB, Col) + TEXT(", so"));
						}
						Help.AddIcon(Row, Icon);
						Help.AddText(TEXT("can't be here"));
						bExplained = true;
					}
				}
			}

			auto Neither = [&](const FString& Place)
			{
				// "Neither [Rocking Horse] nor [Star Flower] can be directly right of this column, so [Dagger] can't be here"
				Help.AddText(TEXT("Neither"));
				Help.AddIcon(SlotRows[A], SlotIcons[A]);
				Help.AddText(TEXT("nor"));
				Help.AddIcon(SlotRows[B], SlotIcons[B]);
				Help.AddText(FString::Printf(TEXT("can be %s, so"), *Place));
				Help.AddIcon(Row, Icon);
				Help.AddText(TEXT("can't be here"));
				bExplained = true;
			};

			// Between (the subject at an end) and Chain (the subject at an end): the middle icon has to be on a side of
			// the subject with the far icon beyond it. Say why each side fails.
			const bool bBetweenEnd = TheClue->m_Type == eClueType::Horizontal && TheClue->m_HorizontalType == eHorizontalType::Between && Subject != 1;
			const bool bChainEnd = TheClue->m_Type == eClueType::Horizontal && TheClue->m_HorizontalType == eHorizontalType::Chain && Subject != 1;
			if ((bBetweenEnd || bChainEnd) && !bExplained)
			{
				const int Middle = 1;
				const int Far = Subject == 0 ? 2 : 0;
				const TArray<int> CanMiddle = Possible(SlotRows[Middle], SlotIcons[Middle]);
				const TArray<int> CanFar = Possible(SlotRows[Far], SlotIcons[Far]);

				// Sides the middle icon could be on: both for Between; for Chain, right of the first icon, left of the last
				TArray<int> Sides;
				if (bBetweenEnd || Subject == 2)
					Sides.Add(-1);
				if (bBetweenEnd || Subject == 0)
					Sides.Add(1);

				// For each side: no middle icon on it at all, or none with the far icon beyond it
				bool bAllFail = true;
				TArray<TPair<int, bool>> Reasons;	// side, true if there's no middle icon on that side
				for (int Side : Sides)
				{
					bool bAnyMiddle = false, bAnyBeyond = false;
					for (int M : CanMiddle)
					{
						if ((M - Col) * Side <= 0)
							continue;
						bAnyMiddle = true;
						for (int F : CanFar)
							bAnyBeyond |= (F - M) * Side > 0 && !(SlotRows[Far] == SlotRows[Middle] && F == M);
					}
					bAllFail &= !bAnyBeyond;
					Reasons.Add({ Side, !bAnyMiddle });
				}

				if (bAllFail)
				{
					// Sides with no middle icon first: "There's no [Rose] left of this column, and there's no
					// [Daisy] right of any [Rose] on the right, so [Bomb] can't be here"
					Reasons.StableSort([](const TPair<int, bool>& X, const TPair<int, bool>& Y) { return X.Value && !Y.Value; });
					for (int i = 0; i < Reasons.Num(); i++)
					{
						const int Side = Reasons[i].Key;
						const TCHAR* SideWord = Side < 0 ? TEXT("left") : TEXT("right");
						Help.AddText(i == 0 ? TEXT("There's no") : TEXT("and there's no"));
						if (Reasons[i].Value)
						{
							Help.AddIcon(SlotRows[Middle], SlotIcons[Middle]);
							Help.AddText(FString::Printf(TEXT("%s of this column,"), SideWord));
						}
						else
						{
							Help.AddIcon(SlotRows[Far], SlotIcons[Far]);
							Help.AddText(FString::Printf(TEXT("%s of any"), SideWord));
							Help.AddIcon(SlotRows[Middle], SlotIcons[Middle]);
							Help.AddText(Reasons.Num() > 1
								? FString::Printf(TEXT("on the %s,"), SideWord)
								: FString::Printf(TEXT("%s of this column,"), SideWord));
						}
					}
					Help.AddText(TEXT("so"));
					Help.AddIcon(Row, Icon);
					Help.AddText(TEXT("can't be here"));
					bExplained = true;
				}
			}

			if (Arrangements.Num() > 0 && !bExplained)
			{
				// A column every arrangement fills, that neither of them can be in
				for (int Spot = 0; Spot < Size && !bExplained; Spot++)
				{
					bool bAlwaysFilled = true, bAFits = false, bBFits = false;
					for (const TPair<int, int>& Arrangement : Arrangements)
					{
						bAlwaysFilled &= Arrangement.Key == Spot || Arrangement.Value == Spot;
						bAFits |= Arrangement.Key == Spot;
						bBFits |= Arrangement.Value == Spot;
					}
					if (bAlwaysFilled && !(bAFits && CanA.Contains(Spot)) && !(bBFits && CanB.Contains(Spot)))
						Neither(DescribeColumn(Spot, Col));
				}

				// A side every arrangement puts one of them on, that neither of them can be on
				for (int Side = -1; Side <= 1 && !bExplained; Side += 2)
				{
					auto OnSide = [&](int Column) { return Side < 0 ? Column < Col : Column > Col; };
					bool bAlwaysUsed = true, bAny = false;
					for (const TPair<int, int>& Arrangement : Arrangements)
					{
						bAlwaysUsed &= OnSide(Arrangement.Key) || OnSide(Arrangement.Value);
					}
					for (const TPair<int, int>& Arrangement : Arrangements)
					{
						bAny |= (OnSide(Arrangement.Key) && CanA.Contains(Arrangement.Key)) || (OnSide(Arrangement.Value) && CanB.Contains(Arrangement.Value));
					}
					if (bAlwaysUsed && !bAny)
						Neither(Side < 0 ? TEXT("left of this column") : TEXT("right of this column"));
				}

				// One of them has a single spot that fits, and the other can't go where that spot needs it
				for (int Pass = 0; Pass < 2 && !bExplained; Pass++)
				{
					const int First = Pass == 0 ? A : B;
					const int Second = Pass == 0 ? B : A;
					const TArray<int>& CanFirst = Pass == 0 ? CanA : CanB;
					// Only when that really is its one column left: "can only be" has to be true on the board
					if (CanFirst.Num() != 1)
						continue;
					TArray<int> Spots = CanFirst;

					TArray<int> SecondSpots;
					for (const TPair<int, int>& Arrangement : Arrangements)
					{
						if ((Pass == 0 ? Arrangement.Key : Arrangement.Value) == Spots[0])
							SecondSpots.AddUnique(Pass == 0 ? Arrangement.Value : Arrangement.Key);
					}
					if (SecondSpots.Num() != 1)
						continue;
					const TArray<int>& CanSecond = Pass == 0 ? CanB : CanA;
					if (CanSecond.Contains(SecondSpots[0]))
						continue;

					// "[Rocking Horse] can only be directly left of this column, and there's no [Star Flower] directly right of it, so [Dagger] can't be here"
					Help.AddIcon(SlotRows[First], SlotIcons[First]);
					Help.AddText(FString::Printf(TEXT("can only be %s, and there's no"), *DescribeColumn(Spots[0], Col)));
					Help.AddIcon(SlotRows[Second], SlotIcons[Second]);
					Help.AddText(DescribeColumn(SecondSpots[0], Col) + TEXT(", so"));
					Help.AddIcon(Row, Icon);
					Help.AddText(TEXT("can't be here"));
					bExplained = true;
				}
			}

			if (!bExplained)
			{
				// "[Bow] and [Hat] can't both fit, so [Top] can't be here"
				Help.AddIcon(SlotRows[A], SlotIcons[A]);
				Help.AddText(TEXT("and"));
				Help.AddIcon(SlotRows[B], SlotIcons[B]);
				Help.AddText(TEXT("can't both fit, so"));
				Help.AddIcon(Row, Icon);
				Help.AddText(TEXT("can't be here"));
				bExplained = true;
			}
		}

		// A clue about the subject alone (Edge)
		if (!bExplained && SlotRows.Num() == 1 && TheClue->m_Type == eClueType::Horizontal)
		{
			Help.AddIcon(Row, Icon);
			Help.AddText(TheClue->m_HorizontalType == eHorizontalType::Edge
				? TEXT("is in the first or last column, so it can't be here")
				: TEXT("isn't in the first or last column, so it can't be here"));
			bExplained = true;
		}
	}
	else
	{
		// Either-or clues with the main icon placed: the subject is the only option that can be where the main icon
		// needs one. "[Drum] is in column 4, and this is the only [Bowling Ball] or [Football] that can be next to
		// it, so [Bowling Ball] goes here" / "[Top] is in this column and [B] can't be, so [A] goes here too"
		if (SlotRows.Num() == 3 && Subject != (TheClue->m_Type == eClueType::Vertical ? 0 : 1))
		{
			const bool bVertical = TheClue->m_Type == eClueType::Vertical && TheClue->m_VerticalType == eVerticalType::EitherOr;
			const bool bNextTo = TheClue->m_Type == eClueType::Horizontal && TheClue->m_HorizontalType == eHorizontalType::NextToEitherOr;
			const int Main = bVertical ? 0 : 1;
			// The two options are the slots other than the main icon's
			const int OtherOption = Main == 0 ? (Subject == 1 ? 2 : 1) : (Subject == 0 ? 2 : 0);
			const int MainCol = (bVertical || bNextTo) ? PlacedColumn(SlotRows[Main], SlotIcons[Main]) : -1;
			if (MainCol >= 0)
			{
				// The spots the main icon needs an option in: its own column, or the columns beside it
				TArray<int> Spots;
				if (bVertical)
					Spots.Add(MainCol);
				else
				{
					if (MainCol > 0)
						Spots.Add(MainCol - 1);
					if (MainCol < Size - 1)
						Spots.Add(MainCol + 1);
				}

				const TArray<int> CanSubject = Possible(Row, Icon);
				const TArray<int> CanOther = Possible(SlotRows[OtherOption], SlotIcons[OtherOption]);
				bool bOnly = Spots.Contains(Col);
				for (int Spot : Spots)
				{
					bOnly &= !CanOther.Contains(Spot);
					bOnly &= Spot == Col || !CanSubject.Contains(Spot);
				}

				if (bOnly)
				{
					const int A = Subject < OtherOption ? Subject : OtherOption;
					const int B = Subject < OtherOption ? OtherOption : Subject;
					Help.AddIcon(SlotRows[Main], SlotIcons[Main]);
					if (bVertical)
					{
						Help.AddText(TEXT("is in this column and"));
						Help.AddIcon(SlotRows[OtherOption], SlotIcons[OtherOption]);
						Help.AddText(TEXT("can't be, so"));
						Help.AddIcon(Row, Icon);
						Help.AddText(TEXT("goes here too"));
					}
					else
					{
						Help.AddText(FString::Printf(TEXT("is in column %d, and this is the only"), MainCol + 1));
						Help.AddIcon(SlotRows[A], SlotIcons[A]);
						Help.AddText(TEXT("or"));
						Help.AddIcon(SlotRows[B], SlotIcons[B]);
						Help.AddText(TEXT("that can be next to it, so"));
						Help.AddIcon(Row, Icon);
						Help.AddText(TEXT("goes here"));
					}
					bExplained = true;
				}
			}
		}

		// All Apart: the other two can only be in the same two columns, so they take both; this is the column left
		// "[Cupcake] and [Cowboy Hat] can only be in columns 1 and 3, so [Bear] goes here"
		if (!bExplained && SlotRows.Num() == 3 && TheClue->m_Type == eClueType::Horizontal && TheClue->m_HorizontalType == eHorizontalType::AllApart)
		{
			TArray<TArray<int>> Columns;
			for (int Slot = 0; Slot < 3; Slot++)
				Columns.Add(Possible(SlotRows[Slot], SlotIcons[Slot]));
			const int A = (Subject + 1) % 3, B = (Subject + 2) % 3;
			const TArray<int> Pair = ApartPairColumns(*TheClue, Columns, A, B);
			TArray<int> Left = Columns[Subject];
			if (Pair.Num() == 2)
			{
				Left.Remove(Pair[0]);
				Left.Remove(Pair[1]);
			}
			if (Pair.Num() == 2 && Left.Num() == 1 && Left[0] == Col)
			{
				Help.AddIcon(SlotRows[A], SlotIcons[A]);
				Help.AddText(TEXT("and"));
				Help.AddIcon(SlotRows[B], SlotIcons[B]);
				Help.AddText(FString::Printf(TEXT("can only be in %s, so"), *ColumnPair(Pair)));
				Help.AddIcon(Row, Icon);
				Help.AddText(TEXT("goes here"));
				bExplained = true;
			}
		}

		// The subject goes here. Icons that must share a column, with this the only column they all can be in:
		// "[Cosmos] and [Sundae] can only be together in this column, so [Cosmos] goes here"
		const TArray<int> Together = GetTogetherSlots(*TheClue);
		if (!bExplained && Together.Contains(Subject))
		{
			bool bOnlyHere = true;
			for (int c = 0; c < Size && bOnlyHere; c++)
			{
				if (c == Col)
					continue;
				bool bAll = true;
				for (int Slot : Together)
					bAll &= Board[SlotRows[Slot]].m_Cells[c].m_bValues[SlotIcons[Slot]];
				bOnlyHere = !bAll;
			}
			for (int Slot : Together)
				bOnlyHere &= Board[SlotRows[Slot]].m_Cells[Col].m_bValues[SlotIcons[Slot]];

			if (bOnlyHere)
			{
				Help.AddIcon(Row, Icon);
				int Listed = 1;
				for (int Slot : Together)
				{
					if (Slot == Subject)
						continue;
					Listed++;
					Help.AddText(Listed == Together.Num() ? TEXT("and") : TEXT(","));
					Help.AddIcon(SlotRows[Slot], SlotIcons[Slot]);
				}
				Help.AddText(TEXT("can only be together in this column, so"));
				Help.AddIcon(Row, Icon);
				Help.AddText(TEXT("goes here"));
				bExplained = true;
			}
		}

		// Both partners of a three-icon clue placed: where the clue leaves the subject with the two of them together
		if (!bExplained && SlotRows.Num() == 3)
		{
			const int A = Subject == 0 ? 1 : 0;
			const int B = Subject == 2 ? 1 : 2;
			const int PlacedA = PlacedColumn(SlotRows[A], SlotIcons[A]);
			const int PlacedB = PlacedColumn(SlotRows[B], SlotIcons[B]);
			if (PlacedA >= 0 && PlacedB >= 0)
			{
				TArray<int> Fits;
				for (int c = 0; c < Size; c++)
				{
					int Cols[3];
					Cols[Subject] = c;
					Cols[A] = PlacedA;
					Cols[B] = PlacedB;
					const bool bSharesCell = (SlotRows[Subject] == SlotRows[A] && c == PlacedA) || (SlotRows[Subject] == SlotRows[B] && c == PlacedB);
					if (!bSharesCell && ThreeSlotsHold(*TheClue, Cols, Size))
						Fits.Add(c);
				}

				const TArray<int> Can = Possible(Row, Icon);
				TArray<int> Gone = Fits;
				Gone.Remove(Col);
				bool bOthersGone = true;
				for (int c : Gone)
					bOthersGone &= !Can.Contains(c);

				// As for one partner: the two of them have to point the subject somewhere (at most half the columns)
				const bool bPositive = IsPositivePair(*TheClue, Subject, A) && IsPositivePair(*TheClue, Subject, B);
				if (Fits.Contains(Col) && bOthersGone && (Fits.Num() == 1 || (bPositive && Fits.Num() * 2 <= Size)))
				{
					// Either-or clues placing the main icon: say what it needs rather than where the options are.
					// "[Top] shares a column with [A] or [B], so it's in column 1 or 4. There's no [Top] in column 1,
					// so it goes here"
					const bool bVerticalMain = TheClue->m_Type == eClueType::Vertical && TheClue->m_VerticalType == eVerticalType::EitherOr && Subject == 0;
					const bool bNextToMain = TheClue->m_Type == eClueType::Horizontal && TheClue->m_HorizontalType == eHorizontalType::NextToEitherOr && Subject == 1;
					if ((bVerticalMain || bNextToMain) && Gone.Num() > 0)
					{
						Help.AddIcon(Row, Icon);
						Help.AddText(bVerticalMain ? TEXT("shares a column with") : TEXT("is next to"));
						Help.AddIcon(SlotRows[A], SlotIcons[A]);
						Help.AddText(TEXT("or"));
						Help.AddIcon(SlotRows[B], SlotIcons[B]);
						Help.AddText(FString::Printf(TEXT(", so it's in %s. There's no"), *ColumnList(Fits)));
						Help.AddIcon(Row, Icon);
						Help.AddText(FString::Printf(TEXT("in %s, so it goes here"), *ColumnList(Gone)));
						bExplained = true;
					}
					else
					{
						// "[Daisy] is in column 1 and [Rose] is in column 4, so ..."
						const int First = PlacedA <= PlacedB ? A : B;
						const int Second = First == A ? B : A;
						Help.AddIcon(SlotRows[First], SlotIcons[First]);
						Help.AddText(FString::Printf(TEXT("is in column %d and"), PlacedColumn(SlotRows[First], SlotIcons[First]) + 1));
						Help.AddIcon(SlotRows[Second], SlotIcons[Second]);
						Help.AddText(FString::Printf(TEXT("is in column %d, so"), PlacedColumn(SlotRows[Second], SlotIcons[Second]) + 1));

						if (Gone.Num() == 0)
						{
							// "... so this is the only column where [Bomb] fits the clue"
							Help.AddText(TEXT("this is the only column where"));
							Help.AddIcon(Row, Icon);
							Help.AddText(TEXT("fits the clue"));
						}
						else
						{
							// "... so [Bomb] has to be right of column 4. There's no [Bomb] in column 6, so it goes here".
							// The place relative to whichever partner it can be described from, else the columns.
							FString Place;
							for (int Partner : { Second, First })
							{
								const int PartnerCol = Partner == A ? PlacedA : PlacedB;
								TArray<int> All;
								for (int c = 0; c < Size; c++)
									All.Add(c);
								Place = DescribePlace(Fits, PartnerCol, Size, GetPlaceWords(*TheClue), All);
								if (!Place.IsEmpty())
								{
									Place.ReplaceInline(TEXT("this column"), *FString::Printf(TEXT("column %d"), PartnerCol + 1));
									Place.ReplaceInline(TEXT("this one"), *FString::Printf(TEXT("column %d"), PartnerCol + 1));
									break;
								}
							}
							Help.AddIcon(Row, Icon);
							Help.AddText(FString::Printf(TEXT("has to be %s. There's no"), Place.IsEmpty() ? *(TEXT("in ") + ColumnList(Fits)) : *Place));
							Help.AddIcon(Row, Icon);
							Help.AddText(FString::Printf(TEXT("in %s, so it goes here"), *ColumnList(Gone)));
						}
						bExplained = true;
					}
				}
			}
		}

		// Otherwise a placed partner that puts it in this column
		for (int Other = 0; Other < SlotRows.Num() && !bExplained; Other++)
		{
			if (Other == Subject)
				continue;

			const int Placed = PlacedColumn(SlotRows[Other], SlotIcons[Other]);
			if (Placed < 0)
				continue;

			const TArray<int> SubjectColumns = Allowed(Other, Placed, Subject);
			if (!SubjectColumns.Contains(Col))
				continue;

			// The clue has to point the subject somewhere: a negative clue (Not Next To, say) only rules a few
			// columns out, and placing the subject from that relies on the rest of the board as well. At most half
			// the columns, or it's not a single step.
			if (SubjectColumns.Num() * 2 > Size)
				continue;
			if (SubjectColumns.Num() > 1 && !IsPositivePair(*TheClue, Other, Subject))
				continue;

			if (SubjectColumns.Num() > 1)
			{
				// "[Horse] is in column 4, so [Banana] would have to be in column 2 or 6. There's no [Banana] in
				// column 6, so it goes here"
				const TArray<int> Can = Possible(Row, Icon);
				TArray<int> Gone = SubjectColumns;
				Gone.Remove(Col);
				bool bOthersGone = true;
				for (int c : Gone)
					bOthersGone &= !Can.Contains(c);
				if (!bOthersGone)
					continue;

				Help.AddIcon(SlotRows[Other], SlotIcons[Other]);
				Help.AddText(Where(Placed));
				Help.AddIcon(Row, Icon);
				Help.AddText(FString::Printf(TEXT("would have to be in %s. There's no"), *ColumnList(SubjectColumns)));
				Help.AddIcon(Row, Icon);
				Help.AddText(FString::Printf(TEXT("in %s, so it goes here"), *ColumnList(Gone)));
				bExplained = true;
				continue;
			}

			// "[Daisy] is in this column, so [Pear] goes here too" / "[Daisy] is in column 3, so [Top] goes here"
			Help.AddIcon(SlotRows[Other], SlotIcons[Other]);
			Help.AddText(Where(Placed));
			Help.AddIcon(Row, Icon);
			Help.AddText(Placed == Col ? TEXT("goes here too") : TEXT("goes here"));
			bExplained = true;
		}

		if (!bExplained)
		{
			// The clue clears every other icon out of this cell, or rules the subject out of its other columns:
			// walk through those. (They explain each step against the board as it is.)
			P->m_Rows = Board;
			FClueHelp WalkHelp;
			// Either way the placement comes after other deductions (clearing the cell, or ruling the icon out of its
			// other columns), so it isn't a single step: a hint gives one of those instead (see Init)
			if (ExplainPlacementByCell(WalkHelp) || ExplainPlacementByRow(WalkHelp))
			{
				Help = WalkHelp;
				bExplained = true;
			}
			bIndirectPlacement = true;
		}

		if (!bExplained)
		{
			// "Working through this clue leaves [Top] only this column, so it goes here". Not a single step.
			bIndirectPlacement = true;
			Help.AddText(TEXT("Working through this clue leaves"));
			Help.AddIcon(Row, Icon);
			Help.AddText(TEXT("only this column, so it goes here"));
			bExplained = true;
		}
	}

	// Put everything back as it was
	P->m_Rows = Board;
	TheClue->m_iUseCount = SavedUseCount;
	P->m_bHypothetical = false;

	return bExplained ? Help : GetActionHelp();
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