#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "HappinessUIToolset.generated.h"

/** Tools for checking UI work visually. */
UCLASS()
class UHappinessUIToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * Renders a Widget Blueprint (e.g. "/Game/Happiness/UI/WBP_CampaignTree") offscreen at Width x Height and
	 * saves it as a PNG. Returns the file path, or "Error: ...". OutputFile defaults to Saved/WidgetRenders/<name>.png.
	 */
	UFUNCTION(meta = (AICallable), Category = "HappinessUI")
	static FString RenderWidget(const FString& WidgetBlueprintPath, int32 Width = 1920, int32 Height = 1080, const FString& OutputFile = TEXT(""), int32 PreviewPuzzleSize = 0);
};
