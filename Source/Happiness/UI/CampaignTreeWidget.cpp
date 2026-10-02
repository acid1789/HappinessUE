#include "UI/CampaignTreeWidget.h"
#include "HappinessClassic/CampaignTree.h"
#include "HappinessClassic/CampaignProgress.h"
#include "UI/LessonPopupWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"
#include "Styling/CoreStyle.h"

namespace
{
	/** Paints the tree's connectors in its own place in the widget hierarchy */
	class SCampaignTreeLines : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SCampaignTreeLines) {}
			SLATE_ARGUMENT(TWeakObjectPtr<UCampaignTreeWidget>, Tree)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Tree = InArgs._Tree;
		}

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			const UCampaignTreeWidget* Owner = Tree.Get();
			return Owner ? Owner->PaintConnectors(AllottedGeometry, OutDrawElements, LayerId) : LayerId;
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D::ZeroVector;
		}

	private:
		TWeakObjectPtr<UCampaignTreeWidget> Tree;
	};
}

TSharedRef<SWidget> UCampaignTreeLines::RebuildWidget()
{
	return SNew(SCampaignTreeLines).Tree(Tree);
}

void UCampaignTreeNodeClick::HandleClicked()
{
	if (UCampaignTreeWidget* Owner = Tree.Get())
	{
		Owner->NotifyNodeClicked(Lesson);
	}
}

UCampaignTreeWidget::UCampaignTreeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Start from the engine's standard button look so nodes are visible before any styling
	NodeStyle = GetDefault<UButton>()->GetStyle();

	// Small enough that two-line lesson names fit a node
	NodeFont = FCoreStyle::GetDefaultFontStyle("Bold", 13);
}

void UCampaignTreeWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// PreConstruct also runs in the UMG designer, so the tree shows up in the preview
	RebuildTree();
}

void UCampaignTreeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BackButton)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &UCampaignTreeWidget::HandleBackClicked);
	}

	if (LessonPopup)
	{
		LessonPopup->OnStageChosen.AddUniqueDynamic(this, &UCampaignTreeWidget::HandlePopupStageChosen);
	}

	if (UCampaignSubsystem* Campaign = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCampaignSubsystem>() : nullptr)
	{
		Campaign->OnProgressChanged.AddUniqueDynamic(this, &UCampaignTreeWidget::HandleProgressChanged);
	}
	LoadProgress();
}

void UCampaignTreeWidget::HandlePopupStageChosen(ECampaignLesson Lesson, int32 Stage)
{
	OnLessonStageChosen.Broadcast(Lesson, Stage);
}

void UCampaignTreeWidget::ShowForLessonSession()
{
	LoadProgress();
	SetVisibility(ESlateVisibility::Visible);

	const UCampaignSubsystem* Campaign = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCampaignSubsystem>() : nullptr;
	if (LessonPopup && Campaign && Campaign->IsInLessonSession())
	{
		// Reopened after a puzzle, not by the player: a newly unlocked Master Mode stays hidden
		LessonPopup->ShowLessonAfterPuzzle(Campaign->GetSessionLesson());
	}
}

void UCampaignTreeWidget::NativeDestruct()
{
	if (UCampaignSubsystem* Campaign = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCampaignSubsystem>() : nullptr)
	{
		Campaign->OnProgressChanged.RemoveDynamic(this, &UCampaignTreeWidget::HandleProgressChanged);
	}
	Super::NativeDestruct();
}

void UCampaignTreeWidget::HandleProgressChanged()
{
	LoadProgress();
}

void UCampaignTreeWidget::LoadProgress()
{
	if (const UCampaignSubsystem* Campaign = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCampaignSubsystem>() : nullptr)
	{
		SetCompletedLessons(Campaign->GetCompletedLessons());
	}
}

void UCampaignTreeWidget::HandleBackClicked()
{
	Close();
}

void UCampaignTreeWidget::Close()
{
	SetVisibility(ESlateVisibility::Collapsed);
	OnClosed.Broadcast();
}

void UCampaignTreeWidget::RebuildTree()
{
	if (!TreeCanvas || !WidgetTree)
	{
		return;
	}

	TreeCanvas->ClearChildren();
	NodeButtons.Reset();
	NodeClicks.Reset();
	NodeLessons.Reset();

	// Connectors first, filling the canvas, so they draw behind the nodes (and under anything above the tree)
	Lines = WidgetTree->ConstructWidget<UCampaignTreeLines>(UCampaignTreeLines::StaticClass());
	Lines->Tree = this;
	Lines->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* LinesSlot = TreeCanvas->AddChildToCanvas(Lines))
	{
		LinesSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		LinesSlot->SetOffsets(FMargin(0.f));
	}

	const TArray<TArray<ECampaignLesson>>& Columns = UCampaignTree::GetColumns();
	for (int32 Column = 0; Column < Columns.Num(); Column++)
	{
		const TArray<ECampaignLesson>& Lessons = Columns[Column];
		for (int32 Row = 0; Row < Lessons.Num(); Row++)
		{
			const ECampaignLesson Lesson = Lessons[Row];

			UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
			Button->SetStyle(NodeStyle);

			UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Label->SetText(UCampaignTree::GetLessonDisplayName(Lesson));
			Label->SetJustification(ETextJustify::Center);
			Label->SetAutoWrapText(true);
			Label->SetColorAndOpacity(NodeTextColor);
			if (NodeFont.HasValidFont())
			{
				Label->SetFont(NodeFont);
			}
			if (UButtonSlot* LabelSlot = Cast<UButtonSlot>(Button->AddChild(Label)))
			{
				LabelSlot->SetHorizontalAlignment(HAlign_Center);
				LabelSlot->SetVerticalAlignment(VAlign_Center);
			}

			// Anchor each node to a point in the canvas so the tree scales with the screen:
			// columns evenly across, a column's nodes evenly down
			UCanvasPanelSlot* NodeSlot = TreeCanvas->AddChildToCanvas(Button);
			const float X = (Column + 0.5f) / Columns.Num();
			const float Y = (Row + 1.f) / (Lessons.Num() + 1.f);
			NodeSlot->SetAnchors(FAnchors(X, Y));
			NodeSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			NodeSlot->SetPosition(FVector2D::ZeroVector);
			NodeSlot->SetSize(NodeSize);

			UCampaignTreeNodeClick* Click = NewObject<UCampaignTreeNodeClick>(this);
			Click->Lesson = Lesson;
			Click->Tree = this;
			Button->OnClicked.AddDynamic(Click, &UCampaignTreeNodeClick::HandleClicked);

			NodeButtons.Add(Button);
			NodeClicks.Add(Click);
			NodeLessons.Add(Lesson);
		}
	}

	RefreshNodeStates();
}

void UCampaignTreeWidget::RefreshNodeStates()
{
	for (int32 i = 0; i < NodeButtons.Num(); i++)
	{
		const ECampaignLesson Lesson = NodeLessons[i];
		const bool bUnlocked = IsLessonUnlocked(Lesson);

		NodeButtons[i]->SetIsEnabled(bUnlocked);
		NodeButtons[i]->SetBackgroundColor(IsLessonCompleted(Lesson) ? CompletedTint : (bUnlocked ? AvailableTint : LockedTint));
	}
}

void UCampaignTreeWidget::SetCompletedLessons(const TArray<ECampaignLesson>& Lessons)
{
	CompletedLessons = TSet<ECampaignLesson>(Lessons);
	RefreshNodeStates();
}

void UCampaignTreeWidget::SetLessonCompleted(ECampaignLesson Lesson, bool bCompleted)
{
	if (bCompleted)
	{
		CompletedLessons.Add(Lesson);
	}
	else
	{
		CompletedLessons.Remove(Lesson);
	}
	RefreshNodeStates();
}

bool UCampaignTreeWidget::IsLessonCompleted(ECampaignLesson Lesson) const
{
	return CompletedLessons.Contains(Lesson);
}

bool UCampaignTreeWidget::IsLessonUnlocked(ECampaignLesson Lesson) const
{
	const int32 LessonColumn = UCampaignTree::GetLessonColumn(Lesson);
	if (LessonColumn == INDEX_NONE)
	{
		return false;
	}

	for (int32 Column = 0; Column < LessonColumn; Column++)
	{
		for (ECampaignLesson Earlier : UCampaignTree::GetColumnLessons(Column))
		{
			if (!CompletedLessons.Contains(Earlier))
			{
				return false;
			}
		}
	}
	return true;
}

void UCampaignTreeWidget::NotifyNodeClicked(ECampaignLesson Lesson)
{
	if (IsLessonUnlocked(Lesson))
	{
		if (LessonPopup)
		{
			LessonPopup->ShowLesson(Lesson);
		}
		OnLessonSelected.Broadcast(Lesson);
	}
}

UButton* UCampaignTreeWidget::FindNode(ECampaignLesson Lesson) const
{
	const int32 Index = NodeLessons.IndexOfByKey(Lesson);
	return Index != INDEX_NONE ? NodeButtons[Index].Get() : nullptr;
}

int32 UCampaignTreeWidget::PaintConnectors(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	// Connectors run edge to edge between nodes, so they never cross one
	struct FNodeEdges
	{
		FVector2f Left;
		FVector2f Right;
	};

	auto GetEdges = [&](ECampaignLesson Lesson, FNodeEdges& Out) -> bool
	{
		const UButton* Button = FindNode(Lesson);
		if (!Button)
		{
			return false;
		}
		const FGeometry& Geometry = Button->GetCachedGeometry();
		if (Geometry.GetLocalSize().IsNearlyZero())
		{
			return false;	// not laid out yet
		}
		Out.Left = FVector2f(AllottedGeometry.AbsoluteToLocal(Geometry.GetAbsolutePositionAtCoordinates(FVector2D(0.0, 0.5))));
		Out.Right = FVector2f(AllottedGeometry.AbsoluteToLocal(Geometry.GetAbsolutePositionAtCoordinates(FVector2D(1.0, 0.5))));
		return true;
	};

	const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();
	auto DrawLine = [&](const FVector2f& From, const FVector2f& To)
	{
		TArray<FVector2f> Points = { From, To };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, PaintGeometry, Points, ESlateDrawEffect::None, LineColor, true, LineThickness);
	};

	const TArray<TArray<ECampaignLesson>>& Columns = UCampaignTree::GetColumns();
	for (int32 Column = 0; Column + 1 < Columns.Num(); Column++)
	{
		TArray<FNodeEdges> From, To;
		for (ECampaignLesson Lesson : Columns[Column])
		{
			FNodeEdges Edges;
			if (GetEdges(Lesson, Edges)) From.Add(Edges);
		}
		for (ECampaignLesson Lesson : Columns[Column + 1])
		{
			FNodeEdges Edges;
			if (GetEdges(Lesson, Edges)) To.Add(Edges);
		}
		if (From.IsEmpty() || To.IsEmpty())
		{
			continue;
		}

		// Each column merges to its middle, one line crosses the gap, then it splits to the next column:
		//   A --+           +-- C
		//       +-----------+
		//   B --+           +-- D
		float MaxRight = From[0].Right.X, MinLeft = To[0].Left.X;
		float FromMinY = From[0].Right.Y, FromMaxY = From[0].Right.Y, FromSumY = 0.f;
		float ToMinY = To[0].Left.Y, ToMaxY = To[0].Left.Y;
		for (const FNodeEdges& E : From)
		{
			MaxRight = FMath::Max(MaxRight, E.Right.X);
			FromMinY = FMath::Min(FromMinY, E.Right.Y);
			FromMaxY = FMath::Max(FromMaxY, E.Right.Y);
			FromSumY += E.Right.Y;
		}
		for (const FNodeEdges& E : To)
		{
			MinLeft = FMath::Min(MinLeft, E.Left.X);
			ToMinY = FMath::Min(ToMinY, E.Left.Y);
			ToMaxY = FMath::Max(ToMaxY, E.Left.Y);
		}

		const float MidY = FromSumY / From.Num();
		const float Gap = MinLeft - MaxRight;
		const float MergeX = MaxRight + Gap * 0.25f;
		const float SplitX = MinLeft - Gap * 0.25f;

		// Merge: each node to the merge line, which spans the column's nodes
		for (const FNodeEdges& E : From) DrawLine(E.Right, FVector2f(MergeX, E.Right.Y));
		if (FromMaxY > FromMinY)
		{
			DrawLine(FVector2f(MergeX, FromMinY), FVector2f(MergeX, FromMaxY));
		}

		// Across the gap from the middle
		DrawLine(FVector2f(MergeX, MidY), FVector2f(SplitX, MidY));

		// Split: the split line spans the next column's nodes (and the middle), then out to each node
		const float SplitMinY = FMath::Min(ToMinY, MidY);
		const float SplitMaxY = FMath::Max(ToMaxY, MidY);
		if (SplitMaxY > SplitMinY)
		{
			DrawLine(FVector2f(SplitX, SplitMinY), FVector2f(SplitX, SplitMaxY));
		}
		for (const FNodeEdges& E : To) DrawLine(FVector2f(SplitX, E.Left.Y), E.Left);
	}

	return LayerId;
}
