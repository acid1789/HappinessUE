#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FreePlayEndScreenWidget.generated.h"

class USoundBase;
class UWidget;

/**
 * Parent of WBP_EndScreen, the Free Play end screen, which does its scoring and staging in Blueprint. Adds the daily
 * puzzle's bonus step: once Total EXP has been revealed (at its normal amount), it counts up to ExpMultiplier times
 * that and "x2 Daily Bonus!" (DailyBonusText) appears, before the level bar takes the experience. Started by
 * UDailySubsystem::ShowDailyEndScreen.
 *
 * Works on the Blueprint's own variables: TotalExpString (the Total EXP text) and TotalExpGained (what the level bar
 * adds), and its TotalExp widget (revealed by its animation).
 */
UCLASS()
class HAPPINESS_API UFreePlayEndScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFreePlayEndScreenWidget(const FObjectInitializer& ObjectInitializer);

	/** Seconds after Total EXP appears before it starts doubling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Daily Bonus")
	float BonusDelay = 0.5f;

	/** Seconds for Total EXP to count up to the doubled amount (the level bar starts 2 s after Total EXP appears) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Daily Bonus")
	float BonusCountTime = 0.8f;

	/** Played every frame while the total counts up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Daily Bonus")
	TObjectPtr<USoundBase> CountSound;

	/** Played when "x2 Daily Bonus!" appears */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Daily Bonus")
	TObjectPtr<USoundBase> BonusSound;

	/** Arm the bonus step for this showing of the end screen (Multiplier 1 or less: none) */
	void StartDailyBonus(int32 Multiplier);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	enum class EBonusState : uint8
	{
		None,
		WaitingForTotal,	// the Blueprint is still revealing the lines
		Delay,				// Total EXP is showing at its normal amount
		Counting,			// counting up to the doubled amount
	};

	int32 GetTotalExp() const;
	void SetTotalExp(int32 Shown, bool bAlsoGained);
	void PlaySound(USoundBase* Sound) const;

	EBonusState State = EBonusState::None;
	int32 Multiplier = 1;
	int32 BaseTotal = 0;
	float Elapsed = 0.f;
};
