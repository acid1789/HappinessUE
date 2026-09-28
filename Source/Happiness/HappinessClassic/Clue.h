#pragma once

#include <CoreMinimal.h>
#include "UObject/Object.h"
#include "ClueHelp.h"
#include "CampaignLesson.h"
#include "Clue.generated.h"

class UPuzzle;

UENUM(BlueprintType)
enum class eClueType : uint8
{
	Given,
	Vertical,
	Horizontal,
	NotHere			// Given variant: icon m_iHorizontal1 is not in cell (m_iRow, m_iCol); applied up front like Given
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
	SpanNotRight,

	// Extended clues. These use display slots 0..2 = (m_iRow, m_iCol), (m_iRow2, m_iCol2), (m_iRow3, m_iCol3);
	// an unused slot has row and column -1. See GetSlots().
	Edge,			// Slot 0 is in the first or last column
	NotEdge,		// Slot 0 is not in the first or last column
	DirectlyLeftOf,	// Slot 0 is immediately left of slot 1
	Gap,			// Slot 0 and slot 1 have exactly one column between them (the middle column is unspecified)
	Between,		// Slot 1 is somewhere between slot 0 and slot 2
	Chain,			// Slot 0 is left of slot 1, which is left of slot 2
	NextToEitherOr,	// Slot 1 is next to slot 0 or slot 2, not both. Slot m_iNotCell (0 or 2) is the false option:
					// its icon is m_iHorizontal1 and its column is -1
	AllApart		// Slots 0, 1 and 2 (three different rows) are all in different columns
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

	// Make this a Given on a random cell that isn't placed yet
	void InitGiven(UPuzzle& P, FRandomStream& Rand);

	bool operator<(const UClue& Other) const
	{
		return m_iUseCount > Other.m_iUseCount;
	}

	void Analyze(UPuzzle& P);

	void Dump(int Index, UPuzzle& P);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Clue")
	FString ToString() const;

	bool GetHintAction(UPuzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);

	// True for clue types analyzed by the generic column constraint solver (the extended horizontal types, Edge and later)
	bool IsConstraintClue() const;

	// Row and icon for each display slot of a constraint clue; unused slots are -1
	void GetSlots(const UPuzzle& P, int Rows[3], int Icons[3]) const;

	// Campaign: add clues equivalent to this clue with its "not" component removed (e.g. SpanNotMid -> Gap,
	// ThreeTopNot -> Two). Adds nothing for purely negative clues, and a copy of this clue if it has no "not".
	void GetPositiveParts(UPuzzle& P, TArray<UClue*>& Out);

	// True if Other is the same clue (used for duplicate rejection of NotHere and constraint clues)
	bool IsSameClue(const UClue& Other) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Clue")
	int64 GetId() const;

	// The campaign lesson that teaches this clue's type (see ECampaignLesson for the ordering)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Clue")
	ECampaignLesson GetCampaignLesson() const;

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

	void GenerateNotHere(UPuzzle& P, FRandomStream& Rand);
	bool GenerateHorizontalExtended(UPuzzle& P, FRandomStream& Rand);

private:

	void AnalyzeGiven(UPuzzle& P);
	void AnalyzeNotHere(UPuzzle& P);

	// Generic analysis for constraint clues: eliminate any icon position that no arrangement satisfying the clue supports
	bool ConstraintHolds(const int Cols[3], int Size) const;
	bool HasSupport(UPuzzle& P, const int Rows[3], const int Icons[3], int Slot, int Col) const;
	void AnalyzeConstraint(UPuzzle& P);
	bool GetHintActionConstraint(UPuzzle& P, bool& bSetFinalIcon, int& Row, int& Col, int& Icon);

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
	bool GetHintActionSpan(int iCol, int iRow1, int iIcon1, bool bNot1, int iRow2, int iIcon2, bool bNot2, int iRow3, int iIcon3, bool bNot3, UPuzzle& P, bool& bSetFinalIcon, int& iOutRow, int& iOutCol, int& iOutIcon);

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
	FString ExtendedToString() const;

	void GenerateClueHelp(UPuzzle& P);
};