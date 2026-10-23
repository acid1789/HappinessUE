#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "HappinessMCPInputToolset.generated.h"

UCLASS()
class UHappinessMCPInputToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/** Clicks at normalized coordinates in the active Play-In-Editor viewport. */
	UFUNCTION(meta = (AICallable), Category = "HappinessMCPInput")
	static bool ClickViewport(float X, float Y, const FString& Button = TEXT("Left"));

	/** Taps (touch start and end) at normalized coordinates in the active Play-In-Editor viewport, like a finger on a phone. */
	UFUNCTION(meta = (AICallable), Category = "HappinessMCPInput")
	static bool TapViewport(float X, float Y);

	/** Runs a console command in the Play-In-Editor world (the game's dev commands, e.g. Happiness.SolvePuzzle). False if PIE isn't running or the command wasn't handled. */
	UFUNCTION(meta = (AICallable), Category = "HappinessMCPInput")
	static bool ExecPIECommand(const FString& Command);

	/**
	 * Sets a property, from its text form, on every object of a class in the Play-In-Editor world (class defaults and
	 * assets untouched), e.g. ("PC_Happiness_C", "InputMode", "NewEnumerator0"). The editor refuses the console's set
	 * command. Returns how many objects were set, -1 if PIE isn't running or the class or property doesn't exist.
	 */
	UFUNCTION(meta = (AICallable), Category = "HappinessMCPInput")
	static int32 SetPIEProperty(const FString& ClassName, const FString& PropertyName, const FString& Value);
};
