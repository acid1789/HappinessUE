#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GamePanelWidget.generated.h"

class UCanvasPanel;
class UGameCellWidget;
class UPuzzle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFireworksFinished);

/**
 * Keeps the puzzle's columns sized to its two rows of candidate icons, and celebrates a solved puzzle before its end
 * screen: every icon bursts into confetti (UGameCellWidget::PlayFirework) in a random order, spread evenly over
 * FireworksDuration. EndScreenDelay after the first burst the screen is told to show the end screen
 * (OnFireworksFinished); the rest go off behind it. The icons stay hidden until the puzzle is no longer complete
 * (restarted, or another puzzle started).
 */
UCLASS(Abstract)
class HAPPINESS_API UGamePanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Seconds every icon bursts within */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fireworks")
	float FireworksDuration = 1.f;

	/** Seconds from the first burst to the end screen */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fireworks")
	float EndScreenDelay = 0.5f;

	/**
	 * Call when the puzzle is completed, before showing its end screen. Returns true if the end screen has to wait:
	 * the fireworks have started (or are still playing) and OnFireworksFinished follows. False to show it now: the
	 * board has mistakes, or the fireworks for this puzzle have just finished.
	 */
	UFUNCTION(BlueprintCallable, Category = "Fireworks")
	bool PlaySolvedFireworks(UPuzzle* Puzzle);

	/** The fireworks are over: show the end screen (call RefreshPuzzle again, which won't wait this time) */
	UPROPERTY(BlueprintAssignable, Category = "Fireworks")
	FOnFireworksFinished OnFireworksFinished;

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCanvasPanel> Border_44;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void UpdateFireworks(float DeltaTime);
	/** Shows the icons of the cells that have burst again */
	void EndFireworks();
	/** The puzzle the fireworks were for has every icon set */
	bool IsFireworksPuzzleComplete() const;

	/** Playing until the end screen is due (bursts may go on behind it: bBursting), then Finished until it shows */
	enum class EFireworks : uint8 { None, Playing, Finished };
	bool bBursting = false;
	EFireworks Fireworks = EFireworks::None;
	TWeakObjectPtr<UPuzzle> FireworksPuzzle;
	/** Seconds since the first burst, and between bursts (FireworksDuration over the number of cells) */
	float FireworksTime = 0.f;
	float FireworksInterval = 0.f;
	int32 NumFired = 0;
	/** The first update after starting only waits a frame, for the cells' effect widgets (PrepareFirework) */
	bool bFireworksPrepared = false;
	/** Cells still to burst, in order; and those that have, to show their icons again once the puzzle changes */
	TArray<TWeakObjectPtr<UGameCellWidget>> FireworkCells;
	TArray<TWeakObjectPtr<UGameCellWidget>> FiredCells;

	bool bSavedHintLayout = false;
	float NormalPanelTop = 0.f;
	ESlateVisibility NormalHintVisibility = ESlateVisibility::SelfHitTestInvisible;
};
