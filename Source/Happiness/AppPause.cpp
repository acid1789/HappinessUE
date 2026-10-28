#include "AppPause.h"

#include "Ads/AdsSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CoreDelegates.h"

namespace
{
	/** It and every widget it is inside show, up to one in the viewport */
	bool IsShowing(const UWidget* Widget)
	{
		while (Widget)
		{
			if (!Widget->IsVisible())
			{
				return false;
			}
			if (const UWidget* Parent = Widget->GetParent())
			{
				Widget = Parent;
				continue;
			}
			const UUserWidget* Top = Cast<UUserWidget>(Widget);
			if (Top && Top->IsInViewport())
			{
				return true;
			}
			// The root of a widget's tree: on to the widget whose tree it is
			Widget = Widget->GetTypedOuter<UUserWidget>();
		}
		return false;
	}
}

void UAppPauseSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Called on the game thread (Android's and iOS's app events are passed to it)
	DeactivateHandle = FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this, &UAppPauseSubsystem::HandleDeactivate);
}

void UAppPauseSubsystem::Deinitialize()
{
	FCoreDelegates::ApplicationWillDeactivateDelegate.Remove(DeactivateHandle);
	Super::Deinitialize();
}

void UAppPauseSubsystem::HandleDeactivate()
{
	// Our own interstitial covering the game (it plays between puzzles, and the game carries on after it)
	const UAdsSubsystem* Ads = UAdsSubsystem::GetInstance();
	if (Ads && Ads->IsInterstitialRecent(InterstitialGraceSeconds))
	{
		return;
	}
	PausePuzzle(GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr);
}

void UAppPauseSubsystem::PausePuzzle(UWorld* World)
{
	// WBP_Happiness has no C++ parent: it is the screen with a Pause function and a PauseMenu
	static const FName Showing[] = { TEXT("PauseMenu"), TEXT("EndScreen"), TEXT("LessonEndScreen"), TEXT("CampaignEndScreen") };
	if (!World)
	{
		return;
	}

	TArray<UUserWidget*> Screens;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Screens, UUserWidget::StaticClass(), false);
	for (UUserWidget* Screen : Screens)
	{
		UFunction* Pause = Screen->FindFunction(TEXT("Pause"));
		if (!Pause || !Screen->WidgetTree || !Screen->WidgetTree->FindWidget(Showing[0]) || !IsShowing(Screen))
		{
			continue;
		}
		bool bPaused = false;
		for (const FName& Name : Showing)
		{
			const UWidget* Overlay = Screen->WidgetTree->FindWidget(Name);
			bPaused |= Overlay && Overlay->IsVisible();
		}
		if (!bPaused)
		{
			Screen->ProcessEvent(Pause, nullptr);
		}
	}
}

#if !UE_BUILD_SHIPPING
static FAutoConsoleCommandWithWorld GSimulateAppDeactivateCommand(
	TEXT("Happiness.SimulateAppDeactivate"),
	TEXT("Do what the phone locking does: mid-puzzle, pause the puzzle. Dev builds only."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		UAppPauseSubsystem::PausePuzzle(World);
	}));
#endif
