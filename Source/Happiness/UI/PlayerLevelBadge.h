#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerLevelBadge.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

/**
 * The player's level on the mode screen (top right): a sun with the level (PC_Happiness.HappinessLevel) in its
 * center, ringed by one icon per lesson tree column. A column's icon shows once every clue in it has completed
 * Campaign Master Mode (UCampaignSubsystem::IsMasterColumnCompleted); until then its place is empty.
 * Builds its own widgets (no Blueprint layout). Without art (SunTexture, IconTextures) it draws placeholders.
 * Display only: it never takes a touch, so the menu under it keeps working.
 */
UCLASS()
class HAPPINESS_API UPlayerLevelBadge : public UUserWidget
{
	GENERATED_BODY()

public:
	UPlayerLevelBadge(const FObjectInitializer& ObjectInitializer);

	/** Width and height of the whole badge, icons included */
	static constexpr float BadgeSize = 300.f;

	/** Reads the level and the unlocked icons again */
	UFUNCTION(BlueprintCallable, Category = "Player Level")
	void Refresh();

	/** The sun behind the level; a gold disc while unset */
	UPROPERTY(EditAnywhere, Category = "Player Level")
	TObjectPtr<UTexture2D> SunTexture;

	/** One icon per tree column, clockwise from the top; a placeholder dot for any unset */
	UPROPERTY(EditAnywhere, Category = "Player Level")
	TArray<TObjectPtr<UTexture2D>> IconTextures;

	/** Four columns by three rows, clockwise from the top; used when IconTextures has no override. */
	UPROPERTY(EditAnywhere, Category = "Player Level")
	TObjectPtr<UTexture2D> RewardAtlasTexture;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildLayout();

	UFUNCTION()
	void HandleProgressChanged();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> Icons;
};
