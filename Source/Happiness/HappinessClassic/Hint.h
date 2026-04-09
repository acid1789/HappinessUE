#pragma once

#include "CoreMinimal.h"
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


	UHint() = default;

	bool Init(UPuzzle& P, UClue& C);

	bool ShouldHide(UPuzzle& P) const;

	bool ShouldDraw(int InRow, int InCol, int InIcon) const;

	bool ShouldDraw(const UClue& C) const;
};