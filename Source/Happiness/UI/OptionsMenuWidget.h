#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OptionsMenuWidget.generated.h"

class UButton;
class UFont;
class UProgressBar;
class USlider;
class UTexture2D;
class UVerticalBox;

/** Garden layout for WBP_Options. Its Blueprint still owns settings, Apply, and Cancel. */
UCLASS()
class HAPPINESS_API UOptionsMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UOptionsMenuWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(Transient) TObjectPtr<USlider> EffectsSlider;
	UPROPERTY(Transient) TObjectPtr<USlider> MusicSlider;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> SleepToggle;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> HintToggle;
	UPROPERTY(Transient) TObjectPtr<UButton> CancelAction;
	UPROPERTY(Transient) TObjectPtr<UButton> ApplyAction;

	/** Add future settings here in the Blueprint designer; these join the scrolling rows. */
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> ExtraSettingsHost;

private:
	void BuildLayout();
	UPROPERTY() TObjectPtr<UTexture2D> PanelTexture;
	UPROPERTY() TObjectPtr<UTexture2D> ButtonsTexture;
	UPROPERTY() TObjectPtr<UTexture2D> UncheckedTexture;
	UPROPERTY() TObjectPtr<UTexture2D> CheckedTexture;
	UPROPERTY() TObjectPtr<UFont> OptionsFont;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> SettingsRows;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> EffectsFill;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> MusicFill;
};
