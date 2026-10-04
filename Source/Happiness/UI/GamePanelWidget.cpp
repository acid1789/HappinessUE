#include "GamePanelWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScaleBox.h"
#include "Components/VerticalBoxSlot.h"

void UGamePanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!Border_44)
	{
		return;
	}

	for (UWidget* Child : Border_44->GetAllChildren())
	{
		UGridPanel* Grid = Cast<UGridPanel>(Child);
		UCanvasPanelSlot* CanvasSlot = Grid ? Cast<UCanvasPanelSlot>(Grid->Slot) : nullptr;
		if (!CanvasSlot || Grid->GetChildrenCount() == 0)
		{
			continue;
		}

		int32 Rows = 0;
		int32 Columns = 0;
		for (UWidget* Cell : Grid->GetAllChildren())
		{
			if (const UGridSlot* CellSlot = Cast<UGridSlot>(Cell->Slot))
			{
				Rows = FMath::Max(Rows, CellSlot->GetRow() + 1);
				Columns = FMath::Max(Columns, CellSlot->GetColumn() + 1);
			}
		}

		// Large puzzles use the strip normally reserved for the inline clue description.
		UUserWidget* Screen = GetParent() ? GetParent()->GetTypedOuter<UUserWidget>() : nullptr;
		UWidget* HintPanel = Screen ? Screen->GetWidgetFromName(TEXT("HelpPanel")) : nullptr;
		UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(Slot);
		UCanvasPanelSlot* HintSlot = HintPanel ? Cast<UCanvasPanelSlot>(HintPanel->Slot) : nullptr;
		if (PanelSlot && HintSlot)
		{
			if (!bSavedHintLayout)
			{
				NormalPanelTop = PanelSlot->GetOffsets().Top;
				NormalHintVisibility = HintPanel->GetVisibility();
				bSavedHintLayout = true;
			}
			const bool bLargePuzzle = Rows >= 7;
			const ESlateVisibility HintVisibility = bLargePuzzle ? ESlateVisibility::Collapsed : NormalHintVisibility;
			if (HintPanel->GetVisibility() != HintVisibility)
			{
				HintPanel->SetVisibility(HintVisibility);
			}
			FMargin PanelOffsets = PanelSlot->GetOffsets();
			const float PanelTop = bLargePuzzle ? HintSlot->GetOffsets().Top : NormalPanelTop;
			if (!FMath::IsNearlyEqual(PanelOffsets.Top, PanelTop))
			{
				PanelOffsets.Top = PanelTop;
				PanelSlot->SetOffsets(PanelOffsets);
			}
		}

		const FVector2D Available = Border_44->GetCachedGeometry().GetLocalSize();
		if (Rows == 0 || Columns == 0 || Available.X <= 0.f || Available.Y <= 0.f)
		{
			continue;
		}
		// Let both icon rows fill the cell; the painted frame draws over their outside edges.
		const float CellHeight = Available.Y / Rows;
		const float FrameInset = 0.f;
		const float IconHeight = FMath::Max(0.f, (CellHeight - FrameInset * 2.f) / 2.f);
		const float ColumnGap = 4.f;
		const int32 IconsPerRow = FMath::DivideAndRoundUp(Columns, 2);
		const float CellWidth = IconHeight * IconsPerRow + FrameInset * 2.f;
		const float GridWidth = FMath::Min(static_cast<float>(Available.X), (CellWidth + ColumnGap) * Columns);
		const float AllocatedCellWidth = GridWidth / Columns - ColumnGap;
		const float ContentWidth = FMath::Max(0.f, AllocatedCellWidth - FrameInset * 2.f);
		const float IconWidth = FMath::Min(IconHeight, ContentWidth / IconsPerRow);
		for (UWidget* Cell : Grid->GetAllChildren())
		{
			if (UGridSlot* CellSlot = Cast<UGridSlot>(Cell->Slot))
			{
				const FMargin CellPadding(ColumnGap / 2.f, 0.f);
				if (CellSlot->GetPadding() != CellPadding)
				{
					CellSlot->SetPadding(CellPadding);
				}
			}
			if (UUserWidget* CellWidget = Cast<UUserWidget>(Cell))
			{
				for (const FName RowName : {FName(TEXT("TopRowIcons")), FName(TEXT("BottomRowIcons"))})
				{
					if (UHorizontalBox* IconRow = Cast<UHorizontalBox>(CellWidget->GetWidgetFromName(RowName)))
					{
						if (UVerticalBoxSlot* RowSlot = Cast<UVerticalBoxSlot>(IconRow->Slot))
						{
							// Give a short row only the width its icons need, with equal space outside the group.
							const float SidePadding = FMath::Max(0.f, (ContentWidth - IconWidth * IconRow->GetChildrenCount()) / 2.f);
							FMargin RowPadding = RowSlot->GetPadding();
							if (!FMath::IsNearlyEqual(RowPadding.Left, SidePadding) || !FMath::IsNearlyEqual(RowPadding.Right, SidePadding))
							{
								RowPadding.Left = RowPadding.Right = SidePadding;
								RowSlot->SetPadding(RowPadding);
							}
							if (RowSlot->GetHorizontalAlignment() != HAlign_Fill)
							{
								RowSlot->SetHorizontalAlignment(HAlign_Fill);
							}
						}
						for (UWidget* Icon : IconRow->GetAllChildren())
						{
							if (UUserWidget* IconWidget = Cast<UUserWidget>(Icon))
							{
								if (UScaleBox* IconScale = Cast<UScaleBox>(IconWidget->GetWidgetFromName(TEXT("ScaleBox"))))
								{
									if (IconScale->GetStretch() != EStretch::ScaleToFit)
									{
										IconScale->SetStretch(EStretch::ScaleToFit);
									}
								}
							}
							if (UHorizontalBoxSlot* IconSlot = Cast<UHorizontalBoxSlot>(Icon->Slot))
							{
								if (IconSlot->GetSize().SizeRule != ESlateSizeRule::Fill)
								{
									IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
								}
							}
						}
					}
				}
			}
		}

		const FAnchors Anchors(0.5f, 0.f, 0.5f, 1.f);
		const FVector2D Alignment(0.5f, 0.f);
		const FMargin Offsets(0.f, 0.f, GridWidth, 0.f);
		if (CanvasSlot->GetAnchors() != Anchors || CanvasSlot->GetAlignment() != Alignment || CanvasSlot->GetOffsets() != Offsets)
		{
			CanvasSlot->SetAnchors(Anchors);
			CanvasSlot->SetAlignment(Alignment);
			CanvasSlot->SetOffsets(Offsets);
		}
		// The outer column padding is only for spacing between cells, not a second border gutter.
		const FMargin SurfaceOffsets(0.f, 0.f, FMath::Max(0.f, GridWidth - ColumnGap), 0.f);
		for (const FName SurfaceName : {FName(TEXT("PanelSurface")), FName(TEXT("PanelFrame"))})
		{
			UWidget* Surface = GetWidgetFromName(SurfaceName);
			if (UCanvasPanelSlot* SurfaceSlot = Surface ? Cast<UCanvasPanelSlot>(Surface->Slot) : nullptr)
			{
				if (SurfaceSlot->GetAnchors() != Anchors || SurfaceSlot->GetAlignment() != Alignment || SurfaceSlot->GetOffsets() != SurfaceOffsets)
				{
					SurfaceSlot->SetAnchors(Anchors);
					SurfaceSlot->SetAlignment(Alignment);
					SurfaceSlot->SetOffsets(SurfaceOffsets);
				}
			}
		}
	}
}
