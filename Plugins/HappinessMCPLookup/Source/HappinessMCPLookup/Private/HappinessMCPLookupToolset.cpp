#include "HappinessMCPLookupToolset.h"

#include "Editor.h"
#include "Dom/JsonObject.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "ToolsetRegistry/Toolset.h"
#include "ToolsetRegistry/ToolsetRegistry.h"
#include "ToolsetRegistry/ToolsetRegistrySubsystem.h"

namespace
{
	using UE::ToolsetRegistry::FToolset;
	using UE::ToolsetRegistry::FToolsetRegistry;

	FToolsetRegistry* GetRegistry()
	{
		UToolsetRegistrySubsystem* Subsystem = GEditor ? GEditor->GetEditorSubsystem<UToolsetRegistrySubsystem>() : nullptr;
		return Subsystem ? &Subsystem->ToolsetRegistry : nullptr;
	}

	// The tool entries of a toolset's schema. The full schema is built here in the editor; only the
	// slice a caller asks for is ever returned, which is what keeps agent token use down.
	TArray<TSharedPtr<FJsonObject>> GetTools(const FToolset& Toolset)
	{
		TArray<TSharedPtr<FJsonObject>> Tools;

		TSharedPtr<FJsonObject> Schema;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Toolset.GetJsonSchema());
		if (!FJsonSerializer::Deserialize(Reader, Schema) || !Schema.IsValid())
		{
			return Tools;
		}

		const TArray<TSharedPtr<FJsonValue>>* ToolValues = nullptr;
		if (Schema->TryGetArrayField(TEXT("tools"), ToolValues))
		{
			for (const TSharedPtr<FJsonValue>& Value : *ToolValues)
			{
				if (TSharedPtr<FJsonObject> Tool = Value->AsObject())
				{
					Tools.Add(Tool);
				}
			}
		}
		return Tools;
	}

	// Tool names are "Toolset.Function"; call_tool wants just the function part
	FString FunctionPart(const FString& ToolName)
	{
		int32 Dot;
		return ToolName.FindLastChar('.', Dot) ? ToolName.RightChop(Dot + 1) : ToolName;
	}

	// First sentence or line of a description, capped so listings stay small
	FString ShortDescription(const FString& Description)
	{
		FString Short = Description;

		int32 End = INDEX_NONE;
		for (int32 i = 0; i < Short.Len(); i++)
		{
			const TCHAR C = Short[i];
			if (C == '\n' || C == '\r' || (C == '.' && (i + 1 == Short.Len() || FChar::IsWhitespace(Short[i + 1]))))
			{
				End = i;
				break;
			}
		}
		if (End != INDEX_NONE)
		{
			Short.LeftInline(End);
		}

		Short.TrimStartAndEndInline();
		if (Short.Len() > 120)
		{
			Short = Short.Left(117) + TEXT("...");
		}
		return Short;
	}

	FString ToolsetNotFound(FToolsetRegistry& Registry, const FString& ToolsetName)
	{
		TArray<FString> Names;
		Registry.ForEachToolset([&Names](const FString& Name, const FToolset&) { Names.Add(Name); });
		Names.Sort();
		return FString::Printf(TEXT("Error: toolset '%s' not found. Toolsets: %s"), *ToolsetName, *FString::Join(Names, TEXT(", ")));
	}
}

FString UHappinessMCPLookupToolset::FindFunctions(const FString& Query, int32 MaxResults)
{
	FToolsetRegistry* Registry = GetRegistry();
	if (!Registry)
	{
		return TEXT("Error: ToolsetRegistry not available.");
	}
	if (Query.IsEmpty())
	{
		return TEXT("Error: Query is empty. Pass a word such as 'widget', 'blueprint' or 'compile'.");
	}

	MaxResults = FMath::Clamp(MaxResults, 1, 200);

	TArray<FString> Lines;
	int32 Matches = 0;
	Registry->ForEachToolset([&](const FString& ToolsetName, const FToolset& Toolset)
	{
		if (!Toolset.IsEnabled())
		{
			return;
		}

		for (const TSharedPtr<FJsonObject>& Tool : GetTools(Toolset))
		{
			const FString Name = Tool->GetStringField(TEXT("name"));
			FString Description;
			Tool->TryGetStringField(TEXT("description"), Description);

			if (Name.Contains(Query) || Description.Contains(Query))
			{
				if (++Matches <= MaxResults)
				{
					Lines.Add(FString::Printf(TEXT("%s.%s: %s"), *ToolsetName, *FunctionPart(Name), *ShortDescription(Description)));
				}
			}
		}
	});

	if (Lines.IsEmpty())
	{
		return FString::Printf(TEXT("No functions match '%s'."), *Query);
	}

	Lines.Sort();
	FString Result = FString::Join(Lines, TEXT("\n"));
	if (Matches > MaxResults)
	{
		Result += FString::Printf(TEXT("\n(%d more; narrow the query or raise MaxResults)"), Matches - MaxResults);
	}
	return Result;
}

FString UHappinessMCPLookupToolset::ListFunctions(const FString& ToolsetName)
{
	FToolsetRegistry* Registry = GetRegistry();
	if (!Registry)
	{
		return TEXT("Error: ToolsetRegistry not available.");
	}

	const TSharedPtr<FToolset> Toolset = Registry->Find(ToolsetName);
	if (!Toolset.IsValid())
	{
		return ToolsetNotFound(*Registry, ToolsetName);
	}

	TArray<FString> Lines;
	for (const TSharedPtr<FJsonObject>& Tool : GetTools(*Toolset))
	{
		FString Description;
		Tool->TryGetStringField(TEXT("description"), Description);
		Lines.Add(FString::Printf(TEXT("%s: %s"), *FunctionPart(Tool->GetStringField(TEXT("name"))), *ShortDescription(Description)));
	}

	Lines.Sort();
	return FString::Printf(TEXT("%s (%d functions)\n%s"), *ToolsetName, Lines.Num(), *FString::Join(Lines, TEXT("\n")));
}

FString UHappinessMCPLookupToolset::DescribeFunction(const FString& ToolsetName, const FString& FunctionName)
{
	FToolsetRegistry* Registry = GetRegistry();
	if (!Registry)
	{
		return TEXT("Error: ToolsetRegistry not available.");
	}

	const TSharedPtr<FToolset> Toolset = Registry->Find(ToolsetName);
	if (!Toolset.IsValid())
	{
		return ToolsetNotFound(*Registry, ToolsetName);
	}

	const FString Wanted = FunctionPart(FunctionName);
	for (const TSharedPtr<FJsonObject>& Tool : GetTools(*Toolset))
	{
		if (FunctionPart(Tool->GetStringField(TEXT("name"))).Equals(Wanted, ESearchCase::IgnoreCase))
		{
			FString Json;
			const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
				TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json);
			FJsonSerializer::Serialize(Tool.ToSharedRef(), Writer);
			return Json;
		}
	}

	return FString::Printf(TEXT("Error: function '%s' not found in toolset '%s'. Use ListFunctions or FindFunctions."), *Wanted, *ToolsetName);
}
