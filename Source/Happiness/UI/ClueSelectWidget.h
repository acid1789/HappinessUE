#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "HappinessClassic/CampaignLesson.h"
#include "ClueSelectWidget.generated.h"

class UButton;
class UClueSelectWidget;
class USoundBase;
class UTextBlock;
class UUniformGridPanel;
class UWidget;

/** Routes one clue checkbox's click to the widget with its clue type (UButton::OnClicked has no parameters) */
UCLASS()
class UClueSelectToggle : public UObject
{
	GENERATED_BODY()

public:
	ECampaignLesson Clue = ECampaignLesson::Given;
	TWeakObjectPtr<UUserWidget> CheckBox;
	TWeakObjectPtr<UClueSelectWidget> Owner;

	UFUNCTION()
	void HandleClicked();
};

/**
 * Free play clue selection, for the free play setup screen: a button showing which clue types are included
 * ("All", "12 of 19") that opens a panel with a checkbox per clue type, All / None buttons and Done.
 * The checkboxes are the game's own WBP_CheckBox (CheckBoxClass), the same as on the options screen.
 * Reads and writes UFreePlaySubsystem, which saves the selection.
 */
UCLASS(Abstract)
class HAPPINESS_API UClueSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UClueSelectWidget(const FObjectInitializer& ObjectInitializer);

	/** Opens the panel */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> OpenButton;

	/** On OpenButton: "All", "12 of 19" or "Givens only" */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> SummaryText;

	/** The panel with the checkboxes; collapsed until opened */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWidget> Panel;

	/** A checkbox per clue type is created in here */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> ClueGrid;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> AllButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> NoneButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> DoneButton;

	/** Open the panel as soon as the widget is constructed (and show it open in the designer, to edit its layout) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clue Select")
	bool bStartOpen = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clue Select")
	int32 Columns = 2;

	/**
	 * The checkbox widget, WBP_CheckBox: a Blueprint with a Text variable (its label, shown by TextBlock_67), a
	 * Checked variable, a Check(ShouldBeChecked) function and a Button_53 that toggles it when pressed
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clue Select")
	TSubclassOf<UUserWidget> CheckBoxClass;

	/** Font size for the checkbox labels (WBP_CheckBox's is sized for the options screen); 0 keeps it */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clue Select")
	float LabelFontSize = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clue Select")
	FMargin CluePadding = FMargin(8.f, 2.f);

	/** Played when a button is pressed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clue Select")
	TObjectPtr<USoundBase> ButtonSound;

	UFUNCTION(BlueprintCallable, Category = "Clue Select")
	void OpenPanel();

	UFUNCTION(BlueprintCallable, Category = "Clue Select")
	void ClosePanel();

	/** Re-read the selection into the checkboxes and summary */
	UFUNCTION(BlueprintCallable, Category = "Clue Select")
	void Refresh();

	void SetClueIncluded(ECampaignLesson Clue, bool bIncluded);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleOpenClicked();

	UFUNCTION()
	void HandleAllClicked();

	UFUNCTION()
	void HandleNoneClicked();

	UFUNCTION()
	void HandleDoneClicked();

	UFUNCTION()
	void HandleCluesChanged();

	void BuildCheckBoxes();
	void PlayButtonSound() const;
	class UFreePlaySubsystem* GetFreePlay() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UUserWidget>> CheckBoxes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UClueSelectToggle>> Toggles;
};
