#include "FreePlaySettings.h"
#include "CampaignTree.h"
#include "DailyPuzzle.h"
#include "HappinessSaveGame.h"
#include "Puzzle.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"

#define LOCTEXT_NAMESPACE "FreePlay"

TWeakObjectPtr<UFreePlaySubsystem> UFreePlaySubsystem::Instance;

void UFreePlaySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Instance = this;
	if (const UHappinessSaveGame* SaveGame = UHappinessSaveGame::LoadFromSlot())
	{
		Data = SaveGame->FreePlay;
	}

	// A pace learned from different ratings would be off; start it over
	if (Data.RatingVersion != RatingVersion)
	{
		Data.SecondsPerPoint = 0.f;
		Data.SolvedPuzzles = 0;
		Data.RatingVersion = RatingVersion;
	}
}

void UFreePlaySubsystem::Deinitialize()
{
	if (Instance == this)
	{
		Instance = nullptr;
	}
	Super::Deinitialize();
}

TArray<ECampaignLesson> UFreePlaySubsystem::GetSelectableClues()
{
	TArray<ECampaignLesson> Clues;
	for (const TArray<ECampaignLesson>& Column : UCampaignTree::GetColumns())
	{
		Clues.Append(Column);
	}
	return Clues;
}

FText UFreePlaySubsystem::GetClueName(ECampaignLesson Clue)
{
	// The lesson names, except the first lesson's, which is named for the game rather than its clue
	if (Clue == ECampaignLesson::VerticalTwo)
	{
		return LOCTEXT("SameColumn", "Same Column");
	}
	return UCampaignTree::GetLessonDisplayName(Clue);
}

bool UFreePlaySubsystem::IsClueIncluded(ECampaignLesson Clue) const
{
	return !Data.ExcludedClues.Contains(Clue);
}

void UFreePlaySubsystem::SetClueIncluded(ECampaignLesson Clue, bool bIncluded)
{
	if (Clue == ECampaignLesson::Given || IsClueIncluded(Clue) == bIncluded)
	{
		return;
	}

	if (bIncluded)
	{
		Data.ExcludedClues.Remove(Clue);
	}
	else
	{
		Data.ExcludedClues.Add(Clue);
	}
	Changed();
}

void UFreePlaySubsystem::SetAllCluesIncluded(bool bIncluded)
{
	Data.ExcludedClues.Reset();
	if (!bIncluded)
	{
		Data.ExcludedClues = GetSelectableClues();
	}
	Changed();
}

int32 UFreePlaySubsystem::GetIncludedClueCount() const
{
	int32 Count = 0;
	for (ECampaignLesson Clue : GetSelectableClues())
	{
		Count += IsClueIncluded(Clue) ? 1 : 0;
	}
	return Count;
}

FText UFreePlaySubsystem::GetSelectionSummary() const
{
	const int32 Total = GetSelectableClues().Num();
	const int32 Included = GetIncludedClueCount();
	if (Included == Total)
	{
		return LOCTEXT("AllClues", "All");
	}
	if (Included == 0)
	{
		return LOCTEXT("GivensOnly", "Givens only");
	}
	return FText::Format(LOCTEXT("SomeClues", "{0} of {1}"), Included, Total);
}

int32 UFreePlaySubsystem::GetExcludedClueMask() const
{
	int32 Mask = 0;
	for (ECampaignLesson Clue : Data.ExcludedClues)
	{
		if (Clue != ECampaignLesson::Given)
		{
			Mask |= 1 << int32(Clue);
		}
	}
	return Mask;
}

UFreePlaySubsystem* UFreePlaySubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UFreePlaySubsystem>() : nullptr;
}

float UFreePlaySubsystem::GetSecondsPerPoint() const
{
	return Data.SecondsPerPoint > 0.f ? Data.SecondsPerPoint : DefaultSecondsPerPoint;
}

float UFreePlaySubsystem::GetParSeconds(const UObject* WorldContextObject, UPuzzle* Puzzle)
{
	const UFreePlaySubsystem* FreePlay = Get(WorldContextObject);
	const float Pace = FreePlay ? FreePlay->GetSecondsPerPoint() : DefaultSecondsPerPoint;
	return Puzzle ? Puzzle->GetRating() * Pace * ParAllowance : 0.f;
}

FFreePlayScore UFreePlaySubsystem::FinishFreePlayPuzzle(const UObject* WorldContextObject, UPuzzle* Puzzle, float PuzzleSeconds)
{
	UFreePlaySubsystem* FreePlay = Get(WorldContextObject);
	if (!FreePlay || !Puzzle)
	{
		return FFreePlayScore();
	}

	// The same finish asked for again (the end screen recomputing): don't count the time twice
	if (FreePlay->LastScoredPuzzle.Get() == Puzzle && FreePlay->LastScore.PuzzleSeconds == PuzzleSeconds)
	{
		return FreePlay->LastScore;
	}

	FFreePlayScore Score;
	UDailySubsystem* Daily = UDailySubsystem::Get(WorldContextObject);
	Score.bDaily = Daily && Daily->IsActiveDailyPuzzle(Puzzle);
	Score.Rating = Puzzle->GetRating();
	Score.ParSeconds = Score.Rating * FreePlay->GetSecondsPerPoint() * ParAllowance;
	Score.PuzzleSeconds = PuzzleSeconds;
	Score.bSolved = Puzzle->IsSolved();

	if (Score.bSolved)
	{
		Score.BaseExp = FMath::RoundToInt(Score.Rating * ExpPerRatingPoint);

		// Nothing at par, a quarter of the maximum at two thirds of it, the maximum at half of it or better
		if (PuzzleSeconds > 0.f && PuzzleSeconds < Score.ParSeconds)
		{
			const float Bonus = FMath::Min(MaxTimeBonus, MaxTimeBonus * (Score.ParSeconds / PuzzleSeconds - 1.f));
			Score.BonusExp = FMath::RoundToInt(Score.BaseExp * Bonus);
		}

		// The daily puzzle: its first solve counts toward the streak and is worth double. The amounts here stay
		// normal; the end screen doubles the total in a step of its own (UFreePlayEndScreenWidget).
		if (Daily && Daily->RecordSolve(Puzzle))
		{
			Score.bDailyBonus = true;
		}

		// Fold this solve into the pace. One very slow (left running) or very fast solve can only move it so far.
		if (Score.Rating > 0.f && PuzzleSeconds > 0.f)
		{
			const float Pace = FreePlay->GetSecondsPerPoint();
			const float Sample = FMath::Clamp(PuzzleSeconds / Score.Rating, Pace / 3.f, Pace * 3.f);
			FreePlay->Data.SecondsPerPoint = FreePlay->Data.SolvedPuzzles == 0 ? Sample : FMath::Lerp(Pace, Sample, PaceSmoothing);
			FreePlay->Data.SolvedPuzzles++;
			UHappinessSaveGame::SaveCurrentSettings();
		}
	}

	if (Score.bDaily)
	{
		Score.DailyStreak = Daily->GetStreak();
	}
	FreePlay->LastScore = Score;
	FreePlay->LastScoredPuzzle = Puzzle;
	return Score;
}

FFreePlayScore UFreePlaySubsystem::GetLastFreePlayScore(const UObject* WorldContextObject)
{
	const UFreePlaySubsystem* FreePlay = Get(WorldContextObject);
	return FreePlay ? FreePlay->LastScore : FFreePlayScore();
}

void UFreePlaySubsystem::Changed()
{
	// Written into the game save now; it would also go with the game's next save
	UHappinessSaveGame::SaveCurrentSettings();
	OnCluesChanged.Broadcast();
}

#undef LOCTEXT_NAMESPACE
