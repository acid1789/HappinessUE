#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "PuzzleCommandlet.generated.h"

class UPuzzle;

/**
 * Headless puzzle generator / solver for testing the puzzle core.
 *
 * UnrealEditor-Cmd.exe Happiness.uproject -run=Puzzle [options]
 *   -seed=N | -seeds=A-B   seed or inclusive seed range (default 1)
 *   -size=N                puzzle size (default 6)
 *   -diff=N                difficulty 0..2 (default 1)
 *   -mode=batch|show|trace batch: one summary line per puzzle (failures only unless -all)
 *                          show:  solution + clue list for each puzzle
 *                          trace: show + every hint step
 *   -all                   in batch mode, print passing puzzles too
 *   -noauto                disable UPuzzle::AutoSetIcons during the hint solve
 *   -out=Path              output file (default <Project>/Saved/PuzzleCLI.txt)
 */
UCLASS()
class UPuzzleCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UPuzzleCommandlet();

	virtual int32 Main(const FString& Params) override;
};
