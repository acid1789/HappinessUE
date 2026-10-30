#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HappinessClassic/IconSets.h"
#include "IconSetsPanel.generated.h"

class UButton;
class UCanvasPanel;
class UFont;
class UVerticalBox;
class UScrollBox;
class UTextBlock;
class UTexture2D;
class USoundBase;
class UIconSetsPanel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIconSetPurchaseRequested, const FString&, ProductId);

/** Routes a row's selection or status button to its set. */
UCLASS()
class UIconSetCardClick : public UObject
{
	GENERATED_BODY()
public:
	TWeakObjectPtr<UIconSetsPanel> Panel;
	int32 Index = INDEX_NONE;
	bool bAction = false;
	UFUNCTION() void HandleClicked();
};

/** A scrolling collection list and a large preview. Hidden locked sets come filtered by the subsystem. */
UCLASS()
class HAPPINESS_API UIconSetsPanel : public UUserWidget
{
	GENERATED_BODY()
public:
	UIconSetsPanel(const FObjectInitializer& ObjectInitializer);
	void Show();
	void Hide();
	void SelectSet(int32 Index);
	void ToggleSet(int32 Index);

	/** Store integration supplies products/ownership and handles checkout; the art never grants a purchase. */
	UPROPERTY(BlueprintAssignable, Category = "Icon Sets")
	FOnIconSetPurchaseRequested OnPurchaseRequested;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient) int32 PreviewIconSetIndex = -1;
#endif
#if WITH_EDITOR
	/** Render-only sample states; never writes player progress or set choices. */
	void PreviewCatalogue(int32 Index);
#endif
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
private:
	void BuildLayout();
	void Refresh();
	void UpdateDetail();
	void UpdateStatus();
	TArray<FIconSetEntry> GetEntries() const;
	UWidget* MakeRow(int32 Index, const FIconSetEntry& Entry);
	UButton* MakeStateButton(UCanvasPanel* Canvas, const FIconSetEntry& Entry, int32 Index,
		FVector2D Position, FVector2D Extent, int32 FontSize);
	FSlateBrush StateBrush(const FIconSetEntry& Entry) const;
	FText StateLabel(const FIconSetEntry& Entry) const;
	void PlaySound(USoundBase* Sound) const;
	UFUNCTION() void HandleCloseClicked();

	UPROPERTY() TObjectPtr<UTexture2D> PanelTexture;
	UPROPERTY() TObjectPtr<UTexture2D> ButtonTexture;
	UPROPERTY() TObjectPtr<UFont> PanelFont;
	UPROPERTY() TObjectPtr<USoundBase> AcceptSound;
	UPROPERTY() TObjectPtr<USoundBase> CancelSound;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> Scroll;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> CardRows;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Detail;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TArray<TObjectPtr<UIconSetCardClick>> CardClicks;
	UPROPERTY(Transient) TArray<TObjectPtr<UIconSetCardClick>> DetailClicks;
	UPROPERTY(Transient) TArray<TObjectPtr<UObject>> CardSets;
	TArray<FIconSetEntry> PreviewEntries;
	FName SelectedId;
	float MessageSeconds = 0.f;
};
