#include "HappinessMCPInputToolset.h"

#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "LevelEditor.h"
#include "SLevelViewport.h"
#include "Widgets/SViewport.h"
#include "Widgets/SWindow.h"

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

	FLevelEditorModule* LevelEditor = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor"));
	if (!LevelEditor)
	{
		return false;
	}

	TSharedPtr<ILevelEditor> LevelEditorInstance = LevelEditor->GetFirstLevelEditor();
	TSharedPtr<SLevelViewport> LevelViewport = LevelEditorInstance.IsValid()
		? LevelEditorInstance->GetActiveViewportInterface()
		: nullptr;
	if (!LevelViewport.IsValid() || !LevelViewport->HasPlayInEditorViewport())
	{
		return false;
	}

	TSharedPtr<SViewport> ViewportWidget = LevelViewport->GetViewportWidget().Pin();
	if (!ViewportWidget.IsValid())
	{
		return false;
	}

	const FGeometry& Geometry = ViewportWidget->GetCachedGeometry();
	const FVector2D ScreenPosition = Geometry.GetAbsolutePositionAtCoordinates(FVector2D(X, Y));
	TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(ViewportWidget.ToSharedRef());
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
