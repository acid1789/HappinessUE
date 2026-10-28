#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AppPause.generated.h"

/**
 * Mid-puzzle, the puzzle pauses when the app stops being active: the phone locked (power button), Home, another app
 * or a call on top (FCoreDelegates::ApplicationWillDeactivateDelegate, Android and iOS). The player comes back to the
 * pause menu with the clock stopped. Android's back button pauses the same way before sending the app to the
 * background (UBackButtonSubsystem). Not for our own interstitials, which cover the game too (UAdsSubsystem).
 */
UCLASS()
class HAPPINESS_API UAppPauseSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Mid-puzzle, pause it (WBP_Happiness's Pause: saves the time, shows the pause menu). Not when it's already paused
	 * or the puzzle is over (an end screen), or when no puzzle screen is showing.
	 */
	static void PausePuzzle(UWorld* World);

private:
	void HandleDeactivate();

	/** Deactivations this soon after an interstitial ends are its own (handed over late), not the player leaving */
	static constexpr double InterstitialGraceSeconds = 2.0;

	FDelegateHandle DeactivateHandle;
};
