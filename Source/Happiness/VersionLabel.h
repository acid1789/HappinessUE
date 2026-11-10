#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "VersionLabel.generated.h"

/**
 * A tiny build label ("v13 Shipping") in the top-left corner, above every screen, so a tester can tell which build is
 * running. The version is the Play version code (StoreVersion in DefaultEngine.ini), baked into the build's config.
 * Plain Slate added straight to the game viewport, so no widget or screen needs to know about it; it ignores input.
 */
UCLASS()
class HAPPINESS_API UVersionLabelSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void AddToViewport();
	void RemoveFromViewport();

	TSharedPtr<class SWidget> Label;
	TWeakObjectPtr<class UGameViewportClient> AddedTo;
	FDelegateHandle ViewportCreatedHandle;
	FDelegateHandle PostLoadMapHandle;
};
