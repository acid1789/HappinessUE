// Console commands (PIE console, ~) for testing the lessons. Development builds only. They change the current
// mode's progress (Lessons, or Campaign once switched to it) and save it: back up the save first if it matters.
//
//   Happiness.Lessons.List                    every lesson's points, stage and state
//   Happiness.Lessons.Reset                   clear every lesson
//   Happiness.Lessons.Clear <Lesson>          clear one lesson
//   Happiness.Lessons.Points <Lesson> <N>     set its points (the stages they unlock open)
//   Happiness.Lessons.Stage <Lesson> <Stage>  just enough points to unlock that stage (0 first, the final last)
//   Happiness.Lessons.ToFinal [Lesson]        every point, the final open but not played
//   Happiness.Lessons.Complete [Lesson]       complete it; the lessons tree, if showing, plays its completion
//   Happiness.Lessons.ToColumn <Column>       complete every lesson before that tree column, clear the rest
//
// <Lesson> is its name (NextTo; any case) or number (ECampaignLesson). Left out, it is the first lesson in the tree
// that isn't completed.

#include "CampaignProgress.h"
#include "CampaignTree.h"
#include "UI/CampaignTreeWidget.h"

#include "HAL/IConsoleManager.h"
#include "UObject/UObjectIterator.h"

#if !UE_BUILD_SHIPPING
namespace
{
	FString LessonEnumName(ECampaignLesson Lesson)
	{
		return StaticEnum<ECampaignLesson>()->GetNameStringByValue(int64(Lesson));
	}

	/** The lesson named by Args[Index]; the first incomplete one when left out and bDefaultNext */
	bool ParseLessonArg(const TArray<FString>& Args, int32 Index, bool bDefaultNext, const UCampaignSubsystem& Campaign,
		ECampaignLesson& Lesson, FOutputDevice& Ar)
	{
		if (!Args.IsValidIndex(Index))
		{
			if (bDefaultNext)
			{
				for (const TArray<ECampaignLesson>& Column : UCampaignTree::GetColumns())
				{
					for (ECampaignLesson Next : Column)
					{
						if (!Campaign.IsLessonCompleted(Next))
						{
							Lesson = Next;
							return true;
						}
					}
				}
				Ar.Log(TEXT("Every lesson is already completed"));
			}
			else
			{
				Ar.Log(TEXT("Name a lesson (e.g. NextTo); Happiness.Lessons.List lists them"));
			}
			return false;
		}

		const FString& Arg = Args[Index];
		const int64 Value = Arg.IsNumeric() ? FCString::Atoi(*Arg) : StaticEnum<ECampaignLesson>()->GetValueByNameString(Arg);
		if (Value < 0 || Value > 255 || UCampaignTree::GetLessonColumn(ECampaignLesson(Value)) == INDEX_NONE)
		{
			Ar.Logf(TEXT("No lesson '%s' in the tree; Happiness.Lessons.List lists them"), *Arg);
			return false;
		}
		Lesson = ECampaignLesson(Value);
		return true;
	}

	bool ParseIntArg(const TArray<FString>& Args, int32 Index, int32& Value, FOutputDevice& Ar)
	{
		if (!Args.IsValidIndex(Index) || !Args[Index].IsNumeric())
		{
			Ar.Log(TEXT("Expected a number"));
			return false;
		}
		Value = FCString::Atoi(*Args[Index]);
		return true;
	}

	void ReportLesson(const UCampaignSubsystem& Campaign, ECampaignLesson Lesson, FOutputDevice& Ar)
	{
		const FLessonProgress Progress = Campaign.GetLessonProgress(Lesson);
		const int32 Stage = Campaign.GetCurrentStage(Lesson);
		const TCHAR* State = Progress.bFinalCompleted ? TEXT("completed")
			: !Campaign.IsLessonUnlocked(Lesson) ? TEXT("locked")
			: Stage == UCampaignSubsystem::GetFinalStage() ? TEXT("final open")
			: TEXT("open");
		Ar.Logf(TEXT("  %-16s column %d  %2d/%d points  stage %d  %s"), *LessonEnumName(Lesson),
			UCampaignTree::GetLessonColumn(Lesson), Progress.Points, UCampaignSubsystem::GetMaxPoints(), Stage, State);
	}

	using FLessonCommand = TFunction<void(UCampaignSubsystem&, const TArray<FString>&, UWorld*, FOutputDevice&)>;

	/** Happiness.Lessons.<Name>; Run gets the world's campaign */
	FAutoConsoleCommandWithWorldArgsAndOutputDevice MakeLessonCommand(const TCHAR* Name, const TCHAR* Help, FLessonCommand Run)
	{
		return FAutoConsoleCommandWithWorldArgsAndOutputDevice(*(FString(TEXT("Happiness.Lessons.")) + Name), Help,
			FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda(
				[Run](const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
				{
					if (UCampaignSubsystem* Campaign = UCampaignSubsystem::Get(World))
					{
						Run(*Campaign, Args, World, Ar);
					}
					else
					{
						Ar.Log(TEXT("No game running (start PIE)"));
					}
				}));
	}

	FAutoConsoleCommandWithWorldArgsAndOutputDevice GListCommand = MakeLessonCommand(TEXT("List"),
		TEXT("Lists every lesson's progress in the current mode"),
		[](UCampaignSubsystem& Campaign, const TArray<FString>&, UWorld*, FOutputDevice& Ar)
		{
			Ar.Logf(TEXT("%s mode:"), UCampaignSubsystem::GetMode() == ECampaignMode::Campaign ? TEXT("Campaign") : TEXT("Lessons"));
			for (const TArray<ECampaignLesson>& Column : UCampaignTree::GetColumns())
			{
				for (ECampaignLesson Lesson : Column)
				{
					ReportLesson(Campaign, Lesson, Ar);
				}
			}
		});

	FAutoConsoleCommandWithWorldArgsAndOutputDevice GResetCommand = MakeLessonCommand(TEXT("Reset"),
		TEXT("Clears every lesson in the current mode"),
		[](UCampaignSubsystem& Campaign, const TArray<FString>&, UWorld*, FOutputDevice& Ar)
		{
			Campaign.DebugResetMode();
			Ar.Log(TEXT("Every lesson cleared"));
		});

	FAutoConsoleCommandWithWorldArgsAndOutputDevice GClearCommand = MakeLessonCommand(TEXT("Clear"),
		TEXT("Happiness.Lessons.Clear <Lesson>: clears one lesson's progress"),
		[](UCampaignSubsystem& Campaign, const TArray<FString>& Args, UWorld*, FOutputDevice& Ar)
		{
			ECampaignLesson Lesson;
			if (ParseLessonArg(Args, 0, false, Campaign, Lesson, Ar))
			{
				Campaign.DebugSetLesson(Lesson, 0, false);
				ReportLesson(Campaign, Lesson, Ar);
			}
		});

	FAutoConsoleCommandWithWorldArgsAndOutputDevice GPointsCommand = MakeLessonCommand(TEXT("Points"),
		TEXT("Happiness.Lessons.Points <Lesson> <N>: sets a lesson's points, its final not completed"),
		[](UCampaignSubsystem& Campaign, const TArray<FString>& Args, UWorld*, FOutputDevice& Ar)
		{
			ECampaignLesson Lesson;
			int32 Points;
			if (ParseLessonArg(Args, 0, false, Campaign, Lesson, Ar) && ParseIntArg(Args, 1, Points, Ar))
			{
				Campaign.DebugSetLesson(Lesson, Points, false);
				ReportLesson(Campaign, Lesson, Ar);
			}
		});

	FAutoConsoleCommandWithWorldArgsAndOutputDevice GStageCommand = MakeLessonCommand(TEXT("Stage"),
		TEXT("Happiness.Lessons.Stage <Lesson> <Stage>: just enough points to unlock the stage (0 first, the final last)"),
		[](UCampaignSubsystem& Campaign, const TArray<FString>& Args, UWorld*, FOutputDevice& Ar)
		{
			ECampaignLesson Lesson;
			int32 Stage;
			if (ParseLessonArg(Args, 0, false, Campaign, Lesson, Ar) && ParseIntArg(Args, 1, Stage, Ar))
			{
				Stage = FMath::Clamp(Stage, 0, UCampaignSubsystem::GetFinalStage());
				Campaign.DebugSetLesson(Lesson, UCampaignSubsystem::GetStagePointsRequired(Stage), false);
				ReportLesson(Campaign, Lesson, Ar);
			}
		});

	FAutoConsoleCommandWithWorldArgsAndOutputDevice GToFinalCommand = MakeLessonCommand(TEXT("ToFinal"),
		TEXT("Happiness.Lessons.ToFinal [Lesson]: every point, the final open but not played (default: the next lesson)"),
		[](UCampaignSubsystem& Campaign, const TArray<FString>& Args, UWorld*, FOutputDevice& Ar)
		{
			ECampaignLesson Lesson;
			if (ParseLessonArg(Args, 0, true, Campaign, Lesson, Ar))
			{
				Campaign.DebugSetLesson(Lesson, UCampaignSubsystem::GetMaxPoints(), false);
				ReportLesson(Campaign, Lesson, Ar);
			}
		});

	FAutoConsoleCommandWithWorldArgsAndOutputDevice GCompleteCommand = MakeLessonCommand(TEXT("Complete"),
		TEXT("Happiness.Lessons.Complete [Lesson]: completes it; a showing lessons tree plays its completion (default: the next lesson)"),
		[](UCampaignSubsystem& Campaign, const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
		{
			ECampaignLesson Lesson;
			if (!ParseLessonArg(Args, 0, true, Campaign, Lesson, Ar))
			{
				return;
			}
			Campaign.DebugSetLesson(Lesson, UCampaignSubsystem::GetMaxPoints(), true, true);
			ReportLesson(Campaign, Lesson, Ar);

			// As if back from its final. Not showing: it plays when the tree next comes back from a lesson puzzle.
			for (TObjectIterator<UCampaignTreeWidget> Tree; Tree; ++Tree)
			{
				if (Tree->GetWorld() == World && Tree->IsVisible())
				{
					Tree->ShowForLessonSession();
				}
			}
		});

	FAutoConsoleCommandWithWorldArgsAndOutputDevice GToColumnCommand = MakeLessonCommand(TEXT("ToColumn"),
		TEXT("Happiness.Lessons.ToColumn <Column>: completes every lesson before the tree column (0 first) and clears the rest"),
		[](UCampaignSubsystem& Campaign, const TArray<FString>& Args, UWorld*, FOutputDevice& Ar)
		{
			int32 Target;
			if (!ParseIntArg(Args, 0, Target, Ar))
			{
				return;
			}
			const TArray<TArray<ECampaignLesson>>& Columns = UCampaignTree::GetColumns();
			for (int32 Column = 0; Column < Columns.Num(); Column++)
			{
				for (ECampaignLesson Lesson : Columns[Column])
				{
					const bool bDone = Column < Target;
					Campaign.DebugSetLesson(Lesson, bDone ? UCampaignSubsystem::GetMaxPoints() : 0, bDone);
				}
			}
			Ar.Logf(TEXT("Columns before %d completed, the rest cleared"), Target);
		});
}
#endif
