#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "HappinessMCPLookupToolset.generated.h"

/**
 * Cheap way to find and learn MCP tools one function at a time. Use these instead of describe_toolset,
 * which returns every parameter schema in a toolset. Typical flow: FindFunctions -> DescribeFunction -> call_tool.
 */
UCLASS()
class UHappinessMCPLookupToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/** Search every toolset for functions whose name or description contains Query (case-insensitive). Returns "Toolset.Function: short description" lines, no schemas. */
	UFUNCTION(meta = (AICallable), Category = "HappinessMCPLookup")
	static FString FindFunctions(const FString& Query, int32 MaxResults = 25);

	/** List a toolset's functions as "Function: short description" lines, without parameter schemas. */
	UFUNCTION(meta = (AICallable), Category = "HappinessMCPLookup")
	static FString ListFunctions(const FString& ToolsetName);

	/** Full description and input schema for one function. FunctionName may be "Function" or "Toolset.Function". */
	UFUNCTION(meta = (AICallable), Category = "HappinessMCPLookup")
	static FString DescribeFunction(const FString& ToolsetName, const FString& FunctionName);
};
