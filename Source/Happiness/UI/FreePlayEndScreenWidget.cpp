#include "UI/FreePlayEndScreenWidget.h"

#include "Components/Widget.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

UFreePlayEndScreenWidget::UFreePlayEndScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// The end screens' fill and reward sounds
	static ConstructorHelpers::FObjectFinder<USoundBase> Count(TEXT("/Game/General/Audio/Cues/GameAction5.GameAction5"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Bonus(TEXT("/Game/General/Audio/Cues/Happiness.Happiness"));
	CountSound = Count.Object;
	BonusSound = Bonus.Object;
}

void UFreePlayEndScreenWidget::StartDailyBonus(int32 InMultiplier)
{
	Multiplier = InMultiplier;
	State = Multiplier > 1 ? EBonusState::WaitingForTotal : EBonusState::None;
	Elapsed = 0.f;
}

void UFreePlayEndScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	switch (State)
	{
		case EBonusState::WaitingForTotal:
		{
			// The Blueprint reveals Total EXP as its last line
			const UWidget* Total = GetWidgetFromName(TEXT("TotalExp"));
			if (Total && Total->IsVisible())
			{
				State = EBonusState::Delay;
				Elapsed = 0.f;
			}
			break;
		}

		case EBonusState::Delay:
			Elapsed += InDeltaTime;
			if (Elapsed >= BonusDelay)
			{
				BaseTotal = GetTotalExp();
				State = EBonusState::Counting;
				Elapsed = 0.f;
			}
			break;

		case EBonusState::Counting:
		{
			Elapsed += InDeltaTime;
			const float Alpha = BonusCountTime > 0.f ? FMath::Min(Elapsed / BonusCountTime, 1.f) : 1.f;
			const int32 Doubled = BaseTotal * Multiplier;
			SetTotalExp(FMath::RoundToInt(FMath::Lerp(float(BaseTotal), float(Doubled), Alpha)), false);
			PlaySound(CountSound);
			if (Alpha >= 1.f)
			{
				// The level bar takes the doubled amount
				SetTotalExp(Doubled, true);
				if (UWidget* Bonus = GetWidgetFromName(TEXT("DailyBonusText")))
				{
					Bonus->SetVisibility(ESlateVisibility::HitTestInvisible);
				}
				PlaySound(BonusSound);
				State = EBonusState::None;
			}
			break;
		}

		default:
			break;
	}
}

int32 UFreePlayEndScreenWidget::GetTotalExp() const
{
	if (const FIntProperty* Gained = FindFProperty<FIntProperty>(GetClass(), TEXT("TotalExpGained")))
	{
		return Gained->GetPropertyValue_InContainer(this);
	}
	if (const FNumericProperty* Gained = FindFProperty<FNumericProperty>(GetClass(), TEXT("TotalExpGained")))
	{
		return int32(Gained->GetFloatingPointPropertyValue(Gained->ContainerPtrToValuePtr<void>(this)));
	}
	return 0;
}

void UFreePlayEndScreenWidget::SetTotalExp(int32 Shown, bool bAlsoGained)
{
	if (const FStrProperty* Text = FindFProperty<FStrProperty>(GetClass(), TEXT("TotalExpString")))
	{
		Text->SetPropertyValue_InContainer(this, FString::FromInt(Shown));
	}
	if (!bAlsoGained)
	{
		return;
	}
	if (const FIntProperty* Gained = FindFProperty<FIntProperty>(GetClass(), TEXT("TotalExpGained")))
	{
		Gained->SetPropertyValue_InContainer(this, Shown);
	}
	else if (const FNumericProperty* Number = FindFProperty<FNumericProperty>(GetClass(), TEXT("TotalExpGained")))
	{
		Number->SetFloatingPointPropertyValue(Number->ContainerPtrToValuePtr<void>(this), double(Shown));
	}
}

void UFreePlayEndScreenWidget::PlaySound(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound, 1.f, 1.f, 0.f, nullptr, nullptr, true);
	}
}
