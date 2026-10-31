#include "GameCellWidget.h"
#include "GamePanelWidget.h"
#include "HintSparkleIconWidget.h"
#include "HappinessClassic/PuzzleCell.h"
#include "HappinessClassic/Puzzle.h"
#include "Kismet/GameplayStatics.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/NativeWidgetHost.h"
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
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Firework(TEXT("/Game/Happiness/UI/Effects/NS_IconFirework.NS_IconFirework"));
	FireworkSystem = Firework.Object;
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
	UpdatePlacedHint(InDeltaTime);
}

bool UGameCellWidget::IsPlacedElsewhere(int32 Icon) const
{
	auto ObjectProperty = [](const UObject* Owner, const TCHAR* Name) -> UObject*
	{
		const FObjectPropertyBase* Property = Owner ? FindFProperty<FObjectPropertyBase>(Owner->GetClass(), Name) : nullptr;
		return Property ? Property->GetObjectPropertyValue_InContainer(Owner) : nullptr;
	};
	auto IntProperty = [this](const TCHAR* Name) -> int32
	{
		const FIntProperty* Property = FindFProperty<FIntProperty>(GetClass(), Name);
		return Property ? Property->GetPropertyValue_InContainer(this) : -1;
	};
	// The dialog's cell has no ThePuzzle of its own (WBP_CellDialog's Populate): the puzzle screen's (PC_Happiness)
	const UPuzzle* Puzzle = Cast<UPuzzle>(ObjectProperty(this, TEXT("ThePuzzle")));
	if (!Puzzle)
		Puzzle = Cast<UPuzzle>(ObjectProperty(ObjectProperty(UGameplayStatics::GetPlayerController(this, 0), TEXT("HappinessWidget")), TEXT("ThePuzzle")));
	return Puzzle && Puzzle->IsPlacedElsewhere(IntProperty(TEXT("Row")), IntProperty(TEXT("Column")), Icon);
}

void UGameCellWidget::UpdatePlacedHint(float DeltaTime)
{
	// WBP_Cell: its puzzle cell (PuzzleCell, placed icon m_iFinalIcon), its candidate icons (AllIcons, WBP_Icon) and
	// the placed image's color (FinalIconColor, bound to Image_FinalIcon)
	UWidget* FinalIcon = GetWidgetFromName(TEXT("Image_FinalIcon"));
	const FStructProperty* CellProperty = FindFProperty<FStructProperty>(GetClass(), TEXT("PuzzleCell"));
	const FArrayProperty* IconsProperty = FindFProperty<FArrayProperty>(GetClass(), TEXT("AllIcons"));
	const FStructProperty* ColorProperty = FindFProperty<FStructProperty>(GetClass(), TEXT("FinalIconColor"));
	UHintSparkleIconWidget* Hinted = nullptr;
	if (FinalIcon && FinalIcon->GetVisibility() != ESlateVisibility::Hidden && FinalIcon->GetVisibility() != ESlateVisibility::Collapsed
		&& CellProperty && CellProperty->Struct == FPuzzleCell::StaticStruct() && IconsProperty)
	{
		const int32 Placed = CellProperty->ContainerPtrToValuePtr<FPuzzleCell>(this)->m_iFinalIcon;
		FScriptArrayHelper Icons(IconsProperty, IconsProperty->ContainerPtrToValuePtr<void>(this));
		const FObjectPropertyBase* IconElement = CastField<FObjectPropertyBase>(IconsProperty->Inner);
		if (IconElement && Icons.IsValidIndex(Placed))
		{
			Hinted = Cast<UHintSparkleIconWidget>(IconElement->GetObjectPropertyValue(Icons.GetRawPtr(Placed)));
			if (Hinted && !Hinted->IsShowingHint())
				Hinted = nullptr;
		}
	}
	FLinearColor* FinalColor = ColorProperty && ColorProperty->Struct == TBaseStructure<FLinearColor>::Get()
		? ColorProperty->ContainerPtrToValuePtr<FLinearColor>(this) : nullptr;
	if (Hinted)
	{
		Hinted->TickHintOver(FinalIcon->GetCachedGeometry(), DeltaTime);
		const FStructProperty* IconColor = FindFProperty<FStructProperty>(Hinted->GetClass(), TEXT("IconColor"));
		if (FinalColor && IconColor && IconColor->Struct == TBaseStructure<FLinearColor>::Get())
		{
			if (!bTintedPlacedIcon)
			{
				PlacedIconColor = *FinalColor;
				bTintedPlacedIcon = true;
			}
			*FinalColor = *IconColor->ContainerPtrToValuePtr<FLinearColor>(Hinted);
		}
	}
	else if (bTintedPlacedIcon)
	{
		if (FinalColor)
			*FinalColor = PlacedIconColor;
		bTintedPlacedIcon = false;
	}
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

namespace
{
	// Effects[] slot of the firework, after the pulse (0) and every candidate icon's shatter (1 + n)
	constexpr int32 FireworkEffect = 32;
}

void UGameCellWidget::PrepareFirework()
{
	UWidget* FinalIcon = GetWidgetFromName(TEXT("Image_FinalIcon"));
	if (IsShown(FinalIcon))
	{
		PlayEffect(FireworkEffect, FireworkSystem, ImageTexture(FinalIcon), *FinalIcon);
	}
}

bool UGameCellWidget::PlayFirework()
{
	UWidget* FinalIcon = GetWidgetFromName(TEXT("Image_FinalIcon"));
	if (!IsShown(FinalIcon))
	{
		return false;
	}
	UNiagaraComponent* Burst = PlayEffect(FireworkEffect, FireworkSystem, ImageTexture(FinalIcon), *FinalIcon);
	if (!Burst)
	{
		return false;
	}
	Burst->SetVariableFloat(TEXT("ShatterDuration"), FireworkDuration);
	Burst->Activate(true);
	FinalIcon->SetRenderOpacity(0.f);
	if (IconRetainer)
	{
		IconRetainer->RequestRender();
	}
	return true;
}

void UGameCellWidget::EndFirework()
{
	if (UWidget* FinalIcon = GetWidgetFromName(TEXT("Image_FinalIcon")))
	{
		FinalIcon->SetRenderOpacity(1.f);
	}
	if (IconRetainer)
	{
		IconRetainer->RequestRender();
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
			// Only the picture pulses, not the frame painted around it
			Effects[0].Material->SetScalarParameterValue(TEXT("FrameInset"), PulseFrameInset);
			Effects[0].Material->SetScalarParameterValue(TEXT("FrameChamfer"), PulseFrameChamfer);
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
	// The effects play in a canvas over the whole screen (not in the cell, where the Niagara UI widget doesn't show,
	// and where neighbouring cells would cover what goes past the cell): in the screen's root canvas under its end
	// screens and dialogs, or failing that over everything in the game viewport
	if (!EffectOverlay.IsValid())
	{
		EffectOverlay = SNew(SCanvas).Visibility(EVisibility::HitTestInvisible);

		// The screen: the widget holding the board (the game panel) this cell is in
		UUserWidget* Screen = nullptr;
		bool bInBoard = false;
		for (UObject* Outer = GetOuter(); Outer && !Screen; Outer = Outer->GetOuter())
		{
			if (Outer->IsA<UGamePanelWidget>())
			{
				bInBoard = true;
			}
			else if (bInBoard && Outer->IsA<UUserWidget>())
			{
				Screen = Cast<UUserWidget>(Outer);
			}
		}
		if (UCanvasPanel* Root = Screen ? Cast<UCanvasPanel>(Screen->GetRootWidget()) : nullptr)
		{
			EffectHost = NewObject<UNativeWidgetHost>(this);
			EffectHost->SetContent(EffectOverlay.ToSharedRef());
			EffectHost->SetVisibility(ESlateVisibility::HitTestInvisible);
			UCanvasPanelSlot* HostSlot = Root->AddChildToCanvas(EffectHost);
			HostSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			HostSlot->SetOffsets(FMargin(0.f));
			HostSlot->SetZOrder(EffectZOrder);
		}
		else
		{
			Viewport->AddViewportWidgetContent(EffectOverlay.ToSharedRef(), 100);
		}
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

	// Where the widget is in the overlay, in its units: in its host, or on the viewport (also the host's space
	// while the host hasn't been laid out yet, as it fills the screen)
	const FGeometry& Geometry = Over.GetCachedGeometry();
	FVector2D Pixel, TopLeft, BottomRight;
	const FGeometry HostGeometry = EffectHost ? EffectHost->GetCachedGeometry() : FGeometry();
	if (EffectHost && HostGeometry.GetLocalSize().X > 0.f)
	{
		TopLeft = HostGeometry.AbsoluteToLocal(Geometry.LocalToAbsolute(FVector2D::ZeroVector));
		BottomRight = HostGeometry.AbsoluteToLocal(Geometry.LocalToAbsolute(Geometry.GetLocalSize()));
	}
	else
	{
		USlateBlueprintLibrary::LocalToViewport(this, Geometry, FVector2D::ZeroVector, Pixel, TopLeft);
		USlateBlueprintLibrary::LocalToViewport(this, Geometry, Geometry.GetLocalSize(), Pixel, BottomRight);
	}
	Effect.Rect = FBox2D(TopLeft, BottomRight);
	const FVector2D Size = Effect.Rect.GetSize();

	Effect.Material->SetTextureParameterValue(TEXT("Icon"), Icon);
	Component->SetVariableMaterial(TEXT("IconMaterial"), Effect.Material);
	Component->SetVariableFloat(TEXT("IconSize"), FMath::Min(Size.X, Size.Y));
	return Component;
}

void UGameCellWidget::RemoveEffects()
{
	if (EffectHost)
	{
		EffectHost->RemoveFromParent();
		EffectHost = nullptr;
	}
	else if (EffectOverlay.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(EffectOverlay.ToSharedRef());
		}
	}
	EffectOverlay.Reset();
	for (FCellEffect& Effect : Effects)
	{
		if (Effect.Widget)
		{
			Effect.Widget->ReleaseSlateResources(true);
		}
	}
	Effects.Reset();
}
