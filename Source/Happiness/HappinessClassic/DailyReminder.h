#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DailyReminder.generated.h"

/**
 * The daily puzzle reminder: a phone notification each day at ReminderHour (local time) saying the new daily puzzle is
 * ready. The next DaysAhead days are scheduled whenever the app starts, comes back to the foreground or the daily
 * changes (a solve), so reminders stop after that many days without opening the app. A day whose puzzle is already
 * solved gets none, and the first reminder mentions the streak while it is still going. With a streak of at least
 * EveningMinStreak days, a second reminder at EveningHour on the next day not yet solved; solving cancels it.
 *
 * Permission: asked for once, after the first daily solve (iOS always asks; Android 13+ asks for POST_NOTIFICATIONS,
 * declared in Happiness_UPL.xml). Players turn reminders off in the phone's settings. Android and iOS only.
 */
UCLASS(Config = Game)
class HAPPINESS_API UDailyReminderSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** The local hour (0-23) and minute the reminder fires each day */
	UPROPERTY(Config)
	int32 ReminderHour = 1;

	UPROPERTY(Config)
	int32 ReminderMinute = 0;

	/** How many days of reminders to schedule ahead */
	UPROPERTY(Config)
	int32 DaysAhead = 7;

	/** The evening reminder: when (local time), for a day whose puzzle isn't solved yet, and the streak it takes */
	UPROPERTY(Config)
	int32 EveningHour = 20;

	UPROPERTY(Config)
	int32 EveningMinute = 0;

	/** More than a week */
	UPROPERTY(Config)
	int32 EveningMinStreak = 8;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Replace the scheduled reminders with the next DaysAhead days' */
	UFUNCTION()
	void Reschedule();

	/** Testing (console Happiness.Reminder.Test [seconds]): a reminder in a few seconds */
	void DebugTest(int32 Seconds);

private:
	void HandleForeground();
	/** Ask for permission to notify, the first time today's daily is solved */
	void RequestPermissionOnce();
	/** Remove reminders already shown and the app icon's badge (iOS) */
	void ClearShown();

	FDelegateHandle ForegroundHandle;
};
