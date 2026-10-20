#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConfirmDialogWidget.generated.h"

class UTextBlock;
class UCanvasPanel;
class UTexture2D;
class UFont;

/** Smooth confirmation/cancel silhouettes and the plaque's small leaf ornament. */
UCLASS()
class HAPPINESS_API UConfirmDialogGlyph : public UWidget
{
	GENERATED_BODY()
public:
	int32 Symbol = 0;
	FLinearColor Color = FLinearColor::White;
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
};

/**
 * Parent of WBP_ConfirmDialog, the puzzle screen's yes/no dialog. Its question is the Blueprint's Message; this adds
 * an optional description below it saying what the action does, in a responsive garden plaque. The description
 * is cleared whenever the dialog closes, so dialogs that don't set one show none.
 */
UCLASS()
class HAPPINESS_API UConfirmDialogWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UConfirmDialogWidget(const FObjectInitializer& ObjectInitializer);

	/** Show this description below the question (empty: none). Set it after opening the dialog. */
	UFUNCTION(BlueprintCallable, Category = "Confirm Dialog")
	void SetDescription(FString Description);

	virtual void SetVisibility(ESlateVisibility InVisibility) override;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;

private:
	void BuildPlaqueLayout();
	UPROPERTY() TObjectPtr<UTexture2D> PlaqueTexture;
	UPROPERTY() TObjectPtr<UTexture2D> ButtonTexture;
	UPROPERTY() TObjectPtr<UTexture2D> ConfirmTexture;
	UPROPERTY() TObjectPtr<UFont> PlaqueFont;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> PlaqueRoot;
};
