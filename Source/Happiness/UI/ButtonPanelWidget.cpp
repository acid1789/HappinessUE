#include "ButtonPanelWidget.h"

#include "Components/Widget.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"

void UButtonPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ShownTime = FSlateApplication::IsInitialized() ? FSlateApplication::Get().GetCurrentTime() : 0.0;
}

void UButtonPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!Button_H || !FSlateApplication::IsInitialized() || IdleSeconds <= 0.f || WiggleSeconds <= 0.f)
	{
		return;
	}

	// Idle since the last click, touch or key (or since the panel was shown)
	const FSlateApplication& Slate = FSlateApplication::Get();
	const double Idle = Slate.GetCurrentTime() - FMath::Max(Slate.GetLastUserInteractionTime(), ShownTime);

	// A wiggle at every IdleSeconds of idle time: left, right, back to straight. Off when the player turned
	// "Animate Hint Button" off in the options.
	float Angle = 0.f;
	if (Idle >= IdleSeconds && IsAnimationEnabled())
	{
		const double Into = FMath::Fmod(Idle, (double)IdleSeconds);
		if (Into < WiggleSeconds)
		{
			Angle = -WiggleDegrees * FMath::Sin(2.f * PI * (float)(Into / WiggleSeconds));
		}
	}

	if (Button_H->GetRenderTransformAngle() != Angle)
	{
		Button_H->SetRenderTransformAngle(Angle);
	}
}

bool UButtonPanelWidget::IsAnimationEnabled() const
{
	// The option lives on the player controller with the other settings (PC_Happiness, saved in SG_Settings)
	const APlayerController* PC = GetOwningPlayer();
	const FBoolProperty* Option = PC ? FindFProperty<FBoolProperty>(PC->GetClass(), TEXT("Happiness_HintAnimation")) : nullptr;
	return !Option || Option->GetPropertyValue_InContainer(PC);
}
