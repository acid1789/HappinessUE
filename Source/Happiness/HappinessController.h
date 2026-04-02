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

private:

protected:
	TObjectPtr<UPuzzle> CurrentPuzzle;
};
