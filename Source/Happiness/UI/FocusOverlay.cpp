#include "UI/FocusOverlay.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/SLeafWidget.h"

/** Draws UFocusOverlay: the dim as a mesh of one-pixel rows, clear inside the circles */
class SFocusOverlay : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SFocusOverlay) {} SLATE_END_ARGS()

	void Construct(const FArguments&) {}

	struct FCircle
	{
		FVector2D Center;
		float Radius;
		TArray<TWeakPtr<SWidget>> Targets;
		float Padding;
		TArray<TWeakPtr<SWidget>> StartTargets;
		float StartPadding;
		bool bTight;
		bool bStartTight;
		float StartRadius;
		float Duration;
		double StartTime;

		/** How far its animation is at Time: 0 at the start, 1 once over (or with none) */
		float Progress(double Time) const
		{
			return Duration <= 0.f ? 1.f : FMath::Clamp(float((Time - StartTime) / Duration), 0.f, 1.f);
		}
	};

	/**
	 * The circle round Widgets, in G's units (X, Y the center, Z the radius): centered on the box holding every one of
	 * them that's drawn, through its corners (bTight: half its longer side), plus Padding in their units. False if none
	 * is drawn.
	 */
	static bool CircleRound(const FGeometry& G, const TArray<TWeakPtr<SWidget>>& Widgets, float Padding, bool bTight,
		FVector3f& Out)
	{
		FBox2D Box(ForceInit);
		float Scale = 0.f;
		for (const TWeakPtr<SWidget>& Weak : Widgets)
		{
			const TSharedPtr<SWidget> Widget = Weak.Pin();
			if (!Widget.IsValid() || !Widget->GetVisibility().IsVisible())
			{
				continue;
			}
			// Both geometries paint-space: window-relative
			const FGeometry& WidgetGeometry = Widget->GetPaintSpaceGeometry();
			const FVector2D Size = WidgetGeometry.GetLocalSize();
			if (Size.IsNearlyZero())
			{
				continue;
			}
			Box += G.AbsoluteToLocal(WidgetGeometry.LocalToAbsolute(FVector2D::ZeroVector));
			Box += G.AbsoluteToLocal(WidgetGeometry.LocalToAbsolute(Size));
			Scale = WidgetGeometry.Scale;
		}
		if (!Box.bIsValid)
		{
			return false;
		}
		const FVector2D Center = Box.GetCenter();
		const FVector2D Extent = Box.GetExtent();
		Out = FVector3f(Center.X, Center.Y, (bTight ? FMath::Max(Extent.X, Extent.Y) : Extent.Size()) + Padding * Scale / G.Scale);
		return true;
	}
	TArray<FCircle> Circles;
	/** When it was last drawn (Slate's clock): it's on screen while that's recent */
	mutable double LastPaintTime = -1.0;
	FLinearColor Color = FLinearColor::Black;
	float EdgeSoftness = 2.f;

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }

	// Followed widgets move: draw again every frame
	virtual bool ComputeVolatility() const override { return true; }

	// While it takes input (UFocusOverlay::SetBlocksInput makes it hit-testable), every press, touch and scroll stops here
	virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
	virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
	virtual FReply OnMouseButtonDoubleClick(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
	virtual FReply OnMouseWheel(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
	virtual FReply OnTouchStarted(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
	virtual FReply OnTouchMoved(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
	virtual FReply OnTouchEnded(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }

	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Elements,
		int32 Layer, const FWidgetStyle& Style, bool) const override
	{
		const FVector2D Size = G.GetLocalSize();
		const float Scale = G.GetAccumulatedLayoutTransform().GetScale();
		if (Size.X <= 0.f || Size.Y <= 0.f || Scale <= 0.f)
		{
			return Layer;
		}
		const float Pixel = 1.f / Scale;
		const float Soft = EdgeSoftness * Pixel;

		// The circles in this widget's units; a followed widget's from where it was last drawn (both geometries
		// paint-space: window-relative)
		TArray<FVector3f> Holes;
		const double Time = FSlateApplication::Get().GetCurrentTime();
		LastPaintTime = Time;
		for (const FCircle& Circle : Circles)
		{
			// Where it ends up: round its widgets, or fixed
			FVector3f End(Circle.Center.X, Circle.Center.Y, Circle.Radius);
			if (Circle.Targets.Num() > 0 ? !CircleRound(G, Circle.Targets, Circle.Padding, Circle.bTight, End) : Circle.Radius <= 0.f)
			{
				continue;
			}
			// On its way there: from round its start widgets (moving and resizing), or from StartRadius where it is
			const float Alpha = Circle.Progress(Time);
			FVector3f Start = End;
			Start.Z = Circle.StartRadius;
			if (Circle.StartTargets.Num() > 0 && !CircleRound(G, Circle.StartTargets, Circle.StartPadding, Circle.bStartTight, Start))
			{
				Start = End;
			}
			Holes.Add(Alpha >= 1.f ? End : FMath::Lerp(Start, End, Alpha));
		}

		FLinearColor Dim = Color * Style.GetColorAndOpacityTint();
		const FColor Solid = Dim.ToFColor(true);
		FColor Clear = Solid;
		Clear.A = 0;

		TArray<FSlateVertex> Vertices;
		TArray<SlateIndex> Indices;
		const FSlateRenderTransform& Transform = G.GetAccumulatedRenderTransform();
		// A quad from X0 to X1 between Y0 and Y1, its left edge colored Left and its right edge Right
		auto Quad = [&](float X0, float X1, float Y0, float Y1, FColor Left, FColor Right)
		{
			if (X1 <= X0 || Y1 <= Y0)
			{
				return;
			}
			const SlateIndex First = Vertices.Num();
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, FVector2f(X0, Y0), FVector2f(0, 0), Left));
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, FVector2f(X1, Y0), FVector2f(1, 0), Right));
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, FVector2f(X0, Y1), FVector2f(0, 1), Left));
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, FVector2f(X1, Y1), FVector2f(1, 1), Right));
			Indices.Append({ SlateIndex(First), SlateIndex(First + 1), SlateIndex(First + 2),
				SlateIndex(First + 1), SlateIndex(First + 3), SlateIndex(First + 2) });
		};

		// Rows above and below every circle are dimmed whole
		float Top = Size.Y, Bottom = 0.f;
		for (const FVector3f& Hole : Holes)
		{
			Top = FMath::Min(Top, Hole.Y - Hole.Z);
			Bottom = FMath::Max(Bottom, Hole.Y + Hole.Z);
		}
		Top = FMath::Clamp(Top, 0.f, Size.Y);
		Bottom = FMath::Clamp(Bottom, Top, (float)Size.Y);
		Quad(0.f, Size.X, 0.f, Top, Solid, Solid);
		Quad(0.f, Size.X, Bottom, Size.Y, Solid, Solid);

		// In between, a pixel row at a time: dimmed outside the circles' spans at the row's middle. The soft edge
		// fades from clear at a span's end to the dim over Soft measured across the circle's edge, so along the row
		// it's wider where the edge slants (towards a circle's top and bottom)
		struct FSpan
		{
			float Start, End, StartSoft, EndSoft;
		};
		TArray<FSpan> Spans;
		for (float Y0 = Top; Y0 < Bottom; Y0 += Pixel)
		{
			const float Y1 = FMath::Min(Y0 + Pixel, Bottom);
			const float Y = (Y0 + Y1) * .5f;
			Spans.Reset();
			for (const FVector3f& Hole : Holes)
			{
				const float Dy = Y - Hole.Y;
				if (FMath::Abs(Dy) >= Hole.Z)
				{
					continue;
				}
				const float Half = FMath::Sqrt(Hole.Z * Hole.Z - Dy * Dy);
				const float Fade = Soft <= 0.f ? 0.f : FMath::Min(Soft * Hole.Z / FMath::Max(Half, Pixel), Soft * 8.f);
				Spans.Add({ Hole.X - Half, Hole.X + Half, Fade, Fade });
			}
			Spans.Sort([](const FSpan& A, const FSpan& B) { return A.Start < B.Start; });

			// Merge overlapping spans, then dim the gaps between them
			float X = 0.f, XSoft = 0.f;
			int32 i = 0;
			while (i <= Spans.Num())
			{
				float GapEnd = Size.X, GapEndSoft = 0.f;
				FSpan Merged = { 0.f, 0.f, 0.f, 0.f };
				if (i < Spans.Num())
				{
					Merged = Spans[i++];
					while (i < Spans.Num() && Spans[i].Start <= Merged.End)
					{
						if (Spans[i].End > Merged.End)
						{
							Merged.End = Spans[i].End;
							Merged.EndSoft = Spans[i].EndSoft;
						}
						i++;
					}
					GapEnd = Merged.Start;
					GapEndSoft = Merged.StartSoft;
				}
				else
				{
					i++;
				}

				// The gap from X to GapEnd: fade in, dim, fade out (both fades shrunk to fit a narrow gap)
				const float Start = FMath::Max(X, 0.f), End = FMath::Min(GapEnd, (float)Size.X);
				if (End > Start)
				{
					float In = X > 0.f ? XSoft : 0.f;
					float Out = GapEnd < Size.X ? GapEndSoft : 0.f;
					const float Width = GapEnd - X;
					if (In + Out > Width && In + Out > 0.f)
					{
						const float Shrink = Width / (In + Out);
						In *= Shrink;
						Out *= Shrink;
					}
					// Fades are measured from the circle's edge, so a fade cut by the screen's edge is cut short
					auto At = [&](float Px)
					{
						float Alpha = 1.f;
						if (In > 0.f) Alpha = FMath::Min(Alpha, (Px - X) / In);
						if (Out > 0.f) Alpha = FMath::Min(Alpha, (GapEnd - Px) / Out);
						FColor C = Solid;
						C.A = (uint8)FMath::RoundToInt(Solid.A * FMath::Clamp(Alpha, 0.f, 1.f));
						return C;
					};
					const float InEnd = FMath::Clamp(X + In, Start, End);
					const float OutStart = FMath::Clamp(GapEnd - Out, InEnd, End);
					Quad(Start, InEnd, Y0, Y1, At(Start), At(InEnd));
					Quad(InEnd, OutStart, Y0, Y1, Solid, Solid);
					Quad(OutStart, End, Y0, Y1, At(OutStart), At(End));
				}
				X = FMath::Max(X, Merged.End);
				XSoft = Merged.EndSoft;
				if (GapEnd >= Size.X)
				{
					break;
				}
			}
		}

		if (Indices.Num() > 0)
		{
			const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox"));
			const FSlateResourceHandle Resource = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*White);
			FSlateDrawElement::MakeCustomVerts(Elements, Layer, Resource, Vertices, Indices, nullptr, 0, 0);
		}
		return Layer;
	}
};

/** Drops presses (and their releases), double clicks and scrolls an overlay's input window doesn't let through */
class FFocusInputFilter : public IInputProcessor
{
public:
	explicit FFocusInputFilter(const UFocusOverlay* InOverlay) : Overlay(InOverlay) {}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& Event) override
	{
		const bool bDrop = Drops(Event);
		const uint64 Pointer = (uint64(Event.GetUserIndex()) << 32) | Event.GetPointerIndex();
		if (bDrop)
		{
			DroppedPointers.Add(Pointer);
		}
		return bDrop;
	}

	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& Event) override
	{
		// The release of a press it dropped (a press it let through is released wherever it ends)
		return DroppedPointers.Remove((uint64(Event.GetUserIndex()) << 32) | Event.GetPointerIndex()) > 0;
	}

	virtual bool HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& Event) override
	{
		return Drops(Event);
	}

	virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& Event,
		const FPointerEvent* GestureEvent) override
	{
		return Drops(Event);
	}

	virtual const TCHAR* GetDebugName() const override { return TEXT("FocusOverlayInputWindow"); }

private:
	bool Drops(const FPointerEvent& Event) const
	{
		return Overlay.IsValid() && Overlay->ShouldDropInput(Event.GetScreenSpacePosition());
	}

	TWeakObjectPtr<const UFocusOverlay> Overlay;
	TSet<uint64> DroppedPointers;
};

UFocusOverlay::UFocusOverlay()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

TSharedRef<SWidget> UFocusOverlay::RebuildWidget()
{
	MyOverlay = SNew(SFocusOverlay);
	return MyOverlay.ToSharedRef();
}

void UFocusOverlay::SetInputWindow(const TArray<UWidget*>& Widgets)
{
	InputWindow.Reset();
	for (UWidget* Widget : Widgets)
	{
		if (Widget)
		{
			InputWindow.Add(Widget);
		}
	}
	if (InputWindow.Num() == 0)
	{
		StopInputFilter();
		return;
	}
	// The window does the blocking: the overlay itself lets everything through to it
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (!InputFilter.IsValid() && FSlateApplication::IsInitialized())
	{
		InputFilter = MakeShared<FFocusInputFilter>(this);
		FSlateApplication::Get().RegisterInputPreProcessor(InputFilter);
	}
}

bool UFocusOverlay::ShouldDropInput(const FVector2D& ScreenPosition) const
{
	// Only while on screen: drawn in the last moment (a closed or hidden screen stops drawing it)
	if (InputWindow.Num() == 0 || !MyOverlay.IsValid() || Now() - MyOverlay->LastPaintTime > .25)
	{
		return false;
	}
	for (const TWeakObjectPtr<UWidget>& Widget : InputWindow)
	{
		// Cached geometry is in desktop space, as pointer positions are
		if (Widget.IsValid() && Widget->IsVisible() && Widget->GetCachedGeometry().IsUnderLocation(ScreenPosition))
		{
			return false;
		}
	}
	return true;
}

void UFocusOverlay::StopInputFilter()
{
	if (InputFilter.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(InputFilter);
	}
	InputFilter.Reset();
}

void UFocusOverlay::BeginDestroy()
{
	StopInputFilter();
	Super::BeginDestroy();
}

void UFocusOverlay::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyOverlay.Reset();
}

void UFocusOverlay::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	PushFocuses();
}

double UFocusOverlay::Now()
{
	return FSlateApplication::IsInitialized() ? FSlateApplication::Get().GetCurrentTime() : 0.0;
}

void UFocusOverlay::AddFocusCircle(FVector2D Center, float Radius, float StartRadius, float Duration)
{
	FFocus& Focus = Focuses.AddDefaulted_GetRef();
	Focus.Center = Center;
	Focus.Radius = Radius;
	Focus.StartRadius = StartRadius;
	Focus.Duration = Duration;
	Focus.StartTime = Now();
	PushFocuses();
}

void UFocusOverlay::AddFocusOnWidget(UWidget* Target, float Padding, float StartRadius, float Duration)
{
	if (!Target)
	{
		return;
	}
	FFocus& Focus = Focuses.AddDefaulted_GetRef();
	Focus.Targets.Add(Target);
	Focus.Padding = Padding;
	Focus.StartRadius = StartRadius;
	Focus.Duration = Duration;
	Focus.StartTime = Now();
	PushFocuses();
}

void UFocusOverlay::AddFocusMove(const TArray<UWidget*>& From, const TArray<UWidget*>& To, float Padding, float Duration,
	float FromPadding, bool bTight, bool bFromTight)
{
	FFocus& Focus = Focuses.AddDefaulted_GetRef();
	for (UWidget* Widget : To)
	{
		if (Widget)
		{
			Focus.Targets.Add(Widget);
		}
	}
	for (UWidget* Widget : From)
	{
		if (Widget)
		{
			Focus.StartTargets.Add(Widget);
		}
	}
	Focus.Padding = Padding;
	Focus.StartPadding = FromPadding;
	Focus.bTight = bTight;
	Focus.bStartTight = bFromTight;
	Focus.Duration = Duration;
	Focus.StartTime = Now();
	PushFocuses();
}

void UFocusOverlay::SetBlocksInput(bool bBlock)
{
	SetVisibility(bBlock ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible);
}

void UFocusOverlay::ClearFocus()
{
	Focuses.Reset();
	PushFocuses();
}

void UFocusOverlay::PushFocuses()
{
	if (!MyOverlay.IsValid())
	{
		return;
	}
	MyOverlay->Color = FLinearColor(Color.R, Color.G, Color.B, Opacity);
	MyOverlay->EdgeSoftness = EdgeSoftness;
	MyOverlay->Circles.Reset();
	for (const FFocus& Focus : Focuses)
	{
		SFocusOverlay::FCircle& Circle = MyOverlay->Circles.AddDefaulted_GetRef();
		Circle.Center = Focus.Center;
		Circle.Radius = Focus.Radius;
		Circle.Padding = Focus.Padding;
		Circle.StartPadding = Focus.StartPadding;
		Circle.bTight = Focus.bTight;
		Circle.bStartTight = Focus.bStartTight;
		Circle.StartRadius = Focus.StartRadius;
		Circle.Duration = Focus.Duration;
		Circle.StartTime = Focus.StartTime;
		// Followed widgets' Slate widgets; one without one yet is left out until the next push
		for (const TWeakObjectPtr<UWidget>& Target : Focus.Targets)
		{
			if (Target.IsValid() && Target->GetCachedWidget().IsValid())
			{
				Circle.Targets.Add(Target->GetCachedWidget());
			}
		}
		for (const TWeakObjectPtr<UWidget>& Target : Focus.StartTargets)
		{
			if (Target.IsValid() && Target->GetCachedWidget().IsValid())
			{
				Circle.StartTargets.Add(Target->GetCachedWidget());
			}
		}
		// A circle round widgets none of which is drawn yet draws nothing
		if (Focus.Targets.Num() > 0 && Circle.Targets.Num() == 0)
		{
			Circle.Radius = 0.f;
		}
	}
}

#if !UE_BUILD_SHIPPING
namespace
{
	/** The overlay the console commands show (weak: it belongs to the play world) */
	TWeakObjectPtr<UFocusOverlay> DebugFocusOverlay;

	UFocusOverlay* GetDebugOverlay(UWorld* World, bool bCreate)
	{
		if (DebugFocusOverlay.IsValid() || !bCreate || !World || !World->GetGameViewport())
		{
			return DebugFocusOverlay.Get();
		}
		UGameInstance* Game = World->GetGameInstance();
		if (!Game)
		{
			return nullptr;
		}
		// Kept by the game instance until removed (only Slate holds it otherwise, and that doesn't keep it from GC)
		UFocusOverlay* Overlay = NewObject<UFocusOverlay>(Game);
		Game->RegisterReferencedObject(Overlay);
		World->GetGameViewport()->AddViewportWidgetContent(Overlay->TakeWidget(), 1000);
		DebugFocusOverlay = Overlay;
		return Overlay;
	}

	FAutoConsoleCommandWithWorldAndArgs FocusCommand(
		TEXT("Happiness.Focus"),
		TEXT("Happiness.Focus x,y,r[,StartRadius,Duration] ...: dim the screen but for those circles (x, y as fractions of ")
		TEXT("the screen's width and height, the radii of its height; each grows from StartRadius over Duration seconds). ")
		TEXT("Alone: remove the dim."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (Args.Num() == 0)
			{
				if (UFocusOverlay* Overlay = GetDebugOverlay(World, false))
				{
					if (World && World->GetGameViewport())
					{
						World->GetGameViewport()->RemoveViewportWidgetContent(Overlay->TakeWidget());
					}
					Overlay->ReleaseSlateResources(true);
					if (UGameInstance* Game = World ? World->GetGameInstance() : nullptr)
					{
						Game->UnregisterReferencedObject(Overlay);
					}
				}
				DebugFocusOverlay.Reset();
				return;
			}
			UFocusOverlay* Overlay = GetDebugOverlay(World, true);
			if (!Overlay)
			{
				return;
			}
			// The viewport's size in the overlay's units: it fills the viewport
			FVector2D Size = Overlay->GetCachedGeometry().GetLocalSize();
			if (Size.IsNearlyZero() && World->GetGameViewport())
			{
				// Not laid out yet: the viewport in pixels, over the UI's scale
				World->GetGameViewport()->GetViewportSize(Size);
				Size /= FMath::Max(UWidgetLayoutLibrary::GetViewportScale(World), 0.01f);
			}
			Overlay->ClearFocus();
			for (const FString& Arg : Args)
			{
				TArray<FString> Parts;
				Arg.ParseIntoArray(Parts, TEXT(","));
				if (Parts.Num() == 3 || Parts.Num() == 5)
				{
					Overlay->AddFocusCircle(FVector2D(FCString::Atof(*Parts[0]) * Size.X, FCString::Atof(*Parts[1]) * Size.Y),
						FCString::Atof(*Parts[2]) * Size.Y, Parts.Num() == 5 ? FCString::Atof(*Parts[3]) * Size.Y : 0.f,
						Parts.Num() == 5 ? FCString::Atof(*Parts[4]) : 0.f);
				}
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs FocusWidgetCommand(
		TEXT("Happiness.FocusWidget"),
		TEXT("Happiness.FocusWidget <WidgetName> [Padding] [StartRadius] [Duration]: add a clear circle round a widget on ")
		TEXT("screen (by its name in its Widget Blueprint, e.g. Button_H), dimming the screen if it isn't yet"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (Args.Num() == 0)
			{
				return;
			}
			UWidget* Found = nullptr;
			for (TObjectIterator<UUserWidget> It; It && !Found; ++It)
			{
				if (It->GetWorld() == World && It->WidgetTree && It->IsVisible())
				{
					UWidget* Widget = It->WidgetTree->FindWidget(FName(*Args[0]));
					Found = Widget && Widget->IsVisible() ? Widget : nullptr;
				}
			}
			UFocusOverlay* Overlay = Found ? GetDebugOverlay(World, true) : nullptr;
			if (Overlay)
			{
				Overlay->AddFocusOnWidget(Found, Args.Num() > 1 ? FCString::Atof(*Args[1]) : 8.f,
					Args.Num() > 2 ? FCString::Atof(*Args[2]) : 0.f, Args.Num() > 3 ? FCString::Atof(*Args[3]) : 0.f);
			}
		}));
}
#endif
