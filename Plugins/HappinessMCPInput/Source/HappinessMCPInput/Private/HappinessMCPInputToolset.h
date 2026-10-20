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

	/** Runs a console command in the Play-In-Editor world (the game's dev commands, e.g. Happiness.SolvePuzzle). False if PIE isn't running or the command wasn't handled. */
	UFUNCTION(meta = (AICallable), Category = "HappinessMCPInput")
	static bool ExecPIECommand(const FString& Command);
};
