#include "DailyPuzzle.h"
#include "CampaignProgress.h"
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

FDailyPuzzleSpec UDailySubsystem::GetSpec(int32 Date, int32 LockedClueMask)
{
	// Everything comes from the date, in this order, so a date always gives the same puzzle
	FRandomStream Rand(Date);
	FDailyPuzzleSpec Spec;
	Spec.Date = Date;

	// A random set of at least 5 clue types, drawn from all of them so the size, difficulty and seed below don't
	// depend on what the player has unlocked
	TArray<ECampaignLesson> Clues = UFreePlaySubsystem::GetSelectableClues();
	for (int32 i = Clues.Num() - 1; i > 0; i--)
	{
		Clues.Swap(i, Rand.RandRange(0, i));
	}
	const int32 Count = Rand.RandRange(FMath::Min(5, Clues.Num()), Clues.Num());

	// Locked ones drop out, and the next unlocked ones in the shuffled order stand in for them up to 5
	auto IsLocked = [LockedClueMask](ECampaignLesson Clue) { return (LockedClueMask & (1 << int32(Clue))) != 0; };
	int32 Chosen = 0;
	for (int32 i = 0; i < Count; i++)
	{
		Chosen += IsLocked(Clues[i]) ? 0 : 1;
	}
	const TArray<ECampaignLesson> Unlocked = Clues.FilterByPredicate([&IsLocked](ECampaignLesson Clue) { return !IsLocked(Clue); });
	Chosen = FMath::Max(Chosen, FMath::Min(5, Unlocked.Num()));
	Spec.Clues = TArray<ECampaignLesson>(Unlocked.GetData(), Chosen);
	for (ECampaignLesson Clue : Clues)
	{
		if (!Spec.Clues.Contains(Clue))
		{
			Spec.ExcludedClueMask |= 1 << int32(Clue);
		}
	}

	// 4x4 to 7x7, mostly 5x5 and 6x6
	const float SizeRoll = Rand.FRand();
	Spec.Size = SizeRoll < 0.15f ? 4 : SizeRoll < 0.5f ? 5 : SizeRoll < 0.85f ? 6 : 7;
	Spec.Difficulty = Rand.RandRange(0, 2);
	Spec.Seed = Rand.RandRange(1, 1 << 30);
	return Spec;
}

FDailyPuzzleSpec UDailySubsystem::GetTodaySpec()
{
	const int32 Today = GetToday();
	const UDailySubsystem* Daily = GetInstance();
	return GetSpec(Today, Daily ? Daily->GetLockedCluesFor(Today) : GetLockedClueMask());
}

int32 UDailySubsystem::GetLockedClueMask()
{
	// Every lesson is unlocked once Campaign mode is (IsLessonUnlocked looks at the current mode's progress)
	const UCampaignSubsystem* Campaign = UCampaignSubsystem::GetInstance();
	if (!Campaign || Campaign->IsCampaignModeUnlocked())
	{
		return 0;
	}
	int32 Mask = 0;
	for (ECampaignLesson Clue : UFreePlaySubsystem::GetSelectableClues())
	{
		if (!Campaign->IsLessonUnlocked(Clue))
		{
			Mask |= 1 << int32(Clue);
		}
	}
	return Mask;
}

int32 UDailySubsystem::GetLockedCluesFor(int32 Date) const
{
	return Data.ActiveDate == Date ? Data.ActiveLockedClues : GetLockedClueMask();
}

UDailySubsystem::FStreakState UDailySubsystem::GetStreakState(int32 Date) const
{
	FStreakState State;
	State.SaverDays = Data.SaverDays;
	if (Data.Streak <= 0 || Data.LastSolvedDate <= 0 || !IsValidDate(Data.LastSolvedDate))
	{
		return State;
	}
	State.LastDay = Data.CoveredThrough > Data.LastSolvedDate && IsValidDate(Data.CoveredThrough) ? Data.CoveredThrough : Data.LastSolvedDate;

	// The days between the streak's last day and Date; Date itself can still be played
	const int32 Missed = DaysBetween(State.LastDay, Date) - 1;
	if (Missed <= 0)
	{
		return State;
	}
	if (Missed <= State.SaverDays)
	{
		State.LastDay = ToDate(FromDate(Date) - FTimespan::FromDays(1));
		State.SaverDays -= Missed;
	}
	else
	{
		State.MissedDays = Missed;
	}
	return State;
}

void UDailySubsystem::ApplySavers(int32 Date)
{
	const FStreakState State = GetStreakState(Date);
	if (State.MissedDays == 0 && State.LastDay > Data.LastSolvedDate && State.LastDay > Data.CoveredThrough)
	{
		Data.CoveredThrough = State.LastDay;
		Data.SaverDays = State.SaverDays;
	}
}

int32 UDailySubsystem::GetStreak() const
{
	// Still going if its last day (solved, or covered by streak savers) was today or yesterday
	const FStreakState State = GetStreakState(GetToday());
	return State.LastDay > 0 && State.MissedDays == 0 ? Data.Streak : 0;
}

int32 UDailySubsystem::GetSaverDays() const
{
	return GetStreakState(GetToday()).SaverDays;
}

bool UDailySubsystem::GetBrokenStreak(int32& OutStreak, int32& OutMissedDays, int32& OutSaverDaysNeeded) const
{
	const FStreakState State = GetStreakState(GetToday());
	OutStreak = State.MissedDays > 0 ? Data.Streak : 0;
	OutMissedDays = State.MissedDays;
	OutSaverDaysNeeded = FMath::Max(State.MissedDays - State.SaverDays, 0);
	return State.MissedDays > 0;
}

void UDailySubsystem::AddSaverDays(int32 Days, const FString& TransactionId)
{
	if (Days <= 0 || (!TransactionId.IsEmpty() && HasSaverTransaction(TransactionId)))
	{
		return;
	}
	if (!TransactionId.IsEmpty())
	{
		Data.SaverTransactions.Add(TransactionId);
	}
	Data.SaverDays += Days;
	ApplySavers(GetToday());
	Save();
	OnChanged.Broadcast();
}

void UDailySubsystem::DebugSetSaverDays(int32 Days)
{
	Data.SaverDays = FMath::Max(Days, 0);
	Save();
	OnChanged.Broadcast();
}

bool UDailySubsystem::StartDaily(const UObject* WorldContextObject, int32& Number, int32& Size, int32& Difficulty)
{
	const FDailyPuzzleSpec Spec = GetSpec(GetToday());
	Number = Spec.Seed;
	Size = Spec.Size;
	Difficulty = Spec.Difficulty;

	UDailySubsystem* Daily = Get(WorldContextObject);
	if (!Daily || Daily->IsTodaySolved())
	{
		return false;
	}
	// The clue types it leaves out are fixed the first time the date's puzzle starts
	Daily->Data.ActiveLockedClues = Daily->GetLockedCluesFor(Spec.Date);
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
	OutSpec = GetSpec(Data.ActiveDate, Data.ActiveLockedClues);
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
		// The day after the streak's last day (solved, or covered by streak savers) carries it on; a gap starts it
		// again, and the savers left wait to protect the new one
		ApplySavers(Spec.Date);
		const FStreakState State = GetStreakState(Spec.Date);
		const bool bConsecutive = State.LastDay > 0 && DaysBetween(State.LastDay, Spec.Date) == 1;
		Data.Streak = bConsecutive ? Data.Streak + 1 : 1;
		Data.BestStreak = FMath::Max(Data.BestStreak, Data.Streak);
		Data.LastSolvedDate = Spec.Date;
		Data.CoveredThrough = 0;
	}
	Save();
	OnChanged.Broadcast();
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
		Data.SaverDays = 0;
		Data.CoveredThrough = 0;
		Data.SaverTransactions.Reset();
	}
	else if (Data.LastSolvedDate >= GetToday())
	{
		// Back to the day before: the streak without today (a streak only ever counts consecutive days)
		Data.Streak = FMath::Max(Data.Streak - 1, 0);
		Data.LastSolvedDate = Data.Streak > 0 ? ToDate(FromDate(GetToday()) - FTimespan::FromDays(1)) : 0;
	}
	Save();
	OnChanged.Broadcast();
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
// Testing: "Happiness.StreakSaver.Set <days>" sets the unused streak saver days (no purchase). With Happiness.DailyDate
// set to a later date (days missed) it shows recovery and protection. Reopen the mode screen to see the change.
static FAutoConsoleCommandWithWorldAndArgs GSetSaverDaysCommand(
	TEXT("Happiness.StreakSaver.Set"),
	TEXT("Happiness.StreakSaver.Set <days>: the unused streak saver days, without a purchase"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		UDailySubsystem* Daily = UDailySubsystem::Get(World);
		if (Daily && Args.Num() > 0 && Args[0].IsNumeric())
		{
			Daily->DebugSetSaverDays(FCString::Atoi(*Args[0]));
		}
	}));

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
