#include "ClueWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"
#include "UI/GameCellWidget.h"
#include "UObject/ConstructorHelpers.h"

UClueWidget::UClueWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mask(TEXT("/Game/Happiness/UI/Materials/M_CellRoundedMask.M_CellRoundedMask"));
	IconMaskMaterial = Mask.Object;
}

void UClueWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	NotOverlay = GetWidgetFromName(TEXT("Overlay_Not"));
	if (NotOverlay)
	{
		// Slate rounds each corner of a drawn image to a whole pixel after its render transform, so a turning mark's
		// corners would snap independently and the mark would seem to pulse in size
		NotOverlay->SetPixelSnapping(EWidgetPixelSnapping::Disabled);
	}
	if (IconRetainer || !IconMaskMaterial)
	{
		return;
	}

	// The icon box: the canvas child holding the clue's icon list
	UWidget* Icons = GetWidgetFromName(TEXT("HorizontalBox"));
	if (!Icons)
	{
		Icons = GetWidgetFromName(TEXT("VerticalBox"));
	}
	UWidget* IconBox = Icons ? Icons->GetParent() : nullptr;
	UCanvasPanelSlot* BoxSlot = IconBox ? Cast<UCanvasPanelSlot>(IconBox->Slot) : nullptr;
	UCanvasPanel* Canvas = BoxSlot ? Cast<UCanvasPanel>(BoxSlot->Parent) : nullptr;
	if (!Canvas)
	{
		return;
	}

	// The retainer takes the icon box's place (same slot, same draw order), with the box inside it
	const FAnchorData Layout = BoxSlot->GetLayout();
	const bool bAutoSize = BoxSlot->GetAutoSize();
	const int32 ZOrder = BoxSlot->GetZOrder();
	const int32 Index = Canvas->GetChildIndex(IconBox);

	IconRetainer = WidgetTree->ConstructWidget<UGameCellIconRetainer>(UGameCellIconRetainer::StaticClass(), TEXT("IconRetainer"));
	IconRetainer->SetTextureParameter(TEXT("Content"));
	IconRetainer->SetEffectMaterial(IconMaskMaterial);
	IconRetainer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	IconBox->RemoveFromParent();
	IconRetainer->SetContent(IconBox);
	if (UCanvasPanelSlot* RetainerSlot = Cast<UCanvasPanelSlot>(Canvas->InsertChildAt(Index, IconRetainer)))
	{
		RetainerSlot->SetLayout(Layout);
		RetainerSlot->SetAutoSize(bAutoSize);
		RetainerSlot->SetZOrder(ZOrder);
	}
}

void UClueWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (NotOverlay && NotOverlay->IsVisible() && NotTurnSeconds > 0.f)
	{
		// From the app clock, so every clue's mark keeps the same angle
		NotOverlay->SetRenderTransformAngle(float(FMath::Fmod(FApp::GetCurrentTime(), double(NotTurnSeconds)) / NotTurnSeconds * 360.0));
	}
	const FVector2D Size = IconRetainer ? IconRetainer->GetCachedGeometry().GetLocalSize() : FVector2D::ZeroVector;
	if (Size.X > 0.f && Size.Y > 0.f && !Size.Equals(MaskSize, 0.1f))
	{
		if (UMaterialInstanceDynamic* Mask = IconRetainer->GetEffectMaterial())
		{
			Mask->SetVectorParameterValue(TEXT("CellSize"), FLinearColor(Size.X, Size.Y, 0.f, 0.f));
			Mask->SetScalarParameterValue(TEXT("CornerRadius"), IconCornerRadius);
			MaskSize = Size;
			IconRetainer->RequestRender();
		}
	}
}
