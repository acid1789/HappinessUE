#include "Puzzle.h"
#include "CampaignTree.h"
#include "Clue.h"
#include "PuzzleRow.h"
#include "Hint.h"


#pragma optimize("", off)

void UPuzzle::Init(int Seed, int Size, int Difficulty)
{
	m_iSeed = Seed;
	m_iSize = Size;
	m_iDifficulty = Difficulty;
	m_bCampaign = false;
	m_HintsUsed = m_LessonHintsUsed = 0;

	m_Rand.Initialize(Seed);

	Generate();
}

bool UPuzzle::InitCampaign(int Seed, int Size, int Difficulty, ECampaignLesson Lesson)
{
	if (!IsLessonAvailable(Lesson, Size))
	{
		UE_LOG(LogTemp, Warning, TEXT("InitCampaign: lesson %s is not available on size %d"),
			*StaticEnum<ECampaignLesson>()->GetNameStringByValue((int64)Lesson), Size);
		return false;
	}

	m_iSeed = Seed;
	m_iSize = Size;
	m_iDifficulty = Difficulty;
	m_bCampaign = true;
	m_HintsUsed = m_LessonHintsUsed = 0;
	m_CampaignLesson = Lesson;

	m_Rand.Initialize(Seed);

	// Keep generating from the same random stream until the lesson's clues are needed to solve the puzzle
	for (int Attempt = 0; Attempt < 50; Attempt++)
	{
		Generate();

		if (RequiresLesson(Lesson))
			return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("InitCampaign: no puzzle requiring lesson %s (seed %d, size %d)"),
		*StaticEnum<ECampaignLesson>()->GetNameStringByValue((int64)Lesson), Seed, Size);
	return false;
}

bool UPuzzle::IsLessonAvailable(ECampaignLesson Lesson, int Size)
{
	// Only lessons in the campaign tree are playable (Given just ranks givens and NotHere).
	// Edge, Between and Chain are kept out of other 3x3 puzzles but still used by their own lesson.
	return UCampaignTree::GetLessonColumn(Lesson) != INDEX_NONE && Size >= 3;
}

bool UPuzzle::RequiresLesson(ECampaignLesson Lesson)
{
	// Remove the lesson's clues. Clues with a "not" component keep their positive part instead
	// (SpanNotMid -> Gap, ThreeTopNot -> Two, ...), so the puzzle only counts if the "not" itself is needed.
	TArray<UClue*> Without;
	for (UClue* C : m_Clues)
	{
		if (C->GetCampaignLesson() != Lesson)
		{
			Without.Add(C);
			continue;
		}

		TArray<UClue*> Parts;
		C->GetPositiveParts(*this, Parts);
		if (!Parts.Contains(C))
			Without.Append(Parts);
	}

	SortCluesByLesson(Without);
	const bool bSolvableWithout = IsSolvableWith(Without);

	// IsSolvableWith leaves the board part solved
	Reset();

	return !bSolvableWithout;
}

void UPuzzle::Generate()
{
	m_Rows.Empty();
	m_Rows.Reserve(m_iSize);
	for (int i = 0; i < m_iSize; i++)
	{
		m_Rows.Emplace(m_iSize);
	}

	GenerateSolution();
	GenerateClues();
}

void UPuzzle::GetGivenRangeForDifficulty(int Size, int Difficulty, int& Min, int& Max)
{
	switch (Difficulty)
	{
		case 0:		// Easy
			Min = Max = FMath::Max(Size - 2, 0);
			break;
		case 1:		// Normal
			Min = 1;
			Max = FMath::Max(Size - 3, 1);
			break;
		default:	// Hard
			Min = Max = 0;
			break;
	}
}

int UPuzzle::GetNumGivenClues()
{
	int Count = 0;

	for (UClue* C : m_Clues)
	{
		if (C->m_Type == eClueType::Given)
			Count++;
	}

	return Count;
}

void UPuzzle::RandomDistribution(FRandomStream& Rand, TArray<int>& Rands)
{
	for (int i = 0; i < Rands.Num(); i++)
	{
		int RandValue = Rand.RandRange(0, Rands.Num() - 1);

		bool bGoodRandom = true;

		for (int j = 0; j < i; j++)
		{
			if (Rands[j] == RandValue)
			{
				bGoodRandom = false;
				break;
			}
		}

		if (bGoodRandom)
			Rands[i] = RandValue;
		else
			i--;
	}
}

void UPuzzle::GenerateSolution()
{
	m_Solution.SetNum(m_iSize * m_iSize);

	TArray<int> Rands;
	Rands.SetNum(m_iSize);

	for (int i = 0; i < m_iSize; i++)
	{
		RandomDistribution(m_Rand, Rands);

		for (int j = 0; j < m_iSize; j++)
		{
			m_Solution[(i * m_iSize) + j] = Rands[j];
		}
	}
}

void UPuzzle::GenerateClues()
{
	UE_LOG(LogTemp, Log, TEXT("-- Generating Clues --"));
	m_Clues.Empty();

	// Givens are placed up front, a count within GetGivenRangeForDifficulty(); the random clues below never pick Given
	int MinGivens, MaxGivens;
	GetGivenRangeForDifficulty(m_iSize, m_iDifficulty, MinGivens, MaxGivens);
	const int NumGivens = m_Rand.RandRange(MinGivens, MaxGivens);
	for (int i = 0; i < NumGivens; i++)
	{
		UClue* C = NewObject<UClue>(this);
		C->InitGiven(*this, m_Rand);
		m_Clues.Add(C);
		C->Analyze(*this);
	}

	while (!IsSolved())
	{
		UClue* C = NewObject<UClue>(this);
		UE_LOG(LogTemp, Log, TEXT("Initializing Clue"));

		C->Init(*this, m_Rand);

		UE_LOG(LogTemp, Log, TEXT("Validating Clue: %s"), *C->ToString());

		if (ValidateClue(*C))
		{
			// Add to front of array
			m_Clues.Insert(C, 0);
			AnalyzeAllClues();
			UE_LOG(LogTemp, Log, TEXT("UClue Added: %d"), m_Clues.Num());
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Generated Invalid Clue"));
		}
	}

	Reset();

	UE_LOG(LogTemp, Log, TEXT("UClue Count after initial generation: %d"), m_Clues.Num());

	// Optimize the clues
	OptimizeClues();

	UE_LOG(LogTemp, Log, TEXT("UClue Count after optimization: %d"), m_Clues.Num());

	// Scramble all the clues
	ScrambleClues();

	// Reset the UPuzzle for actual play
	Reset();
}

bool UPuzzle::IsDuplicateClue(UClue& testClue)
{
	for (int i = 0; i < m_Clues.Num(); i++)
	{
		UClue& C = *m_Clues[i];
		if (testClue.m_Type == C.m_Type)
		{
			if (C.m_Type == eClueType::Vertical)
			{
				if (C.m_VerticalType == testClue.m_VerticalType)
				{
					if (C.m_iCol == testClue.m_iCol)
					{
						switch (C.m_VerticalType)
						{
						case eVerticalType::Two:
							if (C.m_iRow == testClue.m_iRow && C.m_iRow2 == testClue.m_iRow2)
								return true;
							break;
						case eVerticalType::Three:
							if (C.m_iRow == testClue.m_iRow && C.m_iRow2 == testClue.m_iRow2 && C.m_iRow3 == testClue.m_iRow3)
								return true;
							break;
						case eVerticalType::TwoNot:
							if (C.m_iRow == testClue.m_iRow && C.m_iRow2 == testClue.m_iRow2 && C.m_iNotCell == testClue.m_iNotCell)
								return true;
							break;
						case eVerticalType::EitherOr:
						case eVerticalType::ThreeTopNot:
						case eVerticalType::ThreeMidNot:
						case eVerticalType::ThreeBotNot:
							if (C.m_iRow == testClue.m_iRow && C.m_iRow2 == testClue.m_iRow2 && C.m_iRow3 == testClue.m_iRow3 && C.m_iNotCell == testClue.m_iNotCell)
								return true;
							break;
						}
					}
				}
			}
			else if (C.m_Type == eClueType::Horizontal)
			{
				if (C.m_HorizontalType == testClue.m_HorizontalType)
				{
					switch (C.m_HorizontalType)
					{
					case eHorizontalType::NextTo:
						// Order doesn't matter for NextTo: A next to B is B next to A
						if (C.m_iRow == testClue.m_iRow2 && C.m_iCol == testClue.m_iCol2 && C.m_iRow2 == testClue.m_iRow && C.m_iCol2 == testClue.m_iCol)
							return true;
						// fall through to the same-order check
					case eHorizontalType::LeftOf:
					case eHorizontalType::NotLeftOf:
						if (C.m_iRow == testClue.m_iRow && C.m_iCol == testClue.m_iCol && C.m_iRow2 == testClue.m_iRow2 && C.m_iCol2 == testClue.m_iCol2)
							return true;
						break;
					case eHorizontalType::NotNextTo:
						if (C.m_iRow == testClue.m_iRow && C.m_iCol == testClue.m_iCol && C.m_iRow2 == testClue.m_iRow2 && C.m_iHorizontal1 == testClue.m_iHorizontal1)
							return true;
						break;
					case eHorizontalType::Span:
						if (C.m_iRow == testClue.m_iRow && C.m_iCol == testClue.m_iCol && C.m_iRow2 == testClue.m_iRow2 && C.m_iCol2 == testClue.m_iCol2 && C.m_iRow3 == testClue.m_iRow3 && C.m_iCol3 == testClue.m_iCol3)
							return true;
						// The outer two can be either way round
						if (C.m_iRow == testClue.m_iRow3 && C.m_iCol == testClue.m_iCol3 && C.m_iRow2 == testClue.m_iRow2 && C.m_iCol2 == testClue.m_iCol2 && C.m_iRow3 == testClue.m_iRow && C.m_iCol3 == testClue.m_iCol)
							return true;
						break;
					case eHorizontalType::SpanNotLeft:
					case eHorizontalType::SpanNotMid:
					case eHorizontalType::SpanNotRight:
						if (C.m_iRow == testClue.m_iRow && C.m_iCol == testClue.m_iCol && C.m_iRow2 == testClue.m_iRow2 && C.m_iCol2 == testClue.m_iCol2 && C.m_iRow3 == testClue.m_iRow3 && C.m_iCol3 == testClue.m_iCol3 && C.m_iHorizontal1 == testClue.m_iHorizontal1)
							return true;
						break;
					case eHorizontalType::Edge:
					case eHorizontalType::NotEdge:
					case eHorizontalType::DirectlyLeftOf:
					case eHorizontalType::Gap:
					case eHorizontalType::Between:
					case eHorizontalType::Chain:
					case eHorizontalType::NextToEitherOr:
					case eHorizontalType::AllApart:
						if (C.IsSameClue(testClue))
							return true;
						break;
					}
				}
			}
			else if (C.m_Type == eClueType::NotHere)
			{
				if (C.IsSameClue(testClue))
					return true;
			}
			else // Given
			{
				if (C.m_iCol == testClue.m_iCol && C.m_iRow == testClue.m_iRow)
					return true;
			}
		}
	}
	return false;
}

bool UPuzzle::ValidateClue(UClue& C)
{
	if (IsDuplicateClue(C))
		return false;

	if (C.m_Type == eClueType::Vertical)
	{
		int iRow1 = -1;
		int iRow2 = -1;
		int iCol = C.m_iCol;
		switch (C.m_VerticalType)
		{
		case eVerticalType::Two:
		case eVerticalType::ThreeBotNot:
			iRow1 = C.m_iRow;
			iRow2 = C.m_iRow2;
			break;
		case eVerticalType::ThreeMidNot:
			iRow1 = C.m_iRow;
			iRow2 = C.m_iRow3;
			break;
		case eVerticalType::ThreeTopNot:
			iRow1 = C.m_iRow2;
			iRow2 = C.m_iRow3;
			break;
		}
		if (iRow1 >= 0)
		{
			for (int i = 0; i < m_Clues.Num(); i++)
			{
				UClue& cTest = *m_Clues[i];
				if (cTest.m_Type == eClueType::Vertical)
				{
					switch (cTest.m_VerticalType)
					{
					case eVerticalType::Two:
					case eVerticalType::ThreeBotNot:
						if (iRow1 == cTest.m_iRow && iRow2 == cTest.m_iRow2)
							return false;
						break;
					case eVerticalType::ThreeMidNot:
						if (iRow1 == cTest.m_iRow && iRow2 == cTest.m_iRow3)
							return false;
						break;
					case eVerticalType::ThreeTopNot:
						if (iRow1 == cTest.m_iRow2 && iRow2 == cTest.m_iRow3)
							return false;
						break;
					}
				}
			}
		}

		// Two Three clues on the same rows look like the same clue (on a 3x3 every Three spans all rows)
		if (C.m_VerticalType == eVerticalType::Three)
		{
			for (const UClue* cTest : m_Clues)
			{
				if (cTest->m_Type == eClueType::Vertical && cTest->m_VerticalType == eVerticalType::Three &&
					cTest->m_iRow == C.m_iRow && cTest->m_iRow2 == C.m_iRow2 && cTest->m_iRow3 == C.m_iRow3)
				{
					return false;
				}
			}
		}

		// In one column, a clue whose "same column" rows sit inside (or contain) another's says nothing new
		// and looks like a piece of it, e.g. a Two inside a Three
		auto PositiveRows = [](const UClue& Clue) -> TArray<int>
		{
			switch (Clue.m_VerticalType)
			{
				case eVerticalType::Two:			return { Clue.m_iRow, Clue.m_iRow2 };
				case eVerticalType::Three:			return { Clue.m_iRow, Clue.m_iRow2, Clue.m_iRow3 };
				case eVerticalType::ThreeTopNot:	return { Clue.m_iRow2, Clue.m_iRow3 };
				case eVerticalType::ThreeMidNot:	return { Clue.m_iRow, Clue.m_iRow3 };
				case eVerticalType::ThreeBotNot:	return { Clue.m_iRow, Clue.m_iRow2 };
				default:							return {};
			}
		};
		auto IsSubset = [](const TArray<int>& Small, const TArray<int>& Large)
		{
			for (int Row : Small)
			{
				if (!Large.Contains(Row))
					return false;
			}
			return true;
		};

		const TArray<int> NewRows = PositiveRows(C);
		if (NewRows.Num() > 0)
		{
			for (const UClue* cTest : m_Clues)
			{
				if (cTest->m_Type != eClueType::Vertical || cTest->m_iCol != C.m_iCol)
					continue;

				const TArray<int> TestRows = PositiveRows(*cTest);
				if (TestRows.Num() > 0 && (IsSubset(NewRows, TestRows) || IsSubset(TestRows, NewRows)))
					return false;
			}
		}
	}
	return true;
}

void UPuzzle::AnalyzeAllClues()
{
	for (UClue* C : m_Clues)
	{
		C->Analyze(*this);
	}
}

void UPuzzle::Reset()
{
	for (FPuzzleRow& Row : m_Rows)
	{
		Row.Reset();
	}

	for (UClue* C : m_Clues)
	{
		C->m_iUseCount = 0;
	}

	ApplyAllGiven();

	// Restarting the puzzle starts over without hints
	m_HintsUsed = m_LessonHintsUsed = 0;
}

void UPuzzle::ResetRow(int Row)
{
	for (int i = 0; i < m_iSize; i++)
	{
		m_Rows[Row].m_Cells[i].Reset();
	}

	ApplyAllGiven();
}

bool UPuzzle::IsCompleted()
{
	ReEnforceFinalIcons();

	for (FPuzzleRow& Row : m_Rows)
	{
		if (!Row.IsCompleted())
			return false;
	}

	return true;
}

bool UPuzzle::IsSolved()
{
	if (!IsCompleted())
		return false;

	for (int i = 0; i < m_iSize; i++)
	{
		for (int j = 0; j < m_iSize; j++)
		{
			int Final = m_Rows[i].m_Cells[j].m_iFinalIcon;

			if (Final != m_Solution[(i * m_iSize) + j])
				return false;
		}
	}

	return true;
}

bool UPuzzle::IsCorrect(int Row, int Col)
{
	int Final = m_Rows[Row].m_Cells[Col].m_iFinalIcon;
	return Final >= 0 && Final == m_Solution[(Row * m_iSize) + Col];
}

int UPuzzle::SolutionIcon(int Row, int Col)
{
	return m_Solution[(Row * m_iSize) + Col];
}

void UPuzzle::SetFinalIcon(int Row, int Col, int Icon)
{
	SetFinalIconWithClue(nullptr, Row, Col, Icon);
}

void UPuzzle::SetFinalIconWithClue(UClue* TheClue, int Row, int Col, int Icon)
{
	int Final = m_Rows[Row].m_Cells[Col].m_iFinalIcon;

	if (Final >= 0 && Final != Icon)
	{
		UE_LOG(LogTemp, Error, TEXT("SetFinalIcon conflict"));
		DebugError();
	}

	if (!m_Rows[Row].m_Cells[Col].m_bValues[Icon])
	{
		UE_LOG(LogTemp, Error, TEXT("SetFinalIcon already eliminated"));
		DebugError();
	}

	if (Final != Icon)
	{
		if (TheClue)
			TheClue->m_iUseCount += 5;

		m_Rows[Row].m_Cells[Col].m_iFinalIcon = Icon;

		for (int i = 0; i < m_iSize; i++)
		{
			if (i != Icon)
				EliminateIconWithClue(TheClue, Row, Col, i);
		}

		for (int i = 0; i < m_iSize; i++)
		{
			if (i != Col)
				EliminateIconWithClue(TheClue, Row, i, Icon);
		}
	}
}

void UPuzzle::EliminateIcon(int Row, int Col, int Icon)
{
	EliminateIconWithClue(nullptr, Row, Col, Icon);
}

void UPuzzle::EliminateIconWithClue(UClue* TheClue, int Row, int Col, int Icon)
{
	auto& Cell = m_Rows[Row].m_Cells[Col];

	if (Cell.m_iFinalIcon == Icon)
	{
		UE_LOG(LogTemp, Error, TEXT("EliminateIcon removing final"));
		DebugError();
	}

	if (Cell.m_bValues[Icon])
	{
		if (TheClue)
			TheClue->m_iUseCount++;

		Cell.m_bValues[Icon] = false;

		if (AutoSetIcons)
		{
			if (Cell.m_iFinalIcon < 0)
			{
				int Remaining = Cell.GetRemainingIcon();

				if (Remaining >= 0)
				{
					SetFinalIconWithClue(TheClue, Row, Col, Remaining);
				}
			}

			// Check to see if there is only one remaining in the row
			int iFound = -1;
			for (int i = 0; i < m_iSize; i++)
			{
				if (m_Rows[Row].m_Cells[i].m_bValues[Icon]) {
					if (iFound >= 0)
					{
						// We found a second one, no need to set a final ion here
						iFound = -1;
						break;
					}
					else
					{
						// Haven't found one yet, this is the first one
						iFound = i;
					}
				}
			}
			if (iFound >= 0)
			{
				SetFinalIconWithClue(TheClue, Row, iFound, Icon);
			}
		}
	}
}

void UPuzzle::ReEnforceFinalIcons()
{
	for (int iRow = 0; iRow < m_iSize; iRow++)
	{
		for (int iCol = 0; iCol < m_iSize; iCol++)
		{
			int iFinal = m_Rows[iRow].m_Cells[iCol].m_iFinalIcon;
			if (iFinal >= 0)
			{
				for (int j = 0; j < m_iSize; j++)
				{
					if (j != iCol)
						m_Rows[iRow].m_Cells[j].m_bValues[iFinal] = false;
				}
			}
		}
	}
}

UHint* UPuzzle::GenerateHint(const TArray<UClue*>& VisibleClues)
{
	UHint* hRet = nullptr;

	// Pick a clue that we could use for a hint
	for (int i = 0; i < VisibleClues.Num(); i++)
	{
		SetMarker();
		if (VisibleClues[i] != nullptr)
		{
			int iUseCount = VisibleClues[i]->m_iUseCount;
			VisibleClues[i]->Analyze(*this);
			RestoreMarker();

			if (VisibleClues[i]->m_iUseCount > iUseCount)
			{
				// This clue can do something, use it for the hint
				UHint* Hint = NewObject<UHint>(this);
				if (Hint->Init(*this, *VisibleClues[i]))
				{
					hRet = Hint;

					m_HintsUsed++;
					if (m_bCampaign && VisibleClues[i]->GetCampaignLesson() == m_CampaignLesson)
					{
						m_LessonHintsUsed++;
					}
					break;
				}
			}
		}
	}

	return hRet;
}

void UPuzzle::BuildClueLists()
{
	m_GivenClues.Empty();
	m_VeritcalClues.Empty();
	m_HorizontalClues.Empty();
	for (int i = 0; i < m_Clues.Num(); i++)
	{
		UClue* c = m_Clues[i];
		switch (c->m_Type)
		{
			case eClueType::Given:
			case eClueType::NotHere:
				m_GivenClues.Add(c);
				break;
			case eClueType::Vertical:
				m_VeritcalClues.Add(c);
				break;
			case eClueType::Horizontal:
				m_HorizontalClues.Add(c);
				break;
		}
	}
}

void UPuzzle::ScrambleClues()
{
	int NumClues = m_Clues.Num();

	TArray<UClue*> Copy = m_Clues;
	TArray<int> Scramble;
	Scramble.SetNum(NumClues);
	RandomDistribution(m_Rand, Scramble);

	for (int i = 0; i < NumClues; i++)
	{
		m_Clues[i] = Copy[Scramble[i]];
	}

	BuildClueLists();
}

void UPuzzle::ApplyAllGiven()
{
	for (int i = 0; i < m_GivenClues.Num(); i++)
	{
		UClue* c = m_GivenClues[i];
		c->Analyze(*this);	
	}
}

void UPuzzle::SortCluesByLesson(TArray<UClue*>& Clues)
{
	// Easiest campaign lesson first; stable so clues of the same type keep their random generation order
	Clues.StableSort([](const UClue& A, const UClue& B) { return A.GetCampaignLesson() < B.GetCampaignLesson(); });
}

bool UPuzzle::IsSolvableWith(const TArray<UClue*>& SortedClues)
{
	for (FPuzzleRow& Row : m_Rows)
		Row.Reset();

	for (UClue* C : SortedClues)
		C->m_iUseCount = 0;

	// Givens and NotHeres are applied up front, the same as Reset()
	for (UClue* C : SortedClues)
	{
		if (C->m_Type == eClueType::Given || C->m_Type == eClueType::NotHere)
			C->Analyze(*this);
	}

	// Each pass tries the clues easiest first; any deduction bumps a clue's use count, so no change means stuck
	while (!IsSolved())
	{
		int UseBefore = 0;
		for (UClue* C : SortedClues)
			UseBefore += C->m_iUseCount;

		for (UClue* C : SortedClues)
		{
			if (C->m_Type == eClueType::Vertical || C->m_Type == eClueType::Horizontal)
				C->Analyze(*this);
		}

		int UseAfter = 0;
		for (UClue* C : SortedClues)
			UseAfter += C->m_iUseCount;

		if (UseAfter == UseBefore)
			return false;
	}

	return true;
}

void UPuzzle::OptimizeClues()
{
	TArray<UClue*> Clues = m_Clues;
	SortCluesByLesson(Clues);

	if (!IsSolvableWith(Clues))
	{
		UE_LOG(LogTemp, Error, TEXT("Unsolvable in the optimize stage?"));
		BuildClueLists();
		DebugError();
		return;
	}

	// Throw away the hardest clues first: drop each non-given clue unless the puzzle can't be solved without it.
	// Easier clue types are tried first when solving, so harder ones are the likeliest to be redundant.
	// In campaign mode the lesson being taught goes last (second pass), so its clues are the ones that stay needed.
	for (int Pass = 0; Pass < 2; Pass++)
	{
		for (int i = Clues.Num() - 1; i >= 0; i--)
		{
			UClue* C = Clues[i];
			if (C->m_Type == eClueType::Given)
				continue;

			const bool bLessonClue = m_bCampaign && C->GetCampaignLesson() == m_CampaignLesson;
			if (bLessonClue != (Pass == 1))
				continue;

			Clues.RemoveAt(i);
			if (!IsSolvableWith(Clues))
				Clues.Insert(C, i);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UClue Count after optimization: %d"), Clues.Num());

	m_Clues = Clues;
	BuildClueLists();

	// Easy gets some extra clues; normal and hard use exactly the clues needed (they differ only in givens)
	Reset();

	int iUsableClueCount = m_HorizontalClues.Num() + m_VeritcalClues.Num();

	// Skip if the givens alone already solve the board: the clue generators search for an
	// unsolved cell and would never find one
	if (m_iDifficulty == 0 && !IsSolved())
	{
		// Add some clues		
		int iCluesToAdd = FMath::Max((int)(iUsableClueCount * 0.1f), 1);
		int iNewClueCount = m_Clues.Num() + iCluesToAdd;
		while (m_Clues.Num() < iNewClueCount)
		{
			UClue* C = NewObject<UClue>(this);
			C->Init(*this, m_Rand);

			if (ValidateClue(*C))
			{
				m_Clues.Add(C);
			}
		}
	}

	// Rebuild the clues list
	BuildClueLists();

	// Reset the UPuzzle again
	Reset();
}

void UPuzzle::DumpPuzzle()
{
	FString Output;

	for (int i = 0; i < m_iSize; i++)
	{
		for (int j = 0; j < m_iSize; j++)
		{
			Output += TEXT("[");

			for (int k = 0; k < m_iSize; k++)
			{
				if (!m_Rows[i].m_Cells[j].m_bValues[k])
					Output += TEXT(" ");
				else
					Output += FString::FromInt(k);

				if (k < m_iSize - 1)
					Output += TEXT(",");
			}

			if (j < m_iSize - 1)
			{
				Output += TEXT("], ");
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("%s]"), *Output);
				Output.Empty();
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT(""));
	UE_LOG(LogTemp, Log, TEXT(""));
}

void UPuzzle::DumpSolution()
{
	FString Output;

	for (int i = 0; i < m_iSize; i++)
	{
		for (int j = 0; j < m_iSize; j++)
		{
			Output += TEXT("[");
			Output += FString::FromInt(m_Solution[(i * m_iSize) + j]);

			if (j < m_iSize - 1)
			{
				Output += TEXT("], ");
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("%s]"), *Output);
				Output.Empty();
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT(""));
	UE_LOG(LogTemp, Log, TEXT(""));
}

void UPuzzle::DumpClues()
{
	for (int i = 0; i < m_Clues.Num(); i++)
	{
		UClue* C = m_Clues[i];
		C->Dump(i, *this);
	}
}

void UPuzzle::DebugError()
{
	Reset();
	DumpSolution();
	DumpPuzzle();
	DumpClues();
}

void UPuzzle::SetMarker()
{
	m_MarkerRows.SetNum(m_Rows.Num());
	for (int i = 0; i < m_MarkerRows.Num(); i++)
	{
		m_MarkerRows[i] = FPuzzleRow(m_Rows[i]);
	}
}

void UPuzzle::RestoreMarker()
{
	m_Rows.SetNum(m_MarkerRows.Num());
	for (int i = 0; i < m_Rows.Num(); i++)
	{
		m_Rows[i] = FPuzzleRow(m_MarkerRows[i]);
	}
}

FString UPuzzle::FormatTimeString(float Seconds) const
{
	int Hours = (int)(Seconds / 3600.0f);
	Seconds -= Hours * 3600;

	int Minutes = (int)(Seconds / 60.0f);
	Seconds -= Minutes * 60;

	int Secs = (int)Seconds;

	return FString::Printf(TEXT("%02d:%02d:%02d"), Hours, Minutes, Secs);
}

void UPuzzle::FixPuzzle()
{
	for (int i = 0; i < m_Rows.Num(); i++)
	{
		FPuzzleRow& Row = m_Rows[i];
		for (int j = 0; j < Row.m_Cells.Num(); j++) {
			FPuzzleCell& Cell = Row.m_Cells[j];

			int iFinalIcon = m_Solution[(i * m_iSize) + j];
			if (Cell.m_iFinalIcon >= 0 && Cell.m_iFinalIcon != iFinalIcon) 
			{
				// This one is wrong, undo it
				Cell.m_iFinalIcon = -1;

				// Set all small icons to visible for this cell
				for (int k = 0; k < m_iSize; k++)
				{
					Cell.m_bValues[k] = true;
				}
			}
			else if (Cell.m_iFinalIcon < 0)
			{
				// Restore missing icon
				Cell.m_bValues[iFinalIcon] = true;
			}
		}
	}
}
int32 UPuzzle::GetCampaignScore() const
{
	// 3 for completing it, -1 if any hint was used, and another -1 if any hint was on the lesson's clue
	const bool bHints = m_HintsUsed > 0;
	const bool bLessonHints = m_LessonHintsUsed > 0;
	return 3 - (bHints ? 1 : 0) - (bLessonHints ? 1 : 0);
}

// Restore optimization so the "off" above doesn't carry into the next file of a unity build
#pragma optimize("", on)
