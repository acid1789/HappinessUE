#pragma once

#include "CoreMinimal.h"
#include "ClueHelp.h"
#include "Hint.generated.h"

class UPuzzle;
class UClue;

UCLASS(BlueprintType)
class HAPPINESS_API UHint : public UObject
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadOnly)
	UClue* TheClue = nullptr;

	UPROPERTY(BlueprintReadOnly)
	bool bSetFinalIcon = false;
	
	UPROPERTY(BlueprintReadOnly)
	int Row = 0;

	UPROPERTY(BlueprintReadOnly)
	int Col = 0;

	UPROPERTY(BlueprintReadOnly)
	int Icon = 0;

	/** How much the clue works out from the board: candidates it removes when applied fully (Init with bPreferPlacement) */
	UPROPERTY(BlueprintReadOnly)
	int Impact = 0;


	UHint() = default;

	/**
	 * Set up the hint from clue C. With bPreferPlacement, if everything the clue works out from the board settles
	 * one of its icons in a single column, the hint places that icon (the most useful thing the clue says) rather
	 * than giving the clue's first single deduction. The rating solve passes false to keep one deduction per step.
	 */
	bool Init(UPuzzle& P, UClue& C, bool bPreferPlacement = true);

	bool ShouldHide(UPuzzle& P) const;

	bool ShouldDraw(int InRow, int InCol, int InIcon) const;

	bool ShouldDraw(const UClue& C) const;

	/** What the hint says to do, for the hint info panel: "So [icon] can't be in column 3" / "So [icon] goes in column 3" */
	UFUNCTION(BlueprintPure)
	FClueHelp GetActionHelp() const;

	/**
	 * Why the hint follows from its clue and the board, ending with what to do, e.g.
	 * "[Daisy] is in this column, so [Top] can't be here" or "[Bow] can't be in column 2 or 4, so [Top] can't be here".
	 * Call it while the board is as it was when the hint was made. Falls back to GetActionHelp when no single
	 * reason can be found.
	 */
	UFUNCTION(BlueprintCallable)
	FClueHelp GetExplanation();

private:
	/**
	 * For a placement: when the clue clears every other icon out of the hinted cell, explain each of those
	 * eliminations and end with "That leaves [icon], so it goes here". False if that isn't how the clue gets there.
	 */
	bool ExplainPlacementByCell(FClueHelp& Help);

	/**
	 * For a placement: when the clue rules the subject out of its other columns, explain each of those (naming the
	 * columns) and end with "That leaves only this column for [icon], so it goes here". False if it can't.
	 */
	bool ExplainPlacementByRow(FClueHelp& Help);

	/**
	 * Set by GetExplanation: the placement it explained isn't a single step from the board as it is. Only a
	 * placement the clue fixes directly from icons already placed is one step; walk-throughs are not.
	 */
	bool bIndirectPlacement = false;

	/** Set by ExplainPlacementByRow: how many different reasons its walk-through gave */
	int RowWalkReasons = 0;

	/** Why the clue alone keeps the subject out of its column (edge columns and the like), for GetExplanation */
	void ExplainNoRoom(FClueHelp& Help, int Subject, const TArray<int>& SlotRows, const TArray<int>& SlotIcons, int Size) const;
};