#include "BackButton.h"

#include "AppPause.h"
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
					UAppPauseSubsystem::PausePuzzle(Game->GetWorld());
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
