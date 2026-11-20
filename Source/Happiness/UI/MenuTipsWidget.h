#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MenuTipsWidget.generated.h"

class UFont;
class UImage;
class USizeBox;
class UTextBlock;
class UTexture2D;
class UWidget;

/**
 * Tips on the main menu (the mode screen, under the tiles): one at a time, picked at random from a pool, changing
 * every TipSeconds while the menu shows (it only ticks then), with a short fade between tips. Two kinds:
 *   - a written tip about playing (Tips, in the .cpp), e.g. "When you are done with a clue, you can hide it with
 *     the Hide Clue button"
 *   - an icon and its name, "[Cosmos] - Cosmos": a random icon from the player's icon sets that are on
 *     (UIconSetSubsystem), named after its texture ("BlueWildFlower" -> "Blue Wild Flower")
 * On the clue description's frame (T_HintReadingPanel, as UClueDescriptionWidget draws it).
 * Builds its own widgets (no Blueprint layout). Display only: it never takes a touch.
 */
UCLASS()
class HAPPINESS_API UMenuTipsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UMenuTipsWidget(const FObjectInitializer& ObjectInitializer);

	/** Seconds each tip shows, its fades included */
	UPROPERTY(EditAnywhere, Category = "Tips")
	float TipSeconds = 10.f;

	/** Seconds of each fade (out, then the next tip in) */
	UPROPERTY(EditAnywhere, Category = "Tips")
	float FadeSeconds = 0.3f;

	/** How often a tip is an icon and its name rather than a written tip (0 to 1) */
	UPROPERTY(EditAnywhere, Category = "Tips")
	float IconTipChance = 0.35f;

	/** The written tips (also shown in the puzzle screen's clue info while no clue is selected, UClueDescriptionWidget) */
	static TArray<FText> GetWrittenTips();
	/** An icon from a set that is on, and its name; false if there are none */
	static bool PickIcon(const UObject* WorldContext, UTexture2D*& OutTexture, FText& OutName);
	/** The text beside an icon tip's icon: "- Cosmos" */
	static FText MakeIconTipText(const FText& IconName);
	/** The "Show Tips" option (PC_Happiness Happiness_ShowTips, saved in SG_Settings); on if the controller doesn't have it */
	static bool AreTipsEnabled(const UUserWidget& Widget);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildLayout();
	/** Show another tip, not the one just shown */
	void ShowNextTip();

	UPROPERTY(Transient) TObjectPtr<UWidget> Content;
	UPROPERTY(Transient) TObjectPtr<USizeBox> IconBox;
	UPROPERTY(Transient) TObjectPtr<UImage> Icon;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TipText;
	UPROPERTY(Transient) TObjectPtr<UFont> TipFont;
	/** The frame behind the tips: the clue description's (T_HintReadingPanel) */
	UPROPERTY() TObjectPtr<UTexture2D> FrameTexture;

	/** Seconds into the current tip */
	float TipTime = 0.f;
	bool bSwapped = false;
	int32 LastTip = INDEX_NONE;
};
