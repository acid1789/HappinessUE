#pragma once

#include <CoreMinimal.h>

class Puzzle;

enum class eClueType
{
	Given,
	Vertical,
	Horizontal
};

enum class eVerticalType
{
	Two,
	Three,
	EitherOr,
	TwoNot,
	ThreeTopNot,
	ThreeMidNot,
	ThreeBotNot
};

enum class eHorizontalType
{
	NextTo,
	NotNextTo,
	LeftOf,
	NotLeftOf,
	Span,
	SpanNotLeft,
	SpanNotMid,
	SpanNotRight
};

class Clue
{
public:

	eClueType m_Type;

	int m_iUseCount = 0;

	int m_iRow = 0;
	int m_iRow2 = 0;
	int m_iRow3 = 0;

	int m_iCol = 0;
	int m_iCol2 = 0;
	int m_iCol3 = 0;

	int m_iHorizontal1 = 0;
	int m_iNotCell = 0;

	eVerticalType m_VerticalType;
	eHorizontalType m_HorizontalType;

public:

	Clue(Puzzle& P, FRandomStream& Rand);

	bool operator<(const Clue& Other) const
	{
		return m_iUseCount > Other.m_iUseCount;
	}

	void Analyze(Puzzle& P);

	void Dump(int Index, Puzzle& P);

	bool GetHintAction(Puzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);

private:

	void GenerateClue(Puzzle& P, FRandomStream& Rand);
	void PickClueType(Puzzle& P, FRandomStream& Rand);

	void GenerateGiven(Puzzle& P, FRandomStream& Rand);

	void GenerateVertical(Puzzle& P, FRandomStream& Rand);
	void GenerateTwoRowColumn(Puzzle& P, FRandomStream& Rand);
	void GenerateThreeRowColumn(Puzzle& P, FRandomStream& Rand);

	void GenerateHorizontal(Puzzle& P, FRandomStream& Rand);
	void GenerateHorizontalSpan(Puzzle& P, FRandomStream& Rand);

private:

	void AnalyzeGiven(Puzzle& P);

	void AnalyzeVertical(Puzzle& P);
	void AnalyzeVerticalTwo(Puzzle& P);
	void AnalyzeVerticalThree(Puzzle& P);
	void AnalyzeVerticalEitherOr(Puzzle& P);
	void AnalyzeVerticalTwoNot(Puzzle& P);
	void AnalyzeVerticalThreeTopNot(Puzzle& P);
	void AnalyzeVerticalThreeMidNot(Puzzle& P);
	void AnalyzeVerticalThreeBotNot(Puzzle& P);

	void AnalyzeHorizontal(Puzzle& P);
	void AnalyzeHorizontalNextTo(Puzzle& P);
	void AnalyzeHorizontalNotNextTo(Puzzle& P);
	void AnalyzeHorizontalLeftOf(Puzzle& P);
	void AnalyzeHorizontalNotLeftOf(Puzzle& P);
	void AnalyzeHorizontalSpan(Puzzle& P);
	void AnalyzeHorizontalSpanNotLeft(Puzzle& P);
	void AnalyzeHorizontalSpanNotMid(Puzzle& P);
	void AnalyzeHorizontalSpanNotRight(Puzzle& P);

	bool SolveSpan(int iCol, int iRow1, int iIcon1, bool bNot1, int iRow2, int iIcon2, bool bNot2, int iRow3, int iIcon3, bool bNot3, Puzzle& P);
	bool GetHintActionSpan(int iCol, int iRow1, int iIcon1, bool bNot1, int iRow2, int iIcon2, bool bNot2, int iRow3, int iIcon3, bool bNot3, Puzzle P, bool bSetFinalIcon, int iOutRow, int iOutCol, int iOutIcon);

	bool GetHintActionHorizontalNextTo(Puzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionHorizontalNotNextTo(Puzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionHorizontalLeftOf(Puzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionHorizontalNotLeftOf(Puzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionHorizontalSpan(Puzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionHorizontalSpanNotLeft(Puzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionHorizontalSpanNotMid(Puzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionHorizontalSpanNotRight(Puzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);

	bool GetHintActionVerticalTwo(Puzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionVerticalThree(Puzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionVerticalEitherOr(Puzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionVerticalTwoNot(Puzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionVerticalThreeTopNot(Puzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionVerticalThreeMidNot(Puzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionVerticalThreeBotNot(Puzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);

};