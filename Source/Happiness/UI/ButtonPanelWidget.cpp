#include "ButtonPanelWidget.h"

#include "Components/Widget.h"
#include "Framework/Application/SlateApplication.h"

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

	// A wiggle at every IdleSeconds of idle time: left, right, back to straight
	float Angle = 0.f;
	if (Idle >= IdleSeconds)
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
