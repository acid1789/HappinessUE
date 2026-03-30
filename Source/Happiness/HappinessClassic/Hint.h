#pragma once

#include "CoreMinimal.h"

class Clue;
class Puzzle;

class Hint
{
private:

	Clue* TheClue = nullptr;

	bool bSetFinalIcon = false;

	int Row = 0;
	int Col = 0;
	int Icon = 0;

public:

	Hint() = default;

	bool Init(Puzzle& P, Clue& C);

	bool ShouldHide(Puzzle& P) const;

	bool ShouldDraw(int InRow, int InCol, int InIcon) const;

	bool ShouldDraw(const Clue& C) const;
};