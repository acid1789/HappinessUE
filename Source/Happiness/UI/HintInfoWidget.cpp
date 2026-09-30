#include "UI/HintInfoWidget.h"
#include "HappinessClassic/Clue.h"
#include "HappinessClassic/Hint.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Icon Index of an icon set (DA_IconSet, a Blueprint data asset): its Icons array holds the textures, directly or in a struct */
	UTexture2D* GetIconTexture(UObject* IconSet, int32 Index)
	{
		const FArrayProperty* Icons = IconSet ? FindFProperty<FArrayProperty>(IconSet->GetClass(), TEXT("Icons")) : nullptr;
		if (!Icons)
		{
			return nullptr;
		}
		FScriptArrayHelper Array(Icons, Icons->ContainerPtrToValuePtr<void>(IconSet));
		if (!Array.IsValidIndex(Index))
		{
			return nullptr;
		}

		const uint8* Element = Array.GetRawPtr(Index);
		if (const FObjectPropertyBase* ObjectInner = CastField<FObjectPropertyBase>(Icons->Inner))
		{
			return Cast<UTexture2D>(ObjectInner->GetObjectPropertyValue(Element));
		}
		if (const FStructProperty* StructInner = CastField<FStructProperty>(Icons->Inner))
		{
			for (TFieldIterator<FObjectPropertyBase> It(StructInner->Struct); It; ++It)
			{
				if (UTexture2D* Texture = Cast<UTexture2D>(It->GetObjectPropertyValue_InContainer(Element)))
				{
					return Texture;
				}
			}
		}
		return nullptr;
	}
}

UHintInfoWidget::UHintInfoWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FClassFinder<UUserWidget> Text(TEXT("/Game/Happiness/UI/WBP_HelpText"));
	static ConstructorHelpers::FClassFinder<UUserWidget> Icon(TEXT("/Game/Happiness/UI/WBP_HelpIcon"));
	HelpTextClass = Text.Class;
	HelpIconClass = Icon.Class;
}

void UHintInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();
	HideHint();
}

void UHintInfoWidget::ShowHint(UHint* Hint, const TArray<UObject*>& IconSets, UWidget* HintClue)
{
	if (!Hint || !Hint->TheClue)
	{
		HideHint();
		return;
	}

	BuildLine(ClueLine, Hint->TheClue->ClueHelp, IconSets);
	BuildLine(ActionLine, Hint->GetExplanation(), IconSets);

	ShownClue = HintClue;
	PlaceBox(HintClue);
	Box->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UHintInfoWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (Box && Box->GetVisibility() != ESlateVisibility::Collapsed)
	{
		PlaceBox(ShownClue.Get());
	}
}

void UHintInfoWidget::HideHint()
{
	// Only the box: this widget stays laid out so PlaceBox can measure against it
	ShownClue = nullptr;
	Box->SetVisibility(ESlateVisibility::Collapsed);
}

void UHintInfoWidget::BuildLine(UPanelWidget* Line, const FClueHelp& Help, const TArray<UObject*>& IconSets)
{
	Line->ClearChildren();
	for (const FClueHelpSegment& Segment : Help.Segments)
	{
		if (Segment.Type == EClueHelpSegementType::Text)
		{
			UUserWidget* Piece = HelpTextClass ? WidgetTree->ConstructWidget<UUserWidget>(HelpTextClass) : nullptr;
			if (UTextBlock* Text = Piece ? Cast<UTextBlock>(Piece->GetWidgetFromName(TEXT("TextBlock_54"))) : nullptr)
			{
				Text->SetText(FText::FromString(Segment.Text));
				UPanelSlot* PieceSlot = Line->AddChild(Piece);

				// ", which..." / "." right after an icon: pull it in against the icon instead of leaving a gap
				UWrapBoxSlot* WrapSlot = Cast<UWrapBoxSlot>(PieceSlot);
				if (WrapSlot && (Segment.Text.StartsWith(TEXT(",")) || Segment.Text.StartsWith(TEXT("."))))
				{
					WrapSlot->SetPadding(FMargin(-PunctuationTuck, 0.f, 0.f, 0.f));
				}
			}
		}
		else
		{
			UUserWidget* Piece = HelpIconClass ? WidgetTree->ConstructWidget<UUserWidget>(HelpIconClass) : nullptr;
			UImage* Image = Piece ? Cast<UImage>(Piece->GetWidgetFromName(TEXT("Image_28"))) : nullptr;
			UTexture2D* Texture = IconSets.IsValidIndex(Segment.IconRow) ? GetIconTexture(IconSets[Segment.IconRow], Segment.IconColumn) : nullptr;
			if (Image && Texture)
			{
				Image->SetBrushFromTexture(Texture, true);
				Line->AddChild(Piece);
			}
		}
	}
}

void UHintInfoWidget::PlaceBox(UWidget* HintClue)
{
	UCanvasPanelSlot* BoxSlot = Cast<UCanvasPanelSlot>(Box->Slot);
	if (!BoxSlot)
	{
		return;
	}

	const FGeometry& Strip = GetCachedGeometry();
	const float StripWidth = Strip.GetLocalSize().X;
	const float MaxWidth = StripWidth * FMath::Clamp(BoxWidthFraction, 0.1f, 1.f);
	// At the right end unless the hinted clue is under it there
	float Width = MaxWidth;
	float Left = StripWidth - Width;

	// The hinted clue's extent across the strip, if it's in the strip (a horizontal clue is in the other panel)
	if (HintClue && StripWidth > 0.f)
	{
		const FGeometry& Clue = HintClue->GetCachedGeometry();
		const FVector2D ClueMin = Strip.AbsoluteToLocal(Clue.GetAbsolutePositionAtCoordinates(FVector2D(0.f, 0.f)));
		const FVector2D ClueMax = Strip.AbsoluteToLocal(Clue.GetAbsolutePositionAtCoordinates(FVector2D(1.f, 1.f)));
		const FVector2D StripSize = Strip.GetLocalSize();
		const bool bLaidOut = Clue.GetLocalSize().X > 0.f;
		const bool bClueInStrip = bLaidOut && ClueMax.X > 0.f && ClueMin.X < StripSize.X && ClueMax.Y > 0.f && ClueMin.Y < StripSize.Y;
		const bool bUnderBox = ClueMax.X + ClueGap > StripWidth - MaxWidth;
		if (bClueInStrip && bUnderBox)
		{
			// The side with more room, up to the clue less a gap
			const float RoomLeft = ClueMin.X - ClueGap;
			const float RoomRight = StripWidth - ClueMax.X - ClueGap;
			if (RoomRight > RoomLeft)
			{
				Width = FMath::Min(MaxWidth, FMath::Max(RoomRight, 0.f));
				Left = StripWidth - Width;
			}
			else
			{
				Width = FMath::Min(MaxWidth, FMath::Max(RoomLeft, 0.f));
				Left = 0.f;
			}
		}
	}

	// Full height; across, at Left with Width
	FAnchors Anchors;
	Anchors.Minimum = FVector2D(0.f, 0.f);
	Anchors.Maximum = FVector2D(0.f, 1.f);
	BoxSlot->SetAnchors(Anchors);
	BoxSlot->SetAlignment(FVector2D::ZeroVector);
	BoxSlot->SetOffsets(FMargin(Left, 0.f, Width, 0.f));
}
