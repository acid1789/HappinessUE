// Development console commands for testing puzzles in the game. Not in Shipping builds.

#include "Puzzle.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

namespace
{
	UObject* GetObjectProperty(UObject* Owner, const TCHAR* Name)
	{
		const FObjectPropertyBase* Property = Owner ? FindFProperty<FObjectPropertyBase>(Owner->GetClass(), Name) : nullptr;
		return Property ? Property->GetObjectPropertyValue_InContainer(Owner) : nullptr;
	}

	// Fill the open puzzle with its solution and let the puzzle screen finish it as if the player had: its
	// RefreshPuzzle sees a completed board and shows the end screen (scoring, progress, save).
	// Type "Happiness.SolvePuzzle" in the PIE console (~).
	void SolvePuzzle(UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		UObject* Screen = GetObjectProperty(PC, TEXT("HappinessWidget"));	// PC_Happiness: the puzzle screen
		UPuzzle* Puzzle = Cast<UPuzzle>(GetObjectProperty(Screen, TEXT("ThePuzzle")));
		UFunction* Refresh = Screen ? Screen->FindFunction(TEXT("RefreshPuzzle")) : nullptr;
		if (!Puzzle || !Refresh)
		{
			UE_LOG(LogTemp, Warning, TEXT("Happiness.SolvePuzzle: no puzzle is open"));
			return;
		}
		if (Puzzle->IsCompleted())
		{
			UE_LOG(LogTemp, Warning, TEXT("Happiness.SolvePuzzle: the puzzle is already finished"));
			return;
		}

		for (int Row = 0; Row < Puzzle->m_iSize; Row++)
		{
			for (int Col = 0; Col < Puzzle->m_iSize; Col++)
			{
				FPuzzleCell& Cell = Puzzle->m_Rows[Row].m_Cells[Col];
				const int Icon = Puzzle->SolutionIcon(Row, Col);
				for (int i = 0; i < Cell.m_bValues.Num(); i++)
				{
					Cell.m_bValues[i] = i == Icon;
				}
				Cell.m_iFinalIcon = Icon;
			}
		}

		Screen->ProcessEvent(Refresh, nullptr);
	}

	FAutoConsoleCommandWithWorld GSolvePuzzleCommand(
		TEXT("Happiness.SolvePuzzle"),
		TEXT("Completes the open puzzle correctly and shows its end screen"),
		FConsoleCommandWithWorldDelegate::CreateStatic(&SolvePuzzle));
}

#endif
