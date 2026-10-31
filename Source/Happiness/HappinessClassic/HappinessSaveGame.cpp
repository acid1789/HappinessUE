#include "HappinessSaveGame.h"

#include "Puzzle.h"
#include "Ads/AdsSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

bool UHappinessSaveGame::bOldSaveChecked = false;
bool UHappinessSaveGame::bOldSaveReset = false;
TWeakObjectPtr<UHappinessSaveGame> UHappinessSaveGame::LastLoaded;

namespace
{
	UObject* GetSaveObjectProperty(const UObject* Owner, const TCHAR* Name)
	{
		const FObjectPropertyBase* Property = Owner ? FindFProperty<FObjectPropertyBase>(Owner->GetClass(), Name) : nullptr;
		return Property ? Property->GetObjectPropertyValue_InContainer(Owner) : nullptr;
	}

	/** WBP_Happiness's undo list */
	const FArrayProperty* FindActionList(const UObject* Screen)
	{
		return Screen ? FindFProperty<FArrayProperty>(Screen->GetClass(), TEXT("ActionList")) : nullptr;
	}

	/**
	 * The list of clues (its ScrollList) in one of WBP_Happiness's clue panels, found in its widget tree by class
	 * (WBP_HorizontalCluePanel, WBP_VerticalCluePanel)
	 */
	UPanelWidget* FindClueList(const UObject* Screen, const TCHAR* PanelClass)
	{
		const UUserWidget* ScreenWidget = Cast<UUserWidget>(Screen);
		UPanelWidget* List = nullptr;
		if (ScreenWidget && ScreenWidget->WidgetTree)
		{
			ScreenWidget->WidgetTree->ForEachWidget([&List, PanelClass](UWidget* Widget)
			{
				const UUserWidget* Panel = Cast<UUserWidget>(Widget);
				if (!List && Panel && Panel->GetClass()->GetName() == PanelClass)
				{
					List = Cast<UPanelWidget>(Panel->GetWidgetFromName(TEXT("ScrollList")));
				}
			});
		}
		if (!List)
		{
			UE_LOG(LogTemp, Warning, TEXT("Hidden clues: no %s clue list on the puzzle screen"), PanelClass);
		}
		return List;
	}

	/** SG_Happiness's Rows: the saved board */
	const TArray<FPuzzleRow>* GetSavedRows(const UHappinessSaveGame& Save)
	{
		const FArrayProperty* RowsProperty = FindFProperty<FArrayProperty>(Save.GetClass(), TEXT("Rows"));
		const FStructProperty* RowProperty = RowsProperty ? CastField<FStructProperty>(RowsProperty->Inner) : nullptr;
		return RowProperty && RowProperty->Struct == FPuzzleRow::StaticStruct() ? RowsProperty->ContainerPtrToValuePtr<TArray<FPuzzleRow>>(&Save) : nullptr;
	}

	bool SaveGameSameRows(const TArray<FPuzzleRow>& A, const TArray<FPuzzleRow>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}
		for (int32 Row = 0; Row < A.Num(); Row++)
		{
			if (A[Row].m_Cells.Num() != B[Row].m_Cells.Num())
			{
				return false;
			}
			for (int32 Col = 0; Col < A[Row].m_Cells.Num(); Col++)
			{
				const FPuzzleCell& CellA = A[Row].m_Cells[Col];
				const FPuzzleCell& CellB = B[Row].m_Cells[Col];
				if (CellA.m_iFinalIcon != CellB.m_iFinalIcon || CellA.m_bValues != CellB.m_bValues)
				{
					return false;
				}
			}
		}
		return true;
	}
}

void UHappinessSaveGame::CaptureUndo()
{
	UndoActions.Reset();
	UndoSolution.Reset();
	HiddenClues.Reset();
	const UCampaignSubsystem* CampaignSubsystem = UCampaignSubsystem::GetInstance();
	const UWorld* World = CampaignSubsystem && CampaignSubsystem->GetGameInstance() ? CampaignSubsystem->GetGameInstance()->GetWorld() : nullptr;
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const UObject* Screen = GetSaveObjectProperty(PC, TEXT("HappinessWidget"));
	const UPuzzle* Puzzle = Cast<UPuzzle>(GetSaveObjectProperty(PC, TEXT("HappinessPuzzle")));
	const FArrayProperty* Actions = FindActionList(Screen);
	if (!Puzzle || !Actions || GetSaveObjectProperty(Screen, TEXT("ThePuzzle")) != Puzzle)
	{
		return;
	}
	UndoSolution = Puzzle->m_Solution;
	if (FScriptArrayHelper(Actions, Actions->ContainerPtrToValuePtr<void>(Screen)).Num() > 0)
	{
		Actions->ExportText_InContainer(0, UndoActions, Screen, nullptr, nullptr, PPF_None);
	}
	Puzzle->GetHiddenClues(FindClueList(Screen, TEXT("WBP_HorizontalCluePanel_C")), FindClueList(Screen, TEXT("WBP_VerticalCluePanel_C")), HiddenClues);
}

void UHappinessSaveGame::RestoreUndo(UUserWidget* Screen)
{
	// The game save LoadHappiness just loaded (PC_Happiness keeps it only in a local variable). Only for the puzzle set
	// up right after the load: a later SetPuzzle (Restart Puzzle) starts afresh.
	UHappinessSaveGame* Save = LastLoaded.Get();
	LastLoaded = nullptr;
	UPuzzle* Puzzle = Cast<UPuzzle>(GetSaveObjectProperty(Screen, TEXT("ThePuzzle")));
	const FArrayProperty* Actions = FindActionList(Screen);
	if (!Save || !Puzzle || !Actions || Save->UndoSolution.IsEmpty() || Save->UndoSolution != Puzzle->m_Solution)
	{
		UE_LOG(LogTemp, Log, TEXT("RestoreUndo: not this puzzle's save (save %d, puzzle %d, undo list %d, saved solution %d)"),
			Save != nullptr, Puzzle != nullptr, Actions != nullptr, Save ? Save->UndoSolution.Num() : -1);
		return;
	}
	// The board as saved (SG_Happiness's Rows): the puzzle was just loaded from this save
	const TArray<FPuzzleRow>* SavedRows = GetSavedRows(*Save);
	if (!SavedRows || !SaveGameSameRows(*SavedRows, Puzzle->m_Rows))
	{
		UE_LOG(LogTemp, Log, TEXT("RestoreUndo: the board isn't the saved one"));
		return;
	}
	if (!Save->UndoActions.IsEmpty())
	{
		if (!Actions->ImportText_InContainer(*Save->UndoActions, Screen, Screen, PPF_None))
		{
			UE_LOG(LogTemp, Warning, TEXT("RestoreUndo: the saved undo list couldn't be read"));
		}
	}
	Puzzle->RestoreHiddenClues(FindClueList(Screen, TEXT("WBP_HorizontalCluePanel_C")), FindClueList(Screen, TEXT("WBP_VerticalCluePanel_C")), Save->HiddenClues);
	// Once: the next save takes them from the screen again
	Save->UndoActions.Reset();
	Save->UndoSolution.Reset();
	Save->HiddenClues.Reset();
}

bool UHappinessSaveGame::IsBoardSolved() const
{
	const TArray<FPuzzleRow>* Rows = GetSavedRows(*this);
	if (!Rows || Rows->IsEmpty() || UndoSolution.IsEmpty())
	{
		return false;
	}
	const int32 Size = Rows->Num();
	if (UndoSolution.Num() != Size * Size)
	{
		return false;
	}
	for (int32 Row = 0; Row < Size; Row++)
	{
		const TArray<FPuzzleCell>& Cells = (*Rows)[Row].m_Cells;
		if (Cells.Num() != Size)
		{
			return false;
		}
		for (int32 Col = 0; Col < Size; Col++)
		{
			if (Cells[Col].m_iFinalIcon != UndoSolution[Row * Size + Col])
			{
				return false;
			}
		}
	}
	return true;
}

void UHappinessSaveGame::Serialize(FArchive& Ar)
{
	// Writing a save game: take the current campaign progress, free play and daily settings, the ads switch and the icon set choices. Not for the class default object (saved with the
	// Blueprint asset) or for reference collection and the like.
	const bool bWritingSave = Ar.IsSaving() && !Ar.IsObjectReferenceCollector() && !Ar.IsCountingMemory() && !Ar.IsTransacting() &&
		!HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) && !UPuzzleSlotSubsystem::IsSlotIO();
	if (bWritingSave)
	{
		if (const UCampaignSubsystem* CampaignSubsystem = UCampaignSubsystem::GetInstance())
		{
			Campaign = CampaignSubsystem->GetSaveData();
		}
		if (const UFreePlaySubsystem* FreePlaySubsystem = UFreePlaySubsystem::GetInstance())
		{
			FreePlay = FreePlaySubsystem->GetSaveData();
		}
		if (const UDailySubsystem* DailySubsystem = UDailySubsystem::GetInstance())
		{
			Daily = DailySubsystem->GetSaveData();
		}
		if (const UAdsSubsystem* AdsSubsystem = UAdsSubsystem::GetInstance())
		{
			bAdsDisabled = AdsSubsystem->GetAdsDisabledSaveData();
		}
		if (const UIconSetSubsystem* IconSetSubsystem = UIconSetSubsystem::GetInstance())
		{
			IconSetChoices = IconSetSubsystem->GetSaveData();
		}
		DataVersion = CurrentDataVersion;
		if (!bLoadedFromDisk)
		{
			CaptureUndo();
			// The solving move is saved like any other (and the end screen saves again), but the puzzle is over: its
			// mode mustn't resume it, even if the game is left on the end screen
			const FBoolProperty* ActivePuzzle = FindFProperty<FBoolProperty>(GetClass(), TEXT("ActivePuzzle"));
			if (ActivePuzzle && IsBoardSolved())
			{
				ActivePuzzle->SetPropertyValue_InContainer(this, false);
				UndoActions.Reset();
				UndoSolution.Reset();
				HiddenClues.Reset();
			}
		}
	}

	Super::Serialize(Ar);

	if (bWritingSave && !bLoadedFromDisk)
	{
		// A new save of the game (PC_Happiness's SaveHappiness): its puzzle is the playing mode's
		if (UPuzzleSlotSubsystem* Slots = UPuzzleSlotSubsystem::GetInstance())
		{
			Slots->WriteSlot(*this);
		}
	}
	// Read from a save slot (UGameplayStatics::LoadGameFromSlot)
	if (Ar.IsLoading() && !Ar.IsObjectReferenceCollector() && !Ar.IsTransacting() &&
		!HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) && !UPuzzleSlotSubsystem::IsSlotIO())
	{
		bLoadedFromDisk = true;
		LastLoaded = this;
		if (UPuzzleSlotSubsystem* Slots = UPuzzleSlotSubsystem::GetInstance())
		{
			Slots->ApplyResume(*this);
		}
	}
}

void UHappinessSaveGame::ResetOldSave()
{
	if (bOldSaveChecked)
	{
		return;
	}
	bOldSaveChecked = true;

	TArray<uint8> Bytes;
	if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex) || !UGameplayStatics::LoadDataFromSlot(Bytes, SaveSlotName, SaveUserIndex))
	{
		return;
	}
	// Saved properties are tagged with their names, as strings (length with the terminator, then the characters), and
	// a property at its default isn't written. So a save with campaign progress has this tag, and one written by this
	// version or later has a DataVersion; the old game's save has neither.
	static const uint8 CampaignTag[] = {9, 0, 0, 0, 'C', 'a', 'm', 'p', 'a', 'i', 'g', 'n', 0};
	USaveGame* Old = UGameplayStatics::LoadGameFromMemory(Bytes);
	const UHappinessSaveGame* OldSave = Cast<UHappinessSaveGame>(Old);
	bool bHasCampaign = false;
	for (int32 At = 0; At + int32(sizeof(CampaignTag)) <= Bytes.Num() && !bHasCampaign; At++)
	{
		bHasCampaign = FMemory::Memcmp(&Bytes[At], CampaignTag, sizeof(CampaignTag)) == 0;
	}
	if (!Old || (OldSave && OldSave->DataVersion > 0) || bHasCampaign)
	{
		return;
	}

	// A fresh save with the level and experience (SG_Happiness's Level and Experience, read by PC_Happiness)
	USaveGame* Fresh = UGameplayStatics::CreateSaveGameObject(Old->GetClass());
	for (const TCHAR* Name : {TEXT("Level"), TEXT("Experience"), TEXT("SaveVersion")})
	{
		const FIntProperty* Property = FindFProperty<FIntProperty>(Old->GetClass(), Name);
		if (Property && Fresh)
		{
			Property->SetPropertyValue_InContainer(Fresh, Property->GetPropertyValue_InContainer(Old));
		}
	}
	UPuzzleSlotSubsystem::DeleteAllSlots();
	UGameplayStatics::DeleteGameInSlot(SaveSlotName, SaveUserIndex);
	if (Fresh)
	{
		UGameplayStatics::SaveGameToSlot(Fresh, SaveSlotName, SaveUserIndex);
	}
	bOldSaveReset = true;
	UE_LOG(LogTemp, Log, TEXT("Old game save replaced: the game starts over, keeping the player's level"));
}

UHappinessSaveGame* UHappinessSaveGame::LoadFromSlot()
{
	ResetOldSave();
	if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex))
	{
		return nullptr;
	}
	return Cast<UHappinessSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex));
}

void UHappinessSaveGame::SaveCurrentSettings()
{
	// Serialize fills in the current settings as the save is written
	if (UHappinessSaveGame* SaveGame = LoadFromSlot())
	{
		UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, SaveUserIndex);
	}
}

bool UHappinessSaveGame::HasActivePuzzle()
{
	return false;
}
