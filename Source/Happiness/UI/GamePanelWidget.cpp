#include "GamePanelWidget.h"
#include "GameCellWidget.h"
#include "HappinessClassic/Puzzle.h"
#include "HappinessClassic/CampaignProgress.h"
#include "UI/ButtonPanelWidget.h"
#include "UI/CampaignTreeWidget.h"
#include "UI/ClueWidget.h"
#include "UI/FocusOverlay.h"
#include "UI/GameRulesWidget.h"
#include "UI/HappinessModeWidget.h"
#include "UI/TutorialPopup.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"

namespace TutorialMessageIcons
{
	// The tutorial puzzle's icons a message can show (<icon id="Triangle"/>), from its fixed sets (Icons/Basic, cooked
	// with the sets: DA_IconSetUnlocks lists them)
	const TCHAR* const Heart = TEXT("/Game/Happiness/IconSets/Icons/Basic/MasterRewardGems/T_Heart.T_Heart");
	const TCHAR* const Triangle = TEXT("/Game/Happiness/IconSets/Icons/Basic/MasterRewardGems/T_Triangle.T_Triangle");
	const TCHAR* const Star = TEXT("/Game/Happiness/IconSets/Icons/Basic/MasterRewardGems/T_Star.T_Star");
	const TCHAR* const Dragon = TEXT("/Game/Happiness/IconSets/Icons/Basic/MythicalCreatures/T_Dragon.T_Dragon");
	const TCHAR* const Phoenix = TEXT("/Game/Happiness/IconSets/Icons/Basic/MythicalCreatures/T_Phoenix.T_Phoenix");
	const TCHAR* const Unicorn = TEXT("/Game/Happiness/IconSets/Icons/Basic/MythicalCreatures/T_Unicorn.T_Unicorn");
	const TCHAR* const Butterfly = TEXT("/Game/Happiness/IconSets/Icons/Basic/GardenVisitors/T_Butterfly.T_Butterfly");
	const TCHAR* const Bee = TEXT("/Game/Happiness/IconSets/Icons/Basic/GardenVisitors/T_Bee.T_Bee");
	const TCHAR* const Ladybug = TEXT("/Game/Happiness/IconSets/Icons/Basic/GardenVisitors/T_Ladybug.T_Ladybug");

	// The texture an icon's image (WBP_Icon's TheIcon) shows: its brush's, or its material's Icon parameter
	const UTexture* ShownTexture(const UWidget* Image)
	{
		const UImage* AsImage = Cast<UImage>(Image);
		UObject* Resource = AsImage ? AsImage->GetBrush().GetResourceObject() : nullptr;
		if (const UMaterialInterface* Material = Cast<UMaterialInterface>(Resource))
		{
			UTexture* Texture = nullptr;
			return Material->GetTextureParameterValue(FHashedMaterialParameterInfo(TEXT("Icon")), Texture) ? Texture : nullptr;
		}
		return Cast<UTexture>(Resource);
	}

	// The shown WBP_Icon under Widget (into user widgets' trees and panels' children, made at runtime included) that
	// shows Texture
	UWidget* FindShownIcon(UWidget* Widget, const UTexture* Texture)
	{
		if (!Widget || !Widget->IsVisible())
		{
			return nullptr;
		}
		if (UUserWidget* User = Cast<UUserWidget>(Widget))
		{
			const UWidget* Image = User->GetWidgetFromName(TEXT("TheIcon"));
			if (Image && Image->IsVisible() && ShownTexture(Image) == Texture)
			{
				return User;
			}
			return User->WidgetTree ? FindShownIcon(User->WidgetTree->RootWidget, Texture) : nullptr;
		}
		if (const UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
		{
			for (UWidget* Child : Panel->GetAllChildren())
			{
				if (UWidget* Found = FindShownIcon(Child, Texture))
				{
					return Found;
				}
			}
		}
		return nullptr;
	}
}

bool UGamePanelWidget::PlaySolvedFireworks(UPuzzle* Puzzle)
{
	if (Fireworks == EFireworks::Playing)
	{
		return true;
	}
	if (Fireworks == EFireworks::Finished && FireworksPuzzle == Puzzle)
	{
		// Back from the fireworks: the end screen now (a restart and solve plays them again)
		Fireworks = EFireworks::None;
		return false;
	}
	if (!Puzzle || !Puzzle->IsSolved())
	{
		return false;
	}
	// The tutorial puzzle: no fireworks and no end screen (true: the end screen waits, and OnFireworksFinished never
	// comes). The board stays completed for the rest of the tutorial
	if (UCampaignSubsystem::IsPlayingTutorial(this))
	{
		return true;
	}

	EndFireworks();
	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		if (UGameCellWidget* Cell = Cast<UGameCellWidget>(Widget))
		{
			FireworkCells.Add(Cell);
		}
	});
	for (int32 i = FireworkCells.Num() - 1; i > 0; i--)
	{
		FireworkCells.Swap(i, FMath::RandRange(0, i));
	}
	for (const TWeakObjectPtr<UGameCellWidget>& Cell : FireworkCells)
	{
		Cell->PrepareFirework();
	}
	bFireworksPrepared = false;
	Fireworks = EFireworks::Playing;
	FireworksPuzzle = Puzzle;
	FireworksTime = 0.f;
	FireworksInterval = FireworkCells.Num() > 0 ? FireworksDuration / FireworkCells.Num() : 0.f;
	NumFired = 0;
	bBursting = true;
	return true;
}

bool UGamePanelWidget::IsFireworksPuzzleComplete() const
{
	const UPuzzle* Puzzle = FireworksPuzzle.Get();
	if (!Puzzle)
	{
		return false;
	}
	for (const FPuzzleRow& Row : Puzzle->m_Rows)
	{
		if (!Row.IsCompleted())
		{
			return false;
		}
	}
	return true;
}

void UGamePanelWidget::EndFireworks()
{
	for (const TWeakObjectPtr<UGameCellWidget>& Cell : FiredCells)
	{
		if (Cell.IsValid())
		{
			Cell->EndFirework();
		}
	}
	FireworkCells.Reset();
	FiredCells.Reset();
	bBursting = false;
}

void UGamePanelWidget::UpdateFireworks(float DeltaTime)
{
	if (!bFireworksPrepared)
	{
		bFireworksPrepared = true;
		return;
	}

	// The icons go off one after another, evenly spread over FireworksDuration: the first at once, the next
	// FireworksInterval later, and so on
	while (FireworkCells.Num() > 0 && FireworksTime >= NumFired * FireworksInterval)
	{
		NumFired++;
		UGameCellWidget* Cell = FireworkCells.Pop().Get();
		if (Cell && Cell->PlayFirework())
		{
			FiredCells.Add(Cell);
		}
	}

	FireworksTime += DeltaTime;
	if (FireworkCells.Num() == 0 && FireworksTime >= FireworksDuration)
	{
		// All gone off. The icons stay gone until the puzzle changes (NativeTick).
		bBursting = false;
	}
	if (Fireworks == EFireworks::Playing && FireworksTime >= EndScreenDelay)
	{
		Fireworks = EFireworks::Finished;
		OnFireworksFinished.Broadcast();
	}
}

void UGamePanelWidget::UpdateTutorialFocus(UGridPanel* Grid)
{
	// The puzzle screen (this panel's screen, WBP_Happiness) and its canvas, which the focus and popup go in
	UUserWidget* Screen = GetParent() ? GetParent()->GetTypedOuter<UUserWidget>() : nullptr;
	UCanvasPanel* ScreenCanvas = Screen ? Cast<UCanvasPanel>(Screen->GetRootWidget()) : nullptr;

	const int32 Start = UCampaignSubsystem::IsPlayingTutorial(this) ? UCampaignSubsystem::GetTutorialStarts(this) : -1;
	if (Start == TutorialFocusStart && (Start < 0 || TutorialFocus.IsValid()))
	{
		// Once the circle has got where the step takes it: the step's popup
		if (Start >= 0 && Grid && ScreenCanvas)
		{
			TutorialFocusTime += FSlateApplication::Get().GetDeltaTime();
			UpdateTutorialRowBoxes(Grid, FSlateApplication::Get().GetDeltaTime());
			// Mouse, a cells step: its icon clicked into its cell (GetTutorialStepCell; the elimination: the ladybug right
			// clicked out of it, and the game sets the butterfly, its only option left). Input stops, and the popup's Next
			// shows at once (it kept its room)
			const UUserWidget* CellDialog = GetCellDialog();
			const bool bMouseCellsStep = !IsTutorialTouch()
				&& (TutorialStep == 6 || TutorialStep == 11 || TutorialStep == 17 || TutorialStep == 23 || TutorialStep == 27);
			int32 StepRow = 0, StepColumn = 0;
			GetTutorialStepCell(StepRow, StepColumn);
			const bool bStepCellPlaced = GetTutorialCellsStep() >= 0 && IsTutorialCellPlaced(StepRow, StepColumn);
			const bool bCellDialogClosed = !(CellDialog && CellDialog->IsVisible());
			if (bMouseCellsStep && !bTutorialPlacedNext && bStepCellPlaced)
			{
				bTutorialPlacedNext = true;
				if (TutorialFocus.IsValid())
				{
					TutorialFocus->SetInputWindow({});
					TutorialFocus->SetBlocksInput(true);
				}
				if (TutorialPopup.IsValid())
				{
					TutorialPopup->ShowNextNow();
				}
				// Not shown yet: it comes now, with Next (ShowTutorialPopup)
				TutorialFocusTime = FMath::Max(TutorialFocusTime, TutorialFocusSeconds);
			}
			// Touch: the step's icon committed in the cell window (now closed): the star, the triangle, the phoenix, the bee,
			// the ladybug's elimination (the game then sets the cell)
			else if (TutorialStep == 9 && bStepCellPlaced && bCellDialogClosed)
			{
				StartTutorialHorizontalStep();
			}
			else if (TutorialStep == 14 && bStepCellPlaced && bCellDialogClosed)
			{
				StartTutorialOnlyOptionStep();
			}
			else if (TutorialStep == 20 && bStepCellPlaced && bCellDialogClosed)
			{
				StartTutorialHintStep();
			}
			else if (TutorialStep == 26 && bStepCellPlaced && bCellDialogClosed)
			{
				StartTutorialEliminateStep();
			}
			else if (TutorialStep == 30 && bStepCellPlaced && bCellDialogClosed)
			{
				StartTutorialSolvedStep();
			}
			// The hint's confirm window confirmed: the screen has its hint (WBP_Happiness.CurrentHint; Cancel leaves none)
			else if (TutorialStep == 21)
			{
				const FObjectPropertyBase* CurrentHint = FindFProperty<FObjectPropertyBase>(Screen->GetClass(), TEXT("CurrentHint"));
				if (CurrentHint && CurrentHint->GetObjectPropertyValue_InContainer(Screen))
				{
					StartTutorialHintShownStep();
				}
			}
			// Touch, a cells step: the cell window open on its cell (it shows the step's icon) starts the window's steps
			else if (TutorialStep == GetTutorialCellsStep() && IsTutorialTouch())
			{
				if (UWidget* Icon = FindCellDialogIcon(LoadObject<UTexture2D>(nullptr, GetTutorialStepIcon())))
				{
					StartTutorialDialogStep(Icon);
				}
			}
			// The window's icon selected (WBP_CellDialog's SelectedIcon, the icon tapped): on to its Confirm (or Eliminate)
			// button
			else if (GetTutorialCellsStep() >= 0 && TutorialStep == GetTutorialCellsStep() + 1)
			{
				UUserWidget* Dialog = GetCellDialog();
				const FObjectPropertyBase* Selected = Dialog
					? FindFProperty<FObjectPropertyBase>(Dialog->GetClass(), TEXT("SelectedIcon")) : nullptr;
				UWidget* Icon = FindCellDialogIcon(LoadObject<UTexture2D>(nullptr, GetTutorialStepIcon()));
				if (Selected && Icon && Selected->GetObjectPropertyValue_InContainer(Dialog) == Icon)
				{
					StartTutorialConfirmStep(Icon);
				}
			}
			// Confirm pressed (only it takes input now): it places the selected icon and clears SelectedIcon. Eliminate
			// pressed: it hides the icon, and its button says Restore (the window's EliminatedButtonText)
			else if (GetTutorialCellsStep() >= 0 && TutorialStep == GetTutorialCellsStep() + 2)
			{
				UUserWidget* Dialog = GetCellDialog();
				if (IsTutorialEliminateStep())
				{
					const FStrProperty* ButtonText = Dialog
						? FindFProperty<FStrProperty>(Dialog->GetClass(), TEXT("EliminatedButtonText")) : nullptr;
					if (ButtonText && Dialog->IsVisible() && ButtonText->GetPropertyValue_InContainer(Dialog) == TEXT("Restore"))
					{
						StartTutorialCommitStep();
					}
				}
				else
				{
					const FObjectPropertyBase* Selected = Dialog
						? FindFProperty<FObjectPropertyBase>(Dialog->GetClass(), TEXT("SelectedIcon")) : nullptr;
					if (Selected && Dialog->IsVisible() && !Selected->GetObjectPropertyValue_InContainer(Dialog))
					{
						StartTutorialCommitStep();
					}
				}
			}
			if (!TutorialPopup.IsValid() && TutorialStep <= 31 && TutorialFocusTime >= TutorialFocusSeconds)
			{
				const FText Messages[] = {
					NSLOCTEXT("Tutorial", "PuzzleGrid",
						"This is the puzzle grid.\nThe objective of the game is to figure out where each icon belongs in the grid."),
					NSLOCTEXT("Tutorial", "Categories",
						"The puzzle has one category per row. Each icon in a category belongs in one of the columns."),
					NSLOCTEXT("Tutorial", "Columns", "Your goal is to figure out which column each icon belongs in."),
					NSLOCTEXT("Tutorial", "VerticalClues",
						"These are vertical clues.\nThey describe true things about a single column.\nYou can use this "
						"information to determine where icons belong in a single column of the puzzle grid."),
					NSLOCTEXT("Tutorial", "HorizontalClues",
						"These are horizontal clues.\nThey describe true things about multiple columns.\nThey compare columns left to "
						"right, no matter which row each icon is in."),
					// <icon/>: the tutorial's icons (ShowTutorialPopup)
					NSLOCTEXT("Tutorial", "SameColumnClue",
						"This clue says <icon id=\"Star\"/> and <icon id=\"Unicorn\"/> are in the same column."),
					IsTutorialTouch()
						? NSLOCTEXT("Tutorial", "SameColumnCells",
							"Since we know where the <icon id=\"Unicorn\"/> is, the <icon id=\"Star\"/> must be in this column too.\n"
							"Tap the cell with the <icon id=\"Star\"/> above the <icon id=\"Unicorn\"/>.")
						: NSLOCTEXT("Tutorial", "SameColumnCellsMouse",
							"Since we know where the <icon id=\"Unicorn\"/> is, the <icon id=\"Star\"/> must be in this column too.\n"
							"Left Click the <icon id=\"Star\"/> icon to set it in this spot in the grid."),
					NSLOCTEXT("Tutorial", "SelectDialogStar", "Tap on the <icon id=\"Star\"/> icon to select it."),
					NSLOCTEXT("Tutorial", "ConfirmDialogStar", "Tap the Confirm button to set the <icon id=\"Star\"/> in this cell."),
					NSLOCTEXT("Tutorial", "CommitDialog", "Tap the Commit button to apply this change to the puzzle."),
					NSLOCTEXT("Tutorial", "NextToClue",
						"This clue says the <icon id=\"Triangle\"/> is in a column next to the <icon id=\"Star\"/>."),
					IsTutorialTouch()
						? NSLOCTEXT("Tutorial", "NextToCells",
							"There is only one place that <icon id=\"Triangle\"/> can go to comply with the clue.\n"
							"Tap the middle cell containing the <icon id=\"Triangle\"/>.")
						: NSLOCTEXT("Tutorial", "NextToCellsMouse",
							"There is only one place that <icon id=\"Triangle\"/> can go to comply with the clue.\n"
							"Left click the <icon id=\"Triangle\"/> in the middle cell to set it here."),
					NSLOCTEXT("Tutorial", "SelectDialogTriangle", "Tap on the <icon id=\"Triangle\"/> icon to select it."),
					NSLOCTEXT("Tutorial", "ConfirmDialogTriangle",
						"Tap the Confirm button to set the <icon id=\"Triangle\"/> in this cell."),
					NSLOCTEXT("Tutorial", "CommitDialog", "Tap the Commit button to apply this change to the puzzle."),
					NSLOCTEXT("Tutorial", "OnlyOption",
						"Notice the <icon id=\"Heart\"/> is now set here as the only option left for this cell."),
					NSLOCTEXT("Tutorial", "AlsoNextToClue",
						"This clue also says the <icon id=\"Phoenix\"/> is in a column next to the <icon id=\"Triangle\"/>"),
					IsTutorialTouch()
						? NSLOCTEXT("Tutorial", "FarLeftCells",
							"The <icon id=\"Star\"/> is in the right column. The <icon id=\"Triangle\"/> is in the middle column. "
							"The clue says the next column should have the <icon id=\"Phoenix\"/>, so the <icon id=\"Phoenix\"/> "
							"belongs in the left column.\n"
							"Tap the far left cell containing the <icon id=\"Phoenix\"/>")
						: NSLOCTEXT("Tutorial", "FarLeftCellsMouse",
							"The <icon id=\"Star\"/> is in the right column. The <icon id=\"Triangle\"/> is in the middle column. "
							"The clue says the next column should have the <icon id=\"Phoenix\"/>, so the <icon id=\"Phoenix\"/> "
							"belongs in the left column.\n"
							"Left click the <icon id=\"Phoenix\"/> to set it here."),
					NSLOCTEXT("Tutorial", "SelectDialogPhoenix", "Tap on the <icon id=\"Phoenix\"/> icon to select it."),
					NSLOCTEXT("Tutorial", "ConfirmDialogPhoenix", "Tap the Confirm button to set the <icon id=\"Phoenix\"/> in this cell."),
					NSLOCTEXT("Tutorial", "CommitDialog", "Tap the Commit button to apply this change to the puzzle."),
					NSLOCTEXT("Tutorial", "HintButton",
						"If you don't know what to do next, press the Hint button. Hints wont hurt you, they can only help you to "
						"improve. Press the Hint button now."),
					NSLOCTEXT("Tutorial", "HintShown",
						"This clue can help you right now. The highlighted icon is the one it's talking about, and the description tells you "
						"what to do with it."),
					NSLOCTEXT("Tutorial", "FollowHint",
						"Follow the hint and set the <icon id=\"Bee\"/> in the middle column with the "
						"<icon id=\"Dragon\"/>"),
					NSLOCTEXT("Tutorial", "SelectDialogBee", "Tap on the <icon id=\"Bee\"/> icon to select it."),
					NSLOCTEXT("Tutorial", "ConfirmDialogBee", "Tap the Confirm button to set the <icon id=\"Bee\"/> in this cell."),
					NSLOCTEXT("Tutorial", "CommitDialog", "Tap the Commit button to apply this change to the puzzle."),
					IsTutorialTouch()
						? NSLOCTEXT("Tutorial", "NotSameColumnCells",
							"This clue says the <icon id=\"Ladybug\"/> is not in the same column as the <icon id=\"Phoenix\"/>\n"
							"Tap the cell below the <icon id=\"Phoenix\"/> to eliminate the <icon id=\"Ladybug\"/> from it.")
						: NSLOCTEXT("Tutorial", "NotSameColumnCellsMouse",
							"This clue says the <icon id=\"Ladybug\"/> is not in the same column as the <icon id=\"Phoenix\"/>.\n"
							"Right click the <icon id=\"Ladybug\"/> below the <icon id=\"Phoenix\"/> to eliminate it from this cell."),
					NSLOCTEXT("Tutorial", "SelectDialogLadybug", "Tap on the <icon id=\"Ladybug\"/> icon to select it."),
					NSLOCTEXT("Tutorial", "EliminateDialogLadybug",
						"Tap the Eliminate button to remove the <icon id=\"Ladybug\"/> from this cell."),
					NSLOCTEXT("Tutorial", "CommitDialog", "Tap the Commit button to apply this change to the puzzle."),
					NSLOCTEXT("Tutorial", "Solved",
						"This is what a fully solved puzzle looks like. All icons are set correctly in their places. Now you are ready "
						"to move on to the Lessons and learn how each clue works. If you want more information, check out the Game "
						"Rules."),
				};
				ShowTutorialPopup(Screen, ScreenCanvas, Messages[TutorialStep]);
			}
			PlaceTutorialPopup(Grid, ScreenCanvas);
		}
		return;
	}
	// Not the tutorial, or a new start of it: the last focus goes
	if (TutorialFocus.IsValid())
	{
		TutorialFocus->SetInputWindow({});
		TutorialFocus->RemoveFromParent();
	}
	if (TutorialPopup.IsValid())
	{
		TutorialPopup->RemoveFromParent();
	}
	TutorialFocus.Reset();
	TutorialPopup.Reset();
	for (const TWeakObjectPtr<UImage>& Box : TutorialRowBoxes)
	{
		if (Box.IsValid())
		{
			Box->RemoveFromParent();
		}
	}
	TutorialRowBoxes.Reset();
	for (const TPair<TWeakObjectPtr<UButton>, FButtonStyle>& Tinted : TutorialRowButtonStyles)
	{
		if (Tinted.Key.IsValid())
		{
			Tinted.Key->SetStyle(Tinted.Value);
		}
	}
	TutorialRowButtonStyles.Reset();
	TutorialRowTime = -1.f;
	bTutorialColorColumns = false;
	TutorialFocusStart = Start;
	TutorialFocusTime = 0.f;
	TutorialStep = 0;
	bTutorialPlacedNext = false;
	if (Start < 0)
	{
		return;
	}

	// Over the whole puzzle screen, once the grid is laid out
	const FVector2D ScreenSize = ScreenCanvas ? ScreenCanvas->GetCachedGeometry().GetLocalSize() : FVector2D::ZeroVector;
	if (!Grid || !ScreenCanvas || ScreenSize.IsNearlyZero() || Grid->GetCachedGeometry().GetLocalSize().IsNearlyZero())
	{
		TutorialFocusStart = -1; // try again next frame
		return;
	}
	UFocusOverlay* Overlay = Screen->WidgetTree->ConstructWidget<UFocusOverlay>(UFocusOverlay::StaticClass());
	UCanvasPanelSlot* OverlaySlot = ScreenCanvas->AddChildToCanvas(Overlay);
	OverlaySlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	OverlaySlot->SetOffsets(FMargin(0.f));
	OverlaySlot->SetZOrder(TutorialFocusZOrder);
	// From beyond every corner of the screen (its diagonal, wherever the grid is) down to the circle round the grid
	Overlay->AddFocusOnWidget(Grid, 0.f, ScreenSize.Size(), TutorialFocusSeconds);
	// The grid step: nothing but the popup's Next button can be used, from the start of the circle's animation
	Overlay->SetBlocksInput(true);
	TutorialFocus = Overlay;
	TutorialGrid = Grid;
}

void UGamePanelWidget::UpdateTutorialRowBoxes(UGridPanel* Grid, float DeltaTime)
{
	UCanvasPanel* Canvas = Cast<UCanvasPanel>(Grid->GetParent());
	const UCanvasPanelSlot* GridSlot = Cast<UCanvasPanelSlot>(Grid->Slot);
	if (TutorialRowTime < 0.f || !Canvas || !GridSlot)
	{
		return;
	}
	TutorialRowTime += DeltaTime;
	// Each row's color (or, from the columns step, each column's), behind its icons: its cells' buttons (WBP_Cell's
	// Button_Cell, whose background is under the cell's icons) take it, and a box of it behind the grid fills the gaps
	// between them. Row (column) 1's shows first, then each next one's TutorialRowDelay later (a short fade in)
	static const FLinearColor RowColors[] = {
		FLinearColor(.75f, .22f, .1f), FLinearColor(.1f, .32f, .8f), FLinearColor(.15f, .6f, .15f),
		FLinearColor(.75f, .6f, .1f), FLinearColor(.5f, .2f, .75f), FLinearColor(.1f, .6f, .6f),
		FLinearColor(.75f, .25f, .5f), FLinearColor(.4f, .4f, .4f) };
	constexpr float TutorialRowDelay = .5f;
	constexpr float FadeSeconds = .2f;
	constexpr float Outset = 3.f;
	const FGeometry& CanvasGeometry = Canvas->GetCachedGeometry();
	TArray<FBox2D> Rows;
	for (UWidget* Cell : Grid->GetAllChildren())
	{
		const UGridSlot* CellSlot = Cell ? Cast<UGridSlot>(Cell->Slot) : nullptr;
		if (!CellSlot)
		{
			continue;
		}
		const int32 Row = bTutorialColorColumns ? CellSlot->GetColumn() : CellSlot->GetRow();
		const FGeometry& CellGeometry = Cell->GetCachedGeometry();
		const FBox2D Bounds(CanvasGeometry.AbsoluteToLocal(CellGeometry.LocalToAbsolute(FVector2D::ZeroVector)),
			CanvasGeometry.AbsoluteToLocal(CellGeometry.LocalToAbsolute(CellGeometry.GetLocalSize())));
		while (Rows.Num() <= Row)
		{
			Rows.Add(FBox2D(ForceInit));
		}
		Rows[Row] += Bounds;
		// The cell's button: its normal, hovered and pressed backgrounds from their own tints to the row's color
		const UUserWidget* CellWidget = Cast<UUserWidget>(Cell);
		UButton* Button = CellWidget ? Cast<UButton>(CellWidget->GetWidgetFromName(TEXT("Button_Cell"))) : nullptr;
		if (!Button)
		{
			continue;
		}
		if (!TutorialRowButtonStyles.Contains(Button))
		{
			TutorialRowButtonStyles.Add(Button, Button->GetStyle());
		}
		const float Alpha = FMath::Clamp((TutorialRowTime - Row * TutorialRowDelay) / FadeSeconds, 0.f, 1.f);
		const FButtonStyle& Original = TutorialRowButtonStyles[Button];
		const FLinearColor& Color = RowColors[Row % UE_ARRAY_COUNT(RowColors)];
		FButtonStyle Style = Original;
		const auto Tint = [Alpha](FSlateBrush& Brush, const FSlateBrush& From, const FLinearColor& To)
		{
			Brush.TintColor = FMath::Lerp(From.TintColor.GetSpecifiedColor(), To, Alpha);
		};
		Tint(Style.Normal, Original.Normal, Color);
		Tint(Style.Hovered, Original.Hovered, Color * 1.25f);
		Tint(Style.Pressed, Original.Pressed, Color * .8f);
		if (Style.Normal.TintColor != Button->GetStyle().Normal.TintColor)
		{
			Button->SetStyle(Style);
		}
	}
	for (int32 Row = 0; Row < Rows.Num(); ++Row)
	{
		if (!Rows[Row].bIsValid)
		{
			continue;
		}
		if (!TutorialRowBoxes.IsValidIndex(Row) || !TutorialRowBoxes[Row].IsValid())
		{
			UImage* Box = NewObject<UImage>(this);
			FSlateBrush Brush;
			Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
			Brush.TintColor = RowColors[Row % UE_ARRAY_COUNT(RowColors)];
			Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(6.f, 6.f, 6.f, 6.f),
				FSlateColor(RowColors[Row % UE_ARRAY_COUNT(RowColors)]), 0.f);
			Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
			Box->SetBrush(Brush);
			Box->SetVisibility(ESlateVisibility::HitTestInvisible);
			Box->SetRenderOpacity(0.f);
			UCanvasPanelSlot* BoxSlot = Canvas->AddChildToCanvas(Box);
			// Behind the grid (and over the panel's surface, WBP_GamePanel's PanelSurface at Z -10)
			BoxSlot->SetZOrder(GridSlot->GetZOrder() - 1);
			BoxSlot->SetAnchors(FAnchors(0.f, 0.f));
			BoxSlot->SetAlignment(FVector2D::ZeroVector);
			while (TutorialRowBoxes.Num() <= Row)
			{
				TutorialRowBoxes.Add(nullptr);
			}
			TutorialRowBoxes[Row] = Box;
		}
		UImage* Box = TutorialRowBoxes[Row].Get();
		if (UCanvasPanelSlot* BoxSlot = Cast<UCanvasPanelSlot>(Box->Slot))
		{
			// A column's box: out to half the gap between columns (the cells' padding), and a little above and below
			const FVector2D Out = bTutorialColorColumns ? FVector2D(2.f, Outset) : FVector2D(Outset, 0.f);
			const FVector2D Position = Rows[Row].Min - Out;
			const FVector2D Size = Rows[Row].GetSize() + Out * 2.f;
			if (!BoxSlot->GetPosition().Equals(Position, .5f) || !BoxSlot->GetSize().Equals(Size, .5f))
			{
				BoxSlot->SetPosition(Position);
				BoxSlot->SetSize(Size);
			}
		}
		Box->SetRenderOpacity(FMath::Clamp((TutorialRowTime - Row * TutorialRowDelay) / FadeSeconds, 0.f, 1.f));
	}
}

void UGamePanelWidget::HandleTutorialNext()
{
	// Mouse, a cells step's icon placed: its Next moves on as the touch path's Commit does
	if (bTutorialPlacedNext)
	{
		bTutorialPlacedNext = false;
		if (TutorialStep == 6)
		{
			StartTutorialHorizontalStep();
			return;
		}
		if (TutorialStep == 11)
		{
			StartTutorialOnlyOptionStep();
			return;
		}
		if (TutorialStep == 17)
		{
			StartTutorialHintStep();
			return;
		}
		if (TutorialStep == 23)
		{
			StartTutorialEliminateStep();
			return;
		}
		if (TutorialStep == 27)
		{
			StartTutorialSolvedStep();
		}
		return;
	}
	// The last step's Next is Proceed to Lessons
	if (TutorialStep == 31)
	{
		ProceedFromTutorial();
		return;
	}
	// The next step: the popup goes
	TutorialStep++;
	TutorialFocusTime = 0.f;
	if (TutorialPopup.IsValid())
	{
		TutorialPopup->RemoveFromParent();
	}
	TutorialPopup.Reset();
	if (!TutorialFocus.IsValid())
	{
		return;
	}
	if (TutorialStep == 1)
	{
		// The categories: the grid's circle stays (input still blocked but for Next); the rows' boxes show one by one
		TutorialRowTime = 0.f;
	}
	else if (TutorialStep == 2)
	{
		// The columns: the grid's circle stays; the columns' colors show one by one instead of the rows'
		TutorialRowTime = 0.f;
		bTutorialColorColumns = true;
		for (const TWeakObjectPtr<UImage>& Box : TutorialRowBoxes)
		{
			if (Box.IsValid())
			{
				Box->RemoveFromParent();
			}
		}
		TutorialRowBoxes.Reset();
	}
	else if (TutorialStep == 3)
	{
		// The vertical clues: the grid's circle moves to them (input still blocked; their popup follows)
		TutorialFocus->ClearFocus();
		TutorialFocus->AddFocusMove({ TutorialGrid.Get() }, GetShownClues(TEXT("VerticalCluePanel")), 8.f, TutorialFocusSeconds);
	}
	else if (TutorialStep == 4)
	{
		// The horizontal clues: the circle moves on from the vertical ones (input still blocked; their popup follows)
		TutorialFocus->ClearFocus();
		TutorialFocus->AddFocusMove(GetShownClues(TEXT("VerticalCluePanel")), GetShownClues(TEXT("HorizontalCluePanel")), 8.f,
			TutorialFocusSeconds, 8.f);
	}
	else if (TutorialStep == 5)
	{
		// The first vertical clue (the tutorial puzzle's star over the unicorn): the circle moves from the horizontal
		// clues to tight round its two icons
		const TArray<UWidget*> VerticalClues = GetShownClues(TEXT("VerticalCluePanel"), true);
		TutorialFocus->ClearFocus();
		if (VerticalClues.IsValidIndex(0))
		{
			TutorialFocus->AddFocusMove(GetShownClues(TEXT("HorizontalCluePanel")), { VerticalClues[0] }, 0.f,
				TutorialFocusSeconds, 8.f, true);
		}
	}
	else if (TutorialStep == 6)
	{
		// The clue's cells: its circle stays, and a second moves off it to tight round column 3's top two cells (the
		// star's, over the given unicorn). Only the star's takes input (and the cell window, when it opens over everything)
		const TArray<UWidget*> VerticalClues = GetShownClues(TEXT("VerticalCluePanel"), true);
		const TArray<UWidget*> Cells = GetTutorialStepCells();
		if (VerticalClues.IsValidIndex(0))
		{
			TutorialFocus->AddFocusMove({ VerticalClues[0] }, Cells, 0.f, TutorialFocusSeconds, 0.f, true, true);
		}
		TArray<UWidget*> Window = GetGridCells(2, 0, 0);
		const UUserWidget* Screen = GetParent() ? GetParent()->GetTypedOuter<UUserWidget>() : nullptr;
		if (UWidget* CellDialog = Screen && IsTutorialTouch() ? Screen->GetWidgetFromName(TEXT("CellDialog")) : nullptr)
		{
			Window.Add(CellDialog);
		}
		TutorialFocus->SetInputWindow(Window);
	}
	else if (TutorialStep == 11)
	{
		// The triangle's cells: a circle moves from the horizontal clue to tight round row 1's cells in columns 2 and 3
		// (next to the star). Only the middle one takes input (and the cell window, touch)
		// The clue's circle stays (as the cells step's); the second moves off it
		TutorialFocus->AddFocusMove(GetShownClues(TEXT("HorizontalCluePanel")), GetTutorialStepCells(), 0.f, TutorialFocusSeconds, 8.f,
			true);
		TArray<UWidget*> Window = GetGridCells(1, 0, 0);
		if (UWidget* CellDialog = IsTutorialTouch() ? GetCellDialog() : nullptr)
		{
			Window.Add(CellDialog);
		}
		TutorialFocus->SetInputWindow(Window);
	}
	else if (TutorialStep == 16)
	{
		// Back to the horizontal clue: the circle moves from round the heart's cell (input still blocked but for Next)
		TutorialFocus->ClearFocus();
		TutorialFocus->AddFocusMove(GetGridCells(0, 0, 0), GetShownClues(TEXT("HorizontalCluePanel")), 8.f, TutorialFocusSeconds);
	}
	else if (TutorialStep == 17)
	{
		// The phoenix's cells: a circle moves from the horizontal clue to round rows 1-2 of columns 1-2 (through the four
		// cells' outer corners). Only the phoenix's cell (row 2, column 1) takes input (and the cell window, touch)
		// The clue's circle stays (as the cells step's); the second moves off it
		TutorialFocus->AddFocusMove(GetShownClues(TEXT("HorizontalCluePanel")), GetTutorialStepCells(), 0.f, TutorialFocusSeconds, 8.f);
		TArray<UWidget*> Window = GetGridCells(0, 1, 1);
		if (UWidget* CellDialog = IsTutorialTouch() ? GetCellDialog() : nullptr)
		{
			Window.Add(CellDialog);
		}
		TutorialFocus->SetInputWindow(Window);
	}
	else if (TutorialStep == 23)
	{
		// The bee's cell (row 3, column 2): the hint's circles stay; only that cell takes input (and the cell window,
		// touch)
		TArray<UWidget*> Window = GetGridCells(1, 2, 2);
		if (UWidget* CellDialog = IsTutorialTouch() ? GetCellDialog() : nullptr)
		{
			Window.Add(CellDialog);
		}
		TutorialFocus->SetInputWindow(Window);
	}
	else
	{
		// No later step yet: the focus stays, and input goes through again
		TutorialFocus->SetBlocksInput(false);
	}
}

void UGamePanelWidget::ShowTutorialPopup(UUserWidget* Screen, UCanvasPanel* ScreenCanvas, const FText& Message)
{
	UTutorialPopup* Popup = Screen->WidgetTree->ConstructWidget<UTutorialPopup>(UTutorialPopup::StaticClass());
	// The cells step to the cell window's Commit move on by what the player does, not Next
	Popup->bShowNext = TutorialStep < 6 || TutorialStep == 10 || TutorialStep == 15 || TutorialStep == 16 || TutorialStep == 22
		|| bTutorialPlacedNext;
	// Mouse, a cells step: Next comes once the icon is clicked in, so the popup keeps its room from the start (no
	// resize then), and once clicked it's there at once
	Popup->bReserveNext = !IsTutorialTouch()
		&& (TutorialStep == 6 || TutorialStep == 11 || TutorialStep == 17 || TutorialStep == 23 || TutorialStep == 27);
	if (bTutorialPlacedNext)
	{
		Popup->NextDelay = 0.f;
	}
	// The last step: Proceed to Lessons (its Next), Learn More left of it and Restart Tutorial left of that
	if (TutorialStep == 31)
	{
		Popup->bShowNext = true;
		Popup->NextLabel = NSLOCTEXT("Tutorial", "ProceedToLessons", "Proceed to Lessons");
		Popup->SecondLabel = NSLOCTEXT("Tutorial", "LearnMore", "Learn More");
		Popup->OnSecond.AddUniqueDynamic(this, &UGamePanelWidget::HandleTutorialLearnMore);
		Popup->ThirdLabel = NSLOCTEXT("Tutorial", "RestartTutorial", "Restart Tutorial");
		Popup->OnThird.AddUniqueDynamic(this, &UGamePanelWidget::HandleTutorialRestart);
	}
	static const TCHAR* const MessageIcons[][2] = {
		{ TEXT("Triangle"), TutorialMessageIcons::Triangle },
		{ TEXT("Star"), TutorialMessageIcons::Star },
		{ TEXT("Dragon"), TutorialMessageIcons::Dragon },
		{ TEXT("Phoenix"), TutorialMessageIcons::Phoenix },
		{ TEXT("Unicorn"), TutorialMessageIcons::Unicorn },
		{ TEXT("Butterfly"), TutorialMessageIcons::Butterfly },
		{ TEXT("Bee"), TutorialMessageIcons::Bee },
		{ TEXT("Heart"), TutorialMessageIcons::Heart },
		{ TEXT("Ladybug"), TutorialMessageIcons::Ladybug },
	};
	for (const auto& Icon : MessageIcons)
	{
		Popup->SetIcon(Icon[0], LoadObject<UTexture2D>(nullptr, Icon[1]));
	}
	Popup->SetMessage(Message);
	UCanvasPanelSlot* PopupSlot = ScreenCanvas->AddChildToCanvas(Popup);
	PopupSlot->SetAutoSize(true);
	// Just above the focus (which goes above the cell window for its step)
	const UCanvasPanelSlot* FocusSlot = TutorialFocus.IsValid() ? Cast<UCanvasPanelSlot>(TutorialFocus->Slot) : nullptr;
	PopupSlot->SetZOrder((FocusSlot ? FocusSlot->GetZOrder() : TutorialFocusZOrder) + 1);
	Popup->OnNext.AddUniqueDynamic(this, &UGamePanelWidget::HandleTutorialNext);
	TutorialPopup = Popup;
}

TArray<UWidget*> UGamePanelWidget::GetGridCells(int32 Column, int32 FirstRow, int32 LastRow) const
{
	TArray<UWidget*> Cells;
	if (const UGridPanel* Grid = TutorialGrid.Get())
	{
		for (UWidget* Cell : Grid->GetAllChildren())
		{
			const UGridSlot* CellSlot = Cell ? Cast<UGridSlot>(Cell->Slot) : nullptr;
			if (CellSlot && CellSlot->GetColumn() == Column && CellSlot->GetRow() >= FirstRow && CellSlot->GetRow() <= LastRow)
			{
				Cells.Add(Cell);
			}
		}
	}
	return Cells;
}

bool UGamePanelWidget::IsTutorialTouch() const
{
	// PC_Happiness.InputMode, a Blueprint enum: NewEnumerator0 touch, NewEnumerator1 mouse (as UGameRulesWidget reads it)
	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	const FProperty* Property = PC ? PC->GetClass()->FindPropertyByName(TEXT("InputMode")) : nullptr;
	if (Property)
	{
		const void* Value = Property->ContainerPtrToValuePtr<void>(PC);
		if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
		{
			return ByteProperty->GetPropertyValue(Value) == 0;
		}
		if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			return EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(Value) == 0;
		}
	}
	return PLATFORM_ANDROID || PLATFORM_IOS;
}

UUserWidget* UGamePanelWidget::GetCellDialog() const
{
	const UUserWidget* Screen = GetParent() ? GetParent()->GetTypedOuter<UUserWidget>() : nullptr;
	return Screen ? Cast<UUserWidget>(Screen->GetWidgetFromName(TEXT("CellDialog"))) : nullptr;
}

int32 UGamePanelWidget::GetTutorialCellsStep() const
{
	return TutorialStep >= 6 && TutorialStep <= 9 ? 6 : TutorialStep >= 11 && TutorialStep <= 14 ? 11
		: TutorialStep >= 17 && TutorialStep <= 20 ? 17 : TutorialStep >= 23 && TutorialStep <= 26 ? 23
		: TutorialStep >= 27 && TutorialStep <= 30 ? 27 : -1;
}

TArray<UWidget*> UGamePanelWidget::GetTutorialStepCells() const
{
	// The star's: column 3, rows 1 and 2 (over the given unicorn). The triangle's: row 1, columns 2 and 3 (next to the
	// star). The phoenix's: rows 1 and 2, columns 1 and 2. The bee's: column 2, rows 2 and 3 (the hint's, under the
	// dragon). The ladybug's elimination: column 1, rows 2 and 3 (under the phoenix)
	switch (GetTutorialCellsStep())
	{
	case 6:
		return GetGridCells(2, 0, 1);
	case 11:
	{
		TArray<UWidget*> Cells = GetGridCells(1, 0, 0);
		Cells.Append(GetGridCells(2, 0, 0));
		return Cells;
	}
	case 17:
	{
		TArray<UWidget*> Cells = GetGridCells(0, 0, 1);
		Cells.Append(GetGridCells(1, 0, 1));
		return Cells;
	}
	case 23:
		return GetGridCells(1, 1, 2);
	case 27:
		return GetGridCells(0, 1, 2);
	default:
		return {};
	}
}

void UGamePanelWidget::GetTutorialStepCell(int32& Row, int32& Column) const
{
	// The tutorial puzzle's solve (seed 129): the star in row 1, column 3; the triangle in row 1, column 2; the phoenix
	// in row 2, column 1; the bee in row 3, column 2; the ladybug eliminated from row 3, column 1 (the butterfly's)
	switch (GetTutorialCellsStep())
	{
	case 6:		Row = 0; Column = 2; break;
	case 11:		Row = 0; Column = 1; break;
	case 17:	Row = 1; Column = 0; break;
	case 23:	Row = 2; Column = 1; break;
	case 27:	Row = 2; Column = 0; break;
	default:	Row = 0; Column = 0; break;
	}
}

const TCHAR* UGamePanelWidget::GetTutorialStepIcon() const
{
	switch (GetTutorialCellsStep())
	{
	case 6:		return TutorialMessageIcons::Star;
	case 11:		return TutorialMessageIcons::Triangle;
	case 17:	return TutorialMessageIcons::Phoenix;
	case 23:	return TutorialMessageIcons::Bee;
	case 27:	return TutorialMessageIcons::Ladybug;
	default:	return nullptr;
	}
}

bool UGamePanelWidget::IsTutorialEliminateStep() const
{
	return GetTutorialCellsStep() == 27;
}

void UGamePanelWidget::StartTutorialConfirmStep(UWidget* Icon)
{
	++TutorialStep;
	TutorialFocusTime = 0.f;
	if (TutorialPopup.IsValid())
	{
		TutorialPopup->RemoveFromParent();
	}
	TutorialPopup.Reset();
	UUserWidget* Dialog = GetCellDialog();
	UWidget* Confirm = Dialog
		? Dialog->GetWidgetFromName(IsTutorialEliminateStep() ? TEXT("SizeBox_Eliminate") : TEXT("SizeBox_Confirm")) : nullptr;
	if (!TutorialFocus.IsValid() || !Confirm)
	{
		return;
	}
	// From round the icon down to the Confirm (or Eliminate) button (tight: as wide as the button), which alone takes
	// input
	TutorialFocus->ClearFocus();
	TutorialFocus->AddFocusMove({ Icon }, { Confirm }, 8.f, TutorialFocusSeconds, 8.f, true, true);
	TutorialFocus->SetInputWindow({ Confirm });
}

bool UGamePanelWidget::IsTutorialCellPlaced(int32 Row, int32 Column) const
{
	// The puzzle screen's puzzle (WBP_Happiness.ThePuzzle)
	const UUserWidget* Screen = GetParent() ? GetParent()->GetTypedOuter<UUserWidget>() : nullptr;
	const FObjectPropertyBase* Property = Screen ? FindFProperty<FObjectPropertyBase>(Screen->GetClass(), TEXT("ThePuzzle")) : nullptr;
	UPuzzle* Puzzle = Property ? Cast<UPuzzle>(Property->GetObjectPropertyValue_InContainer(Screen)) : nullptr;
	return Puzzle && Row < Puzzle->m_iSize && Column < Puzzle->m_iSize && Puzzle->GetCell(Row, Column).m_iFinalIcon >= 0;
}

void UGamePanelWidget::StartTutorialHorizontalStep()
{
	TutorialStep = 10;
	TutorialFocusTime = 0.f;
	if (TutorialPopup.IsValid())
	{
		TutorialPopup->RemoveFromParent();
	}
	TutorialPopup.Reset();
	if (!TutorialFocus.IsValid())
	{
		return;
	}
	// Back under the screen's dialogs (touch raised it over the cell window), and only the popup's Next takes input
	if (UCanvasPanelSlot* FocusSlot = Cast<UCanvasPanelSlot>(TutorialFocus->Slot))
	{
		FocusSlot->SetZOrder(TutorialFocusZOrder);
	}
	TutorialFocus->SetInputWindow({});
	TutorialFocus->SetBlocksInput(true);
	// From round the star's cell to the horizontal clue (as the horizontal clues step)
	TutorialFocus->ClearFocus();
	TutorialFocus->AddFocusMove(GetGridCells(2, 0, 0), GetShownClues(TEXT("HorizontalCluePanel")), 8.f, TutorialFocusSeconds, 0.f,
		false, true);
}

void UGamePanelWidget::StartTutorialOnlyOptionStep()
{
	const TArray<UWidget*> TriangleCells = GetTutorialStepCells();
	TutorialStep = 15;
	TutorialFocusTime = 0.f;
	if (TutorialPopup.IsValid())
	{
		TutorialPopup->RemoveFromParent();
	}
	TutorialPopup.Reset();
	if (!TutorialFocus.IsValid())
	{
		return;
	}
	// Back under the screen's dialogs (touch raised it over the cell window), and only the popup's Next takes input
	if (UCanvasPanelSlot* FocusSlot = Cast<UCanvasPanelSlot>(TutorialFocus->Slot))
	{
		FocusSlot->SetZOrder(TutorialFocusZOrder);
	}
	TutorialFocus->SetInputWindow({});
	TutorialFocus->SetBlocksInput(true);
	// From round the triangle's cells to round the heart's cell (row 1, column 1)
	TutorialFocus->ClearFocus();
	TutorialFocus->AddFocusMove(TriangleCells, GetGridCells(0, 0, 0), 0.f, TutorialFocusSeconds, 0.f, false, true);
}

UWidget* UGamePanelWidget::GetHintButton() const
{
	const UUserWidget* Screen = GetParent() ? GetParent()->GetTypedOuter<UUserWidget>() : nullptr;
	UWidget* Hint = nullptr;
	if (Screen && Screen->WidgetTree)
	{
		Screen->WidgetTree->ForEachWidget([&Hint](UWidget* Widget)
		{
			const UButtonPanelWidget* Panel = Cast<UButtonPanelWidget>(Widget);
			if (!Hint && Panel && Panel->Button_H)
			{
				Hint = Panel->Button_H;
			}
		});
	}
	return Hint;
}

void UGamePanelWidget::StartTutorialHintStep()
{
	// The phoenix's cells (rows 1-2, columns 1-2), where its circle is
	TArray<UWidget*> PhoenixCells = GetGridCells(0, 0, 1);
	PhoenixCells.Append(GetGridCells(1, 0, 1));
	TutorialStep = 21;
	TutorialFocusTime = 0.f;
	if (TutorialPopup.IsValid())
	{
		TutorialPopup->RemoveFromParent();
	}
	TutorialPopup.Reset();
	if (!TutorialFocus.IsValid())
	{
		return;
	}
	// Back under the screen's dialogs (touch raised it over the cell window)
	if (UCanvasPanelSlot* FocusSlot = Cast<UCanvasPanelSlot>(TutorialFocus->Slot))
	{
		FocusSlot->SetZOrder(TutorialFocusZOrder);
	}
	// The horizontal clue's circle goes; one moves from round the phoenix's cells to the Hint button, which alone takes input
	// (and the confirm window it opens, the screen's ConfirmDialog)
	UWidget* Hint = GetHintButton();
	TutorialFocus->ClearFocus();
	TutorialFocus->SetBlocksInput(true);
	TArray<UWidget*> Window;
	if (Hint)
	{
		TutorialFocus->AddFocusMove(PhoenixCells, { Hint }, 8.f, TutorialFocusSeconds);
		Window.Add(Hint);
	}
	const UUserWidget* Screen = GetParent() ? GetParent()->GetTypedOuter<UUserWidget>() : nullptr;
	if (UWidget* ConfirmDialog = Screen ? Screen->GetWidgetFromName(TEXT("ConfirmDialog")) : nullptr)
	{
		Window.Add(ConfirmDialog);
	}
	TutorialFocus->SetInputWindow(Window);
}

void UGamePanelWidget::StartTutorialHintShownStep()
{
	TutorialStep = 22;
	TutorialFocusTime = 0.f;
	if (TutorialPopup.IsValid())
	{
		TutorialPopup->RemoveFromParent();
	}
	TutorialPopup.Reset();
	if (!TutorialFocus.IsValid())
	{
		return;
	}
	// Only the popup's Next takes input
	TutorialFocus->SetInputWindow({});
	TutorialFocus->SetBlocksInput(true);
	// Two circles from the Hint button: one to tight round the second vertical clue's icons (the hint's: the dragon over
	// the bee), one to tight round column 2's rows 2 and 3 (centered between them)
	UWidget* Hint = GetHintButton();
	const TArray<UWidget*> HintFrom = Hint ? TArray<UWidget*>{ Hint } : TArray<UWidget*>();
	TutorialFocus->ClearFocus();
	const TArray<UWidget*> VerticalClues = GetShownClues(TEXT("VerticalCluePanel"), true);
	if (VerticalClues.IsValidIndex(1))
	{
		TutorialFocus->AddFocusMove(HintFrom, { VerticalClues[1] }, 0.f, TutorialFocusSeconds, 8.f, true);
	}
	TutorialFocus->AddFocusMove(HintFrom, GetGridCells(1, 1, 2), 0.f, TutorialFocusSeconds, 8.f, true);
}

void UGamePanelWidget::StartTutorialEliminateStep()
{
	TutorialStep = 27;
	TutorialFocusTime = 0.f;
	if (TutorialPopup.IsValid())
	{
		TutorialPopup->RemoveFromParent();
	}
	TutorialPopup.Reset();
	if (!TutorialFocus.IsValid())
	{
		return;
	}
	// Back under the screen's dialogs (touch raised it over the cell window)
	if (UCanvasPanelSlot* FocusSlot = Cast<UCanvasPanelSlot>(TutorialFocus->Slot))
	{
		FocusSlot->SetZOrder(TutorialFocusZOrder);
	}
	// From the hint's circles: the second vertical clue's to tight round the third's icons (Not Same Column: the phoenix
	// and the ladybug), column 2's rows 2-3 to tight round column 1's (the phoenix and the cell under it)
	TutorialFocus->ClearFocus();
	const TArray<UWidget*> VerticalClues = GetShownClues(TEXT("VerticalCluePanel"), true);
	if (VerticalClues.IsValidIndex(2))
	{
		TutorialFocus->AddFocusMove(VerticalClues.IsValidIndex(1) ? TArray<UWidget*>{ VerticalClues[1] } : TArray<UWidget*>(),
			{ VerticalClues[2] }, 0.f, TutorialFocusSeconds, 0.f, true, true);
	}
	TutorialFocus->AddFocusMove(GetGridCells(1, 1, 2), GetGridCells(0, 1, 2), 0.f, TutorialFocusSeconds, 0.f, true, true);
	// Only the ladybug's cell (row 3, column 1) takes input (and the cell window, touch)
	TArray<UWidget*> Window = GetGridCells(0, 2, 2);
	if (UWidget* CellDialog = IsTutorialTouch() ? GetCellDialog() : nullptr)
	{
		Window.Add(CellDialog);
	}
	TutorialFocus->SetBlocksInput(true);
	TutorialFocus->SetInputWindow(Window);
}

void UGamePanelWidget::StartTutorialSolvedStep()
{
	// The tutorial is completed: the mode screen doesn't offer it at start any more (saved)
	if (UCampaignSubsystem* Campaign = UCampaignSubsystem::Get(this))
	{
		Campaign->SetTutorialCompleted();
	}
	TutorialStep = 31;
	TutorialFocusTime = 0.f;
	if (TutorialPopup.IsValid())
	{
		TutorialPopup->RemoveFromParent();
	}
	TutorialPopup.Reset();
	if (!TutorialFocus.IsValid())
	{
		return;
	}
	// Back under the screen's dialogs (touch raised it over the cell window); only the popup's buttons take input
	if (UCanvasPanelSlot* FocusSlot = Cast<UCanvasPanelSlot>(TutorialFocus->Slot))
	{
		FocusSlot->SetZOrder(TutorialFocusZOrder);
	}
	TutorialFocus->SetInputWindow({});
	TutorialFocus->SetBlocksInput(true);
	// The clue's circle goes; column 1's grows to round the whole grid (as the grid step's)
	TutorialFocus->ClearFocus();
	if (UGridPanel* Grid = TutorialGrid.Get())
	{
		TutorialFocus->AddFocusMove(GetGridCells(0, 1, 2), { Grid }, 0.f, TutorialFocusSeconds, 0.f, false, true);
	}
}

void UGamePanelWidget::HandleTutorialLearnMore()
{
	UUserWidget* Screen = GetParent() ? GetParent()->GetTypedOuter<UUserWidget>() : nullptr;
	UCanvasPanel* ScreenCanvas = Screen ? Cast<UCanvasPanel>(Screen->GetRootWidget()) : nullptr;
	if (!TutorialRulesWidget)
	{
		const TSoftClassPtr<UUserWidget> RulesClass(FSoftObjectPath(TEXT("/Game/Happiness/UI/WBP_GameRules.WBP_GameRules_C")));
		UClass* Loaded = RulesClass.LoadSynchronous();
		TutorialRulesWidget = Loaded ? CreateWidget<UUserWidget>(this, Loaded) : nullptr;
		if (!TutorialRulesWidget)
		{
			return;
		}
		// Over the whole puzzle screen, above the tutorial's focus and popup; closing it comes back to them
		if (UCanvasPanelSlot* RulesSlot = ScreenCanvas ? ScreenCanvas->AddChildToCanvas(TutorialRulesWidget) : nullptr)
		{
			RulesSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			RulesSlot->SetOffsets(FMargin(0.f));
			RulesSlot->SetZOrder(300);
		}
		else
		{
			TutorialRulesWidget->AddToViewport(300);
		}
	}
	if (UGameRulesWidget* Rules = Cast<UGameRulesWidget>(TutorialRulesWidget))
	{
		Rules->Show(GetOwningPlayer());
	}
	else
	{
		TutorialRulesWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void UGamePanelWidget::ProceedFromTutorial()
{
	// Then on from the mode screen to the Lessons tree, Game Basics' popup open (the menu holds both screens)
	if (UCanvasPanel* Menu = CloseTutorialScreen())
	{
		for (UWidget* Child : Menu->GetAllChildren())
		{
			if (UHappinessModeWidget* Mode = Cast<UHappinessModeWidget>(Child))
			{
				Mode->ShowLessonsAt(ECampaignLesson::VerticalTwo);	// Game Basics
				break;
			}
		}
	}
}

void UGamePanelWidget::HandleTutorialRestart()
{
	// Then from the mode screen the tutorial again, as the mode screen's offer at start does: the mode screen hides
	// and the Lessons tree plays it (WBP_GameSelect's OnLessonStageChosen)
	UCanvasPanel* Menu = CloseTutorialScreen();
	if (!Menu)
	{
		return;
	}
	UCampaignTreeWidget* Tree = nullptr;
	for (UWidget* Child : Menu->GetAllChildren())
	{
		if (UHappinessModeWidget* Mode = Cast<UHappinessModeWidget>(Child))
		{
			Mode->Hide();
		}
		Tree = Tree ? Tree : Cast<UCampaignTreeWidget>(Child);
	}
	if (Tree)
	{
		Tree->PlayTutorial();
	}
}

UCanvasPanel* UGamePanelWidget::CloseTutorialScreen()
{
	UUserWidget* Screen = GetParent() ? GetParent()->GetTypedOuter<UUserWidget>() : nullptr;
	if (!Screen)
	{
		return nullptr;
	}
	if (TutorialRulesWidget)
	{
		TutorialRulesWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	// The puzzle screen closes as from its pause menu: WBP_Happiness's Close dispatcher, which WBP_GameSelect answers
	// with CloseHappiness and (not a lesson) the mode screen
	if (const FMulticastDelegateProperty* Close = FindFProperty<FMulticastDelegateProperty>(Screen->GetClass(), TEXT("Close")))
	{
		if (const FMulticastScriptDelegate* Delegate = Close->GetMulticastDelegate(Close->ContainerPtrToValuePtr<void>(Screen)))
		{
			Delegate->ProcessDelegate<UObject>(nullptr);
		}
	}
	return Cast<UCanvasPanel>(Screen->GetParent());
}

void UGamePanelWidget::StartTutorialCommitStep()
{
	++TutorialStep;
	TutorialFocusTime = 0.f;
	if (TutorialPopup.IsValid())
	{
		TutorialPopup->RemoveFromParent();
	}
	TutorialPopup.Reset();
	UUserWidget* Dialog = GetCellDialog();
	// The previous step's button: Confirm, or the elimination's Eliminate
	UWidget* Confirm = Dialog
		? Dialog->GetWidgetFromName(IsTutorialEliminateStep() ? TEXT("SizeBox_Eliminate") : TEXT("SizeBox_Confirm")) : nullptr;
	UWidget* Commit = Dialog ? Dialog->GetWidgetFromName(TEXT("Button_Done")) : nullptr;
	if (!TutorialFocus.IsValid() || !Commit)
	{
		return;
	}
	// From round Confirm (or Eliminate) up to the Commit button (tight round it), which alone takes input
	TutorialFocus->ClearFocus();
	TutorialFocus->AddFocusMove({ Confirm }, { Commit }, 8.f, TutorialFocusSeconds, 8.f, true, true);
	TutorialFocus->SetInputWindow({ Commit });
}

UWidget* UGamePanelWidget::FindCellDialogIcon(const UTexture* Texture) const
{
	UWidget* Dialog = GetCellDialog();
	UWidget* Icon = Texture ? TutorialMessageIcons::FindShownIcon(Dialog, Texture) : nullptr;
	// Once it's laid out (the window's first frame open has no geometry yet)
	return Icon && !Icon->GetCachedGeometry().GetLocalSize().IsNearlyZero() ? Icon : nullptr;
}

void UGamePanelWidget::StartTutorialDialogStep(UWidget* Icon)
{
	const TArray<UWidget*> Cells = GetTutorialStepCells();
	++TutorialStep;
	TutorialFocusTime = 0.f;
	if (TutorialPopup.IsValid())
	{
		TutorialPopup->RemoveFromParent();
	}
	TutorialPopup.Reset();
	if (!TutorialFocus.IsValid())
	{
		return;
	}
	// Above the cell window (it's drawn over the screen's dim otherwise), so its icon is what's in focus
	const UUserWidget* Screen = GetParent() ? GetParent()->GetTypedOuter<UUserWidget>() : nullptr;
	const UWidget* Dialog = Screen ? Screen->GetWidgetFromName(TEXT("CellDialog")) : nullptr;
	const UCanvasPanelSlot* DialogSlot = Dialog ? Cast<UCanvasPanelSlot>(Dialog->Slot) : nullptr;
	if (UCanvasPanelSlot* FocusSlot = Cast<UCanvasPanelSlot>(TutorialFocus->Slot); FocusSlot && DialogSlot)
	{
		FocusSlot->SetZOrder(FMath::Max(TutorialFocusZOrder, DialogSlot->GetZOrder() + 1));
	}
	// One circle: from round the cells to round the window's icon, which alone takes input
	TutorialFocus->ClearFocus();
	TutorialFocus->AddFocusMove(Cells, { Icon }, 8.f, TutorialFocusSeconds, 0.f, true, true);
	TutorialFocus->SetInputWindow({ Icon });
}

TArray<UWidget*> UGamePanelWidget::GetShownClues(FName PanelName, bool bIconBoxes) const
{
	TArray<UWidget*> Clues;
	const UUserWidget* Screen = GetParent() ? GetParent()->GetTypedOuter<UUserWidget>() : nullptr;
	const UUserWidget* Panel = Screen ? Cast<UUserWidget>(Screen->GetWidgetFromName(PanelName)) : nullptr;
	if (Panel && Panel->WidgetTree)
	{
		// The clues the panel made for the puzzle (hidden ones are taken off it), or each one's icon box
		Panel->WidgetTree->ForEachWidget([&Clues, bIconBoxes](UWidget* Widget)
		{
			UClueWidget* Clue = Cast<UClueWidget>(Widget);
			if (Clue && Clue->IsVisible())
			{
				UWidget* Icons = bIconBoxes ? Clue->GetWidgetFromName(TEXT("HorizontalBox")) : nullptr;
				Icons = Icons || !bIconBoxes ? Icons : Clue->GetWidgetFromName(TEXT("VerticalBox"));
				Clues.Add(Icons ? Icons : Clue);
			}
		});
	}
	return Clues;
}

void UGamePanelWidget::PlaceTutorialPopup(UGridPanel* Grid, UCanvasPanel* ScreenCanvas)
{
	UCanvasPanelSlot* PopupSlot = TutorialPopup.IsValid() ? Cast<UCanvasPanelSlot>(TutorialPopup->Slot) : nullptr;
	if (!PopupSlot)
	{
		return;
	}
	// Each step's popup goes in an area of the screen (in the screen canvas' units), centered across it as wide as fits
	// (up to a step's limit), at a height it prefers:
	// - the grid: right of the grid (up to 620), level with the grid's middle, so the grid stays in view
	// - the vertical and horizontal clues: the whole screen (up to 1180), over the grid, a little above the middle,
	//   clear of the clue panels' corners
	// - a vertical clue up close: right of the vertical clues and below the grid's second row, in its middle
	const FGeometry& CanvasGeometry = ScreenCanvas->GetCachedGeometry();
	const FGeometry& GridGeometry = Grid->GetCachedGeometry();
	const FVector2D CanvasSize = CanvasGeometry.GetLocalSize();
	const FVector2D GridSize = GridGeometry.GetLocalSize();
	const FVector2D GridTopLeft = CanvasGeometry.AbsoluteToLocal(GridGeometry.LocalToAbsolute(FVector2D::ZeroVector));
	const FVector2D GridBottomRight = CanvasGeometry.AbsoluteToLocal(GridGeometry.LocalToAbsolute(GridSize));
	constexpr float Margin = 30.f;
	FBox2D Area(FVector2D::ZeroVector, CanvasSize);
	float MaxWidth = 1180.f;
	float PreferredY = CanvasSize.Y * .42f;
	if (TutorialStep <= 2)
	{
		Area.Min.X = GridBottomRight.X;
		MaxWidth = 620.f;
		PreferredY = (GridTopLeft.Y + GridBottomRight.Y) * .5f;
	}
	else if (TutorialStep == 31)
	{
		// The solved grid: across the screen, at its bottom
		PreferredY = CanvasSize.Y;
	}
	else if (TutorialStep == 5 || TutorialStep == 6 || TutorialStep == 10 || TutorialStep == 11 || TutorialStep == 15 || TutorialStep == 16
		|| TutorialStep == 17 || TutorialStep == 21 || TutorialStep == 27)
	{
		int32 Rows = 0;
		for (UWidget* Cell : Grid->GetAllChildren())
		{
			if (const UGridSlot* CellSlot = Cast<UGridSlot>(Cell->Slot))
			{
				Rows = FMath::Max(Rows, CellSlot->GetRow() + 1);
			}
		}
		// Right of the vertical clues themselves (their panel is wider than they are); the horizontal clue's popup
		// (step 10) as wide as the screen allows
		// (and the ladybug's, step 27: right of the vertical clues too, clear of column 1 and the clue)
		const bool bRightOfClues = TutorialStep < 10 || TutorialStep == 27;
		for (const UWidget* Clue : bRightOfClues ? GetShownClues(TEXT("VerticalCluePanel"), true) : TArray<UWidget*>())
		{
			const FGeometry& ClueGeometry = Clue->GetCachedGeometry();
			Area.Min.X = FMath::Max(Area.Min.X,
				CanvasGeometry.AbsoluteToLocal(ClueGeometry.LocalToAbsolute(FVector2D(ClueGeometry.GetLocalSize().X, 0.f))).X);
		}
		Area.Min.Y = GridTopLeft.Y + (GridBottomRight.Y - GridTopLeft.Y) * FMath::Min(2, Rows) / FMath::Max(Rows, 1);
		// Below the grid's second row: in the middle of the space there, or (step 10) just under the row
		PreferredY = bRightOfClues ? Area.GetCenter().Y : Area.Min.Y;
		// The bee's: below the whole grid, just under it (clear of column 1's circle)
		if (TutorialStep == 27)
		{
			Area.Min.Y = GridBottomRight.Y;
			PreferredY = Area.Min.Y;
		}
	}
	else if (TutorialStep == 22 || TutorialStep == 23)
	{
		// The hint (and the butterfly's step): at the top of the screen, above the grid's second row
		int32 Rows = 0;
		for (UWidget* Cell : Grid->GetAllChildren())
		{
			if (const UGridSlot* CellSlot = Cast<UGridSlot>(Cell->Slot))
			{
				Rows = FMath::Max(Rows, CellSlot->GetRow() + 1);
			}
		}
		Area.Max.Y = GridTopLeft.Y + (GridBottomRight.Y - GridTopLeft.Y) * FMath::Min(1, Rows) / FMath::Max(Rows, 1);
		PreferredY = Area.Min.Y;
	}
	else if ((TutorialStep >= 7 && TutorialStep <= 9) || (TutorialStep >= 12 && TutorialStep <= 14)
		|| (TutorialStep >= 18 && TutorialStep <= 20) || (TutorialStep >= 24 && TutorialStep <= 26)
		|| (TutorialStep >= 28 && TutorialStep <= 30))
	{
		// The cell window fills most of the screen: over its bottom row of buttons, right of Confirm (Reset All and
		// Eliminate, which take no input in this step), clear of its icons and of Confirm
		const UUserWidget* Screen = GetParent() ? GetParent()->GetTypedOuter<UUserWidget>() : nullptr;
		const UUserWidget* Dialog = Screen ? Cast<UUserWidget>(Screen->GetWidgetFromName(TEXT("CellDialog"))) : nullptr;
		if (const UWidget* Confirm = Dialog ? Dialog->GetWidgetFromName(TEXT("SizeBox_Confirm")) : nullptr)
		{
			const FGeometry& ConfirmGeometry = Confirm->GetCachedGeometry();
			const FVector2D ConfirmTopLeft = CanvasGeometry.AbsoluteToLocal(ConfirmGeometry.LocalToAbsolute(FVector2D::ZeroVector));
			const FVector2D ConfirmBottomRight =
				CanvasGeometry.AbsoluteToLocal(ConfirmGeometry.LocalToAbsolute(ConfirmGeometry.GetLocalSize()));
			Area = FBox2D(FVector2D(ConfirmBottomRight.X, ConfirmTopLeft.Y), FVector2D(CanvasSize.X, ConfirmBottomRight.Y));
			if (const UWidget* Eliminate = Dialog->GetWidgetFromName(TEXT("SizeBox_Eliminate")))
			{
				const FGeometry& EliminateGeometry = Eliminate->GetCachedGeometry();
				Area.Max.X = CanvasGeometry.AbsoluteToLocal(EliminateGeometry.LocalToAbsolute(EliminateGeometry.GetLocalSize())).X;
				// The elimination's Eliminate step: over Confirm and Reset All instead, left of Eliminate (which takes input)
				if (TutorialStep == 29)
				{
					Area.Min.X = ConfirmTopLeft.X;
					Area.Max.X = CanvasGeometry.AbsoluteToLocal(EliminateGeometry.LocalToAbsolute(FVector2D::ZeroVector)).X;
				}
			}
		}
		PreferredY = Area.GetCenter().Y;
	}
	const FVector2D AreaSize = Area.GetSize();
	TutorialPopup->SetWidth(FMath::Clamp((float)AreaSize.X - Margin * 2.f, 200.f, MaxWidth));
	// Kept inside the area top to bottom: a tall popup moves off its preferred height rather than past the area's edge
	const float HalfHeight = TutorialPopup->GetDesiredSize().Y * .5f;
	const float MiddleY = HalfHeight * 2.f + Margin * 2.f >= AreaSize.Y ? (float)Area.GetCenter().Y
		: FMath::Clamp(PreferredY, (float)Area.Min.Y + HalfHeight + Margin, (float)Area.Max.Y - HalfHeight - Margin);
	const FVector2D Position(Area.GetCenter().X, MiddleY);
	PopupSlot->SetAnchors(FAnchors(0.f, 0.f));
	PopupSlot->SetAlignment(FVector2D(.5f, .5f));
	if (!PopupSlot->GetPosition().Equals(Position, .5f))
	{
		PopupSlot->SetPosition(Position);
	}
}

void UGamePanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (bBursting)
	{
		UpdateFireworks(InDeltaTime);
	}
	else if (FiredCells.Num() > 0 && !IsFireworksPuzzleComplete())
	{
		// Restarted or another puzzle: the board shows its icons again
		EndFireworks();
	}
	if (!Border_44)
	{
		return;
	}

	UGridPanel* BoardGrid = nullptr;
	for (UWidget* Child : Border_44->GetAllChildren())
	{
		UGridPanel* Grid = Cast<UGridPanel>(Child);
		BoardGrid = !BoardGrid && Grid && Grid->GetChildrenCount() > 0 ? Grid : BoardGrid;
	}
	UpdateTutorialFocus(BoardGrid);

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
		// Icons stay square. Where the width limits them (wider screens, like iPads), the grid gets shorter instead of
		// leaving spare height in every cell, so the icons still fill their cells top to bottom.
		const float GridHeight = FMath::Min(static_cast<float>(Available.Y), (IconWidth * 2.f + FrameInset * 2.f) * Rows);
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
									// Width-limited icons meet at the cell center; spare height stays outside both rows.
									UWidget* Content = IconScale->GetContent();
									if (UScaleBoxSlot* ContentSlot = Content ? Cast<UScaleBoxSlot>(Content->Slot) : nullptr)
									{
										const EVerticalAlignment Align = RowName == TEXT("TopRowIcons") ? VAlign_Bottom : VAlign_Top;
										if (ContentSlot->GetVerticalAlignment() != Align) ContentSlot->SetVerticalAlignment(Align);
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

		// Centred in the area: a grid shorter than the area gets equal space above and below
		const FAnchors Anchors(0.5f, 0.5f, 0.5f, 0.5f);
		const FVector2D Alignment(0.5f, 0.5f);
		const FMargin Offsets(0.f, 0.f, GridWidth, GridHeight);
		if (CanvasSlot->GetAnchors() != Anchors || CanvasSlot->GetAlignment() != Alignment || CanvasSlot->GetOffsets() != Offsets)
		{
			CanvasSlot->SetAnchors(Anchors);
			CanvasSlot->SetAlignment(Alignment);
			CanvasSlot->SetOffsets(Offsets);
		}
		// The outer column padding is only for spacing between cells, not a second border gutter.
		const FMargin SurfaceOffsets(0.f, 0.f, FMath::Max(0.f, GridWidth - ColumnGap), GridHeight);
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
