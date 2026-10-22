#include "BackButton.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#endif

namespace
{
	void MoveAppToBack()
	{
#if PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID,
				"AndroidThunkJava_Happiness_MoveToBack", "()V", false);
			if (Method)
			{
				FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, Method);
			}
		}
#endif
	}

	/** It and every widget it is inside show, up to one in the viewport */
	bool IsShowing(const UWidget* Widget)
	{
		while (Widget)
		{
			if (!Widget->IsVisible())
			{
				return false;
			}
			if (const UWidget* Parent = Widget->GetParent())
			{
				Widget = Parent;
				continue;
			}
			const UUserWidget* Top = Cast<UUserWidget>(Widget);
			if (Top && Top->IsInViewport())
			{
				return true;
			}
			// The root of a widget's tree: on to the widget whose tree it is
			Widget = Widget->GetTypedOuter<UUserWidget>();
		}
		return false;
	}

	/**
	 * Mid-puzzle, pause it first (WBP_Happiness's Pause: saves the time, shows the pause menu), so the clock stops and
	 * the player comes back to the pause menu. Not when it's already paused or the puzzle is over (an end screen).
	 * WBP_Happiness has no C++ parent: it is the screen with a Pause function and a PauseMenu.
	 */
	void PausePuzzle(UWorld* World)
	{
		static const FName Showing[] = { TEXT("PauseMenu"), TEXT("EndScreen"), TEXT("LessonEndScreen"), TEXT("CampaignEndScreen") };

		TArray<UUserWidget*> Screens;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Screens, UUserWidget::StaticClass(), false);
		for (UUserWidget* Screen : Screens)
		{
			UFunction* Pause = Screen->FindFunction(TEXT("Pause"));
			if (!Pause || !Screen->WidgetTree || !Screen->WidgetTree->FindWidget(Showing[0]) || !IsShowing(Screen))
			{
				continue;
			}
			bool bPaused = false;
			for (const FName& Name : Showing)
			{
				const UWidget* Overlay = Screen->WidgetTree->FindWidget(Name);
				bPaused |= Overlay && Overlay->IsVisible();
			}
			if (!bPaused)
			{
				Screen->ProcessEvent(Pause, nullptr);
			}
		}
	}

	class FBackButtonProcessor : public IInputProcessor
	{
	public:
		explicit FBackButtonProcessor(UGameInstance* InGameInstance) : GameInstance(InGameInstance) {}

		virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

		virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
		{
			if (InKeyEvent.GetKey() != EKeys::Android_Back)
			{
				return false;
			}
			if (!InKeyEvent.IsRepeat())
			{
				if (UGameInstance* Game = GameInstance.Get())
				{
					PausePuzzle(Game->GetWorld());
				}
				MoveAppToBack();
			}
			return true;
		}

		virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
		{
			return InKeyEvent.GetKey() == EKeys::Android_Back;
		}

		virtual const TCHAR* GetDebugName() const override { return TEXT("HappinessBackButton"); }

	private:
		TWeakObjectPtr<UGameInstance> GameInstance;
	};
}

bool UBackButtonSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return PLATFORM_ANDROID && Super::ShouldCreateSubsystem(Outer);
}

void UBackButtonSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (FSlateApplication::IsInitialized())
	{
		Processor = MakeShared<FBackButtonProcessor>(GetGameInstance());
		FSlateApplication::Get().RegisterInputPreProcessor(Processor);
	}
}

void UBackButtonSubsystem::Deinitialize()
{
	if (Processor && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(Processor);
	}
	Processor.Reset();
	Super::Deinitialize();
}
