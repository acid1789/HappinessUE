#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HappinessModeWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHappinessModeEvent);

/**
 * Happiness mode select, between the game select screen and playing: Classic (the classic puzzle flow)
 * or Lessons (the campaign tree). The owner reacts to the events; the screen just hides itself.
 */
UCLASS(Abstract)
class HAPPINESS_API UHappinessModeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> ClassicButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> LessonsButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackButton;

	UPROPERTY(BlueprintAssignable, Category = "Happiness Mode")
	FOnHappinessModeEvent OnClassicChosen;

	UPROPERTY(BlueprintAssignable, Category = "Happiness Mode")
	FOnHappinessModeEvent OnLessonsChosen;

	/** Back pressed: return to the game select screen */
	UPROPERTY(BlueprintAssignable, Category = "Happiness Mode")
	FOnHappinessModeEvent OnClosed;

	UFUNCTION(BlueprintCallable, Category = "Happiness Mode")
	void Show();

	UFUNCTION(BlueprintCallable, Category = "Happiness Mode")
	void Hide();

protected:
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void HandleClassicClicked();

	UFUNCTION()
	void HandleLessonsClicked();

	UFUNCTION()
	void HandleBackClicked();
};
