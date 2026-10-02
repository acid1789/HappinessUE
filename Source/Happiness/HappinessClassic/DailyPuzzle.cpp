#include "DailyPuzzle.h"
#include "FreePlaySettings.h"
#include "HappinessSaveGame.h"
#include "Puzzle.h"

#include "Blueprint/UserWidget.h"
#include "UI/FreePlayEndScreenWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "Math/RandomStream.h"
#include "Misc/DateTime.h"

TWeakObjectPtr<UDailySubsystem> UDailySubsystem::Instance;

namespace
{
#if !UE_BUILD_SHIPPING
	// Testing: play as if today were another date (YYYYMMDD; 0 = the real date), e.g. to check streaks
	TAutoConsoleVariable<int32> CVarDailyDate(
		TEXT("Happiness.DailyDate"), 0,
		TEXT("Daily puzzle: use this date (YYYYMMDD) as today; 0 = the real date"));
#endif

	FDateTime FromDate(int32 Date)
	{
		return FDateTime(Date / 10000, (Date / 100) % 100, Date % 100);
	}

	int32 ToDate(const FDateTime& Day)
	{
		return Day.GetYear() * 10000 + Day.GetMonth() * 100 + Day.GetDay();
	}

	bool IsValidDate(int32 Date)
	{
		return FDateTime::Validate(Date / 10000, (Date / 100) % 100, Date % 100, 0, 0, 0, 0);
	}

	/** Days from one date (YYYYMMDD) to another */
	int32 DaysBetween(int32 From, int32 To)
	{
		return (FromDate(To) - FromDate(From)).GetDays();
	}
}

void UDailySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Instance = this;

	if (const UHappinessSaveGame* SaveGame = UHappinessSaveGame::LoadFromSlot())
	{
		Data = SaveGame->Daily;
	}
}

void UDailySubsystem::Deinitialize()
{
	if (Instance == this)
	{
		Instance = nullptr;
	}
	Super::Deinitialize();
}

int32 UDailySubsystem::GetToday()
{
#if !UE_BUILD_SHIPPING
	const int32 Override = CVarDailyDate.GetValueOnGameThread();
	if (Override > 0 && IsValidDate(Override))
	{
		return Override;
	}
#endif
	return ToDate(FDateTime::Now());
}

FDailyPuzzleSpec UDailySubsystem::GetSpec(int32 Date)
{
	// Everything comes from the date, in this order, so a date always gives the same puzzle
	FRandomStream Rand(Date);
	FDailyPuzzleSpec Spec;
	Spec.Date = Date;

	// A random set of at least 5 clue types
	TArray<ECampaignLesson> Clues = UFreePlaySubsystem::GetSelectableClues();
	for (int32 i = Clues.Num() - 1; i > 0; i--)
	{
		Clues.Swap(i, Rand.RandRange(0, i));
	}
	const int32 Count = Rand.RandRange(FMath::Min(5, Clues.Num()), Clues.Num());
	Spec.Clues = TArray<ECampaignLesson>(Clues.GetData(), Count);
	for (int32 i = Count; i < Clues.Num(); i++)
	{
		Spec.ExcludedClueMask |= 1 << int32(Clues[i]);
	}

	// 4x4 to 7x7, mostly 5x5 and 6x6
	const float SizeRoll = Rand.FRand();
	Spec.Size = SizeRoll < 0.15f ? 4 : SizeRoll < 0.5f ? 5 : SizeRoll < 0.85f ? 6 : 7;
	Spec.Difficulty = Rand.RandRange(0, 2);
	Spec.Seed = Rand.RandRange(1, 1 << 30);
	return Spec;
}

int32 UDailySubsystem::GetStreak() const
{
	// Still going if the latest solve was today or yesterday
	if (Data.LastSolvedDate <= 0 || !IsValidDate(Data.LastSolvedDate))
	{
		return 0;
	}
	return DaysBetween(Data.LastSolvedDate, GetToday()) <= 1 ? Data.Streak : 0;
}

bool UDailySubsystem::StartDaily(const UObject* WorldContextObject, int32& Number, int32& Size, int32& Difficulty)
{
	const FDailyPuzzleSpec Spec = GetTodaySpec();
	Number = Spec.Seed;
	Size = Spec.Size;
	Difficulty = Spec.Difficulty;

	UDailySubsystem* Daily = Get(WorldContextObject);
	if (!Daily || Daily->IsTodaySolved())
	{
		return false;
	}
	Daily->Data.bActive = true;
	Daily->Data.ActiveDate = Spec.Date;
	Daily->Save();
	return true;
}

void UDailySubsystem::StopDaily(const UObject* WorldContextObject)
{
	UDailySubsystem* Daily = Get(WorldContextObject);
	if (Daily && Daily->Data.bActive)
	{
		Daily->Data.bActive = false;
		Daily->Save();
	}
}

bool UDailySubsystem::IsPlayingDaily(const UObject* WorldContextObject)
{
	const UDailySubsystem* Daily = Get(WorldContextObject);
	return Daily && Daily->Data.bActive;
}

bool UDailySubsystem::GetActiveSpec(FDailyPuzzleSpec& OutSpec) const
{
	if (!Data.bActive || !IsValidDate(Data.ActiveDate))
	{
		return false;
	}
	OutSpec = GetSpec(Data.ActiveDate);
	return true;
}

bool UDailySubsystem::IsActiveDailyPuzzle(const UPuzzle* Puzzle) const
{
	FDailyPuzzleSpec Spec;
	return Puzzle && GetActiveSpec(Spec) && Puzzle->m_iSeed == Spec.Seed && Puzzle->m_iSize == Spec.Size;
}

void UDailySubsystem::ShowDailyEndScreen(UUserWidget* EndScreen)
{
	if (!EndScreen)
	{
		return;
	}
	const FFreePlayScore Score = UFreePlaySubsystem::GetLastFreePlayScore(EndScreen);

	// One daily puzzle a day: no restarting it or moving on to another from here
	for (const TCHAR* Name : { TEXT("Button_RestartPuzzle"), TEXT("Button_NextPuzzle") })
	{
		if (UWidget* Button = EndScreen->GetWidgetFromName(Name))
		{
			Button->SetVisibility(Score.bDaily ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		}
	}

	// The bonus line appears in the end screen's doubling step, after Total EXP
	if (UTextBlock* Bonus = Cast<UTextBlock>(EndScreen->GetWidgetFromName(TEXT("DailyBonusText"))))
	{
		Bonus->SetText(FText::Format(NSLOCTEXT("Daily", "Bonus", "x{0} Daily Bonus!"), ExpMultiplier));
		Bonus->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (UFreePlayEndScreenWidget* FreePlayScreen = Cast<UFreePlayEndScreenWidget>(EndScreen))
	{
		FreePlayScreen->StartDailyBonus(Score.bDailyBonus ? ExpMultiplier : 1);
	}
	if (UTextBlock* Streak = Cast<UTextBlock>(EndScreen->GetWidgetFromName(TEXT("DailyStreakText"))))
	{
		Streak->SetText(FText::Format(NSLOCTEXT("Daily", "Streak", "Daily streak: {0} {0}|plural(one=day,other=days)"),
			Score.DailyStreak));
		Streak->SetVisibility(Score.bDaily && Score.DailyStreak > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

bool UDailySubsystem::RecordSolve(UPuzzle* Puzzle)
{
	FDailyPuzzleSpec Spec;
	if (!IsActiveDailyPuzzle(Puzzle) || !GetActiveSpec(Spec))
	{
		return false;
	}

	Data.bActive = false;
	const bool bFirstSolve = Spec.Date > Data.LastSolvedDate;
	if (bFirstSolve)
	{
		// The day after the last solve carries the streak on; a gap starts it again
		const bool bConsecutive = Data.LastSolvedDate > 0 && IsValidDate(Data.LastSolvedDate)
			&& DaysBetween(Data.LastSolvedDate, Spec.Date) == 1;
		Data.Streak = bConsecutive ? Data.Streak + 1 : 1;
		Data.BestStreak = FMath::Max(Data.BestStreak, Data.Streak);
		Data.LastSolvedDate = Spec.Date;
	}
	Save();
	return bFirstSolve;
}

void UDailySubsystem::DebugReset(bool bAll)
{
	Data.bActive = false;
	if (bAll)
	{
		Data.LastSolvedDate = 0;
		Data.Streak = 0;
		Data.BestStreak = 0;
	}
	else if (Data.LastSolvedDate >= GetToday())
	{
		// Back to the day before: the streak without today (a streak only ever counts consecutive days)
		Data.Streak = FMath::Max(Data.Streak - 1, 0);
		Data.LastSolvedDate = Data.Streak > 0 ? ToDate(FromDate(GetToday()) - FTimespan::FromDays(1)) : 0;
	}
	Save();
}

UDailySubsystem* UDailySubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UDailySubsystem>() : nullptr;
}

void UDailySubsystem::Save()
{
	UHappinessSaveGame::SaveCurrentSettings();
}

#if !UE_BUILD_SHIPPING
// Testing: "Happiness.ResetDaily" makes today's daily puzzle playable again; "Happiness.ResetDaily all" also
// forgets every earlier solve and the streaks. Reopen the mode screen to see the change.
static FAutoConsoleCommandWithWorldAndArgs GResetDailyCommand(
	TEXT("Happiness.ResetDaily"),
	TEXT("Undo today's daily puzzle solve (streak back to before it); 'all' wipes the streaks too"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (UDailySubsystem* Daily = UDailySubsystem::Get(World))
		{
			Daily->DebugReset(Args.Num() > 0 && Args[0].Equals(TEXT("all"), ESearchCase::IgnoreCase));
		}
	}));
#endif
