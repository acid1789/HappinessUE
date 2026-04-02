#pragma once

#include "CoreMinimal.h"
#include "ClueHelp.generated.h"


UENUM(BlueprintType)
enum class EClueHelpSegementType : uint8
{
	Text,
	Icon
};

USTRUCT(BlueprintType)
struct FClueHelpSegment
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	EClueHelpSegementType Type;

	UPROPERTY(BlueprintReadOnly)
	int IconRow;

	UPROPERTY(BlueprintReadOnly)
	int IconColumn;

	UPROPERTY(BlueprintReadOnly)
	FString Text;

	
};

USTRUCT(BlueprintType)
struct FClueHelp
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	TArray<FClueHelpSegment> Segments;

	void AddText(FString InText) 
	{ 
		FClueHelpSegment segment = { EClueHelpSegementType::Text, 0, 0, InText };
		Segments.Add(segment);
	}

	void AddIcon(int Row, int Col)
	{
		FClueHelpSegment segment = { EClueHelpSegementType::Icon, Row, Col, TEXT("") };
		Segments.Add(segment);
	}
};