#include "Puzzle.h"
#include "Clue.h"
#include "PuzzleRow.h"
#include "Hint.h"

#pragma optimize("", off)

void UPuzzle::Init(int Seed, int Size, int Difficulty)
{
	m_iSeed = Seed;
	m_iSize = Size;
	m_iDifficulty = Difficulty;

	m_Rand.Initialize(Seed);

	m_Rows.Empty();
	m_Rows.Reserve(Size);
	for (int i = 0; i < Size; i++)
	{
		m_Rows.Emplace(Size);
	}

	GenerateSolution();
	GenerateClues();
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
						break;
					case eHorizontalType::SpanNotLeft:
					case eHorizontalType::SpanNotMid:
					case eHorizontalType::SpanNotRight:
						if (C.m_iRow == testClue.m_iRow && C.m_iCol == testClue.m_iCol && C.m_iRow2 == testClue.m_iRow2 && C.m_iCol2 == testClue.m_iCol2 && C.m_iRow3 == testClue.m_iRow3 && C.m_iCol3 == testClue.m_iCol3 && C.m_iHorizontal1 == testClue.m_iHorizontal1)
							return true;
						break;
					}
				}
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

		if (Cell.m_iFinalIcon < 0)
		{
			int Remaining = Cell.GetRemainingIcon();

			if (Remaining >= 0)
			{
				SetFinalIconWithClue(TheClue, Row, Col, Remaining);
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

UHint* UPuzzle::GenerateHint(TArray<UClue*>& VisibleClues)
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
				hRet = NewObject<UHint>(this);
				if (!hRet->Init(*this, *VisibleClues[i]))
				{
					continue;
				}
				break;
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
	int32 NumClues = m_Clues.Num();

	TArray<UClue*> Copy = m_Clues;
	TArray<int32> Scramble;
	Scramble.SetNum(NumClues);
	RandomDistribution(m_Rand, Scramble);

	for (int32 i = 0; i < NumClues; i++)
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

void UPuzzle::OptimizeClues()
{
	// Build the separate clue lists
	BuildClueLists();

	// Reset the UPuzzle & apply all the givens
	Reset();

	// Solve again with the new clue order
	int32 iPass = 0;

	while (!IsSolved())
	{
		for (int32 i = 0; i < m_VeritcalClues.Num(); i++)
		{
			UClue& C = *m_VeritcalClues[i];
			C.Analyze(*this);
		}

		for (int32 i = 0; i < m_HorizontalClues.Num(); i++)
		{
			UClue& C = *m_HorizontalClues[i];
			C.Analyze(*this);
		}

		iPass++;

		if (iPass > 100)
		{
			UE_LOG(LogTemp, Error, TEXT("Unsolvable in the optimize stage?"));
			DumpPuzzle();
			DebugError();
			return;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Passes: %d"), iPass++);

	// Sort the list based on most used
	m_Clues.Sort();

	// Remove any zero use count clues
	for (int32 i = m_Clues.Num() - 1; i > 0; i--)
	{
		UClue& C = *m_Clues[i];

		if (C.m_iUseCount > 0)
			break;

		m_Clues.RemoveAt(i);
	}

	// Resolve sorted
	UE_LOG(LogTemp, Log, TEXT("UClue Count Before 2nd stage optimization: %d"), m_Clues.Num());

	BuildClueLists();
	Reset();

	iPass = 0;

	while (!IsSolved())
	{
		for (int32 i = 0; i < m_VeritcalClues.Num(); i++)
		{
			UClue& C = *m_VeritcalClues[i];
			C.Analyze(*this);
		}

		for (int32 i = 0; i < m_HorizontalClues.Num(); i++)
		{
			UClue& C = *m_HorizontalClues[i];
			C.Analyze(*this);
		}

		iPass++;

		if (iPass > 100)
		{
			UE_LOG(LogTemp, Error, TEXT("Unsolvable in the optimize stage?"));
			DumpPuzzle();
			DebugError();
			return;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Passes: %d"), iPass);

	// Sort the list based on most used
	m_Clues.Sort();

	// Remove any zero use count clues
	for (int32 i = m_Clues.Num() - 1; i > 0; i--)
	{
		UClue& C = *m_Clues[i];

		if (C.m_iUseCount > 0)
			break;

		m_Clues.RemoveAt(i);
	}

	// Add/Remove clues for difficulty
	Reset();

	if (m_iDifficulty == 0)
	{
		// Add some clues
		int32 iCluesToAdd = FMath::RandRange(1, 4);
		int32 iNewClueCount = m_Clues.Num() + iCluesToAdd;

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
	else if (m_iDifficulty == 2)
	{
		// Remove some clues
		int32 iCluesToRemove = FMath::RandRange(2, (m_iSize / 3) + 1);
		int32 iUseCount = 1;

		while (iCluesToRemove > 0)
		{
			int32 iClueCount = m_Clues.Num();

			for (int32 i = 0; i < m_Clues.Num(); i++)
			{
				UClue& C = *m_Clues[i];

				if (C.m_iUseCount <= iUseCount)
				{
					iCluesToRemove--;
					m_Clues.RemoveAt(i);
					break;
				}
			}

			if (iClueCount == m_Clues.Num())
				iUseCount++;
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

	for (int32 i = 0; i < m_iSize; i++)
	{
		for (int32 j = 0; j < m_iSize; j++)
		{
			Output += TEXT("[");

			for (int32 k = 0; k < m_iSize; k++)
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

	for (int32 i = 0; i < m_iSize; i++)
	{
		for (int32 j = 0; j < m_iSize; j++)
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
	for (int32 i = 0; i < m_Clues.Num(); i++)
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
	for (int32 i = 0; i < m_MarkerRows.Num(); i++)
	{
		m_MarkerRows[i] = FPuzzleRow(m_Rows[i]);
	}
}

void UPuzzle::RestoreMarker()
{
	m_Rows.SetNum(m_MarkerRows.Num());
	for (int32 i = 0; i < m_Rows.Num(); i++)
	{
		m_Rows[i] = FPuzzleRow(m_MarkerRows[i]);
	}
}

FString UPuzzle::FormatTimeString(float Seconds) const
{
	int32 Hours = (int32)(Seconds / 3600.0f);
	Seconds -= Hours * 3600;

	int32 Minutes = (int32)(Seconds / 60.0f);
	Seconds -= Minutes * 60;

	int32 Secs = (int32)Seconds;

	return FString::Printf(TEXT("%02d:%02d:%02d"), Hours, Minutes, Secs);
}