// Console commands (PIE console, ~) for testing the lessons and the campaign. Development builds only. Each set
// switches to its mode first (Happiness.Campaign.* completes every lesson first if Campaign mode is still locked),
// then changes that mode's progress and saves it: back up the save first if it matters.
//
//   Happiness.Lessons.List                    every lesson's points, stage, state and time played
//   Happiness.Lessons.Reset                   clear every lesson
//   Happiness.Lessons.Clear <Lesson>          clear one lesson
//   Happiness.Lessons.Points <Lesson> <N>     set its points (the stages they unlock open)
//   Happiness.Lessons.Stage <Lesson> <Stage>  just enough points to unlock that stage (0 first, the final last)
//   Happiness.Lessons.ToFinal [Lesson]        every point, the final open but not played
//   Happiness.Lessons.Complete [Lesson]       complete it; the lessons tree, if showing, plays its completion
//   Happiness.Lessons.ToColumn <Column>       complete every lesson before that tree column, clear the rest
//
//   Happiness.Campaign.<the same>             the same for the campaign (points are solved puzzles there)
//   Happiness.Campaign.Master <Lesson> <N>    complete it and set its Master Mode puzzles solved (shown already)
//   Happiness.Campaign.MasterAll              complete every lesson and all of its Master Mode puzzles
//   Happiness.Campaign.CompleteAll            complete every lesson, the last as if just played: the campaign's
//                                             congratulations play (a showing tree first celebrates the last lesson)
//
// <Lesson> is its name (NextTo; any case) or number (ECampaignLesson). Left out, it is the first lesson in the tree
// that isn't completed.

#include "CampaignProgress.h"
#include "CampaignTree.h"
#include "UI/CampaignTreeWidget.h"
#include "UI/HappinessModeWidget.h"

#include "HAL/IConsoleManager.h"
#include "UObject/UObjectIterator.h"

#if !UE_BUILD_SHIPPING
namespace
{
	/** One set of commands: Happiness.Lessons.* or Happiness.Campaign.* */
	struct FTrack
	{
		ECampaignMode Mode;
		const TCHAR* Name;
	};

	FString LessonEnumName(ECampaignLesson Lesson)
	{
		return StaticEnum<ECampaignLesson>()->GetNameStringByValue(int64(Lesson));
	}

	/** The lesson named by Args[Index]; the first incomplete one when left out and bDefaultNext */
	bool ParseLessonArg(const TArray<FString>& Args, int32 Index, bool bDefaultNext, const UCampaignSubsystem& Campaign,
		const FTrack& Track, ECampaignLesson& Lesson, FOutputDevice& Ar)
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
				Ar.Logf(TEXT("Name a lesson (e.g. NextTo); Happiness.%s.List lists them"), Track.Name);
			}
			return false;
		}

		const FString& Arg = Args[Index];
		const int64 Value = Arg.IsNumeric() ? FCString::Atoi(*Arg) : StaticEnum<ECampaignLesson>()->GetValueByNameString(Arg);
		if (Value < 0 || Value > 255 || UCampaignTree::GetLessonColumn(ECampaignLesson(Value)) == INDEX_NONE)
		{
			Ar.Logf(TEXT("No lesson '%s' in the tree; Happiness.%s.List lists them"), *Arg, Track.Name);
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
		const FString Master = Campaign.IsMasterUnlocked(Lesson)
			? FString::Printf(TEXT("  master %d/%d%s"), Progress.MasterPoints, UCampaignSubsystem::GetMasterMaxPoints(),
				Progress.bMasterRevealed ? TEXT("") : TEXT(" (not shown yet)"))
			: FString();
		Ar.Logf(TEXT("  %-16s column %d  %2d/%d points  stage %d  %s%s  time %.0fs"), *LessonEnumName(Lesson),
			UCampaignTree::GetLessonColumn(Lesson), Progress.Points, UCampaignSubsystem::GetMaxPoints(), Stage, State, *Master,
			Progress.PlaySeconds);
	}

	/** Switch to the track's mode; Campaign mode needs every lesson completed */
	void EnterTrack(UCampaignSubsystem& Campaign, const FTrack& Track, FOutputDevice& Ar)
	{
		if (Track.Mode == ECampaignMode::Campaign && !Campaign.IsCampaignModeUnlocked())
		{
			Campaign.CompleteAllLessons();
			Ar.Log(TEXT("Campaign mode was locked: every lesson completed"));
		}
		if (UCampaignSubsystem::GetMode() != Track.Mode)
		{
			Campaign.SetMode(Track.Mode);
			Ar.Logf(TEXT("Switched to %s mode"), Track.Name);
		}
	}

	using FTrackCommand = TFunction<void(UCampaignSubsystem&, const FTrack&, const TArray<FString>&, UWorld*, FOutputDevice&)>;

	TArray<TUniquePtr<FAutoConsoleCommandWithWorldArgsAndOutputDevice>> GCommands;

	/** Happiness.<Track>.<Name>; Run gets the world's campaign, already in the track's mode */
	void AddCommand(const FTrack& Track, const TCHAR* Name, const TCHAR* Help, FTrackCommand Run)
	{
		GCommands.Add(MakeUnique<FAutoConsoleCommandWithWorldArgsAndOutputDevice>(
			*FString::Printf(TEXT("Happiness.%s.%s"), Track.Name, Name),
			*FString::Printf(TEXT("Happiness.%s.%s%s"), Track.Name, Name, Help),
			FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda(
				[Track, Run](const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
				{
					if (UCampaignSubsystem* Campaign = UCampaignSubsystem::Get(World))
					{
						EnterTrack(*Campaign, Track, Ar);
						Run(*Campaign, Track, Args, World, Ar);
					}
					else
					{
						Ar.Log(TEXT("No game running (start PIE)"));
					}
				})));
	}

	void AddTrackCommands(const FTrack& InTrack)
	{
		AddCommand(InTrack, TEXT("List"), TEXT(": lists every lesson's progress"),
			[](UCampaignSubsystem& Campaign, const FTrack& Track, const TArray<FString>&, UWorld*, FOutputDevice& Ar)
			{
				Ar.Logf(TEXT("%s mode:"), Track.Name);
				for (const TArray<ECampaignLesson>& Column : UCampaignTree::GetColumns())
				{
					for (ECampaignLesson Lesson : Column)
					{
						ReportLesson(Campaign, Lesson, Ar);
					}
				}
			});

		AddCommand(InTrack, TEXT("Reset"), TEXT(": clears every lesson"),
			[](UCampaignSubsystem& Campaign, const FTrack&, const TArray<FString>&, UWorld*, FOutputDevice& Ar)
			{
				Campaign.DebugResetMode();
				Ar.Log(TEXT("Every lesson cleared"));
			});

		AddCommand(InTrack, TEXT("Clear"), TEXT(" <Lesson>: clears one lesson's progress"),
			[](UCampaignSubsystem& Campaign, const FTrack& Track, const TArray<FString>& Args, UWorld*, FOutputDevice& Ar)
			{
				ECampaignLesson Lesson;
				if (ParseLessonArg(Args, 0, false, Campaign, Track, Lesson, Ar))
				{
					Campaign.DebugSetLesson(Lesson, 0, false);
					ReportLesson(Campaign, Lesson, Ar);
				}
			});

		AddCommand(InTrack, TEXT("Points"), TEXT(" <Lesson> <N>: sets a lesson's points, its final not completed"),
			[](UCampaignSubsystem& Campaign, const FTrack& Track, const TArray<FString>& Args, UWorld*, FOutputDevice& Ar)
			{
				ECampaignLesson Lesson;
				int32 Points;
				if (ParseLessonArg(Args, 0, false, Campaign, Track, Lesson, Ar) && ParseIntArg(Args, 1, Points, Ar))
				{
					Campaign.DebugSetLesson(Lesson, Points, false);
					ReportLesson(Campaign, Lesson, Ar);
				}
			});

		AddCommand(InTrack, TEXT("Stage"), TEXT(" <Lesson> <Stage>: just enough points to unlock the stage (0 first, the final last)"),
			[](UCampaignSubsystem& Campaign, const FTrack& Track, const TArray<FString>& Args, UWorld*, FOutputDevice& Ar)
			{
				ECampaignLesson Lesson;
				int32 Stage;
				if (ParseLessonArg(Args, 0, false, Campaign, Track, Lesson, Ar) && ParseIntArg(Args, 1, Stage, Ar))
				{
					Stage = FMath::Clamp(Stage, 0, UCampaignSubsystem::GetFinalStage());
					Campaign.DebugSetLesson(Lesson, UCampaignSubsystem::GetStagePointsRequired(Stage), false);
					ReportLesson(Campaign, Lesson, Ar);
				}
			});

		AddCommand(InTrack, TEXT("ToFinal"), TEXT(" [Lesson]: every point, the final open but not played (default: the next lesson)"),
			[](UCampaignSubsystem& Campaign, const FTrack& Track, const TArray<FString>& Args, UWorld*, FOutputDevice& Ar)
			{
				ECampaignLesson Lesson;
				if (ParseLessonArg(Args, 0, true, Campaign, Track, Lesson, Ar))
				{
					Campaign.DebugSetLesson(Lesson, UCampaignSubsystem::GetMaxPoints(), false);
					ReportLesson(Campaign, Lesson, Ar);
				}
			});

		AddCommand(InTrack, TEXT("Complete"), TEXT(" [Lesson]: completes it; a showing lessons tree plays its completion (default: the next lesson)"),
			[](UCampaignSubsystem& Campaign, const FTrack& Track, const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
			{
				ECampaignLesson Lesson;
				if (!ParseLessonArg(Args, 0, true, Campaign, Track, Lesson, Ar))
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

		AddCommand(InTrack, TEXT("ToColumn"), TEXT(" <Column>: completes every lesson before the tree column (0 first) and clears the rest"),
			[](UCampaignSubsystem& Campaign, const FTrack&, const TArray<FString>& Args, UWorld*, FOutputDevice& Ar)
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

	struct FRegisterCommands
	{
		FRegisterCommands()
		{
			const FTrack LessonsTrack{ECampaignMode::Lessons, TEXT("Lessons")};
			const FTrack CampaignTrack{ECampaignMode::Campaign, TEXT("Campaign")};
			AddTrackCommands(LessonsTrack);
			AddTrackCommands(CampaignTrack);

			AddCommand(CampaignTrack, TEXT("Master"), TEXT(" <Lesson> <N>: completes it and sets its Master Mode puzzles solved, already shown"),
				[](UCampaignSubsystem& Campaign, const FTrack& Track, const TArray<FString>& Args, UWorld*, FOutputDevice& Ar)
				{
					ECampaignLesson Lesson;
					int32 Points;
					if (ParseLessonArg(Args, 0, false, Campaign, Track, Lesson, Ar) && ParseIntArg(Args, 1, Points, Ar))
					{
						Campaign.DebugSetMaster(Lesson, Points);
						ReportLesson(Campaign, Lesson, Ar);
					}
				});

			AddCommand(CampaignTrack, TEXT("CompleteAll"), TEXT(": completes every lesson, the last as if just played, so the campaign's congratulations play"),
				[](UCampaignSubsystem& Campaign, const FTrack&, const TArray<FString>&, UWorld* World, FOutputDevice& Ar)
				{
					const TArray<TArray<ECampaignLesson>>& Columns = UCampaignTree::GetColumns();
					const ECampaignLesson Last = Columns.Last().Last();
					for (const TArray<ECampaignLesson>& Column : Columns)
					{
						for (ECampaignLesson Lesson : Column)
						{
							if (Lesson != Last && !Campaign.IsLessonCompleted(Lesson))
							{
								Campaign.DebugSetLesson(Lesson, UCampaignSubsystem::GetMaxPoints(), true);
							}
						}
					}
					// The last cleared first, so completing it completes the campaign again (repeatable)
					Campaign.DebugSetLesson(Last, 0, false);
					Campaign.DebugSetLesson(Last, UCampaignSubsystem::GetMaxPoints(), true, true);
					Ar.Log(TEXT("Every lesson completed: the campaign's congratulations play on the mode screen"));

					// A showing tree celebrates the last lesson, then closes to the mode screen; a showing mode
					// screen opens the congratulations now. Otherwise they play when the mode screen next shows.
					bool bShown = false;
					for (TObjectIterator<UCampaignTreeWidget> Tree; Tree; ++Tree)
					{
						if (Tree->GetWorld() == World && Tree->IsVisible())
						{
							Tree->ShowForLessonSession();
							bShown = true;
						}
					}
					for (TObjectIterator<UHappinessModeWidget> Mode; Mode && !bShown; ++Mode)
					{
						if (Mode->GetWorld() == World && Mode->IsVisible())
						{
							Mode->Show();
							bShown = true;
						}
					}
				});

			AddCommand(CampaignTrack, TEXT("MasterAll"), TEXT(": completes every lesson and all of its Master Mode puzzles"),
				[](UCampaignSubsystem& Campaign, const FTrack&, const TArray<FString>&, UWorld*, FOutputDevice& Ar)
				{
					for (const TArray<ECampaignLesson>& Column : UCampaignTree::GetColumns())
					{
						for (ECampaignLesson Lesson : Column)
						{
							Campaign.DebugSetMaster(Lesson, UCampaignSubsystem::GetMasterMaxPoints());
						}
					}
					Ar.Log(TEXT("Every lesson mastered"));
				});
		}
	} GRegisterCommands;
}
#endif
