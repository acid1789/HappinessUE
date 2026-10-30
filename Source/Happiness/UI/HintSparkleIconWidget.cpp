#include "UI/HintSparkleIconWidget.h"

#include "UI/ButtonPanelWidget.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/NativeWidgetHost.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraUIWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"
#include "Widgets/SCanvas.h"

TWeakObjectPtr<UHintSparkleIconWidget> UHintSparkleIconWidget::ShownClue;

UHintSparkleIconWidget::UHintSparkleIconWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> System(TEXT("/Game/Happiness/UI/Effects/NS_IconPulse.NS_IconPulse"));
	SparkleSystem = System.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/Happiness/UI/Materials/M_IconPulse.M_IconPulse"));
	SparkleMaterial = Material.Object;
}

bool UHintSparkleIconWidget::IsShowingHint() const
{
	const FBoolProperty* HintProperty = FindFProperty<FBoolProperty>(GetClass(), TEXT("ShowingHint"));
	return HintProperty && HintProperty->GetPropertyValue_InContainer(this);
}

void UHintSparkleIconWidget::HideOldOutline()
{
	// The Blueprint still owns the original outline for hint cleanup, but it must not paint.
	if (const FObjectProperty* DisplayProperty = FindFProperty<FObjectProperty>(GetClass(), TEXT("HintDisplay")))
	{
		if (UImage* OldOutline = Cast<UImage>(DisplayProperty->GetObjectPropertyValue_InContainer(this)))
			OldOutline->SetVisibility(ESlateVisibility::Collapsed);
	}
}

UUserWidget* UHintSparkleIconWidget::FindClue() const
{
	for (UObject* Outer = GetOuter(); Outer; Outer = Outer->GetOuter())
	{
		if (UUserWidget* ParentWidget = Cast<UUserWidget>(Outer))
		{
			const FString ClassName = ParentWidget->GetClass()->GetName();
			if (ClassName.StartsWith(TEXT("WBP_HorizontalClue_C")) || ClassName.StartsWith(TEXT("WBP_VerticalClue_C")))
				return ParentWidget;
		}
	}
	return nullptr;
}

void UHintSparkleIconWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (IsDesignTime()) return;
	// The clue's first icon runs its sparkles. Found by its place in the clue, not its name: the clues make their icons
	// at runtime, and cooked builds (phones) don't number runtime objects' names from _0 as the editor does
	UUserWidget* Clue = FindClue();
	const UPanelWidget* Parent = GetParent();
	if (Clue && Parent && Parent->GetChildIndex(this) == 0 && !ClueTicker.IsValid())
	{
		TickedClue = Clue;
		ClueTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UHintSparkleIconWidget::TickClue));
	}
}

void UHintSparkleIconWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (IsDesignTime()) return;

	if (!IsShowingHint())
		bStreamed = false;
	if (!IsShowingHint() || !IsVisible() || MyGeometry.GetLocalSize().IsNearlyZero())
	{
		if (!ClueTicker.IsValid()) StopSparkles();
		return;
	}
	HideOldOutline();
	// A clue's first icon runs from TickClue; its other icons show nothing
	if (ClueTicker.IsValid() || FindClue()) return;
	UpdateSparkles(InDeltaTime, MyGeometry, false);
	LastTickFrame = GFrameCounter;
	if (!BoardWatch.IsValid())
		BoardWatch = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UHintSparkleIconWidget::TickBoardWatch));
}

bool UHintSparkleIconWidget::TickBoardWatch(float DeltaTime)
{
	// The sparkles are drawn over the screen, not in the icon: when the icon is no longer painted (e.g. its cell
	// resolved and collapsed its candidates), nothing else would end them
	if (SparkleCanvas.IsValid() && GFrameCounter - LastTickFrame <= 2) return true;
	StopSparkles();
	BoardWatch.Reset();
	return false;
}

namespace
{
	/** Descendant's geometry, arranged down from Ancestor (at AncestorGeometry) whether or not it was last painted */
	bool FindDescendantGeometry(const TSharedRef<SWidget>& Ancestor, const FGeometry& AncestorGeometry,
		const TSharedRef<SWidget>& Descendant, FGeometry& OutGeometry)
	{
		TArray<TSharedRef<SWidget>> Path;
		for (TSharedPtr<SWidget> Widget = Descendant; Widget != Ancestor; Widget = Widget->GetParentWidget())
		{
			if (!Widget) return false;
			Path.Add(Widget.ToSharedRef());
		}
		OutGeometry = AncestorGeometry;
		TSharedRef<SWidget> Parent = Ancestor;
		for (int32 Index = Path.Num() - 1; Index >= 0; --Index)
		{
			FArrangedChildren Arranged(EVisibility::All);
			Parent->ArrangeChildren(OutGeometry, Arranged);
			int32 Found = 0;
			while (Found < Arranged.Num() && Arranged[Found].Widget != Path[Index]) ++Found;
			if (Found == Arranged.Num()) return false;
			OutGeometry = Arranged[Found].Geometry;
			Parent = Path[Index];
		}
		return true;
	}
}

bool UHintSparkleIconWidget::TickClue(float DeltaTime)
{
	UUserWidget* Clue = TickedClue.Get();
	const TSharedPtr<SWidget> ClueSlate = Clue ? Clue->GetCachedWidget() : nullptr;
	if (!ClueSlate || !IsShowingHint() || !IsVisible() || !Clue->IsVisible())
	{
		if (!IsShowingHint()) bStreamed = false;
		bClueScrolled = false;
		StopSparkles();
		return true;
	}

	// The clue's place in its scroll box, from the scroll box's own geometry: it's up to date even while the clue is
	// scrolled out of view (and not painted)
	UScrollBox* Scroll = nullptr;
	for (UWidget* Parent = Clue->GetParent(); Parent && !Scroll; Parent = Parent->GetParent())
		Scroll = Cast<UScrollBox>(Parent);
	const TSharedPtr<SWidget> ScrollSlate = Scroll ? Scroll->GetCachedWidget() : nullptr;
	if (!ScrollSlate)
	{
		UpdateSparkles(DeltaTime, Clue->GetCachedGeometry(), true);
		return true;
	}
	const FGeometry ScrollGeometry = Scroll->GetCachedGeometry();
	if (ScrollGeometry.GetLocalSize().IsNearlyZero()) return true;
	FGeometry ClueGeometry;
	if (!FindDescendantGeometry(ScrollSlate.ToSharedRef(), ScrollGeometry, ClueSlate.ToSharedRef(), ClueGeometry)) return true;

	if (!bClueScrolled)
	{
		// The hint is starting: bring its clue into view, with room for the sparkles around it
		Scroll->ScrollWidgetIntoView(Clue, true, EDescendantScrollDestination::IntoView, 16.f);
		bClueScrolled = true;
	}
	UpdateSparkles(DeltaTime, ClueGeometry, true, &ScrollGeometry);
	return true;
}

void UHintSparkleIconWidget::StopTickers()
{
	if (BoardWatch.IsValid())
	{
		FTSTicker::RemoveTicker(BoardWatch);
		BoardWatch.Reset();
	}
	if (ClueTicker.IsValid())
	{
		FTSTicker::RemoveTicker(ClueTicker);
		ClueTicker.Reset();
	}
	TickedClue = nullptr;
}

void UHintSparkleIconWidget::UpdateSparkles(float InDeltaTime, const FGeometry& Geometry, bool bWholeClue, const FGeometry* Scroll)
{
	StartSparkles();
	if (SparkleCanvas.IsValid() && !bStreamed)
	{
		// The hint has just started: the clue streams in from the hint button now, the board icon from the clue later.
		// The cell dialog's icon is the board's, up close: its ring is already there
		bStreamed = true;
		StreamTime = bWholeClue ? 0.f : -CellStreamDelay;
		if (!bWholeClue && IsInCellDialog())
		{
			StreamTime = StreamSeconds;
			UpdateSparklePosition(Geometry, bWholeClue);
			PrefillRing();
		}
	}
	if (bWholeClue && SparkleCanvas.IsValid())
		ShownClue = this;
	UpdateSparklePosition(Geometry, bWholeClue);
	SparkleClip = FBox2D(ForceInit);
	FVector2D ClipTopLeft, ClipBottomRight;
	if (Scroll && ToSparkleSpace(*Scroll, ClipTopLeft, ClipBottomRight))
	{
		// The scroll box's area, widened by the orbit's overhang so that a clue in full view keeps its whole ring
		FVector2D ClueTopLeft, ClueBottomRight;
		const FVector2D Overhang = ToSparkleSpace(Geometry, ClueTopLeft, ClueBottomRight)
			? (SparkleRect.GetSize() - (ClueBottomRight - ClueTopLeft)) * .5f + FVector2D(SpriteSize * .5f)
			: FVector2D(SpriteSize);
		SparkleClip = FBox2D(ClipTopLeft - Overhang, ClipBottomRight + Overhang);
	}
	OrbitTime += InDeltaTime;
	StreamTime += InDeltaTime;
	SpawnClock += InDeltaTime;
	while (SpawnClock >= SpawnInterval)
	{
		SpawnClock -= SpawnInterval;
		// None while waiting for the stream, then StreamSpawns at a time while streaming
		const int32 Spawns = StreamTime < 0.f ? 0 : StreamTime < StreamSeconds ? StreamSpawns : 1;
		for (int32 Spawn = 0; Spawn < Spawns; ++Spawn)
			SpawnSparkle();
	}
	for (int32 Index = 0; Index < Sparkles.Num(); ++Index)
	{
		FOrbitSparkle& State = SparkleStates[Index];
		if (!State.bActive) continue;
		State.Age += InDeltaTime;
		if (State.Age >= State.Lifetime)
		{
			State.bActive = false;
			Sparkles[Index]->SetVisibility(ESlateVisibility::Collapsed);
			SparkleImages[Index]->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}
		UNiagaraUIWidget* Sparkle = Sparkles[Index];
		// Sparkles still flying in from the hint button are never clipped to the clue's panel
		const bool bInside = !SparkleClip.bIsValid || State.Age < State.Travel
			|| SparkleClip.IsInsideOrOn(GetSparklePosition(Index) + FVector2D(GetSparkleSize(Index) * .5f));
		const ESlateVisibility Shown = bInside ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
		Sparkle->SetVisibility(Shown);
		SparkleImages[Index]->SetVisibility(Shown);
		const float Fade = FMath::Min3(1.f, State.Age / .18f, (State.Lifetime - State.Age) / .3f);
		const float Twinkle = .72f + .28f * FMath::Sin(OrbitTime * (4.3f + Index * .2f) + State.Phase);
		Sparkle->SetRenderOpacity(Fade * Twinkle);
		SparkleImages[Index]->SetRenderOpacity(Fade * Twinkle);
	}
	if (SparkleCanvas.IsValid()) SparkleCanvas->Invalidate(EInvalidateWidgetReason::Layout);
}

UTexture2D* UHintSparkleIconWidget::MakeSparkleTexture(const FLinearColor& Color)
{
	constexpr int32 Size = 32;
	UTexture2D* Texture = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8);
	if (!Texture) return nullptr;
	Texture->SRGB = true;
#if WITH_EDITORONLY_DATA
	// Editor-only property (a transient texture has a single mip anyway); game builds don't have it
	Texture->MipGenSettings = TMGS_NoMipmaps;
#endif
	Texture->Filter = TF_Bilinear;
	FColor* Pixels = static_cast<FColor*>(Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
	for (int32 Y = 0; Y < Size; ++Y)
	{
		for (int32 X = 0; X < Size; ++X)
		{
			const float Dx = (X - 15.5f) / 15.5f;
			const float Dy = (Y - 15.5f) / 15.5f;
			const float Radius = FMath::Sqrt(Dx * Dx + Dy * Dy);
			const float Core = FMath::Exp(-Radius * Radius * 8.f);
			const float Rays = FMath::Exp(-FMath::Min(FMath::Abs(Dx), FMath::Abs(Dy)) * 11.f) * FMath::Exp(-Radius * 2.7f);
			const float Alpha = FMath::Clamp(Core + Rays * .75f, 0.f, 1.f);
			Pixels[Y * Size + X] = FColor(Color.R * 255.f, Color.G * 255.f, Color.B * 255.f, Alpha * 255.f);
		}
	}
	Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
	Texture->UpdateResource();
	return Texture;
}

float UHintSparkleIconWidget::GetSparkleSize(int32 Index) const
{
	return SparkleStates.IsValidIndex(Index) ? SpriteSize * SparkleStates[Index].Scale : 0.f;
}

FVector2D UHintSparkleIconWidget::GetSparklePosition(int32 Index) const
{
	if (!SparkleStates.IsValidIndex(Index) || !SparkleStates[Index].bActive) return FVector2D::ZeroVector;
	const FOrbitSparkle& State = SparkleStates[Index];
	FVector2D Position = GetOrbitPosition(Index);
	if (State.Age < State.Travel)
	{
		// Flying in: a curve from the hint button that ends where the sparkle is in the orbit by then, so it joins it
		const float Along = FMath::InterpEaseInOut(0.f, 1.f, State.Age / State.Travel, 2.f);
		const FVector2D Delta = Position - State.From;
		const FVector2D Control = (State.From + Position) * .5f + FVector2D(-Delta.Y, Delta.X) * State.Bend;
		Position = State.From * FMath::Square(1.f - Along) + Control * (2.f * (1.f - Along) * Along) + Position * FMath::Square(Along);
	}
	return Position - FVector2D(GetSparkleSize(Index) * .5f);
}

bool UHintSparkleIconWidget::GetStreamSource(FVector2D& OutFrom) const
{
	const UHintSparkleIconWidget* Clue = ShownClue.Get();
	if (!TickedClue.IsValid() && Clue && Clue->SparkleCanvas.IsValid() && Clue->SparkleRect.bIsValid)
	{
		// Peel off a random point of the clue's ring (both canvases cover the same screen)
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		OutFrom = Clue->SparkleRect.GetCenter() + Clue->SparkleRect.GetSize() * .5f * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
		return true;
	}
	const UWidget* Button = HintButton.Get();
	FVector2D TopLeft, BottomRight;
	if (!Button || !ToSparkleSpace(Button->GetCachedGeometry(), TopLeft, BottomRight)) return false;
	OutFrom = (TopLeft + BottomRight) * .5f + FVector2D(FMath::FRandRange(-.2f, .2f), FMath::FRandRange(-.2f, .2f)) * (BottomRight - TopLeft);
	return true;
}

FVector2D UHintSparkleIconWidget::GetOrbitPosition(int32 Index) const
{
	const FOrbitSparkle& State = SparkleStates[Index];
	const float Angle = State.Phase + OrbitTime * State.Speed;
	const FVector2D Center = SparkleRect.GetCenter();
	const FVector2D Radius = SparkleRect.GetSize() * .5f;
	const float CosAngle = FMath::Cos(Angle);
	const float SinAngle = FMath::Sin(Angle);
	// The ellipse normal points perpendicular to the orbit tangent, even for wide clues.
	const FVector2D Normal = FVector2D(CosAngle / FMath::Max(Radius.X, 1.f),
		SinAngle / FMath::Max(Radius.Y, 1.f)).GetSafeNormal();
	return Center + FVector2D(CosAngle * Radius.X, SinAngle * Radius.Y)
		+ Normal * (State.NormalOffset * SpriteSize);
}

bool UHintSparkleIconWidget::IsInCellDialog() const
{
	for (UObject* Outer = GetOuter(); Outer; Outer = Outer->GetOuter())
	{
		if (const UUserWidget* ParentWidget = Cast<UUserWidget>(Outer); ParentWidget && ParentWidget->GetClass()->GetName().StartsWith(TEXT("WBP_CellDialog_C")))
			return true;
	}
	return false;
}

void UHintSparkleIconWidget::PrefillRing()
{
	// One spawn each SpawnInterval, each living 2 seconds on average: the ring settles at about this many
	const int32 Count = FMath::Min(SparkleCount, FMath::RoundToInt(2.f / SpawnInterval));
	for (int32 Spawn = 0; Spawn < Count; ++Spawn)
	{
		const int32 Index = SpawnSparkle();
		if (Index == INDEX_NONE) break;
		FOrbitSparkle& State = SparkleStates[Index];
		State.Age = FMath::FRandRange(0.f, State.Lifetime);
	}
}

int32 UHintSparkleIconWidget::SpawnSparkle()
{
	for (int32 Index = 0; Index < SparkleStates.Num(); ++Index)
	{
		FOrbitSparkle& State = SparkleStates[Index];
		if (State.bActive) continue;
		State.Age = 0.f;
		State.Lifetime = FMath::FRandRange(1.f, 3.f);
		State.Phase = FMath::FRandRange(0.f, 2.f * PI);
		State.Speed = FMath::FRandRange(1.7f, 2.7f);
		State.Scale = FMath::FRandRange(.1f, .5f);
		State.NormalOffset = FMath::FRandRange(-.15f, .15f);
		State.Travel = 0.f;
		if (StreamTime < StreamSeconds && GetStreamSource(State.From))
		{
			State.Travel = TravelSeconds * FMath::FRandRange(.85f, 1.15f);
			State.Lifetime += State.Travel;
			State.Bend = FMath::FRandRange(.08f, .3f);
			State.Scale = FMath::FRandRange(.25f, .6f);
		}
		State.bActive = true;
		SparkleImages[Index]->SetVisibility(ESlateVisibility::HitTestInvisible);
		Sparkles[Index]->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UNiagaraComponent* Component = Sparkles[Index]->GetNiagaraComponent())
		{
			Component->SetVariableFloat(TEXT("IconSize"), GetSparkleSize(Index));
			Component->SetVariableFloat(TEXT("PulseDuration"), State.Lifetime);
			Component->Activate(true);
		}
		return Index;
	}
	return INDEX_NONE;
}

void UHintSparkleIconWidget::StartSparkles()
{
	if (SparkleCanvas.IsValid() || !SparkleSystem || !SparkleMaterial) return;
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!Viewport) return;

	PurpleTexture = MakeSparkleTexture(FLinearColor(.67f, .36f, .95f));
	SilverTexture = MakeSparkleTexture(FLinearColor(.94f, .95f, 1.f));
	if (!PurpleTexture || !SilverTexture) return;
	PurpleMaterial = UMaterialInstanceDynamic::Create(SparkleMaterial, this);
	PurpleMaterial->SetTextureParameterValue(TEXT("Icon"), PurpleTexture);
	SilverMaterial = UMaterialInstanceDynamic::Create(SparkleMaterial, this);
	SilverMaterial->SetTextureParameterValue(TEXT("Icon"), SilverTexture);
	SparkleCanvas = SNew(SCanvas).Visibility(EVisibility::HitTestInvisible);
	SparkleStates.SetNum(SparkleCount);
	TWeakObjectPtr<UHintSparkleIconWidget> WeakThis(this);
	for (int32 Index = 0; Index < SparkleCount; ++Index)
	{
		UImage* SparkleImage = NewObject<UImage>(this);
		SparkleImage->SetBrushFromTexture(Index % 3 == 0 ? SilverTexture : PurpleTexture);
		SparkleImage->SetVisibility(ESlateVisibility::Collapsed);
		SparkleImages.Add(SparkleImage);
		UNiagaraUIWidget* Sparkle = NewObject<UNiagaraUIWidget>(this);
		Sparkle->SetNiagaraSystem(SparkleSystem);
		Sparkle->SetVisibility(ESlateVisibility::Collapsed);
		Sparkles.Add(Sparkle);
		auto Position = TAttribute<FVector2D>::CreateLambda([WeakThis, Index]
		{
			return WeakThis.IsValid() ? WeakThis->GetSparklePosition(Index) : FVector2D::ZeroVector;
		});
		auto Size = TAttribute<FVector2D>::CreateLambda([WeakThis, Index]
		{
			return WeakThis.IsValid() ? FVector2D(WeakThis->GetSparkleSize(Index)) : FVector2D::ZeroVector;
		});
		SparkleCanvas->AddSlot().Position(Position).Size(Size)[SparkleImage->TakeWidget()];
		SparkleCanvas->AddSlot().Position(Position).Size(Size)[Sparkle->TakeWidget()];
	}
	UCanvasPanel* ScreenCanvas = nullptr;
	for (UObject* Outer = GetOuter(); Outer && !ScreenCanvas; Outer = Outer->GetOuter())
	{
		if (UUserWidget* ParentWidget = Cast<UUserWidget>(Outer))
		{
			if (ParentWidget->GetClass()->GetName().StartsWith(TEXT("WBP_Happiness_C")))
			{
				ScreenCanvas = Cast<UCanvasPanel>(ParentWidget->GetRootWidget());
				HintButton = nullptr;
				if (ParentWidget->WidgetTree)
				{
					ParentWidget->WidgetTree->ForEachWidget([this](UWidget* Widget)
					{
						if (const UButtonPanelWidget* Panel = Cast<UButtonPanelWidget>(Widget); Panel && Panel->Button_H)
							HintButton = Panel->Button_H;
					});
				}
			}
		}
	}
	if (ScreenCanvas)
	{
		SparkleHost = NewObject<UNativeWidgetHost>(this);
		SparkleHost->SetContent(SparkleCanvas.ToSharedRef());
		SparkleHost->SetVisibility(ESlateVisibility::HitTestInvisible);
		// Over the board (12), and just over the screen's overlay this icon is in when that is higher: the cell
		// dialog (20) shows the hinted icon too
		int32 ZOrder = 12;
		for (UObject* Outer = this; Outer; Outer = Outer->GetOuter())
		{
			const UWidget* Widget = Cast<UWidget>(Outer);
			const UCanvasPanelSlot* ScreenSlot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
			if (ScreenSlot && ScreenSlot->Parent == ScreenCanvas)
			{
				ZOrder = FMath::Max(ZOrder, ScreenSlot->GetZOrder() + 1);
				break;
			}
		}
		UCanvasPanelSlot* HostSlot = ScreenCanvas->AddChildToCanvas(SparkleHost);
		HostSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		HostSlot->SetOffsets(FMargin(0.f));
		HostSlot->SetZOrder(ZOrder);
	}
	else
	{
		Viewport->AddViewportWidgetContent(SparkleCanvas.ToSharedRef(), 100);
	}
	for (int32 Index = 0; Index < Sparkles.Num(); ++Index)
	{
		if (UNiagaraComponent* Component = Sparkles[Index]->GetNiagaraComponent())
		{
			Component->SetVariableMaterial(TEXT("IconMaterial"), Index % 3 == 0 ? SilverMaterial : PurpleMaterial);
			Component->SetVariableFloat(TEXT("PulseScale"), 1.2f);
		}
	}
}

bool UHintSparkleIconWidget::ToSparkleSpace(const FGeometry& Geometry, FVector2D& OutTopLeft, FVector2D& OutBottomRight) const
{
	FVector2D Pixel;
	const FGeometry HostGeometry = SparkleHost ? SparkleHost->GetCachedGeometry() : FGeometry();
	if (SparkleHost && HostGeometry.GetLocalSize().X > 0.f)
	{
		OutTopLeft = HostGeometry.AbsoluteToLocal(Geometry.LocalToAbsolute(FVector2D::ZeroVector));
		OutBottomRight = HostGeometry.AbsoluteToLocal(Geometry.LocalToAbsolute(Geometry.GetLocalSize()));
	}
	else
	{
		USlateBlueprintLibrary::LocalToViewport(this, Geometry, FVector2D::ZeroVector, Pixel, OutTopLeft);
		USlateBlueprintLibrary::LocalToViewport(this, Geometry, Geometry.GetLocalSize(), Pixel, OutBottomRight);
	}
	const FVector2D Size = OutBottomRight - OutTopLeft;
	return Size.X > 0.f && Size.Y > 0.f;
}

void UHintSparkleIconWidget::UpdateSparklePosition(const FGeometry& Geometry, bool bWholeClue)
{
	FVector2D TopLeft, BottomRight;
	if (!ToSparkleSpace(Geometry, TopLeft, BottomRight)) return;
	const FVector2D Size = BottomRight - TopLeft;
	const FVector2D OrbitSize = bWholeClue
		? FVector2D(Size.X * 1.08f, Size.Y * 1.16f)
		: FVector2D(FMath::Max(Size.X, Size.Y) * 1.13f);
	const FVector2D Center = (TopLeft + BottomRight) * .5f;
	SparkleRect = FBox2D(Center - OrbitSize * .5f, Center + OrbitSize * .5f);
	SpriteSize = FMath::Clamp(FMath::Min(Size.X, Size.Y) * .32f, 16.f, 28.f);
}

void UHintSparkleIconWidget::StopSparkles()
{
	if (ShownClue.Get() == this)
		ShownClue = nullptr;
	if (SparkleCanvas.IsValid())
	{
		if (SparkleHost)
			SparkleHost->RemoveFromParent();
		else if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
			Viewport->RemoveViewportWidgetContent(SparkleCanvas.ToSharedRef());
		SparkleCanvas.Reset();
	}
	SparkleHost = nullptr;
	Sparkles.Reset();
	SparkleImages.Reset();
	SparkleStates.Reset();
	PurpleMaterial = nullptr;
	SilverMaterial = nullptr;
	PurpleTexture = nullptr;
	SilverTexture = nullptr;
	OrbitTime = 0.f;
	SpawnClock = 0.f;
}

void UHintSparkleIconWidget::NativeDestruct()
{
	StopTickers();
	StopSparkles();
	Super::NativeDestruct();
}

void UHintSparkleIconWidget::BeginDestroy()
{
	StopTickers();
	Super::BeginDestroy();
}
