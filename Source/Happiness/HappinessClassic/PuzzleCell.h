#pragma once

#include "CoreMinimal.h"

class PuzzleCell
{
public:

	// True if the icon is still in the cell
	TArray<bool> m_bValues;

	int m_iFinalIcon;

public:

	PuzzleCell(int iSize)
	{
		m_bValues.SetNum(iSize);
		Reset();
	}

	PuzzleCell(const PuzzleCell& Other)
	{
		m_bValues = Other.m_bValues;
		m_iFinalIcon = Other.m_iFinalIcon;
	}

	int GetRemainingIcon() const
	{
		int iRemaining = -1;
		int iCount = 0;

		for (int i = 0; i < m_bValues.Num(); i++)
		{
			if (m_bValues[i])
			{
				iCount++;
				iRemaining = i;
			}
		}

		if (iCount > 1)
			iRemaining = -1;

		return iRemaining;
	}

	void Reset()
	{
		for (int i = 0; i < m_bValues.Num(); i++)
		{
			m_bValues[i] = true;
		}

		m_iFinalIcon = -1;
	}
};