#include "GameCellWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/Image.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraUIWidget.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/SCanvas.h"

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
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Pulse(TEXT("/Game/Happiness/UI/Effects/NS_IconPulse.NS_IconPulse"));
	PulseSystem = Pulse.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PulseIcon(TEXT("/Game/Happiness/UI/Materials/M_IconPulse.M_IconPulse"));
	PulseMaterial = PulseIcon.Object;
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
	UpdatePulse(InDeltaTime);
}

void UGameCellWidget::NativeDestruct()
{
	RemovePulseOverlay();
	Super::NativeDestruct();
}

void UGameCellWidget::UpdatePulse(float DeltaTime)
{
	// WBP_Cell's ApplyPuzzleCell shows Image_FinalIcon when the cell has its icon
	const UWidget* FinalIcon = GetWidgetFromName(TEXT("Image_FinalIcon"));
	const bool bShowsFinalIcon = FinalIcon && FinalIcon->GetVisibility() != ESlateVisibility::Hidden
		&& FinalIcon->GetVisibility() != ESlateVisibility::Collapsed;
	if (bKnowsFinalIcon && bShowsFinalIcon && !bShowedFinalIcon)
	{
		StartPulse();
	}
	bKnowsFinalIcon = true;
	bShowedFinalIcon = bShowsFinalIcon;
}

void UGameCellWidget::StartPulse()
{
	const UImage* FinalIcon = Cast<UImage>(GetWidgetFromName(TEXT("Image_FinalIcon")));
	UTexture2D* Icon = FinalIcon ? Cast<UTexture2D>(FinalIcon->GetBrush().GetResourceObject()) : nullptr;
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!Icon || !PulseSystem || !PulseMaterial || !Viewport)
	{
		return;
	}

	// Where the cell is on the viewport: the effect is drawn there, in an overlay above the whole game screen (not
	// in the cell, where the Niagara UI widget doesn't show; and above the neighbouring cells as it grows)
	const FGeometry& Geometry = GetCachedGeometry();
	FVector2D Pixel, TopLeft, BottomRight;
	USlateBlueprintLibrary::LocalToViewport(this, Geometry, FVector2D::ZeroVector, Pixel, TopLeft);
	USlateBlueprintLibrary::LocalToViewport(this, Geometry, Geometry.GetLocalSize(), Pixel, BottomRight);
	PulseRect = FBox2D(TopLeft, BottomRight);

	// Made on the first pulse
	if (!PulseEffect)
	{
		PulseEffect = NewObject<UNiagaraUIWidget>(this);
		PulseEffect->SetNiagaraSystem(PulseSystem);
		PulseEffect->SetVisibility(ESlateVisibility::HitTestInvisible);
		PulseMaterialInstance = UMaterialInstanceDynamic::Create(PulseMaterial, this);
		TWeakObjectPtr<UGameCellWidget> WeakThis(this);
		PulseOverlay = SNew(SCanvas)
			.Visibility(EVisibility::HitTestInvisible)
			+ SCanvas::Slot()
			.Position(TAttribute<FVector2D>::CreateLambda([WeakThis] { return WeakThis.IsValid() ? WeakThis->PulseRect.Min : FVector2D::ZeroVector; }))
			.Size(TAttribute<FVector2D>::CreateLambda([WeakThis] { return WeakThis.IsValid() ? WeakThis->PulseRect.GetSize() : FVector2D::ZeroVector; }))
			[
				PulseEffect->TakeWidget()
			];
		Viewport->AddViewportWidgetContent(PulseOverlay.ToSharedRef(), 100);
	}
	UNiagaraComponent* Component = PulseEffect->GetNiagaraComponent();
	if (!Component || !PulseMaterialInstance)
	{
		return;
	}

	// This cell's own material instance, showing this cell's icon; sizes in the overlay's units
	PulseMaterialInstance->SetTextureParameterValue(TEXT("Icon"), Icon);
	const FVector2D Size = PulseRect.GetSize();
	Component->SetVariableMaterial(TEXT("IconMaterial"), PulseMaterialInstance);
	Component->SetVariableFloat(TEXT("IconSize"), FMath::Min(Size.X, Size.Y));
	Component->SetVariableFloat(TEXT("PulseScale"), PulseScale);
	Component->SetVariableFloat(TEXT("PulseDuration"), PulseDuration);
	// Start over (ResetSystem alone doesn't start it again once the last pulse has finished)
	Component->Activate(true);
}

void UGameCellWidget::RemovePulseOverlay()
{
	if (PulseOverlay.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(PulseOverlay.ToSharedRef());
		}
		PulseOverlay.Reset();
	}
	if (PulseEffect)
	{
		PulseEffect->ReleaseSlateResources(true);
		PulseEffect = nullptr;
	}
}
