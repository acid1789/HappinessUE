#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "HappinessMCPLookupCommandlet.generated.h"

/**
 * Runs the MCP lookup functions from the command line, no MCP server needed.
 *
 * UnrealEditor-Cmd.exe Happiness.uproject -run=HappinessMCPLookup -out=Path and one of:
 *   -toolsets                         list toolset names
 *   -find=Query [-max=N]              FindFunctions
 *   -list=Toolset                     ListFunctions
 *   -describe=Toolset.Function        DescribeFunction
 */
UCLASS()
class UHappinessMCPLookupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UHappinessMCPLookupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
