#include "UI/ClueSelectWidget.h"
#include "HappinessClassic/FreePlaySettings.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// WBP_CheckBox is a Blueprint: its label, state and check function are reached by name

	void SetCheckBoxText(UUserWidget* CheckBox, const FText& Text)
	{
		FProperty* Property = FindFProperty<FProperty>(CheckBox->GetClass(), TEXT("Text"));
		if (FTextProperty* TextProperty = CastField<FTextProperty>(Property))
		{
			TextProperty->SetPropertyValue_InContainer(CheckBox, Text);
		}
		else if (FStrProperty* StringProperty = CastField<FStrProperty>(Property))
		{
			StringProperty->SetPropertyValue_InContainer(CheckBox, Text.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("%s has no Text or String variable named Text"), *CheckBox->GetClass()->GetName());
		}
	}

	bool IsCheckBoxChecked(const UUserWidget* CheckBox)
	{
		const FBoolProperty* Property = FindFProperty<FBoolProperty>(CheckBox->GetClass(), TEXT("Checked"));
		return Property && Property->GetPropertyValue_InContainer(CheckBox);
	}

	void SetCheckBoxChecked(UUserWidget* CheckBox, bool bChecked)
	{
		// Check() also shows or hides the check mark
		UFunction* Check = CheckBox->FindFunction(TEXT("Check"));
		const FBoolProperty* Param = Check ? FindFProperty<FBoolProperty>(Check, TEXT("ShouldBeChecked")) : nullptr;
		if (!Param)
		{
			return;
		}
		uint8* Params = static_cast<uint8*>(FMemory_Alloca(Check->ParmsSize));
		FMemory::Memzero(Params, Check->ParmsSize);
		Param->SetPropertyValue_InContainer(Params, bChecked);
		CheckBox->ProcessEvent(Check, Params);
	}
}

void UClueSelectToggle::HandleClicked()
{
	// The checkbox toggled itself when pressed; take its new state
	UClueSelectWidget* Widget = Owner.Get();
	const UUserWidget* Box = CheckBox.Get();
	if (Widget && Box)
	{
		Widget->SetClueIncluded(Clue, IsCheckBoxChecked(Box));
	}
}

UClueSelectWidget::UClueSelectWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<USoundBase> Button(TEXT("/Game/General/Audio/Cues/MenuAccept.MenuAccept"));
	ButtonSound = Button.Object;

	static ConstructorHelpers::FClassFinder<UUserWidget> CheckBox(TEXT("/Game/General/UI/WBP_CheckBox"));
	CheckBoxClass = CheckBox.Class;
}

void UClueSelectWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// Also runs in the designer, where every clue shows as included
	BuildCheckBoxes();
	Refresh();
	Panel->SetVisibility(bStartOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UClueSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	OpenButton->OnClicked.AddUniqueDynamic(this, &UClueSelectWidget::HandleOpenClicked);
	DoneButton->OnClicked.AddUniqueDynamic(this, &UClueSelectWidget::HandleDoneClicked);
	if (AllButton)
	{
		AllButton->OnClicked.AddUniqueDynamic(this, &UClueSelectWidget::HandleAllClicked);
	}
	if (NoneButton)
	{
		NoneButton->OnClicked.AddUniqueDynamic(this, &UClueSelectWidget::HandleNoneClicked);
	}
	if (UFreePlaySubsystem* FreePlay = GetFreePlay())
	{
		FreePlay->OnCluesChanged.AddUniqueDynamic(this, &UClueSelectWidget::HandleCluesChanged);
	}

	Panel->SetVisibility(bStartOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	Refresh();
}

void UClueSelectWidget::NativeDestruct()
{
	if (UFreePlaySubsystem* FreePlay = GetFreePlay())
	{
		FreePlay->OnCluesChanged.RemoveDynamic(this, &UClueSelectWidget::HandleCluesChanged);
	}
	Super::NativeDestruct();
}

UFreePlaySubsystem* UClueSelectWidget::GetFreePlay() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UFreePlaySubsystem>() : nullptr;
}

void UClueSelectWidget::BuildCheckBoxes()
{
	if (!ClueGrid || !WidgetTree)
	{
		return;
	}

	ClueGrid->ClearChildren();
	CheckBoxes.Reset();
	Toggles.Reset();

	// Down the first column, then the next
	const TArray<ECampaignLesson> Clues = UFreePlaySubsystem::GetSelectableClues();
	const int32 NumColumns = FMath::Max(1, Columns);
	const int32 Rows = FMath::DivideAndRoundUp(Clues.Num(), NumColumns);
	for (int32 i = 0; i < Clues.Num(); i++)
	{
		UUserWidget* CheckBox = CheckBoxClass ? WidgetTree->ConstructWidget<UUserWidget>(CheckBoxClass) : nullptr;
		if (!CheckBox)
		{
			return;
		}
		SetCheckBoxText(CheckBox, UFreePlaySubsystem::GetClueName(Clues[i]));
		if (UTextBlock* Label = LabelFontSize > 0.f ? Cast<UTextBlock>(CheckBox->GetWidgetFromName(TEXT("TextBlock_67"))) : nullptr)
		{
			// Smaller text rather than scaling the checkboxes, which would blur their one-pixel shadows away
			FSlateFontInfo Font = Label->GetFont();
			Font.Size = LabelFontSize;
			Label->SetFont(Font);
		}
		SetCheckBoxChecked(CheckBox, true);

		UClueSelectToggle* Toggle = NewObject<UClueSelectToggle>(this);
		Toggle->Clue = Clues[i];
		Toggle->CheckBox = CheckBox;
		Toggle->Owner = this;
		if (UButton* Button = Cast<UButton>(CheckBox->GetWidgetFromName(TEXT("Button_53"))))
		{
			Button->OnClicked.AddDynamic(Toggle, &UClueSelectToggle::HandleClicked);
		}

		UUniformGridSlot* GridSlot = ClueGrid->AddChildToUniformGrid(CheckBox, i % Rows, i / Rows);
		GridSlot->SetHorizontalAlignment(HAlign_Left);
		GridSlot->SetVerticalAlignment(VAlign_Center);

		CheckBoxes.Add(CheckBox);
		Toggles.Add(Toggle);
	}
	ClueGrid->SetSlotPadding(CluePadding);
}

void UClueSelectWidget::Refresh()
{
	const UFreePlaySubsystem* FreePlay = GetFreePlay();
	for (int32 i = 0; i < CheckBoxes.Num(); i++)
	{
		SetCheckBoxChecked(CheckBoxes[i], !FreePlay || FreePlay->IsClueIncluded(Toggles[i]->Clue));
	}
	if (SummaryText)
	{
		SummaryText->SetText(FreePlay ? FreePlay->GetSelectionSummary() : NSLOCTEXT("FreePlay", "AllClues", "All"));
	}
}

void UClueSelectWidget::SetClueIncluded(ECampaignLesson Clue, bool bIncluded)
{
	if (UFreePlaySubsystem* FreePlay = GetFreePlay())
	{
		FreePlay->SetClueIncluded(Clue, bIncluded);
	}
}

void UClueSelectWidget::OpenPanel()
{
	Refresh();
	Panel->SetVisibility(ESlateVisibility::Visible);
}

void UClueSelectWidget::ClosePanel()
{
	Panel->SetVisibility(ESlateVisibility::Collapsed);
	Refresh();
}

void UClueSelectWidget::HandleOpenClicked()
{
	PlayButtonSound();
	OpenPanel();
}

void UClueSelectWidget::HandleAllClicked()
{
	PlayButtonSound();
	if (UFreePlaySubsystem* FreePlay = GetFreePlay())
	{
		FreePlay->SetAllCluesIncluded(true);
	}
}

void UClueSelectWidget::HandleNoneClicked()
{
	PlayButtonSound();
	if (UFreePlaySubsystem* FreePlay = GetFreePlay())
	{
		FreePlay->SetAllCluesIncluded(false);
	}
}

void UClueSelectWidget::HandleDoneClicked()
{
	PlayButtonSound();
	ClosePanel();
}

void UClueSelectWidget::HandleCluesChanged()
{
	Refresh();
}

void UClueSelectWidget::PlayButtonSound() const
{
	if (ButtonSound)
	{
		UGameplayStatics::PlaySound2D(this, ButtonSound, 1.f, 1.f, 0.f, nullptr, nullptr, true);
	}
}
