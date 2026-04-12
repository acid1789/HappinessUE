#pragma once

#include <CoreMinimal.h>
#include "UObject/Object.h"
#include "ClueHelp.h"
#include "Clue.generated.h"

class UPuzzle;

UENUM(BlueprintType)
enum class eClueType : uint8
{
	Given,
	Vertical,
	Horizontal
};

UENUM(BlueprintType)
enum class eVerticalType : uint8
{
	Two,
	Three,
	EitherOr,
	TwoNot,
	ThreeTopNot,
	ThreeMidNot,
	ThreeBotNot
};

UENUM(BlueprintType)
enum class eHorizontalType : uint8
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

UCLASS(BlueprintType)
class HAPPINESS_API UClue : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly)
	eClueType m_Type;

	UPROPERTY(BlueprintReadOnly)
	int m_iUseCount = 0;

	UPROPERTY(BlueprintReadOnly)
	int m_iRow = 0;

	UPROPERTY(BlueprintReadOnly)
	int m_iRow2 = 0;

	UPROPERTY(BlueprintReadOnly)
	int m_iRow3 = 0;

	UPROPERTY(BlueprintReadOnly)
	int m_iCol = 0;

	UPROPERTY(BlueprintReadOnly)
	int m_iCol2 = 0;

	UPROPERTY(BlueprintReadOnly)
	int m_iCol3 = 0;

	UPROPERTY(BlueprintReadOnly)
	int m_iHorizontal1 = 0;

	UPROPERTY(BlueprintReadOnly)
	int m_iNotCell = 0;

	UPROPERTY(BlueprintReadOnly)
	eVerticalType m_VerticalType;

	UPROPERTY(BlueprintReadOnly)
	eHorizontalType m_HorizontalType;

	UPROPERTY(BlueprintReadOnly)
	FClueHelp ClueHelp;

public:

	UFUNCTION(BlueprintCallable)
	void GetRows(TArray<int>& Rows);

	UFUNCTION(BlueprintCallable)
	void GetIcons(UPuzzle* Puzzle, TArray<int>& Icons);
	
	void Init(UPuzzle& UPuzzle, FRandomStream& Rand);

	bool operator<(const UClue& Other) const
	{
		return m_iUseCount > Other.m_iUseCount;
	}

	void Analyze(UPuzzle& P);

	void Dump(int Index, UPuzzle& P);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Clue")
	FString ToString() const;

	bool GetHintAction(UPuzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Clue")
	int64 GetId() const;

	UFUNCTION(BlueprintCallable)
	void SetFromId(int64 Id);

private:

	void GenerateClue(UPuzzle& P, FRandomStream& Rand);
	void PickClueType(UPuzzle& P, FRandomStream& Rand);

	void GenerateGiven(UPuzzle& P, FRandomStream& Rand);

	void GenerateVertical(UPuzzle& P, FRandomStream& Rand);
	void GenerateTwoRowColumn(UPuzzle& P, FRandomStream& Rand);
	void GenerateThreeRowColumn(UPuzzle& P, FRandomStream& Rand);

	void GenerateHorizontal(UPuzzle& P, FRandomStream& Rand);
	void GenerateHorizontalSpan(UPuzzle& P, FRandomStream& Rand);

private:

	void AnalyzeGiven(UPuzzle& P);

	void AnalyzeVertical(UPuzzle& P);
	void AnalyzeVerticalTwo(UPuzzle& P);
	void AnalyzeVerticalThree(UPuzzle& P);
	void AnalyzeVerticalEitherOr(UPuzzle& P);
	void AnalyzeVerticalTwoNot(UPuzzle& P);
	void AnalyzeVerticalThreeTopNot(UPuzzle& P);
	void AnalyzeVerticalThreeMidNot(UPuzzle& P);
	void AnalyzeVerticalThreeBotNot(UPuzzle& P);

	void AnalyzeHorizontal(UPuzzle& P);
	void AnalyzeHorizontalNextTo(UPuzzle& P);
	void AnalyzeHorizontalNotNextTo(UPuzzle& P);
	void AnalyzeHorizontalLeftOf(UPuzzle& P);
	void AnalyzeHorizontalNotLeftOf(UPuzzle& P);
	void AnalyzeHorizontalSpan(UPuzzle& P);
	void AnalyzeHorizontalSpanNotLeft(UPuzzle& P);
	void AnalyzeHorizontalSpanNotMid(UPuzzle& P);
	void AnalyzeHorizontalSpanNotRight(UPuzzle& P);

	bool SolveSpan(int iCol, int iRow1, int iIcon1, bool bNot1, int iRow2, int iIcon2, bool bNot2, int iRow3, int iIcon3, bool bNot3, UPuzzle& P);
	bool GetHintActionSpan(int iCol, int iRow1, int iIcon1, bool bNot1, int iRow2, int iIcon2, bool bNot2, int iRow3, int iIcon3, bool bNot3, UPuzzle& P, bool bSetFinalIcon, int iOutRow, int iOutCol, int iOutIcon);

	bool GetHintActionHorizontalNextTo(UPuzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionHorizontalNotNextTo(UPuzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionHorizontalLeftOf(UPuzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionHorizontalNotLeftOf(UPuzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionHorizontalSpan(UPuzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionHorizontalSpanNotLeft(UPuzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionHorizontalSpanNotMid(UPuzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionHorizontalSpanNotRight(UPuzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);

	bool GetHintActionVerticalTwo(UPuzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionVerticalThree(UPuzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionVerticalEitherOr(UPuzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionVerticalTwoNot(UPuzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionVerticalThreeTopNot(UPuzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionVerticalThreeMidNot(UPuzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);
	bool GetHintActionVerticalThreeBotNot(UPuzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);

	FString HorizontalToString() const;
	FString VerticalToString() const;

	void GenerateClueHelp(UPuzzle& P);
};