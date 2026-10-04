#include "GameCellWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

UGameCellIconRetainer::UGameCellIconRetainer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InitRenderOnInvalidation(true);
	InitRenderOnPhase(false);
}

UGameCellWidget::UGameCellWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mask(TEXT("/Game/Happiness/UI/Materials/M_CellRoundedMask.M_CellRoundedMask"));
	IconMaskMaterial = Mask.Object;
}

void UGameCellWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (IconRetainer)
	{
		return;
	}
	UCanvasPanel* Root = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	UWidget* Candidates = GetWidgetFromName(TEXT("SmallIconBox"));
	UWidget* Resolved = GetWidgetFromName(TEXT("SizeBox_0"));
	if (!Root || !Candidates || !Resolved || !IconMaskMaterial)
	{
		return;
	}

	UCanvasPanel* Content = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MaskedIconContent"));
	IconRetainer = WidgetTree->ConstructWidget<UGameCellIconRetainer>(UGameCellIconRetainer::StaticClass(), TEXT("IconRetainer"));
	IconRetainer->SetTextureParameter(TEXT("Content"));
	IconRetainer->SetEffectMaterial(IconMaskMaterial);
	IconRetainer->SetContent(Content);
	IconRetainer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UCanvasPanelSlot* RetainerSlot = Root->AddChildToCanvas(IconRetainer);
	RetainerSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	RetainerSlot->SetOffsets(FMargin(0.f));
	// Keep the existing frame above the clipped icons and the cell button beneath them.
	RetainerSlot->SetZOrder(5);
	for (UWidget* Icons : {Candidates, Resolved})
	{
		Icons->RemoveFromParent();
		UCanvasPanelSlot* IconSlot = Content->AddChildToCanvas(Icons);
		IconSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		IconSlot->SetOffsets(FMargin(0.f));
	}
}

void UGameCellWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const FVector2D Size = MyGeometry.GetLocalSize();
	if (IconRetainer && Size.X > 0.f && Size.Y > 0.f && !Size.Equals(MaskSize, 0.1f))
	{
		if (UMaterialInstanceDynamic* Mask = IconRetainer->GetEffectMaterial())
		{
			Mask->SetVectorParameterValue(TEXT("CellSize"), FLinearColor(Size.X, Size.Y, 0.f, 0.f));
			Mask->SetScalarParameterValue(TEXT("CornerRadius"), 6.f);
			MaskSize = Size;
			IconRetainer->RequestRender();
		}
	}
}
