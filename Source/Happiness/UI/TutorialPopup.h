#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/RichTextBlockDecorator.h"
#include "TutorialPopup.generated.h"

class UButton;
class UFont;
class URichTextBlock;
class USizeBox;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTutorialPopupNext);

/**
 * A tutorial text popup (made in code): a cream panel with a gold rim and the lessons' serif text, wrapped to Width.
 * It fades in over FadeSeconds once shown. A Next button (bottom right) appears NextDelay seconds after it's shown and
 * fires OnNext (labelled NextLabel); a second button (SecondLabel), left of it, comes with it and fires OnSecond, and a
 * third (ThirdLabel), left of that, fires OnThird; nothing else in it takes input. Placed by the tutorial (UGamePanelWidget's tutorial focus).
 *
 * The message is rich text: <icon id="Name"/> shows the icon given to SetIcon under that name, in the line, a little
 * taller than the text (UTutorialIconDecorator).
 */
UCLASS()
class HAPPINESS_API UTutorialPopup : public UUserWidget
{
	GENERATED_BODY()

public:
	UTutorialPopup(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	float FadeSeconds = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	int32 FontSize = 28;

	/** Seconds after it's shown before its Next button appears (and can be pressed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	float NextDelay = 3.f;

	/** Whether it has a Next button at all (false: the step moves on some other way) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	bool bShowNext = true;

	/** Without Next (bShowNext false): still keep its room (hidden), so ShowNextNow doesn't resize the popup */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	bool bReserveNext = false;

	/** Shows the Next button at once (no delay or fade), pressable */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ShowNextNow();

	/** The Next button's label (empty: "Next"). Give it before the popup is shown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FText NextLabel;

	/** A second button, left of Next, with this label (empty: none). It shows with Next and fires OnSecond. Give it
	 *  before the popup is shown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FText SecondLabel;

	/** A third button, left of the second, with this label (empty: none). It shows with Next and fires OnThird. Give
	 *  it before the popup is shown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FText ThirdLabel;

	/** The Next button was pressed */
	UPROPERTY(BlueprintAssignable, Category = "Tutorial")
	FOnTutorialPopupNext OnNext;

	/** The second button (SecondLabel) was pressed */
	UPROPERTY(BlueprintAssignable, Category = "Tutorial")
	FOnTutorialPopupNext OnSecond;

	/** The third button (ThirdLabel) was pressed */
	UPROPERTY(BlueprintAssignable, Category = "Tutorial")
	FOnTutorialPopupNext OnThird;

	/** The text it shows (rich text: <icon id="Name"/> for an icon given to SetIcon) */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SetMessage(const FText& Message);

	/** An icon the message can show as <icon id="Id"/>. Give them before the message. */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SetIcon(FName Id, UTexture2D* Texture);

	/** The brush of an icon given to SetIcon, or null (UTutorialIconDecorator) */
	const FSlateBrush* FindIcon(FName Id) const;

	/** The panel's width, in the screen's units; the text wraps inside it */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SetWidth(float Width);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildLayout();

	UPROPERTY()
	TObjectPtr<UFont> TextFont;

	UPROPERTY()
	TObjectPtr<USizeBox> Box;

	UPROPERTY()
	TObjectPtr<URichTextBlock> Text;

	/** SetIcon's textures (kept loaded) and their brushes (shared: the text's image widgets point at them) */
	UPROPERTY()
	TArray<TObjectPtr<UTexture2D>> IconTextures;
	TMap<FName, TSharedPtr<FSlateBrush>> IconBrushes;

	UPROPERTY()
	TObjectPtr<UButton> NextButton;

	/** SecondLabel's button, if it has one */
	UPROPERTY()
	TObjectPtr<UButton> SecondButton;

	/** ThirdLabel's button, if it has one */
	UPROPERTY()
	TObjectPtr<UButton> ThirdButton;

	UFUNCTION()
	void HandleNextClicked();

	UFUNCTION()
	void HandleSecondClicked();

	UFUNCTION()
	void HandleThirdClicked();

	/** A sage button with ivory Label text (Next's look) */
	UButton* MakeButton(const FText& Label);

	FText PendingMessage;
	float PendingWidth = 560.f;
	/** Seconds since it was shown */
	float FadeTime = 0.f;
};

/** UTutorialPopup's <icon id="Name"/> in its message: the popup's icon of that name, a little taller than the text */
UCLASS()
class HAPPINESS_API UTutorialIconDecorator : public URichTextBlockDecorator
{
	GENERATED_BODY()

public:
	UTutorialIconDecorator(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
	virtual TSharedPtr<ITextDecorator> CreateDecorator(URichTextBlock* InOwner) override;
};
