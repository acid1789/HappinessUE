#include "UI/HintSparkleIconWidget.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/NativeWidgetHost.h"
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

UHintSparkleIconWidget::UHintSparkleIconWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> System(TEXT("/Game/Happiness/UI/Effects/NS_IconPulse.NS_IconPulse"));
	SparkleSystem = System.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/Happiness/UI/Materials/M_IconPulse.M_IconPulse"));
	SparkleMaterial = Material.Object;
}

void UHintSparkleIconWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (IsDesignTime()) return;

	const FBoolProperty* HintProperty = FindFProperty<FBoolProperty>(GetClass(), TEXT("ShowingHint"));
	const bool bShowingHint = HintProperty && HintProperty->GetPropertyValue_InContainer(this);
	if (!bShowingHint || !IsVisible() || MyGeometry.GetLocalSize().IsNearlyZero())
	{
		StopSparkles();
		return;
	}

	// The Blueprint still owns the original outline for hint cleanup, but it must not paint.
	if (const FObjectProperty* DisplayProperty = FindFProperty<FObjectProperty>(GetClass(), TEXT("HintDisplay")))
	{
		if (UImage* OldOutline = Cast<UImage>(DisplayProperty->GetObjectPropertyValue_InContainer(this)))
			OldOutline->SetVisibility(ESlateVisibility::Collapsed);
	}
	UUserWidget* Clue = nullptr;
	for (UObject* Outer = GetOuter(); Outer && !Clue; Outer = Outer->GetOuter())
	{
		if (UUserWidget* ParentWidget = Cast<UUserWidget>(Outer))
		{
			const FString ClassName = ParentWidget->GetClass()->GetName();
			if (ClassName.StartsWith(TEXT("WBP_HorizontalClue_C")) || ClassName.StartsWith(TEXT("WBP_VerticalClue_C")))
				Clue = ParentWidget;
		}
	}
	if (Clue && !GetName().EndsWith(TEXT("_0")))
	{
		StopSparkles();
		return;
	}

	StartSparkles();
	UpdateSparklePosition(Clue ? Clue->GetCachedGeometry() : MyGeometry, Clue != nullptr);
	OrbitTime += InDeltaTime;
	SpawnClock += InDeltaTime;
	while (SpawnClock >= SpawnInterval)
	{
		SpawnClock -= SpawnInterval;
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
	Texture->MipGenSettings = TMGS_NoMipmaps;
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
	const float Angle = State.Phase + OrbitTime * State.Speed;
	const FVector2D Center = SparkleRect.GetCenter();
	const FVector2D Radius = SparkleRect.GetSize() * .5f;
	const float CosAngle = FMath::Cos(Angle);
	const float SinAngle = FMath::Sin(Angle);
	// The ellipse normal points perpendicular to the orbit tangent, even for wide clues.
	const FVector2D Normal = FVector2D(CosAngle / FMath::Max(Radius.X, 1.f),
		SinAngle / FMath::Max(Radius.Y, 1.f)).GetSafeNormal();
	return Center + FVector2D(CosAngle * Radius.X, SinAngle * Radius.Y)
		+ Normal * (State.NormalOffset * SpriteSize)
		- FVector2D(GetSparkleSize(Index) * .5f);
}

void UHintSparkleIconWidget::SpawnSparkle()
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
		State.bActive = true;
		SparkleImages[Index]->SetVisibility(ESlateVisibility::HitTestInvisible);
		Sparkles[Index]->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UNiagaraComponent* Component = Sparkles[Index]->GetNiagaraComponent())
		{
			Component->SetVariableFloat(TEXT("IconSize"), GetSparkleSize(Index));
			Component->SetVariableFloat(TEXT("PulseDuration"), State.Lifetime);
			Component->Activate(true);
		}
		return;
	}
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
				ScreenCanvas = Cast<UCanvasPanel>(ParentWidget->GetRootWidget());
		}
	}
	if (ScreenCanvas)
	{
		SparkleHost = NewObject<UNativeWidgetHost>(this);
		SparkleHost->SetContent(SparkleCanvas.ToSharedRef());
		SparkleHost->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* HostSlot = ScreenCanvas->AddChildToCanvas(SparkleHost);
		HostSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		HostSlot->SetOffsets(FMargin(0.f));
		HostSlot->SetZOrder(12);
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

void UHintSparkleIconWidget::UpdateSparklePosition(const FGeometry& Geometry, bool bWholeClue)
{
	FVector2D Pixel, TopLeft, BottomRight;
	const FGeometry HostGeometry = SparkleHost ? SparkleHost->GetCachedGeometry() : FGeometry();
	if (SparkleHost && HostGeometry.GetLocalSize().X > 0.f)
	{
		TopLeft = HostGeometry.AbsoluteToLocal(Geometry.LocalToAbsolute(FVector2D::ZeroVector));
		BottomRight = HostGeometry.AbsoluteToLocal(Geometry.LocalToAbsolute(Geometry.GetLocalSize()));
	}
	else
	{
		USlateBlueprintLibrary::LocalToViewport(this, Geometry, FVector2D::ZeroVector, Pixel, TopLeft);
		USlateBlueprintLibrary::LocalToViewport(this, Geometry, Geometry.GetLocalSize(), Pixel, BottomRight);
	}
	const FVector2D Size = BottomRight - TopLeft;
	if (Size.X <= 0.f || Size.Y <= 0.f) return;
	const FVector2D OrbitSize = bWholeClue
		? FVector2D(Size.X * 1.08f, Size.Y * 1.16f)
		: FVector2D(FMath::Max(Size.X, Size.Y) * 1.13f);
	const FVector2D Center = (TopLeft + BottomRight) * .5f;
	SparkleRect = FBox2D(Center - OrbitSize * .5f, Center + OrbitSize * .5f);
	SpriteSize = FMath::Clamp(FMath::Min(Size.X, Size.Y) * .32f, 16.f, 28.f);
}

void UHintSparkleIconWidget::StopSparkles()
{
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
	StopSparkles();
	Super::NativeDestruct();
}
