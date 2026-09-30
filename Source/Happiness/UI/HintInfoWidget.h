#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HappinessClassic/ClueHelp.h"
#include "HintInfoWidget.generated.h"

class UHint;
class UPanelWidget;
class UTextBlock;
class UWidget;

/**
 * Hint info panel: while a hint is showing, explains it:
 *   - the clue it comes from, as the help line shows it ("[icon] is directly left of [icon]")
 *   - why the hint follows and what to do (UHint::GetExplanation), e.g. "Since [Daisy] is in this column, [Top] can't be here"
 *
 * The widget covers the vertical clue strip and stays laid out (only Box is hidden), so it always knows where the
 * hinted clue is: Box goes on the side of the clue with more room and stops short of it.
 * Clue and action lines are built from the same WBP_HelpText / WBP_HelpIcon pieces as the help line.
 */
UCLASS(Abstract)
class HAPPINESS_API UHintInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UHintInfoWidget(const FObjectInitializer& ObjectInitializer);

	/** The visible panel; placed over the left or right part of this widget */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWidget> Box;

	/** The hinted clue, as text and icons */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPanelWidget> ClueLine;

	/** Why the hint follows and what to do, as text and icons */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPanelWidget> ActionLine;

	/** Text piece of a line: WBP_HelpText (its TextBlock_54 shows the text) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint Info")
	TSubclassOf<UUserWidget> HelpTextClass;

	/** Icon piece of a line: WBP_HelpIcon (its Image_28 shows the icon) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint Info")
	TSubclassOf<UUserWidget> HelpIconClass;

	/** Widest Box gets, as a fraction of this widget */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint Info")
	float BoxWidthFraction = 0.55f;

	/** How far a text piece starting with punctuation (", which...") moves in against the piece before it */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint Info")
	float PunctuationTuck = 6.f;

	/** Space kept between Box and the hinted clue */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint Info")
	float ClueGap = 10.f;

	/**
	 * Show the panel for Hint. IconSets are the puzzle's icon sets, one per row (DA_IconSet: an Icons array);
	 * HintClue is the hinted clue's widget, which the panel keeps clear of.
	 */
	UFUNCTION(BlueprintCallable, Category = "Hint Info")
	void ShowHint(UHint* Hint, const TArray<UObject*>& IconSets, UWidget* HintClue);

	UFUNCTION(BlueprintCallable, Category = "Hint Info")
	void HideHint();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildLine(UPanelWidget* Line, const FClueHelp& Help, const TArray<UObject*>& IconSets);
	void PlaceBox(UWidget* HintClue);

	/** The hinted clue while the box is shown; the box is placed again each frame (a clue just unhidden has no
	 *  layout yet when the hint is shown) */
	TWeakObjectPtr<UWidget> ShownClue;
};
