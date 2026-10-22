#include "BackButton.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "UI/GamePanelWidget.h"

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

	/**
	 * Mid-puzzle, pause it first (WBP_Happiness's Pause: saves the time, shows the pause menu), so the clock stops and
	 * the player comes back to the pause menu. Not when it's already paused or the puzzle is over (an end screen).
	 */
	void PausePuzzle(UWorld* World)
	{
		static const FName Showing[] = { TEXT("PauseMenu"), TEXT("EndScreen"), TEXT("LessonEndScreen"), TEXT("CampaignEndScreen") };

		TArray<UUserWidget*> Screens;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Screens, UGamePanelWidget::StaticClass(), true);
		for (UUserWidget* Screen : Screens)
		{
			if (!Screen->IsInViewport() || !Screen->IsVisible())
			{
				continue;
			}
			bool bPaused = false;
			for (const FName& Name : Showing)
			{
				const UWidget* Overlay = Screen->WidgetTree ? Screen->WidgetTree->FindWidget(Name) : nullptr;
				bPaused |= Overlay && Overlay->IsVisible();
			}
			if (UFunction* Pause = !bPaused ? Screen->FindFunction(TEXT("Pause")) : nullptr)
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
