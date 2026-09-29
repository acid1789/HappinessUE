#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CampaignLesson.h"
#include "CampaignTree.generated.h"

/**
 * The campaign's lesson tree. Lessons in the same column can be played in any order, and every
 * lesson in a column must be completed before the next column unlocks.
 *
 * Campaign puzzles for a lesson only use clue types from earlier columns plus the lesson itself,
 * so a lesson never shows a clue type from a sibling lesson the player may not have done yet.
 */
UCLASS()
class HAPPINESS_API UCampaignTree : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// The tree, left to right. Edit this to change the campaign's shape.
	static const TArray<TArray<ECampaignLesson>>& GetColumns();

	UFUNCTION(BlueprintPure, Category = "Campaign")
	static int32 GetNumColumns();

	UFUNCTION(BlueprintPure, Category = "Campaign")
	static TArray<ECampaignLesson> GetColumnLessons(int32 Column);

	// Column the lesson is in, or -1 if it isn't a playable lesson (Given)
	UFUNCTION(BlueprintPure, Category = "Campaign")
	static int32 GetLessonColumn(ECampaignLesson Lesson);

	// Name shown on the lesson's tree node
	UFUNCTION(BlueprintPure, Category = "Campaign")
	static FText GetLessonDisplayName(ECampaignLesson Lesson);

	// Explanation shown in the lesson popup
	UFUNCTION(BlueprintPure, Category = "Campaign")
	static FText GetLessonDescription(ECampaignLesson Lesson);

	// True if a clue of ClueLesson may appear in puzzles for CurrentLesson: givens always, the lesson
	// itself, and anything from an earlier column
	static bool IsClueLessonAllowed(ECampaignLesson ClueLesson, ECampaignLesson CurrentLesson);
};
