#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BackButton.generated.h"

/**
 * Android's back button sends the app to the background (like Home), from any screen: the game keeps its state and
 * resumes where it was; mid-puzzle the puzzle is paused first (UAppPauseSubsystem::PausePuzzle). Caught before the UI sees it (a Slate input preprocessor), so no widget handles back itself.
 * The Java side is AndroidThunkJava_Happiness_MoveToBack in Happiness_UPL.xml. Android only.
 */
UCLASS()
class HAPPINESS_API UBackButtonSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	TSharedPtr<class IInputProcessor> Processor;
};
