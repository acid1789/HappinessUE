#include "UI/HintInfoWidget.h"
#include "HappinessClassic/Clue.h"
#include "HappinessClassic/Hint.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/Border.h"
#include "Components/GridPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Font.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
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
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	HintFont = Font.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> Panel(TEXT("/Game/Happiness/UI/Textures/T_HintReadingPanel.T_HintReadingPanel"));
	HintPanelTexture = Panel.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> Badge(TEXT("/Game/Happiness/UI/Textures/T_HintTitleBadge.T_HintTitleBadge"));
	HintBadgeTexture = Badge.Object;

}

void UHintInfoWidget::BuildPresentation()
{
	if (HintContent || !WidgetTree || !Box) return;
	UCanvasPanel* Root = Cast<UCanvasPanel>(Box->GetParent());
	if (!Root) return;
	Box->RemoveFromParent();
	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>();
	FSlateBrush Surface;
	Surface.SetResourceObject(HintPanelTexture);
	Surface.ImageType = ESlateBrushImageType::FullColor;
	Surface.ImageSize = FVector2D(256, 128);
	Surface.DrawAs = ESlateBrushDrawType::Box;
	Surface.Margin = FMargin(.07f, .14f);
	Frame->SetBrush(Surface);
	Frame->SetPadding(FMargin(12, 3));
	Box = Frame;
	UCanvasPanelSlot* FrameSlot = Root->AddChildToCanvas(Frame);
	FrameSlot->SetAnchors(FAnchors(.45f, 0, 1, 1));
	FrameSlot->SetOffsets(FMargin(0, -7, 0, -7));
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
	HintContent = Content;
	Frame->AddChild(Content);
	UHorizontalBox* ClueRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	Content->AddChildToVerticalBox(ClueRow)->SetPadding(FMargin(0, 14, 0, 0));
	USizeBox* Heading = WidgetTree->ConstructWidget<USizeBox>();
	Heading->SetWidthOverride(96); Heading->SetHeightOverride(28);
	UImage* Badge = WidgetTree->ConstructWidget<UImage>();
	HintBadgeTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Happiness/UI/Textures/T_HintTitleBadge.T_HintTitleBadge"));
	Badge->SetBrushFromTexture(HintBadgeTexture);
	Heading->AddChild(Badge);
	UHorizontalBoxSlot* HeadingSlot = ClueRow->AddChildToHorizontalBox(Heading);
	HeadingSlot->SetPadding(FMargin(0, 8, 10, 0));
	HeadingSlot->SetVerticalAlignment(VAlign_Top);
	UWrapBox* Clue = WidgetTree->ConstructWidget<UWrapBox>();
	Clue->SetExplicitWrapSize(false);
	Clue->SetHorizontalAlignment(HAlign_Left);
	Clue->SetInnerSlotPadding(FVector2D(0, 4));
	UHorizontalBoxSlot* ClueSlot = ClueRow->AddChildToHorizontalBox(Clue);
	ClueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ClueLine = Clue;
	auto AddLine = [&]() -> UPanelWidget*
	{
		UWrapBox* Line = WidgetTree->ConstructWidget<UWrapBox>();
		Line->SetExplicitWrapSize(false);
		Line->SetHorizontalAlignment(HAlign_Left);
		Line->SetInnerSlotPadding(FVector2D(0, 4));
		Content->AddChildToVerticalBox(Line);
		return Line;
	};
	UBorder* Divider = WidgetTree->ConstructWidget<UBorder>();
	Divider->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.48f, .32f, .10f, .5f), 0.f));
	Divider->SetPadding(FMargin(0));
	USizeBox* DividerSize = WidgetTree->ConstructWidget<USizeBox>();
	DividerSize->SetHeightOverride(1);
	DividerSize->AddChild(Divider);
	Content->AddChildToVerticalBox(DividerSize)->SetPadding(FMargin(0, 5, 0, 5));
	ActionLine = AddLine();
}

void UHintInfoWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	SetColorAndOpacity(FLinearColor::White);
	BuildPresentation();
	if (!GetGameInstance() && HintContent)
	{
		FClueHelp Clue, Action;
		Clue.AddText(TEXT("The flower must be somewhere to the left of the accordion, but they do not have to be next to each other."));
		Action.AddText(TEXT("The accordion can only be in column 2. There is only one column to its left, so the flower must be in column 1. You can eliminate its other positions in this row."));
		BuildLine(ClueLine, Clue, {});
		BuildLine(ActionLine, Action, {});
		Box->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UHintInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (GetGameInstance()) HideHint();
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

	LastReadingSize = FVector2D::ZeroVector;
	ShownClue = HintClue;
	PlaceBox(HintClue);
	Box->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UHintInfoWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (Box && Box->GetVisibility() != ESlateVisibility::Collapsed)
	{
		MatchClueStripBounds();
		PlaceBox(ShownClue.Get());
		FitTextToPanel();
	}
}

void UHintInfoWidget::MatchClueStripBounds()
{
	// Keep the strip's horizontal bounds, but use all the vertical space below
	// the actual puzzle grid. The grid moves when the puzzle size changes.
	UUserWidget* Owner = GetParent() ? GetParent()->GetTypedOuter<UUserWidget>() : nullptr;
	UWidget* Strip = Owner ? Owner->GetWidgetFromName(TEXT("VerticalCluePanel")) : nullptr;
	UCanvasPanelSlot* StripSlot = Strip ? Cast<UCanvasPanelSlot>(Strip->Slot) : nullptr;
	UCanvasPanelSlot* ReadingSlot = Cast<UCanvasPanelSlot>(Slot);
	if (!Owner || !StripSlot || !ReadingSlot) return;

	FAnchorData Layout = ReadingSlot->GetLayout();
	const FAnchorData StripLayout = StripSlot->GetLayout();
	Layout.Anchors = StripLayout.Anchors;
	Layout.Alignment = StripLayout.Alignment;
	Layout.Offsets.Left = StripLayout.Offsets.Left;
	Layout.Offsets.Right = StripLayout.Offsets.Right;
	UUserWidget* GamePanel = Cast<UUserWidget>(Owner->GetWidgetFromName(TEXT("GamePanel")));
	UCanvasPanel* GridContainer = GamePanel ? Cast<UCanvasPanel>(GamePanel->GetWidgetFromName(TEXT("Border_44"))) : nullptr;
	const FGeometry& OwnerGeometry = Owner->GetCachedGeometry();
	const float ScreenHeight = OwnerGeometry.GetLocalSize().Y;
	if (GridContainer && ScreenHeight > 0.f)
	{
		for (UWidget* Child : GridContainer->GetAllChildren())
		{
			UGridPanel* Grid = Cast<UGridPanel>(Child);
			if (!Grid || Grid->GetChildrenCount() == 0 || Grid->GetCachedGeometry().GetLocalSize().Y <= 0.f) continue;
			const float GridBottom = OwnerGeometry.AbsoluteToLocal(
				Grid->GetCachedGeometry().GetAbsolutePositionAtCoordinates(FVector2D(0.f, 1.f))).Y;
			if (GridBottom > 0.f && GridBottom < ScreenHeight)
			{
				Layout.Offsets.Top = GridBottom - ScreenHeight;
				Layout.Offsets.Bottom = ScreenHeight - GridBottom;
			}
			break;
		}
	}
	ReadingSlot->SetLayout(Layout);
}

void UHintInfoWidget::FitTextToPanel()
{
	if (!HintContent) return;
	const FVector2D Available = Box->GetCachedGeometry().GetLocalSize() - FVector2D(24, 6);
	if (Available.X <= 0 || Available.Y <= 0 || Available.Equals(LastReadingSize, .5f)) return;
	LastReadingSize = Available;
	// Keep both paragraphs together. Choose the largest type that fits the actual panel,
	// including inline icons, instead of clipping text or adding a scroll bar.
	const auto Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	int32 BestPoints = 12;
	for (int32 Points = 36; Points >= 12; --Points)
	{
		float Height = 32.f; // Clue row's top inset, divider, and paragraph spacing.
		for (UPanelWidget* Line : {ClueLine.Get(), ActionLine.Get()})
		{
			const float LineWidth = Line == ClueLine.Get() ? Available.X - 106.f : Available.X;
			float X = 0, RowHeight = 0;
			for (UWidget* Piece : Line->GetAllChildren())
			{
				FVector2D PieceSize(Points * 1.5f, Points * 1.5f);
				if (UTextBlock* Text = Cast<UTextBlock>(Piece))
					PieceSize = Measure->Measure(Text->GetText(), FSlateFontInfo(HintFont, Points));
				const float SlotWidth = PieceSize.X + 14.f;
				if (X > 0 && X + SlotWidth > LineWidth)
				{
					Height += RowHeight + 4;
					X = 0; RowHeight = 0;
				}
				X += SlotWidth;
				RowHeight = FMath::Max(RowHeight, FMath::Max(float(PieceSize.Y), Points * 1.45f));
			}
			Height += RowHeight;
			if (Line == ClueLine.Get()) Height = FMath::Max(Height, 50.f);
		}
		if (Height <= Available.Y) { BestPoints = Points; break; }
	}
	for (UPanelWidget* Line : {ClueLine.Get(), ActionLine.Get()})
	{
		UWrapBox* Wrap = CastChecked<UWrapBox>(Line);
		Wrap->SetExplicitWrapSize(true);
		Wrap->SetWrapSize(Line == ClueLine.Get() ? Available.X - 106.f : Available.X);
		for (UWidget* Piece : Line->GetAllChildren())
		{
			if (UTextBlock* Text = Cast<UTextBlock>(Piece))
				Text->SetFont(FSlateFontInfo(HintFont, BestPoints));
			else if (USizeBox* Icon = Cast<USizeBox>(Piece))
			{
				Icon->SetWidthOverride(BestPoints * 1.5f);
				Icon->SetHeightOverride(BestPoints * 1.5f);
			}
		}
	}
	HintContent->ForceLayoutPrepass();
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
			// Word-sized runs wrap naturally alongside inline icons at the available width.
			TArray<FString> Words;
			Segment.Text.ParseIntoArrayWS(Words);
			for (const FString& Word : Words)
			{
				UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
				Text->SetText(FText::FromString(Word));
				Text->SetFont(FSlateFontInfo(HintFont, 28));
				Text->SetColorAndOpacity(FLinearColor(.17f, .085f, .035f));
				UWrapBoxSlot* WordSlot = CastChecked<UWrapBoxSlot>(Line->AddChild(Text));
				const bool bPunctuation = Word.StartsWith(TEXT(",")) || Word.StartsWith(TEXT("."));
				WordSlot->SetPadding(FMargin(bPunctuation ? -PunctuationTuck : 0, 0, 7, 0));
				WordSlot->SetHorizontalAlignment(HAlign_Left);
				WordSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
		else if (UTexture2D* Texture = IconSets.IsValidIndex(Segment.IconRow) ? GetIconTexture(IconSets[Segment.IconRow], Segment.IconColumn) : nullptr)
		{
			USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
			Size->SetWidthOverride(42); Size->SetHeightOverride(42);
			UImage* Image = WidgetTree->ConstructWidget<UImage>();
			Image->SetBrushFromTexture(Texture);
			Size->AddChild(Image);
			UWrapBoxSlot* WordSlot = CastChecked<UWrapBoxSlot>(Line->AddChild(Size));
			WordSlot->SetPadding(FMargin(0, 0, 7, 0));
			WordSlot->SetVerticalAlignment(VAlign_Center);
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
	BoxSlot->SetOffsets(FMargin(Left, -7.f, Width, -7.f));
}
