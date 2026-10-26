// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "HappinessClassic/Puzzle.h"

#include "HappinessController.generated.h"

/**
 * 
 */
UCLASS()
class HAPPINESS_API AHappinessController : public APlayerController
{
	GENERATED_BODY()

public:
	AHappinessController();
	virtual ~AHappinessController();

	virtual void BeginPlay() override;

private:
	/**
	 * Reads the player's level and EXP (SG_Happiness's Level and Experience) into PC_Happiness's HappinessLevel and
	 * HappinessExp. Its LoadHappiness only runs on the way into a classic puzzle, so without this a game started from
	 * the lessons tree would show and save the defaults over the player's level.
	 */
	void LoadPlayerLevel();

protected:
	TObjectPtr<UPuzzle> CurrentPuzzle;
};
