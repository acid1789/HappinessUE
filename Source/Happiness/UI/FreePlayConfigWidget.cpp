#include "UI/FreePlayConfigWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "HappinessClassic/FreePlaySettings.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

#define LOCTEXT_NAMESPACE "FreePlayConfig"

void UFreePlayConfigWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	BuildGardenLayout();
	RefreshSegments();
}

void UFreePlayConfigWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildGardenLayout();
	FitToMenuCanvas();
	if (RandomButton) RandomButton->OnClicked.AddUniqueDynamic(this, &UFreePlayConfigWidget::HandleRandomClicked);
}

void UFreePlayConfigWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	FitToMenuCanvas();
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshSegments();
}

void UFreePlayConfigWidget::HandleRandomClicked()
{
	UGameplayStatics::PlaySound2D(this, LoadObject<USoundBase>(nullptr, TEXT("/Game/General/Audio/Cues/MenuAccept.MenuAccept")));

	// Puzzle number
	if (UEditableTextBox* Number = Cast<UEditableTextBox>(GetWidgetFromName(TEXT("PuzzleNumberBox"))))
	{
		Number->SetText(FText::AsNumber(FMath::RandRange(1, FMath::Max(1, MaxRandomPuzzleNumber)), &FNumberFormattingOptions::DefaultNoGrouping()));
	}

	// Size and difficulty, of those the player can play
	auto PickOption = [this](const TCHAR* SelectionName, FName Check)
	{
		UUserWidget* Selection = Cast<UUserWidget>(GetWidgetFromName(SelectionName));
		TArray<int32> Playable;
		for (int32 Index = 0; Index < GetOptionCount(Selection); Index++)
		{
			if (CanPlay(Check, Index))
			{
				Playable.Add(Index);
			}
		}
		if (Playable.Num() > 0)
		{
			SelectOption(Selection, Playable[FMath::RandRange(0, Playable.Num() - 1)]);
		}
	};
	PickOption(TEXT("PuzzleSize"), TEXT("CanPlayHappinessSize"));
	PickOption(TEXT("PuzzleDifficulty"), TEXT("CanPlayHappinessDifficulty"));

	// Clue types: a random number of them (at least one), chosen at random. The clue selection follows the subsystem.
	if (UFreePlaySubsystem* FreePlay = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFreePlaySubsystem>() : nullptr)
	{
		TArray<ECampaignLesson> Clues = UFreePlaySubsystem::GetSelectableClues();
		if (Clues.Num() > 0)
		{
			for (int32 Index = Clues.Num() - 1; Index > 0; Index--)
			{
				Clues.Swap(Index, FMath::RandRange(0, Index));
			}
			const int32 Count = FMath::RandRange(1, Clues.Num());
			FreePlay->SetAllCluesIncluded(false);
			for (int32 Index = 0; Index < Count; Index++)
			{
				FreePlay->SetClueIncluded(Clues[Index], true);
			}
		}
	}
}

void UFreePlayConfigWidget::SelectOption(UUserWidget* Selection, int32 Index)
{
	UFunction* Select = Selection ? Selection->FindFunction(TEXT("Select")) : nullptr;
	if (!Select)
	{
		return;
	}
	uint8* Params = static_cast<uint8*>(FMemory_Alloca(Select->ParmsSize));
	FMemory::Memzero(Params, Select->ParmsSize);
	Select->InitializeStruct(Params);
	if (const FIntProperty* IndexParam = FindFProperty<FIntProperty>(Select, TEXT("Index")))
	{
		IndexParam->SetPropertyValue_InContainer(Params, Index);
	}
	Selection->ProcessEvent(Select, Params);
	Select->DestroyStruct(Params);
}

int32 UFreePlayConfigWidget::GetOptionCount(const UUserWidget* Selection)
{
	const FArrayProperty* Options = Selection ? FindFProperty<FArrayProperty>(Selection->GetClass(), TEXT("OptionStrings")) : nullptr;
	if (!Options)
	{
		return 0;
	}
	FScriptArrayHelper Array(Options, Options->ContainerPtrToValuePtr<void>(Selection));
	return Array.Num();
}

bool UFreePlayConfigWidget::CanPlay(FName Check, int32 Value) const
{
	APlayerController* PC = GetOwningPlayer();
	UFunction* Function = PC ? PC->FindFunction(Check) : nullptr;
	if (!Function)
	{
		return true;
	}
	uint8* Params = static_cast<uint8*>(FMemory_Alloca(Function->ParmsSize));
	FMemory::Memzero(Params, Function->ParmsSize);
	Function->InitializeStruct(Params);
	bool bResult = true;
	for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		if (const FIntProperty* IntParam = CastField<FIntProperty>(*It); IntParam && !It->HasAnyPropertyFlags(CPF_OutParm | CPF_ReturnParm))
		{
			IntParam->SetPropertyValue_InContainer(Params, Value);
		}
	}
	PC->ProcessEvent(Function, Params);
	for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		if (const FBoolProperty* Result = CastField<FBoolProperty>(*It); Result && It->HasAnyPropertyFlags(CPF_OutParm | CPF_ReturnParm))
		{
			bResult = Result->GetPropertyValue_InContainer(Params);
		}
	}
	Function->DestroyStruct(Params);
	return bResult;
}

#undef LOCTEXT_NAMESPACE
