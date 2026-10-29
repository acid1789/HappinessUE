// Development console commands for testing puzzles in the game. Not in Shipping builds.

#include "Puzzle.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "UObject/EnumProperty.h"

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

	// Switch PC_Happiness.InputMode (Blueprint enum: NewEnumerator0 touch, NewEnumerator1 mouse), as BeginPlay sets
	// it on the phone. The grid wires its cells for the mode when it is built, so start a new puzzle afterwards (Reset
	// Puzzle keeps the cells). Type "Happiness.InputMode [touch|mouse]" in the PIE console (~); left out, it toggles.
	void SetInputMode(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		const FProperty* Property = PC ? PC->GetClass()->FindPropertyByName(TEXT("InputMode")) : nullptr;
		const FByteProperty* ByteProperty = CastField<FByteProperty>(Property);
		const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property);
		if (!ByteProperty && !EnumProperty)
		{
			Ar.Logf(ELogVerbosity::Warning, TEXT("Happiness.InputMode: no PC_Happiness (start the game first)"));
			return;
		}
		void* Value = Property->ContainerPtrToValuePtr<void>(PC);
		const int64 Current = ByteProperty ? ByteProperty->GetPropertyValue(Value) : EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(Value);
		int64 Mode = Current == 0 ? 1 : 0;
		if (Args.Num() > 0)
		{
			if (Args[0].Equals(TEXT("touch"), ESearchCase::IgnoreCase))
			{
				Mode = 0;
			}
			else if (Args[0].Equals(TEXT("mouse"), ESearchCase::IgnoreCase))
			{
				Mode = 1;
			}
			else
			{
				Ar.Logf(ELogVerbosity::Warning, TEXT("Happiness.InputMode: say touch or mouse (or nothing to toggle)"));
				return;
			}
		}
		if (ByteProperty)
		{
			ByteProperty->SetPropertyValue(Value, uint8(Mode));
		}
		else
		{
			EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(Value, Mode);
		}
		Ar.Logf(TEXT("Happiness.InputMode: %s. Start a new puzzle for the grid to use it"), Mode == 0 ? TEXT("touch") : TEXT("mouse"));
	}

	FAutoConsoleCommandWithWorldArgsAndOutputDevice GInputModeCommand(
		TEXT("Happiness.InputMode"),
		TEXT("Happiness.InputMode [touch|mouse]: switch the player's input mode (left out, toggle). Start a new puzzle afterwards"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&SetInputMode));
}

#endif
