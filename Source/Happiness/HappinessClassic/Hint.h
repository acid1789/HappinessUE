#pragma once

#include "CoreMinimal.h"
#include "Hint.generated.h"

class UPuzzle;
class UClue;

UCLASS(BlueprintType)
class HAPPINESS_API UHint : public UObject
{
	GENERATED_BODY()

	UClue* TheClue = nullptr;

	bool bSetFinalIcon = false;

	int Row = 0;
	int Col = 0;
	int Icon = 0;

public:

	UHint() = default;

	bool Init(UPuzzle& P, UClue& C);

	bool ShouldHide(UPuzzle& P) const;

	bool ShouldDraw(int InRow, int InCol, int InIcon) const;

	bool ShouldDraw(const UClue& C) const;
};