#include "VersionLabel.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Styling/CoreStyle.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSafeZone.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// Above every screen and popup (those use Z orders up to a few hundred).
	constexpr int32 VersionLabelZOrder = 10000;

	FString VersionText()
	{
		int32 StoreVersion = 0;
		GConfig->GetInt(TEXT("/Script/AndroidRuntimeSettings.AndroidRuntimeSettings"), TEXT("StoreVersion"), StoreVersion, GEngineIni);
		const EBuildConfiguration Config = FApp::GetBuildConfiguration();
		const TCHAR* ConfigName = Config == EBuildConfiguration::Shipping ? TEXT("Shipping")
			: Config == EBuildConfiguration::Development ? TEXT("Dev")
			: LexToString(Config);
		return FString::Printf(TEXT("v%d %s"), StoreVersion, ConfigName);
	}
}

void UVersionLabelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// The game viewport may not exist yet when the game instance starts, and loading a map clears viewport widgets.
	ViewportCreatedHandle = UGameViewportClient::OnViewportCreated().AddUObject(this, &UVersionLabelSubsystem::AddToViewport);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddWeakLambda(this, [this](UWorld*) { AddToViewport(); });
	AddToViewport();
}

void UVersionLabelSubsystem::Deinitialize()
{
	UGameViewportClient::OnViewportCreated().Remove(ViewportCreatedHandle);
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	RemoveFromViewport();
	Label.Reset();
	Super::Deinitialize();
}

void UVersionLabelSubsystem::AddToViewport()
{
	UGameInstance* Game = GetGameInstance();
	UGameViewportClient* Viewport = Game ? Game->GetGameViewportClient() : nullptr;
	if (!Viewport)
	{
		return;
	}
	if (!Label)
	{
		FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 7);
		Label = SNew(SSafeZone)
			.Visibility(EVisibility::HitTestInvisible)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			.Padding(FMargin(4.f, 2.f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(VersionText()))
				.Font(Font)
				.ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.75f))
				.ShadowOffset(FVector2D(1.f, 1.f))
				.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f))
			];
	}
	// Remove first so a re-add (after a map load, or a second callback) never leaves two labels.
	Viewport->RemoveViewportWidgetContent(Label.ToSharedRef());
	Viewport->AddViewportWidgetContent(Label.ToSharedRef(), VersionLabelZOrder);
	AddedTo = Viewport;
}

void UVersionLabelSubsystem::RemoveFromViewport()
{
	if (UGameViewportClient* Viewport = AddedTo.Get(); Viewport && Label)
	{
		Viewport->RemoveViewportWidgetContent(Label.ToSharedRef());
	}
	AddedTo.Reset();
}
