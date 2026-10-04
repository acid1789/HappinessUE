#include "UI/PauseMenuWidget.h"

#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "HappinessClassic/Puzzle.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

UPauseMenuWidget::UPauseMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// The old pause menu's sounds and Options screen
	static ConstructorHelpers::FObjectFinder<USoundBase> Accept(TEXT("/Game/General/Audio/Cues/MenuAccept.MenuAccept"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Cancel(TEXT("/Game/General/Audio/Cues/MenuCancel.MenuCancel"));
	AcceptSound = Accept.Object;
	CancelSound = Cancel.Object;
	OptionsClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/Happiness/UI/WBP_Options.WBP_Options_C")));
}

void UPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ResumeButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleResume);
	ResetButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleReset);
	FixButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleFix);
	UnhideButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleUnhide);
	RulesButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleRules);
	OptionsButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleOptions);
	MainMenuButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleMainMenu);
	QuitAppButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleQuitApp);

#if PLATFORM_IOS
	// iOS apps don't quit themselves
	QuitAppButton->SetVisibility(ESlateVisibility::Collapsed);
	if (UWidget* QuitParent = QuitAppButton->GetParent())
	{
		QuitParent->SetVisibility(ESlateVisibility::Collapsed);
	}
#endif
}

void UPauseMenuWidget::Show(APlayerController* PlayerController)
{
	PC = PlayerController;
	UpdateTime();
	SetVisibility(ESlateVisibility::Visible);
}

void UPauseMenuWidget::UpdateTime()
{
	// The puzzle screen (WBP_Happiness) this menu is part of: its puzzle and the time it saved when pausing
	const UUserWidget* Screen = GetTypedOuter<UUserWidget>();
	if (!TimeText || !Screen)
	{
		return;
	}
	const FObjectPropertyBase* PuzzleProperty = FindFProperty<FObjectPropertyBase>(Screen->GetClass(), TEXT("ThePuzzle"));
	const FNumericProperty* TimeProperty = FindFProperty<FNumericProperty>(Screen->GetClass(), TEXT("PuzzleTime"));
	const UPuzzle* Puzzle = PuzzleProperty ? Cast<UPuzzle>(PuzzleProperty->GetObjectPropertyValue_InContainer(Screen)) : nullptr;
	if (!Puzzle || !TimeProperty)
	{
		TimeText->SetText(FText::GetEmpty());
		return;
	}
	const void* Time = TimeProperty->ContainerPtrToValuePtr<void>(Screen);
	const double Seconds = TimeProperty->IsFloatingPoint() ? TimeProperty->GetFloatingPointPropertyValue(Time)
		: static_cast<double>(TimeProperty->GetSignedIntPropertyValue(Time));
	TimeText->SetText(FText::FromString(Puzzle->FormatTimeString(static_cast<float>(Seconds))));
}

void UPauseMenuWidget::SetVisibility(ESlateVisibility InVisibility)
{
	Super::SetVisibility(InVisibility);
	HideScreen(InVisibility != ESlateVisibility::Hidden && InVisibility != ESlateVisibility::Collapsed);
}

void UPauseMenuWidget::HideScreen(bool bHide)
{
	if (!bHide)
	{
		for (const TPair<TWeakObjectPtr<UWidget>, ESlateVisibility>& Hidden : HiddenScreen)
		{
			if (UWidget* Widget = Hidden.Key.Get())
			{
				Widget->SetVisibility(Hidden.Value);
			}
		}
		HiddenScreen.Reset();
		return;
	}

	// Already hidden (shown again while showing): keep the first visibilities
	const UPanelWidget* Screen = GetParent();
	if (!Screen || HiddenScreen.Num() > 0)
	{
		return;
	}
	for (UWidget* Sibling : Screen->GetAllChildren())
	{
		if (Sibling && Sibling != this && Sibling->IsVisible())
		{
			HiddenScreen.Add(Sibling, Sibling->GetVisibility());
			Sibling->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UPauseMenuWidget::HandleResume()
{
	PlaySound(CancelSound);
	OnResumeGame.Broadcast();
}

void UPauseMenuWidget::HandleReset()
{
	PlaySound(AcceptSound);
	OnResetPuzzle.Broadcast();
}

void UPauseMenuWidget::HandleFix()
{
	PlaySound(AcceptSound);
	OnFixPuzzle.Broadcast();
}

void UPauseMenuWidget::HandleUnhide()
{
	PlaySound(AcceptSound);
	OnUnhideClues.Broadcast();
}

void UPauseMenuWidget::HandleRules()
{
	PlaySound(AcceptSound);
	OnGameRules.Broadcast();
}

void UPauseMenuWidget::HandleOptions()
{
	PlaySound(AcceptSound);
	UCanvasPanel* Canvas = Cast<UCanvasPanel>(GetRootWidget());
	if (!Canvas)
	{
		return;
	}
	const bool bNew = !OptionsWidget;
	if (bNew)
	{
		if (UClass* Class = OptionsClass.LoadSynchronous())
		{
			OptionsWidget = CreateWidget<UUserWidget>(this, Class);
		}
	}
	UUserWidget* Options = OptionsWidget;
	if (!Options)
	{
		return;
	}
	Options->SetVisibility(ESlateVisibility::Visible);

	// WBP_Options.Show(PC) loads the current settings into it
	if (UFunction* ShowFunction = Options->FindFunction(TEXT("Show")))
	{
		uint8* Params = static_cast<uint8*>(FMemory_Alloca(ShowFunction->ParmsSize));
		FMemory::Memzero(Params, ShowFunction->ParmsSize);
		if (const FObjectPropertyBase* PCParam = CastField<FObjectPropertyBase>(ShowFunction->ChildProperties))
		{
			PCParam->SetObjectPropertyValue_InContainer(Params, PC ? PC.Get() : GetOwningPlayer());
		}
		Options->ProcessEvent(ShowFunction, Params);
	}

	// Over the whole screen, above the menu
	if (UCanvasPanelSlot* OptionsSlot = bNew ? Canvas->AddChildToCanvas(Options) : nullptr)
	{
		OptionsSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		OptionsSlot->SetOffsets(FMargin(0.f));
		OptionsSlot->SetZOrder(300);
	}
}

void UPauseMenuWidget::HandleMainMenu()
{
	PlaySound(CancelSound);
	OnMainMenu.Broadcast();
}

void UPauseMenuWidget::HandleQuitApp()
{
	PlaySound(CancelSound);
	OnQuitApp.Broadcast();
}

void UPauseMenuWidget::PlaySound(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound, 1.f, 1.f, 0.f, nullptr, nullptr, true);
	}
}
