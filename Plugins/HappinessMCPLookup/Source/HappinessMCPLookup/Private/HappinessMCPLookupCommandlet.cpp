#include "HappinessMCPLookupCommandlet.h"
#include "HappinessMCPLookupToolset.h"

#include "Editor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ToolsetRegistry/Toolset.h"
#include "ToolsetRegistry/ToolsetRegistry.h"
#include "ToolsetRegistry/ToolsetRegistrySubsystem.h"

UHappinessMCPLookupCommandlet::UHappinessMCPLookupCommandlet()
{
	LogToConsole = false;
	ShowErrorCount = false;
}

int32 UHappinessMCPLookupCommandlet::Main(const FString& Params)
{
	const TCHAR* Cmd = *Params;

	FString OutPath = FPaths::ProjectSavedDir() / TEXT("MCPLookup.txt");
	FParse::Value(Cmd, TEXT("out="), OutPath);

	FString Result;
	FString Value;
	int32 Max = 25;
	FParse::Value(Cmd, TEXT("max="), Max);

	if (FParse::Param(Cmd, TEXT("toolsets")))
	{
		UToolsetRegistrySubsystem* Subsystem = GEditor ? GEditor->GetEditorSubsystem<UToolsetRegistrySubsystem>() : nullptr;
		if (!Subsystem)
		{
			Result = TEXT("Error: ToolsetRegistry not available.");
		}
		else
		{
			TArray<FString> Lines;
			Subsystem->ToolsetRegistry.ForEachToolset([&Lines](const FString& Name, const UE::ToolsetRegistry::FToolset& Toolset)
			{
				// Descriptions wrap across lines; keep one line
				FString Description = Toolset.GetToolsetDescription().Replace(TEXT("\r"), TEXT(" ")).Replace(TEXT("\n"), TEXT(" "));
				while (Description.ReplaceInline(TEXT("  "), TEXT(" ")) > 0) {}
				Lines.Add(FString::Printf(TEXT("%s%s: %s"), *Name, Toolset.IsEnabled() ? TEXT("") : TEXT(" (disabled)"),
					*Description.Left(120)));
			});
			Lines.Sort();
			Result = FString::Join(Lines, TEXT("\n"));
		}
	}
	else if (FParse::Value(Cmd, TEXT("schemasize="), Value))
	{
		// Size of what describe_toolset would return, for comparison
		UToolsetRegistrySubsystem* Subsystem = GEditor ? GEditor->GetEditorSubsystem<UToolsetRegistrySubsystem>() : nullptr;
		const TSharedPtr<UE::ToolsetRegistry::FToolset> Toolset = Subsystem ? Subsystem->ToolsetRegistry.Find(Value) : nullptr;
		Result = Toolset.IsValid()
			? FString::Printf(TEXT("%s full schema: %d characters"), *Value, Toolset->GetJsonSchema().Len())
			: TEXT("Error: toolset not found");
	}
	else if (FParse::Value(Cmd, TEXT("find="), Value))
	{
		Result = UHappinessMCPLookupToolset::FindFunctions(Value, Max);
	}
	else if (FParse::Value(Cmd, TEXT("list="), Value))
	{
		Result = UHappinessMCPLookupToolset::ListFunctions(Value);
	}
	else if (FParse::Value(Cmd, TEXT("describe="), Value))
	{
		FString Toolset, Function;
		// Toolset names contain dots, so the function is after the last one
		Result = Value.Split(TEXT("."), &Toolset, &Function, ESearchCase::IgnoreCase, ESearchDir::FromEnd)
			? UHappinessMCPLookupToolset::DescribeFunction(Toolset, Function)
			: TEXT("Error: use -describe=Toolset.Function");
	}
	else
	{
		Result = TEXT("Usage: -toolsets | -find=Query [-max=N] | -list=Toolset | -describe=Toolset.Function");
	}

	FFileHelper::SaveStringToFile(Result + TEXT("\n"), *OutPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	return 0;
}
