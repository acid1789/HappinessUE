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
};
