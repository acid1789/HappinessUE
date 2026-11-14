#include "ButtonPanelWidget.h"

#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HappinessClassic/CampaignProgress.h"

#if PLATFORM_IOS && !PLATFORM_TVOS
#include "IOS/IOSAppDelegate.h"
#include "IOS/IOSView.h"
#include <atomic>

namespace
{
	// The notch's width in pixels when it is on the left, else 0 (-1 before the first reading). Read from UIKit on the
	// main thread: the engine's own orientation (FIOSApplication::CachedOrientation) is only right once the phone has
	// been turned, so at launch it said the notch was on the right.
	std::atomic<float> GLeftNotchPixels{ -1.f };
	std::atomic<bool> GLeftNotchQueued{ false };

	void RefreshLeftNotch()
	{
		if (GLeftNotchQueued.exchange(true))
		{
			return;
		}
		dispatch_async(dispatch_get_main_queue(), ^{
			IOSAppDelegate* Delegate = [IOSAppDelegate GetDelegate];
			UIWindow* Window = Delegate ? Delegate.Window : nil;
			float Pixels = 0.f;
			// iOS keeps landscape's safe area the same on both sides, whichever side the notch is on. The notch is
			// at the phone's top, opposite where the home button would be: on the left with the "home button" on
			// the right.
			if (Window != nil && Window.windowScene != nil
				&& Window.windowScene.effectiveGeometry.interfaceOrientation == UIInterfaceOrientationLandscapeRight)
			{
				const float Scale = Delegate.IOSView != nil ? Delegate.IOSView.contentScaleFactor : Window.screen.scale;
				Pixels = Window.safeAreaInsets.left * Scale;
			}
			GLeftNotchPixels = Pixels;
			GLeftNotchQueued = false;
		});
	}
}
#endif

void UButtonPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ShownTime = FSlateApplication::IsInitialized() ? FSlateApplication::Get().GetCurrentTime() : 0.0;
}

void UButtonPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateNotchShift(MyGeometry.Scale);
	if (!Button_H || !FSlateApplication::IsInitialized())
	{
		return;
	}

	// No hints or fixing on Master Mode stages
	const bool bHintsAllowed = UCampaignSubsystem::AreAssistsAllowed(this);
	for (UWidget* Button : { Button_H.Get(), Button_Fix.Get() })
	{
		if (Button && Button->GetIsEnabled() != bHintsAllowed)
		{
			Button->SetIsEnabled(bHintsAllowed);
		}
	}
	if (IdleSeconds <= 0.f || WiggleSeconds <= 0.f)
	{
		return;
	}

	// Idle since the last click, touch or key (or since the panel was shown)
	const FSlateApplication& Slate = FSlateApplication::Get();
	const double Idle = Slate.GetCurrentTime() - FMath::Max(Slate.GetLastUserInteractionTime(), ShownTime);

	// A wiggle at every IdleSeconds of idle time: left, right, back to straight. Off when the player turned
	// "Animate Hint Button" off in the options, and in the tutorial puzzle.
	float Angle = 0.f;
	if (bHintsAllowed && Idle >= IdleSeconds && IsAnimationEnabled() && !UCampaignSubsystem::IsPlayingTutorial(this))
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

float UButtonPanelWidget::GetLeftNotchInset(float Scale) const
{
	if (!FSlateApplication::IsInitialized() || Scale <= 0.f)
	{
		return 0.f;
	}
#if PLATFORM_IOS && !PLATFORM_TVOS
	// Read again a few times a second, so it follows the phone being turned around
	const double Now = FPlatformTime::Seconds();
	if (Now >= NextNotchReadTime)
	{
		NextNotchReadTime = Now + 0.25;
		RefreshLeftNotch();
	}
	const float Left = GLeftNotchPixels.load();
#else
	FMargin SafeZone;
	FSlateApplication::Get().GetSafeZoneSize(SafeZone, FVector2D::ZeroVector);
	const float Left = SafeZone.Left;
#endif
	// Pixels to widget units
	return FMath::RoundToFloat(FMath::Max(0.f, Left) / Scale);
}

void UButtonPanelWidget::UpdateNotchShift(float Scale)
{
	const float Inset = GetLeftNotchInset(Scale);
	if (Inset == AppliedNotchInset)
	{
		return;
	}
	AppliedNotchInset = Inset;

	// This panel and the panels right of it (stretched, Left the margin from the canvas's left edge) move right by
	// the inset; the panels right of it keep their right edges, so they lose that much width
	auto Shift = [this, Inset](UWidget* Widget)
	{
		UCanvasPanelSlot* CanvasSlot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
		if (!CanvasSlot)
		{
			return;
		}
		FMargin Offsets = CanvasSlot->GetOffsets();
		const float Designed = DesignedLeftOffsets.FindOrAdd(Widget->GetFName(), Offsets.Left);
		Offsets.Left = Designed + Inset;
		CanvasSlot->SetOffsets(Offsets);
	};
	Shift(this);
	if (const UPanelWidget* Parent = GetParent())
	{
		for (UWidget* Sibling : Parent->GetAllChildren())
		{
			if (Sibling && NotchMakeRoom.Contains(Sibling->GetFName()))
			{
				Shift(Sibling);
			}
		}
	}
}
