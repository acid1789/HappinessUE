// Fill out your copyright notice in the Description page of Project Settings.


#include "HappinessController.h"

#include "HappinessClassic/HappinessSaveGame.h"

AHappinessController::AHappinessController()
{
}

AHappinessController::~AHappinessController()
{
}

void AHappinessController::BeginPlay()
{
	LoadPlayerLevel();
	Super::BeginPlay();
}

void AHappinessController::LoadPlayerLevel()
{
	const UHappinessSaveGame* SaveGame = UHappinessSaveGame::LoadFromSlot();
	if (!SaveGame)
	{
		return;
	}

	// All four are Blueprint variables (SG_Happiness, PC_Happiness)
	const TPair<const TCHAR*, const TCHAR*> Copies[] = {
		{ TEXT("Level"), TEXT("HappinessLevel") },
		{ TEXT("Experience"), TEXT("HappinessExp") },
	};
	for (const TPair<const TCHAR*, const TCHAR*>& Copy : Copies)
	{
		const FIntProperty* From = FindFProperty<FIntProperty>(SaveGame->GetClass(), Copy.Key);
		const FIntProperty* To = FindFProperty<FIntProperty>(GetClass(), Copy.Value);
		if (From && To)
		{
			To->SetPropertyValue_InContainer(this, From->GetPropertyValue_InContainer(SaveGame));
		}
	}
}
