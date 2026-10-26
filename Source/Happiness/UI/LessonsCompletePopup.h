#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LessonsCompletePopup.generated.h"

class UButton;
class UFont;
class UImage;
class UTexture2D;
class UWidget;

/**
 * Congratulations for completing every lesson, over the whole mode screen (UHappinessModeWidget makes it the first
 * time it's needed): what the player unlocked, the Garden Visitors icon set and Campaign mode, then Continue closes
 * it. Built in code, in the confirm dialog's garden plaque style.
 */
UCLASS()
class HAPPINESS_API ULessonsCompletePopup : public UUserWidget
{
	GENERATED_BODY()

public:
	ULessonsCompletePopup(const FObjectInitializer& ObjectInitializer);

	/** Fade in. CampaignArt: the mode screen's Campaign picture, shown on the Campaign card (none: no picture). */
	void Show(const FSlateBrush* CampaignArt);

	void Hide();

	/** The plaque, without the dimmed screen around it (none until it's built) */
	UWidget* GetWindow() const { return Window; }

	/** Seconds the popup takes to fade in */
	UPROPERTY(EditAnywhere, Category = "Lessons Complete")
	float FadeInTime = 0.35f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildLayout();

	UFUNCTION()
	void HandleContinueClicked();

	UPROPERTY() TObjectPtr<UTexture2D> PlaqueTexture;
	UPROPERTY() TObjectPtr<UTexture2D> ButtonTexture;
	UPROPERTY() TObjectPtr<UFont> PlaqueFont;
	/** A few of the Garden Visitors, for its card */
	UPROPERTY() TArray<TObjectPtr<UTexture2D>> VisitorTextures;

	UPROPERTY(Transient) TObjectPtr<UWidget> Window;
	UPROPERTY(Transient) TObjectPtr<UImage> CampaignImage;
	UPROPERTY(Transient) TObjectPtr<UButton> ContinueButton;

	/** Seconds since Show, while fading in (below 0: not fading) */
	float FadeTime = -1.f;
};
