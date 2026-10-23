#include "HappinessMCPInputToolset.h"

#include "Editor.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Input/Events.h"
#include "UObject/UObjectIterator.h"
#include "LevelEditor.h"
#include "SLevelViewport.h"
#include "Widgets/SViewport.h"
#include "Widgets/SWindow.h"

namespace
{
	/** The window of the active Play-In-Editor viewport, and the screen position at normalized (X, Y) in it */
	TSharedPtr<SWindow> FindPIEViewportPoint(float X, float Y, FVector2D& OutScreenPosition)
	{
		if (!FSlateApplication::IsInitialized() || X < 0.f || X > 1.f || Y < 0.f || Y > 1.f)
		{
			return nullptr;
		}

		FLevelEditorModule* LevelEditor = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor"));
		if (!LevelEditor)
		{
			return nullptr;
		}

		TSharedPtr<ILevelEditor> LevelEditorInstance = LevelEditor->GetFirstLevelEditor();
		TSharedPtr<SLevelViewport> LevelViewport = LevelEditorInstance.IsValid()
			? LevelEditorInstance->GetActiveViewportInterface()
			: nullptr;
		if (!LevelViewport.IsValid() || !LevelViewport->HasPlayInEditorViewport())
		{
			return nullptr;
		}

		TSharedPtr<SViewport> ViewportWidget = LevelViewport->GetViewportWidget().Pin();
		if (!ViewportWidget.IsValid())
		{
			return nullptr;
		}

		const FGeometry& Geometry = ViewportWidget->GetCachedGeometry();
		OutScreenPosition = Geometry.GetAbsolutePositionAtCoordinates(FVector2D(X, Y));
		return FSlateApplication::Get().FindWidgetWindow(ViewportWidget.ToSharedRef());
	}
}

bool UHappinessMCPInputToolset::ClickViewport(float X, float Y, const FString& Button)
{
	if (!FSlateApplication::IsInitialized() || X < 0.f || X > 1.f || Y < 0.f || Y > 1.f)
	{
		return false;
	}

	FKey MouseButton;
	if (Button.Equals(TEXT("Left"), ESearchCase::IgnoreCase))
	{
		MouseButton = EKeys::LeftMouseButton;
	}
	else if (Button.Equals(TEXT("Right"), ESearchCase::IgnoreCase))
	{
		MouseButton = EKeys::RightMouseButton;
	}
	else if (Button.Equals(TEXT("Middle"), ESearchCase::IgnoreCase))
	{
		MouseButton = EKeys::MiddleMouseButton;
	}
	else
	{
		return false;
	}

	FVector2D ScreenPosition;
	TSharedPtr<SWindow> Window = FindPIEViewportPoint(X, Y, ScreenPosition);
	if (!Window.IsValid())
	{
		return false;
	}

	FSlateApplication& SlateApplication = FSlateApplication::Get();
	const FVector2D PreviousPosition = SlateApplication.GetCursorPos();
	const TSet<FKey> PressedButtons{ MouseButton };
	const FModifierKeysState Modifiers;

	SlateApplication.ProcessMouseMoveEvent(FPointerEvent(
		0, 0, ScreenPosition, PreviousPosition, TSet<FKey>(), FKey(), 0.f, Modifiers), true);
	SlateApplication.ProcessMouseButtonDownEvent(Window->GetNativeWindow(), FPointerEvent(
		0, 0, ScreenPosition, ScreenPosition, PressedButtons, MouseButton, 0.f, Modifiers));
	SlateApplication.ProcessMouseButtonUpEvent(FPointerEvent(
		0, 0, ScreenPosition, ScreenPosition, TSet<FKey>(), MouseButton, 0.f, Modifiers));
	return true;
}

bool UHappinessMCPInputToolset::TapViewport(float X, float Y)
{
	FVector2D ScreenPosition;
	TSharedPtr<SWindow> Window = FindPIEViewportPoint(X, Y, ScreenPosition);
	if (!Window.IsValid())
	{
		return false;
	}
	// A finger: touch events, as a phone sends them (no hover, pointer index 0 like the first finger)
	FSlateApplication& SlateApplication = FSlateApplication::Get();
	SlateApplication.ProcessTouchStartedEvent(Window->GetNativeWindow(), FPointerEvent(
		0, 0, ScreenPosition, ScreenPosition, 1.f, true));
	SlateApplication.ProcessTouchEndedEvent(FPointerEvent(
		0, 0, ScreenPosition, ScreenPosition, 0.f, true));
	return true;
}

bool UHappinessMCPInputToolset::ExecPIECommand(const FString& Command)
{
	UWorld* World = GEditor ? GEditor->PlayWorld.Get() : nullptr;
	if (!World || Command.IsEmpty())
	{
		return false;
	}
	// Through the player controller when there is one, like typing it in the game's console
	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		PC->ConsoleCommand(Command, true);
		return true;
	}
	return GEngine->Exec(World, *Command);
}

int32 UHappinessMCPInputToolset::SetPIEProperty(const FString& ClassName, const FString& PropertyName, const FString& Value)
{
	UWorld* World = GEditor ? GEditor->PlayWorld.Get() : nullptr;
	UClass* Class = FindFirstObject<UClass>(*ClassName, EFindFirstObjectOptions::None);
	FProperty* Property = Class ? FindFProperty<FProperty>(Class, *PropertyName) : nullptr;
	if (!World || !Property)
	{
		return -1;
	}
	int32 Count = 0;
	for (TObjectIterator<UObject> It; It; ++It)
	{
		UObject* Object = *It;
		if (Object->IsA(Class) && !Object->HasAnyFlags(RF_ClassDefaultObject) && Object->GetWorld() == World
			&& Property->ImportText_InContainer(*Value, Object, Object, PPF_None))
		{
			++Count;
		}
	}
	return Count;
}
