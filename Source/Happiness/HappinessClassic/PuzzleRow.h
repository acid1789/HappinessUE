#pragma once

#include "CoreMinimal.h"
#include "PuzzleCell.h"
#include "PuzzleRow.generated.h"

USTRUCT(BlueprintType)
struct FPuzzleRow
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite)
	TArray<FPuzzleCell> m_Cells;

public:

	FPuzzleRow() {}          // required by TArray

	FPuzzleRow(int iSize)
	{
		m_Cells.Reserve(iSize);

		for (int i = 0; i < iSize; i++)
		{
			m_Cells.Emplace(iSize);
		}
	}

	FPuzzleRow(const FPuzzleRow& Other)
	{
		m_Cells = Other.m_Cells;
	}

	void Reset()
	{
		for (int i = 0; i < m_Cells.Num(); i++)
		{
			m_Cells[i].Reset();
		}
	}

	void Reset(int iColumn)
	{
		int iFinal = m_Cells[iColumn].m_iFinalIcon;

		m_Cells[iColumn].Reset();

		for (int i = 0; i < m_Cells.Num(); i++)
		{
			if (i == iColumn)
				continue;

			if (m_Cells[i].m_iFinalIcon >= 0)
			{
				m_Cells[iColumn].m_bValues[m_Cells[i].m_iFinalIcon] = false;
			}
			else if (iFinal >= 0)
			{
				m_Cells[i].m_bValues[iFinal] = true;
			}
		}

		m_Cells[iColumn].m_iFinalIcon = m_Cells[iColumn].GetRemainingIcon();
	}

	bool IsCompleted() const
	{
		for (int i = 0; i < m_Cells.Num(); i++)
		{
			if (m_Cells[i].m_iFinalIcon < 0)
				return false;
		}

		return true;
	}
};