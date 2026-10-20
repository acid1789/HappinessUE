#include "GameCellWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
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
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Shatter(TEXT("/Game/Happiness/UI/Effects/NS_IconShatter.NS_IconShatter"));
	ShatterSystem = Shatter.Object;
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
	UpdateEffects();
}

void UGameCellWidget::NativeDestruct()
{
	RemoveEffects();
	Super::NativeDestruct();
}

namespace
{
	bool IsShown(const UWidget* Widget)
	{
		return Widget && Widget->GetVisibility() != ESlateVisibility::Hidden && Widget->GetVisibility() != ESlateVisibility::Collapsed;
	}

	// A candidate icon (WBP_Icon) and its image (TheIcon): eliminated icons are hidden or faded (WBP_Icon's SetVisible)
	const UWidget* CandidateImage(const UWidget* Candidate)
	{
		const UUserWidget* Icon = Cast<UUserWidget>(Candidate);
		return Icon ? Icon->GetWidgetFromName(TEXT("TheIcon")) : nullptr;
	}

	UTexture* ImageTexture(const UWidget* Image)
	{
		if (const UImage* AsImage = Cast<UImage>(Image))
		{
			return Cast<UTexture>(AsImage->GetBrush().GetResourceObject());
		}
		if (const UBorder* AsBorder = Cast<UBorder>(Image))
		{
			return Cast<UTexture>(AsBorder->Background.GetResourceObject());
		}
		return nullptr;
	}
}

void UGameCellWidget::UpdateEffects()
{
	// WBP_Cell's ApplyPuzzleCell shows Image_FinalIcon when the cell has its icon, and its candidate icons otherwise
	const UWidget* FinalIcon = GetWidgetFromName(TEXT("Image_FinalIcon"));
	const bool bShowsFinalIcon = IsShown(FinalIcon);
	TArray<const UWidget*> Candidates;
	for (const TCHAR* Row : { TEXT("TopRowIcons"), TEXT("BottomRowIcons") })
	{
		if (const UPanelWidget* Panel = Cast<UPanelWidget>(GetWidgetFromName(Row)))
		{
			for (int32 i = 0; i < Panel->GetChildrenCount(); i++)
			{
				Candidates.Add(Panel->GetChildAt(i));
			}
		}
	}
	TArray<bool> ShowsCandidates;
	for (const UWidget* Candidate : Candidates)
	{
		const UWidget* Image = CandidateImage(Candidate);
		ShowsCandidates.Add(IsShown(Image) && Image->GetRenderOpacity() > 0.99f);
	}

	if (bKnowsShown && bShowsFinalIcon && !bShowedFinalIcon)
	{
		// Over the cell: the final icon has only just been shown, so it has no place on screen yet
		if (UNiagaraComponent* Pulse = PlayEffect(0, PulseSystem, ImageTexture(FinalIcon), *this))
		{
			Pulse->SetVariableFloat(TEXT("PulseScale"), PulseScale);
			Pulse->SetVariableFloat(TEXT("PulseDuration"), PulseDuration);
			// Start over (ResetSystem alone doesn't start it again once the last one has finished)
			Pulse->Activate(true);
		}
	}
	// An eliminated candidate breaks (not when the cell gets its icon: that's the pulse)
	if (bKnowsShown && !bShowsFinalIcon && ShowsCandidates.Num() == ShowedCandidates.Num())
	{
		for (int32 i = 0; i < Candidates.Num(); i++)
		{
			if (ShowedCandidates[i] && !ShowsCandidates[i])
			{
				if (UNiagaraComponent* Shatter = PlayEffect(1 + i, ShatterSystem, ImageTexture(CandidateImage(Candidates[i])), *Candidates[i]))
				{
					Shatter->SetVariableFloat(TEXT("ShatterDuration"), ShatterDuration);
					Shatter->Activate(true);
				}
			}
		}
	}
	bKnowsShown = true;
	bShowedFinalIcon = bShowsFinalIcon;
	ShowedCandidates = MoveTemp(ShowsCandidates);
}

UNiagaraComponent* UGameCellWidget::PlayEffect(int32 Index, UNiagaraSystem* System, UTexture* Icon, const UWidget& Over)
{
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!Icon || !System || !PulseMaterial || !Viewport)
	{
		return nullptr;
	}
	// The effects play in a canvas over the whole game screen, above everything (not in the cell, where the Niagara
	// UI widget doesn't show, and where neighbouring cells would cover what goes past the cell)
	if (!EffectOverlay.IsValid())
	{
		EffectOverlay = SNew(SCanvas).Visibility(EVisibility::HitTestInvisible);
		Viewport->AddViewportWidgetContent(EffectOverlay.ToSharedRef(), 100);
	}
	if (Effects.Num() <= Index)
	{
		Effects.SetNum(Index + 1);
	}

	// Made the first time it plays, in its own slot of the overlay
	FCellEffect& Effect = Effects[Index];
	if (!Effect.Widget)
	{
		Effect.Widget = NewObject<UNiagaraUIWidget>(this);
		Effect.Widget->SetNiagaraSystem(System);
		Effect.Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
		Effect.Material = UMaterialInstanceDynamic::Create(PulseMaterial, this);
		TWeakObjectPtr<UGameCellWidget> WeakThis(this);
		auto Rect = [WeakThis, Index]
		{
			return WeakThis.IsValid() && WeakThis->Effects.IsValidIndex(Index) ? WeakThis->Effects[Index].Rect : FBox2D(FVector2D::ZeroVector, FVector2D::ZeroVector);
		};
		EffectOverlay->AddSlot()
			.Position(TAttribute<FVector2D>::CreateLambda([Rect] { return Rect().Min; }))
			.Size(TAttribute<FVector2D>::CreateLambda([Rect] { return Rect().GetSize(); }))
			[
				Effect.Widget->TakeWidget()
			];
	}
	UNiagaraComponent* Component = Effect.Widget->GetNiagaraComponent();
	if (!Component || !Effect.Material)
	{
		return nullptr;
	}

	// Where the widget is on the viewport; sizes in the overlay's units
	const FGeometry& Geometry = Over.GetCachedGeometry();
	FVector2D Pixel, TopLeft, BottomRight;
	USlateBlueprintLibrary::LocalToViewport(this, Geometry, FVector2D::ZeroVector, Pixel, TopLeft);
	USlateBlueprintLibrary::LocalToViewport(this, Geometry, Geometry.GetLocalSize(), Pixel, BottomRight);
	Effect.Rect = FBox2D(TopLeft, BottomRight);
	const FVector2D Size = Effect.Rect.GetSize();

	Effect.Material->SetTextureParameterValue(TEXT("Icon"), Icon);
	Component->SetVariableMaterial(TEXT("IconMaterial"), Effect.Material);
	Component->SetVariableFloat(TEXT("IconSize"), FMath::Min(Size.X, Size.Y));
	return Component;
}

void UGameCellWidget::RemoveEffects()
{
	if (EffectOverlay.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(EffectOverlay.ToSharedRef());
		}
		EffectOverlay.Reset();
	}
	for (FCellEffect& Effect : Effects)
	{
		if (Effect.Widget)
		{
			Effect.Widget->ReleaseSlateResources(true);
		}
	}
	Effects.Reset();
}
