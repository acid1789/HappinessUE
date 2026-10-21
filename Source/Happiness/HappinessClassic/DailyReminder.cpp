#include "DailyReminder.h"
#include "DailyPuzzle.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/BlueprintPlatformLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CoreDelegates.h"
#include "Misc/DateTime.h"

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#endif

#if PLATFORM_IOS
#include <UIKit/UIKit.h>
#include <UserNotifications/UserNotifications.h>
#endif

#define LOCTEXT_NAMESPACE "DailyReminder"

DEFINE_LOG_CATEGORY_STATIC(LogDailyReminder, Log, All);

namespace
{
	constexpr bool bHasNotifications = PLATFORM_ANDROID || PLATFORM_IOS;

	// GGameUserSettingsIni: the player has been asked for permission to notify
	const TCHAR* SettingsSection = TEXT("/Script/Happiness.DailyReminderSubsystem");
	const TCHAR* AskedKey = TEXT("bAskedPermission");

	// Passed back when a reminder opens the app
	const TCHAR* ActivationEvent = TEXT("DailyPuzzle");
}

void UDailyReminderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UDailySubsystem* Daily = Collection.InitializeDependency<UDailySubsystem>();
	if (!bHasNotifications)
	{
		return;
	}
	if (Daily)
	{
		Daily->OnChanged.AddUniqueDynamic(this, &UDailyReminderSubsystem::Reschedule);
	}
	ForegroundHandle = FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this, &UDailyReminderSubsystem::HandleForeground);
	ClearShown();
	Reschedule();
}

void UDailyReminderSubsystem::Deinitialize()
{
	FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(ForegroundHandle);
	Super::Deinitialize();
}

void UDailyReminderSubsystem::HandleForeground()
{
	// The day may have changed while the app was away
	ClearShown();
	Reschedule();
}

void UDailyReminderSubsystem::Reschedule()
{
	if (!bHasNotifications)
	{
		return;
	}
	const UDailySubsystem* Daily = GetGameInstance() ? GetGameInstance()->GetSubsystem<UDailySubsystem>() : nullptr;
	const bool bTodaySolved = Daily && Daily->IsTodaySolved();
	const int32 Streak = Daily ? Daily->GetStreak() : 0;
	if (bTodaySolved)
	{
		RequestPermissionOnce();
	}

	UBlueprintPlatformLibrary::ClearAllLocalNotifications();
	const FDateTime Now = FDateTime::Now();
	const FDateTime Today = Now.GetDate();
	const FText Title = LOCTEXT("Title", "Today's puzzle is ready");
	bool bFirst = true;
	for (int32 Day = 0; Day <= FMath::Max(DaysAhead, 1); ++Day)
	{
		const FDateTime Fire = Today + FTimespan::FromDays(Day) + FTimespan(ReminderHour, ReminderMinute, 0);
		if (Fire <= Now || (Day == 0 && bTodaySolved))
		{
			continue;
		}
		// The streak is still going on the first reminder's day if it falls today, or today's puzzle is solved
		const bool bStreakGoing = bFirst && Streak > 0 && (Day == 0 || bTodaySolved);
		const FText Body = bStreakGoing
			? FText::Format(LOCTEXT("BodyStreak", "Keep your {0}-day streak going!"), Streak)
			: LOCTEXT("Body", "A new daily puzzle is waiting for you.");
		UBlueprintPlatformLibrary::ScheduleLocalNotificationAtTime(Fire, true, Title, Body, FText::GetEmpty(), ActivationEvent);
		if (bFirst)
		{
			UE_LOG(LogDailyReminder, Log, TEXT("Daily reminders from %s"), *Fire.ToString());
		}
		bFirst = false;
	}

	// The evening nudge, for a long streak: on the first day not yet solved (today, or tomorrow once today is done).
	// Only that one: a later day's streak depends on solves that would reschedule anyway.
	if (Streak >= EveningMinStreak)
	{
		const FDateTime Evening = Today + FTimespan::FromDays(bTodaySolved ? 1 : 0) + FTimespan(EveningHour, EveningMinute, 0);
		if (Evening > Now)
		{
			UBlueprintPlatformLibrary::ScheduleLocalNotificationAtTime(Evening, true, LOCTEXT("EveningTitle", "Your streak is waiting"),
				FText::Format(LOCTEXT("EveningBody", "Don't lose your {0}-day streak! Today's puzzle is still waiting."), Streak),
				FText::GetEmpty(), ActivationEvent);
			UE_LOG(LogDailyReminder, Log, TEXT("Streak reminder at %s"), *Evening.ToString());
		}
	}
}

void UDailyReminderSubsystem::RequestPermissionOnce()
{
	bool bAsked = false;
	GConfig->GetBool(SettingsSection, AskedKey, bAsked, GGameUserSettingsIni);
	if (bAsked)
	{
		return;
	}
	GConfig->SetBool(SettingsSection, AskedKey, true, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);

#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		// Happiness_UPL.xml: asks for POST_NOTIFICATIONS on Android 13+, nothing before
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID,
			"AndroidThunkJava_Happiness_RequestNotifications", "()V", false);
		if (Method)
		{
			FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, Method);
		}
	}
#elif PLATFORM_IOS
	// Alerts and sound only: no push notifications (the engine's RegisterForRemoteNotifications also registers those)
	dispatch_async(dispatch_get_main_queue(), ^{
		[[UNUserNotificationCenter currentNotificationCenter]
			requestAuthorizationWithOptions:(UNAuthorizationOptionAlert | UNAuthorizationOptionSound)
			completionHandler:^(BOOL bGranted, NSError* _Nullable Error) {}];
	});
#endif
}

void UDailyReminderSubsystem::ClearShown()
{
#if PLATFORM_IOS
	dispatch_async(dispatch_get_main_queue(), ^{
		UNUserNotificationCenter* Center = [UNUserNotificationCenter currentNotificationCenter];
		[Center removeAllDeliveredNotifications];
		// The engine gives each notification a badge of 1
		if (@available(iOS 16.0, *))
		{
			[Center setBadgeCount:0 withCompletionHandler:nil];
		}
		else
		{
			[UIApplication sharedApplication].applicationIconBadgeNumber = 0;
		}
	});
#endif
}

void UDailyReminderSubsystem::DebugTest(int32 Seconds)
{
	if (!bHasNotifications)
	{
		UE_LOG(LogDailyReminder, Warning, TEXT("Reminders only exist on Android and iOS"));
		return;
	}
	RequestPermissionOnce();
	UBlueprintPlatformLibrary::ScheduleLocalNotificationFromNow(FMath::Max(Seconds, 1), LOCTEXT("Title", "Today's puzzle is ready"),
		LOCTEXT("Body", "A new daily puzzle is waiting for you."), FText::GetEmpty(), ActivationEvent);
}

#if !UE_BUILD_SHIPPING
namespace
{
	FAutoConsoleCommandWithWorldAndArgs ReminderTestCommand(
		TEXT("Happiness.Reminder.Test"),
		TEXT("Daily reminder: show one in N seconds (default 10). Asks for permission if it hasn't been asked."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		if (UDailyReminderSubsystem* Reminder = GameInstance ? GameInstance->GetSubsystem<UDailyReminderSubsystem>() : nullptr)
		{
			Reminder->DebugTest(Args.Num() > 0 && Args[0].IsNumeric() ? FCString::Atoi(*Args[0]) : 10);
		}
	}));
}
#endif

#undef LOCTEXT_NAMESPACE
