#include "PuzzleCommandlet.h"
#include "HappinessClassic/Puzzle.h"
#include "HappinessClassic/Clue.h"
#include "HappinessClassic/Hint.h"
#include "HappinessClassic/CampaignTree.h"
#include "HappinessClassic/CampaignProgress.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/OutputDevice.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	// Counts error-level log lines (e.g. "SetFinalIcon conflict") emitted by the puzzle core
	struct FErrorCounter : public FOutputDevice
	{
		int32 Count = 0;
		FString First;

		virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if ((Verbosity & ELogVerbosity::VerbosityMask) <= ELogVerbosity::Error)
			{
				if (Count++ == 0)
					First = V;
			}
		}
	};

	struct FSolveResult
	{
		bool bSolved = false;
		int32 Steps = 0;
		FString Failure;
	};

	FString CellString(const FPuzzleCell& Cell)
	{
		if (Cell.m_iFinalIcon >= 0)
			return FString::FromInt(Cell.m_iFinalIcon);

		FString S = TEXT("{");
		for (int i = 0; i < Cell.m_bValues.Num(); i++)
		{
			if (Cell.m_bValues[i])
				S += FString::FromInt(i);
		}
		return S + TEXT("}");
	}

	FString StateString(UPuzzle& P)
	{
		FString S;
		for (int r = 0; r < P.m_iSize; r++)
		{
			S += FString::Printf(TEXT("  r%d:"), r);
			for (int c = 0; c < P.m_iSize; c++)
				S += TEXT(" ") + CellString(P.m_Rows[r].m_Cells[c]);
			S += TEXT("\n");
		}
		return S;
	}

	FString SolutionString(UPuzzle& P)
	{
		FString S;
		for (int r = 0; r < P.m_iSize; r++)
		{
			S += FString::Printf(TEXT("  r%d:"), r);
			for (int c = 0; c < P.m_iSize; c++)
				S += FString::Printf(TEXT(" %d"), P.SolutionIcon(r, c));
			S += TEXT("\n");
		}
		return S;
	}

	// Returns an empty string if the board agrees with the solution, otherwise a description of the first conflict
	FString CheckConsistent(UPuzzle& P)
	{
		for (int r = 0; r < P.m_iSize; r++)
		{
			for (int c = 0; c < P.m_iSize; c++)
			{
				const FPuzzleCell& Cell = P.m_Rows[r].m_Cells[c];
				int Sol = P.SolutionIcon(r, c);
				if (Cell.m_iFinalIcon >= 0 && Cell.m_iFinalIcon != Sol)
					return FString::Printf(TEXT("r%dc%d final=%d sol=%d"), r, c, Cell.m_iFinalIcon, Sol);
				if (!Cell.m_bValues[Sol])
					return FString::Printf(TEXT("r%dc%d sol=%d eliminated"), r, c, Sol);
			}
		}
		return FString();
	}

	// Repeatedly applies every clue's full analysis until solved or no progress
	FSolveResult AnalyzeSolve(UPuzzle& P)
	{
		FSolveResult Res;
		P.Reset();

		FString Prev = StateString(P);
		while (!P.IsSolved())
		{
			for (UClue* C : P.m_Clues)
				C->Analyze(P);
			Res.Steps++;

			FString Bad = CheckConsistent(P);
			if (!Bad.IsEmpty())
			{
				Res.Failure = TEXT("wrong: ") + Bad;
				return Res;
			}

			FString Cur = StateString(P);
			if (Cur == Prev)
			{
				Res.Failure = TEXT("stuck");
				return Res;
			}
			Prev = MoveTemp(Cur);
		}

		Res.bSolved = true;
		return Res;
	}

	// Solves the way a player relying on hints would: ask for a hint, apply it, repeat
	FSolveResult HintSolve(UPuzzle& P, FString* Trace, int32* HintsByLesson)
	{
		FSolveResult Res;
		P.Reset();

		const int MaxSteps = P.m_iSize * P.m_iSize * P.m_iSize * 2;
		while (!P.IsSolved())
		{
			if (Res.Steps >= MaxSteps)
			{
				Res.Failure = TEXT("step limit");
				return Res;
			}

			UHint* H = P.GenerateHint(P.m_Clues);
			if (!H)
			{
				// List clues whose Analyze still makes progress, i.e. GetHintAction couldn't express it
				Res.Failure = TEXT("no hint; progress by:");
				const FString Before = StateString(P);
				for (int i = 0; i < P.m_Clues.Num(); i++)
				{
					UClue* C = P.m_Clues[i];
					if (C->m_Type == eClueType::Given)
						continue;
					TArray<FPuzzleRow> Saved = P.m_Rows;
					C->Analyze(P);
					if (StateString(P) != Before)
						Res.Failure += FString::Printf(TEXT(" C%d[%s]"), i, *C->ToString());
					P.m_Rows = Saved;
				}
				return Res;
			}

			int ClueIdx = P.m_Clues.IndexOfByKey(H->TheClue);
			if (H->Row < 0 || H->Row >= P.m_iSize || H->Col < 0 || H->Col >= P.m_iSize || H->Icon < 0 || H->Icon >= P.m_iSize)
			{
				Res.Failure = FString::Printf(TEXT("invalid hint #%d C%d (%s) %s r%dc%d icon=%d"), Res.Steps, ClueIdx,
					H->TheClue ? *H->TheClue->ToString() : TEXT("null"), H->bSetFinalIcon ? TEXT("set") : TEXT("elim"), H->Row, H->Col, H->Icon);
				return Res;
			}

			int Sol = P.SolutionIcon(H->Row, H->Col);
			FString Desc = FString::Printf(TEXT("#%d C%d %s r%dc%d%s%d"), Res.Steps, ClueIdx,
				H->bSetFinalIcon ? TEXT("set") : TEXT("elim"), H->Row, H->Col, H->bSetFinalIcon ? TEXT("=") : TEXT("-"), H->Icon);

			if (Trace)
				*Trace += TEXT("  ") + Desc + TEXT("\n");

			if (H->ShouldHide(P))
			{
				Res.Failure = TEXT("noop hint ") + Desc;
				return Res;
			}
			if (H->bSetFinalIcon ? (H->Icon != Sol) : (H->Icon == Sol))
			{
				Res.Failure = FString::Printf(TEXT("bad hint %s (sol=%d)"), *Desc, Sol);
				return Res;
			}

			if (H->bSetFinalIcon)
				P.SetFinalIcon(H->Row, H->Col, H->Icon);
			else
				P.EliminateIcon(H->Row, H->Col, H->Icon);
			Res.Steps++;
			HintsByLesson[(int32)H->TheClue->GetCampaignLesson()]++;

			FString Bad = CheckConsistent(P);
			if (!Bad.IsEmpty())
			{
				Res.Failure = FString::Printf(TEXT("wrong after %s: %s"), *Desc, *Bad);
				return Res;
			}
		}

		Res.bSolved = true;
		return Res;
	}
}

UPuzzleCommandlet::UPuzzleCommandlet()
{
	LogToConsole = false;
	ShowErrorCount = false;
}

int32 UPuzzleCommandlet::Main(const FString& Params)
{
	const TCHAR* Cmd = *Params;

	int32 SeedMin = 1, SeedMax = 1, Size = 6, Diff = 1;
	FString SeedRange, Mode = TEXT("batch");
	FString OutPath = FPaths::ProjectSavedDir() / TEXT("PuzzleCLI.txt");

	if (FParse::Value(Cmd, TEXT("seeds="), SeedRange))
	{
		FString A, B;
		if (SeedRange.Split(TEXT("-"), &A, &B))
		{
			SeedMin = FCString::Atoi(*A);
			SeedMax = FCString::Atoi(*B);
		}
		else
		{
			SeedMin = SeedMax = FCString::Atoi(*SeedRange);
		}
	}
	else if (FParse::Value(Cmd, TEXT("seed="), SeedMin))
	{
		SeedMax = SeedMin;
	}
	FParse::Value(Cmd, TEXT("size="), Size);
	FParse::Value(Cmd, TEXT("diff="), Diff);
	FParse::Value(Cmd, TEXT("mode="), Mode);
	FParse::Value(Cmd, TEXT("out="), OutPath);
	const bool bAll = FParse::Param(Cmd, TEXT("all"));
	const bool bNoAuto = FParse::Param(Cmd, TEXT("noauto"));
	const bool bShow = Mode == TEXT("show") || Mode == TEXT("trace");
	const bool bTrace = Mode == TEXT("trace");

	// -lesson=Name generates campaign puzzles for that lesson
	FString LessonName;
	int64 LessonValue = INDEX_NONE;
	if (FParse::Value(Cmd, TEXT("lesson="), LessonName) && LessonName == TEXT("all"))
	{
		// Run every lesson in this one process (engine startup dominates), one summary line per lesson
		const UEnum* LessonEnum = StaticEnum<ECampaignLesson>();
		const FString TempOut = FPaths::ProjectSavedDir() / TEXT("PuzzleCLI_lesson.txt");
		FString AllOut;
		for (int32 i = 0; i < LessonEnum->NumEnums() - 1; i++)
		{
			const ECampaignLesson ThisLesson = (ECampaignLesson)LessonEnum->GetValueByIndex(i);
			if (ThisLesson == ECampaignLesson::Given)
				continue;	// Not a playable lesson

			const FString Name = LessonEnum->GetNameStringByIndex(i);
			if (!UPuzzle::IsLessonAvailable(ThisLesson, Size))
			{
				AllOut += FString::Printf(TEXT("%-15s not available at size %d\n"), *Name, Size);
				continue;
			}
			FString SubParams = Params.Replace(TEXT("-lesson=all"), *(TEXT("-lesson=") + Name));
			SubParams = SubParams.Replace(*(TEXT("-out=") + OutPath), TEXT("")) + TEXT(" -out=") + TempOut;
			Main(SubParams);

			FString Result;
			FFileHelper::LoadFileToString(Result, *TempOut);
			FString Summary, Campaign;
			TArray<FString> Lines;
			Result.ParseIntoArrayLines(Lines);
			for (const FString& Line : Lines)
			{
				if (Line.StartsWith(TEXT("SUMMARY")))
					Summary = Line;
				else if (Line.StartsWith(TEXT("CAMPAIGN")))
					Campaign = Line;
			}
			AllOut += FString::Printf(TEXT("%-15s %s | %s\n"), *Name, *Summary.Replace(TEXT("SUMMARY "), TEXT("")), *Campaign.Replace(TEXT("CAMPAIGN "), TEXT("")));
		}
		FFileHelper::SaveStringToFile(AllOut, *OutPath);
		return 0;
	}
	else if (!LessonName.IsEmpty())
	{
		LessonValue = StaticEnum<ECampaignLesson>()->GetValueByNameString(LessonName);
		if (LessonValue == INDEX_NONE)
		{
			FFileHelper::SaveStringToFile(FString::Printf(TEXT("Unknown lesson '%s'\n"), *LessonName), *OutPath);
			return 1;
		}
	}
	const bool bCampaign = LessonValue != INDEX_NONE;

	// -stage=N (with -lesson): generate exactly what the campaign plays for that stage: its size, difficulty and seed mapping
	int32 Stage = -1;
	if (bCampaign && FParse::Value(Cmd, TEXT("stage="), Stage))
	{
		Size = UCampaignSubsystem::GetStageSize(Stage);
		Diff = UCampaignSubsystem::GetStageDifficulty(Stage);
	}
	const ECampaignLesson Lesson = bCampaign ? (ECampaignLesson)LessonValue : ECampaignLesson::Given;
	int32 CampaignFails = 0;

	// Free play clue selection (without -lesson): -only=A,B keeps just those clue types, -exclude=A,B drops them
	int32 ExcludedClues = 0;
	FString OnlyList, ExcludeList;
	const bool bOnly = FParse::Value(Cmd, TEXT("only="), OnlyList, false);
	if (bOnly || FParse::Value(Cmd, TEXT("exclude="), ExcludeList, false))
	{
		TArray<FString> Names;
		(bOnly ? OnlyList : ExcludeList).ParseIntoArray(Names, TEXT(","));
		int32 Listed = 0;
		for (const FString& Name : Names)
		{
			const int64 Value = StaticEnum<ECampaignLesson>()->GetValueByNameString(Name.TrimStartAndEnd());
			if (Value == INDEX_NONE)
			{
				FFileHelper::SaveStringToFile(FString::Printf(TEXT("Unknown clue type '%s'\n"), *Name), *OutPath);
				return 1;
			}
			Listed |= 1 << int32(Value);
		}
		ExcludedClues = bOnly ? ~Listed : Listed;
		ExcludedClues &= ~(1 << int32(ECampaignLesson::Given));
	}

	if (bCampaign && !UPuzzle::IsLessonAvailable(Lesson, Size))
	{
		FFileHelper::SaveStringToFile(FString::Printf(TEXT("Lesson %s is not available at size %d\n"), *LessonName, Size), *OutPath);
		return 0;
	}

	// The puzzle core logs every generated clue to LogTemp; keep only errors
	LogTemp.SetVerbosity(ELogVerbosity::Error);

	FErrorCounter Errors;
	GLog->AddOutputDevice(&Errors);

	FString Out = FString::Printf(TEXT("size=%d diff=%d seeds=%d-%d mode=%s%s\n"), Size, Diff, SeedMin, SeedMax, *Mode,
		bNoAuto ? TEXT(" noauto") : TEXT(""));

	int32 Total = 0, AnalyzeFails = 0, HintFails = 0, ErrorPuzzles = 0;
	int32 CluesByLesson[256] = {}, HintsByLesson[256] = {}, NotHereClues = 0;
	int32 GivenCounts[16] = {};
	int64 ClueSum = 0, HintStepSum = 0;
	double GenTime = 0.0;
	const double StartTime = FPlatformTime::Seconds();

	for (int32 Seed = SeedMin; Seed <= SeedMax; Seed++)
	{
		Errors.Count = 0;
		Errors.First.Empty();

		TStrongObjectPtr<UPuzzle> Puzzle(NewObject<UPuzzle>(GetTransientPackage()));
		UPuzzle& P = *Puzzle;

		double T0 = FPlatformTime::Seconds();
		if (bCampaign)
		{
			if (!P.InitCampaign(Stage >= 0 ? UCampaignSubsystem::GetGenerationSeed(Lesson, Stage, Seed) : Seed, Size, Diff, Lesson))
				CampaignFails++;
		}
		else
		{
			P.m_ExcludedClues = ExcludedClues;
			P.Init(Seed, Size, Diff);
		}
		double GenMs = (FPlatformTime::Seconds() - T0) * 1000.0;
		GenTime += GenMs;
		const int32 GenErrors = Errors.Count;

		if (bCampaign)
		{
			// No clue may come from a later lesson, and the puzzle must need the lesson's clues
			for (UClue* C : P.m_Clues)
			{
				if (!UCampaignTree::IsClueLessonAllowed(C->GetCampaignLesson(), Lesson))
					UE_LOG(LogTemp, Error, TEXT("Clue above lesson: %s"), *C->ToString());
			}
			if (!P.RequiresLesson(Lesson))
				UE_LOG(LogTemp, Error, TEXT("Puzzle does not require lesson"));
		}
		else if (ExcludedClues != 0)
		{
			// No excluded clue type may appear
			for (UClue* C : P.m_Clues)
			{
				if (P.IsClueExcluded(C->GetCampaignLesson()))
					UE_LOG(LogTemp, Error, TEXT("Excluded clue type: %s"), *C->ToString());
			}
		}

		FSolveResult A = AnalyzeSolve(P);

		P.AutoSetIcons = !bNoAuto;
		FString Trace;
		FSolveResult H = HintSolve(P, bTrace ? &Trace : nullptr, HintsByLesson);
		P.AutoSetIcons = true;

		Total++;
		ClueSum += P.m_Clues.Num();
		int32 MinGivens, MaxGivens;
		UPuzzle::GetGivenRangeForDifficulty(Size, Diff, MinGivens, MaxGivens);
		const int32 NumGivens = P.GetNumGivenClues();
		GivenCounts[FMath::Clamp(NumGivens, 0, 15)]++;
		if (NumGivens < MinGivens || NumGivens > MaxGivens)
			UE_LOG(LogTemp, Error, TEXT("Given count %d, expected %d-%d"), NumGivens, MinGivens, MaxGivens);

		for (UClue* C : P.m_Clues)
		{
			// Saved games store clues by id; every clue must survive the round trip
			UClue* Copy = NewObject<UClue>(GetTransientPackage());
			Copy->SetFromId(C->GetId());
			if (!Copy->IsSameClue(*C))
				UE_LOG(LogTemp, Error, TEXT("Id round trip failed: %s -> %s"), *C->ToString(), *Copy->ToString());

			CluesByLesson[(int32)C->GetCampaignLesson()]++;
			NotHereClues += C->m_Type == eClueType::NotHere ? 1 : 0;
		}
		HintStepSum += H.Steps;
		AnalyzeFails += A.bSolved ? 0 : 1;
		HintFails += H.bSolved ? 0 : 1;
		ErrorPuzzles += Errors.Count > 0 ? 1 : 0;

		const bool bFailed = !A.bSolved || !H.bSolved || Errors.Count > 0;
		if (bShow || bAll || bFailed)
		{
			Out += FString::Printf(TEXT("seed=%d clues=%d(g%d v%d h%d) gen=%.0fms analyze=%s/%d hint=%s/%d errs=%d(gen %d)"),
				Seed, P.m_Clues.Num(), P.m_GivenClues.Num(), P.m_VeritcalClues.Num(), P.m_HorizontalClues.Num(), GenMs,
				A.bSolved ? TEXT("OK") : TEXT("FAIL"), A.Steps, H.bSolved ? TEXT("OK") : TEXT("FAIL"), H.Steps, Errors.Count, GenErrors);
			if (!A.bSolved)
				Out += TEXT(" | analyze: ") + A.Failure;
			if (!H.bSolved)
				Out += TEXT(" | hint: ") + H.Failure;
			if (Errors.Count > 0)
				Out += TEXT(" | firstErr: ") + Errors.First;
			Out += TEXT("\n");
		}

		if (bShow)
		{
			Out += TEXT(" solution:\n") + SolutionString(P);
			Out += TEXT(" clues:\n");
			for (int i = 0; i < P.m_Clues.Num(); i++)
			{
				const UClue* C = P.m_Clues[i];
				const FString Column = C->m_Type == eClueType::Vertical ? FString::Printf(TEXT(" [col %d]"), C->m_iCol) : FString();
				Out += FString::Printf(TEXT("  C%d %s%s\n"), i, *C->ToString(), *Column);
			}
			if (!H.bSolved)
				Out += TEXT(" board at hint failure:\n") + StateString(P);
			if (bTrace)
				Out += TEXT(" hints:\n") + Trace;
		}

		Puzzle.Reset();
		if (Total % 200 == 0)
			CollectGarbage(RF_NoFlags);
	}

	GLog->RemoveOutputDevice(&Errors);

	Out += FString::Printf(TEXT("SUMMARY n=%d analyzeFail=%d hintFail=%d withErrors=%d avgClues=%.1f avgHintSteps=%.1f avgGen=%.1fms total=%.1fs\n"),
		Total, AnalyzeFails, HintFails, ErrorPuzzles, Total ? (double)ClueSum / Total : 0.0, Total ? (double)HintStepSum / Total : 0.0,
		Total ? GenTime / Total : 0.0, FPlatformTime::Seconds() - StartTime);

	// Per campaign lesson: clues in the final puzzles / hint steps that used that lesson's clues
	Out += TEXT("LESSONS clues/hints:");
	const UEnum* LessonEnum = StaticEnum<ECampaignLesson>();
	for (int32 i = 0; i < LessonEnum->NumEnums() - 1; i++)
	{
		int32 Value = (int32)LessonEnum->GetValueByIndex(i);
		if (CluesByLesson[Value] || HintsByLesson[Value])
			Out += FString::Printf(TEXT(" %s=%d/%d"), *LessonEnum->GetNameStringByIndex(i), CluesByLesson[Value], HintsByLesson[Value]);
	}
	Out += FString::Printf(TEXT(" (NotHere clues=%d)\n"), NotHereClues);

	// How many puzzles got each number of givens
	if (bCampaign)
		Out += FString::Printf(TEXT("CAMPAIGN lesson=%s failedToRequireLesson=%d\n"), *LessonName, CampaignFails);

	Out += TEXT("GIVENS count:puzzles");
	for (int32 i = 0; i < 16; i++)
	{
		if (GivenCounts[i])
			Out += FString::Printf(TEXT(" %d:%d"), i, GivenCounts[i]);
	}
	Out += TEXT("\n");

	FFileHelper::SaveStringToFile(Out, *OutPath);
	return 0;
}
