#include "UI/GameRulesWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "HappinessClassic/Clue.h"
#include "HappinessClassic/ClueExample.h"
#include "HappinessClassic/Puzzle.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/SlateBrush.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "GameRules"

namespace
{
	// How many seeds to try for a puzzle with the clue type in it
	const int32 ExampleSeedTries = 40;

	/** Icon Index of an icon set (DA_IconSet): its Icons array holds the textures, directly or in a struct. As UHintInfoWidget's. */
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

UGameRulesWidget::UGameRulesWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	VerticalClueClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/Happiness/UI/WBP_VerticalClue.WBP_VerticalClue_C")));
	HorizontalClueClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/Happiness/UI/WBP_HorizontalClue.WBP_HorizontalClue_C")));
	ButtonPanelClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/Happiness/UI/WBP_ButtonPanel.WBP_ButtonPanel_C")));

	// The game's font; entries are plain text that lights up when hovered or pressed
	static ConstructorHelpers::FObjectFinder<UFont> Comic(TEXT("/Game/Happiness/UI/Fonts/COMIC_Font.COMIC_Font"));
	EntryFont = FSlateFontInfo(Comic.Object, 15);
	SectionFont = FSlateFontInfo(Comic.Object, 18);
	auto Fill = [](float Alpha)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FLinearColor(1.f, 1.f, 1.f, Alpha));
		Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(6.f, 6.f, 6.f, 6.f));
		return Brush;
	};
	EntryStyle.SetNormal(Fill(0.f)).SetHovered(Fill(0.12f)).SetPressed(Fill(0.22f));
	EntryStyle.SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f));
}

TArray<FRulesSubject> UGameRulesWidget::MakeSubjects()
{
	TArray<FRulesSubject> Result;
	auto Basic = [&Result](const FText& Section, const FText& Title, const FText& Description, const FText& PictureNote)
	{
		FRulesSubject& Subject = Result.AddDefaulted_GetRef();
		Subject.Section = Section;
		Subject.Title = Title;
		Subject.Description = Description;
		Subject.PictureNote = PictureNote;
	};
	auto Clue = [&Result](const FText& Section, ECampaignLesson Lesson, const FText& Description, const FText& Title = FText())
	{
		FRulesSubject& Subject = Result.AddDefaulted_GetRef();
		Subject.Section = Section;
		Subject.Title = Title.IsEmpty() ? UCampaignTree::GetLessonDisplayName(Lesson) : Title;
		Subject.Description = Description;
		Subject.bClue = true;
		Subject.Lesson = Lesson;
	};

	const FText Basics = LOCTEXT("Basics", "Basics");
	Basic(Basics, LOCTEXT("Objective", "Objective"),
		LOCTEXT("ObjectiveDesc", "Work out which icon goes in each column. Every row holds each of its icons exactly once."),
		LOCTEXT("ObjectivePic", "A solved puzzle grid, with one row and one column highlighted"));
	Basic(FText(), LOCTEXT("Layout", "Game Layout"),
		LOCTEXT("LayoutDesc",
			"Puzzle Area - the grid. Tap a cell to work on it.\n"
			"Vertical Clues - clues about icons in the same column.\n"
			"Horizontal Clues - clues about icons in different columns.\n"
			"Clue Info - what the selected clue means."),
		FText());
	Result.Last().bLayoutDiagram = true;
	Basic(FText(), LOCTEXT("Grid", "Puzzle Grid"),
		LOCTEXT("GridDesc", "Tap a cell to rule icons out, bring them back, or place its answer."),
		LOCTEXT("GridPic", "A cell with all its icons, a cell with one ruled out, a cell with its answer placed, and the cell window with its buttons"));
	Basic(FText(), LOCTEXT("Clues", "Clues"),
		LOCTEXT("CluesDesc", "Tap a clue to see what it means. Hide clues you're done with (HC); Unhide Clues in the pause menu brings them back."),
		LOCTEXT("CluesPic", "A selected clue, with its explanation above the grid"));
	Basic(FText(), LOCTEXT("Hints", "Hints"),
		LOCTEXT("HintsDesc", "Stuck? H points to a clue that helps, and explains why. Each hint adds 10 seconds."),
		LOCTEXT("HintsPic", "The hint panel pointing at a clue and the cell it helps with"));

	const FText Vertical = LOCTEXT("Vertical", "Vertical Clues");
	Clue(Vertical, ECampaignLesson::VerticalTwo, LOCTEXT("SameColumnDesc", "These icons are in the same column."), LOCTEXT("SameColumn", "Same Column"));
	Clue(FText(), ECampaignLesson::VerticalThree, LOCTEXT("VerticalThreeDesc", "All three icons are in the same column."));
	Clue(FText(), ECampaignLesson::TwoNot, LOCTEXT("TwoNotDesc", "The crossed-out icon is never in the same column as the other."));
	Clue(FText(), ECampaignLesson::ThreeNot, LOCTEXT("ThreeNotDesc", "Two icons share a column; the crossed-out one isn't in it."));
	Clue(FText(), ECampaignLesson::EitherOr, LOCTEXT("EitherOrDesc", "The top icon shares a column with exactly one of the two below it."));

	const FText Horizontal = LOCTEXT("Horizontal", "Horizontal Clues");
	Clue(Horizontal, ECampaignLesson::NextTo, LOCTEXT("NextToDesc", "The icons are in neighboring columns, either way round."));
	Clue(FText(), ECampaignLesson::NotNextTo, LOCTEXT("NotNextToDesc", "The crossed-out icon is never in a column next to the other."));
	Clue(FText(), ECampaignLesson::DirectlyLeftOf, LOCTEXT("DirectlyLeftOfDesc", "The first icon is in the column just left of the second."));
	Clue(FText(), ECampaignLesson::LeftOf, LOCTEXT("LeftOfDesc", "The first icon is somewhere left of the second."));
	Clue(FText(), ECampaignLesson::NotLeftOf, LOCTEXT("NotLeftOfDesc", "The first icon is never left of the second."));
	Clue(FText(), ECampaignLesson::Edge, LOCTEXT("EdgeDesc", "The icon is in the first or last column. Crossed out: it never is."));
	Clue(FText(), ECampaignLesson::Span, LOCTEXT("SpanDesc", "The middle icon has the other two on either side of it."));
	Clue(FText(), ECampaignLesson::SpanNotSide, LOCTEXT("SpanNotSideDesc", "One icon is beside the middle one; the crossed-out icon isn't on its other side."));
	Clue(FText(), ECampaignLesson::Gap, LOCTEXT("GapDesc", "Exactly one column between the two icons."));
	Clue(FText(), ECampaignLesson::SpanNotMid, LOCTEXT("SpanNotMidDesc", "One column between the outer icons, and the crossed-out icon isn't in it."));
	Clue(FText(), ECampaignLesson::Between, LOCTEXT("BetweenDesc", "The middle icon is somewhere between the other two."));
	Clue(FText(), ECampaignLesson::Chain, LOCTEXT("ChainDesc", "Left to right in this order, with any gaps."));
	Clue(FText(), ECampaignLesson::NextToEitherOr, LOCTEXT("NextToEitherOrDesc", "The middle icon is next to exactly one of the other two."));
	Clue(FText(), ECampaignLesson::AllApart, LOCTEXT("AllApartDesc", "All three icons are in different columns."));
	return Result;
}

void UGameRulesWidget::NativeConstruct()
{
	Super::NativeConstruct();
	CloseButton->OnClicked.AddUniqueDynamic(this, &UGameRulesWidget::HandleClose);
	if (Subjects.Num() == 0)
	{
		Subjects = MakeSubjects();
		BuildIndex();
	}
	if (Selected == INDEX_NONE)
	{
		Select(PC ? 0 : FMath::Clamp(PreviewSubject, 0, Subjects.Num() - 1));
	}
}

void UGameRulesWidget::Show(APlayerController* PlayerController)
{
	PC = PlayerController;
	SetVisibility(ESlateVisibility::Visible);
	if (Subjects.Num() > 0)
	{
		// The examples need the player controller's icon sets
		Select(Selected == INDEX_NONE ? 0 : Selected);
	}
}

void UGameRulesWidget::HandleClose()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UGameRulesWidget::BuildIndex()
{
	SubjectList->ClearChildren();
	EntryButtons.Reset();
	EntryTexts.Reset();
	EntryHandlers.Reset();

	for (int32 Index = 0; Index < Subjects.Num(); Index++)
	{
		const FRulesSubject& Subject = Subjects[Index];
		if (!Subject.Section.IsEmpty())
		{
			UTextBlock* Header = NewObject<UTextBlock>(this);
			Header->SetText(Subject.Section);
			Header->SetFont(SectionFont);
			Header->SetColorAndOpacity(FSlateColor(SectionColor));
			Header->SetShadowOffset(FVector2D(1.f, 1.f));
			if (UVerticalBoxSlot* HeaderSlot = Cast<UVerticalBoxSlot>(SubjectList->AddChild(Header)))
			{
				HeaderSlot->SetPadding(FMargin(4.f, Index == 0 ? 0.f : 14.f, 4.f, 4.f));
			}
		}

		UButton* Button = NewObject<UButton>(this);
		Button->SetStyle(EntryStyle);
		UTextBlock* Text = NewObject<UTextBlock>(this);
		Text->SetText(Subject.Title);
		Text->SetFont(EntryFont);
		Text->SetColorAndOpacity(FSlateColor(EntryColor));
		Text->SetShadowOffset(FVector2D(1.f, 1.f));
		if (UButtonSlot* TextSlot = Cast<UButtonSlot>(Button->AddChild(Text)))
		{
			TextSlot->SetHorizontalAlignment(HAlign_Left);
			TextSlot->SetPadding(FMargin(10.f, 4.f));
		}
		if (UVerticalBoxSlot* ButtonSlot = Cast<UVerticalBoxSlot>(SubjectList->AddChild(Button)))
		{
			ButtonSlot->SetPadding(FMargin(4.f, 2.f));
		}

		URulesEntryHandler* Handler = NewObject<URulesEntryHandler>(this);
		Handler->Owner = this;
		Handler->Index = Index;
		Button->OnClicked.AddDynamic(Handler, &URulesEntryHandler::HandleClicked);

		EntryButtons.Add(Button);
		EntryTexts.Add(Text);
		EntryHandlers.Add(Handler);
	}
}

void URulesEntryHandler::HandleClicked()
{
	if (Owner)
	{
		Owner->Select(Index);
	}
}

void UGameRulesWidget::Select(int32 Index)
{
	if (!Subjects.IsValidIndex(Index))
	{
		return;
	}
	Selected = Index;
	const FRulesSubject& Subject = Subjects[Index];
	TitleText->SetText(Subject.Title);
	DescriptionText->SetText(Subject.Description);

	for (int32 Entry = 0; Entry < EntryTexts.Num(); Entry++)
	{
		EntryTexts[Entry]->SetColorAndOpacity(FSlateColor(Entry == Index ? SelectedColor : EntryColor));
	}

	ExampleBox->ClearChildren();
	IllustrationBox->ClearChildren();
	UUserWidget* Example = nullptr;
	if (Subject.bClue)
	{
		Example = MakeExample(Subject.Lesson);
		UPuzzle* Puzzle = nullptr;
		UClue* Clue = nullptr;
		const TArray<UObject*> IconSets = GetIconSets();
		if (FindExampleClue(Subject.Lesson, Puzzle, Clue) && IconSets.Num() > 0)
		{
			AddBoards(*Puzzle, *Clue, IconSets);
		}
	}
	else if (Subject.bLayoutDiagram)
	{
		AddLayoutDiagram();
		AddButtonLegend();
	}
	else
	{
		AddPicture(Subject);
	}
	if (Example)
	{
		Example->SetVisibility(ESlateVisibility::HitTestInvisible);
		ExampleBox->AddChild(Example);
	}
	ExampleBox->GetParent()->SetVisibility(Example ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

TArray<UObject*> UGameRulesWidget::GetIconSets() const
{
	TArray<UObject*> Result;
	const FArrayProperty* Property = PC ? FindFProperty<FArrayProperty>(PC->GetClass(), TEXT("HappinessIconSets")) : nullptr;
	const FObjectPropertyBase* Element = Property ? CastField<FObjectPropertyBase>(Property->Inner) : nullptr;
	if (!Element)
	{
		return Result;
	}
	FScriptArrayHelper Array(Property, Property->ContainerPtrToValuePtr<void>(PC));
	for (int32 Set = 0; Set < Array.Num(); Set++)
	{
		Result.Add(Element->GetObjectPropertyValue(Array.GetRawPtr(Set)));
	}
	return Result;
}

void UGameRulesWidget::AddBoards(const UPuzzle& Puzzle, const UClue& Clue, const TArray<UObject*>& IconSets)
{
	FClueExample Example;
	if (!Example.Build(Clue, Puzzle))
	{
		return;
	}
	UHorizontalBox* Row = NewObject<UHorizontalBox>(this);
	auto AddBoard = [this, Row](UWidget* Board)
	{
		if (UHorizontalBoxSlot* BoardSlot = Row->AddChildToHorizontalBox(Board))
		{
			BoardSlot->SetPadding(FMargin(0.f, 0.f, 40.f, 0.f));
			BoardSlot->SetVerticalAlignment(VAlign_Top);
		}
	};
	AddBoard(MakeBoard(Example, Example.SolutionColumns(), INDEX_NONE, IconSets, LOCTEXT("Fits", "Fits the clue"), FitsColor));
	TArray<int32> Broken;
	int32 Moved = INDEX_NONE;
	if (Example.FindBroken(Broken, Moved))
	{
		AddBoard(MakeBoard(Example, Broken, Moved, IconSets, LOCTEXT("Breaks", "Breaks the clue"), BreaksColor));
	}
	IllustrationBox->AddChild(Row);
}

UWidget* UGameRulesWidget::MakeBoard(const FClueExample& Example, const TArray<int32>& Columns, int32 Highlight,
	const TArray<UObject*>& IconSets, const FText& Caption, const FLinearColor& Color)
{
	UVerticalBox* Board = NewObject<UVerticalBox>(this);

	UTextBlock* Label = NewObject<UTextBlock>(this);
	Label->SetText(Caption);
	Label->SetFont(EntryFont);
	Label->SetColorAndOpacity(FSlateColor(Color));
	Label->SetShadowOffset(FVector2D(1.f, 1.f));
	if (UVerticalBoxSlot* LabelSlot = Board->AddChildToVerticalBox(Label))
	{
		LabelSlot->SetPadding(FMargin(2.f, 0.f, 0.f, 6.f));
	}

	// The clue's rows (in the clue's order), every column
	TArray<int32> Rows;
	for (const FClueExamplePiece& Piece : Example.Pieces)
	{
		Rows.AddUnique(Piece.Row);
	}

	UBorder* Frame = NewObject<UBorder>(this);
	Frame->SetBrushColor(Color);
	Frame->SetPadding(FMargin(3.f));
	UUniformGridPanel* Grid = NewObject<UUniformGridPanel>(this);
	Grid->SetSlotPadding(FMargin(2.f));
	for (int32 RowIndex = 0; RowIndex < Rows.Num(); RowIndex++)
	{
		for (int32 Column = 0; Column < Example.Size; Column++)
		{
			int32 PieceHere = INDEX_NONE;
			for (int32 Piece = 0; Piece < Example.Pieces.Num(); Piece++)
			{
				if (Example.Pieces[Piece].Row == Rows[RowIndex] && Columns[Piece] == Column)
				{
					PieceHere = Piece;
				}
			}

			USizeBox* CellSize = NewObject<USizeBox>(this);
			CellSize->SetWidthOverride(BoardCellSize);
			CellSize->SetHeightOverride(BoardCellSize);
			UBorder* Cell = NewObject<UBorder>(this);
			Cell->SetBrushColor(PieceHere != INDEX_NONE && PieceHere == Highlight ? BreaksColor : FLinearColor(0.08f, 0.09f, 0.12f, 1.f));
			Cell->SetPadding(FMargin(4.f));
			if (PieceHere != INDEX_NONE)
			{
				const FClueExamplePiece& Piece = Example.Pieces[PieceHere];
				UTexture2D* Texture = IconSets.IsValidIndex(Piece.Row) ? GetIconTexture(IconSets[Piece.Row], Piece.Icon) : nullptr;
				if (Texture)
				{
					UImage* Icon = NewObject<UImage>(this);
					Icon->SetBrushFromTexture(Texture, false);
					Cell->SetContent(Icon);
				}
			}
			CellSize->SetContent(Cell);
			Grid->AddChildToUniformGrid(CellSize, RowIndex, Column);
		}
	}
	Frame->SetContent(Grid);
	Board->AddChildToVerticalBox(Frame);
	return Board;
}

void UGameRulesWidget::AddPicture(const FRulesSubject& Subject)
{
	USizeBox* PictureSize = NewObject<USizeBox>(this);
	PictureSize->SetWidthOverride(560.f);
	PictureSize->SetHeightOverride(250.f);
	if (UTexture2D* Picture = Pictures.FindRef(Subject.Title.ToString()))
	{
		UImage* Image = NewObject<UImage>(this);
		Image->SetBrushFromTexture(Picture, false);
		PictureSize->SetContent(Image);
	}
	else
	{
		// Until there's artwork: a box saying what the picture should show
		UBorder* Placeholder = NewObject<UBorder>(this);
		Placeholder->SetBrushColor(FLinearColor(1.f, 1.f, 1.f, 0.08f));
		Placeholder->SetPadding(FMargin(20.f));
		Placeholder->SetHorizontalAlignment(HAlign_Center);
		Placeholder->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Note = NewObject<UTextBlock>(this);
		Note->SetText(FText::Format(LOCTEXT("Placeholder", "Picture to come:\n{0}"), Subject.PictureNote));
		Note->SetFont(EntryFont);
		Note->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.55f)));
		Note->SetJustification(ETextJustify::Center);
		Note->SetAutoWrapText(true);
		Placeholder->SetContent(Note);
		PictureSize->SetContent(Placeholder);
	}
	IllustrationBox->AddChild(PictureSize);
}

void UGameRulesWidget::AddLayoutDiagram()
{
	struct FArea
	{
		const TCHAR* Widget;	// in WBP_Happiness
		FText Label;
		FLinearColor Color;
		FBox2D Fallback;		// where it is on a 1600x720 screen, as fractions (no puzzle screen to measure)
	};
	const FArea Areas[] =
	{
		{ TEXT("GamePanel"), LOCTEXT("AreaPuzzle", "Puzzle Area"), FLinearColor(0.25f, 0.6f, 1.f), FBox2D(FVector2D(0.061, 0.056), FVector2D(0.875, 0.722)) },
		{ TEXT("ButtonPanel"), LOCTEXT("AreaButtons", "Buttons"), FLinearColor(1.f, 0.6f, 0.2f), FBox2D(FVector2D(0.0, 0.0), FVector2D(0.061, 0.743)) },
		{ TEXT("VerticalCluePanel"), LOCTEXT("AreaVertical", "Vertical Clues"), FLinearColor(0.35f, 0.85f, 0.4f), FBox2D(FVector2D(0.006, 0.725), FVector2D(0.872, 0.994)) },
		{ TEXT("HorizontalCluePanel"), LOCTEXT("AreaHorizontal", "Horizontal Clues"), FLinearColor(0.75f, 0.45f, 1.f), FBox2D(FVector2D(0.875, 0.007), FVector2D(1.0, 1.0)) },
		{ TEXT("HelpPanel"), LOCTEXT("AreaInfo", "Clue Info"), FLinearColor(1.f, 0.9f, 0.3f), FBox2D(FVector2D(0.061, 0.007), FVector2D(0.875, 0.049)) },
	};

	// The puzzle screen (PC_Happiness's HappinessWidget): where its areas were last drawn (they hide while paused)
	UUserWidget* Screen = nullptr;
	if (const FObjectPropertyBase* ScreenProperty = PC ? FindFProperty<FObjectPropertyBase>(PC->GetClass(), TEXT("HappinessWidget")) : nullptr)
	{
		Screen = Cast<UUserWidget>(ScreenProperty->GetObjectPropertyValue_InContainer(PC));
	}
	const FGeometry ScreenGeometry = Screen ? Screen->GetCachedGeometry() : FGeometry();
	const FVector2D ScreenSize = ScreenGeometry.GetLocalSize();

	USizeBox* DiagramSize = NewObject<USizeBox>(this);
	DiagramSize->SetWidthOverride(LayoutDiagramSize.X);
	DiagramSize->SetHeightOverride(LayoutDiagramSize.Y);
	UBorder* Frame = NewObject<UBorder>(this);
	Frame->SetBrushColor(FLinearColor(0.08f, 0.09f, 0.12f, 1.f));
	Frame->SetPadding(FMargin(0.f));
	UCanvasPanel* Canvas = NewObject<UCanvasPanel>(this);
	Frame->SetContent(Canvas);
	DiagramSize->SetContent(Frame);

	// Where each area is: on the live screen, else the fallback
	TArray<FBox2D> Rects;
	for (const FArea& Area : Areas)
	{
		FBox2D Rect = Area.Fallback;
		const UWidget* Live = Screen ? Screen->GetWidgetFromName(Area.Widget) : nullptr;
		if (Live && ScreenSize.X > 0.0 && ScreenSize.Y > 0.0)
		{
			const FGeometry& Geometry = Live->GetCachedGeometry();
			const FVector2D Min = ScreenGeometry.AbsoluteToLocal(Geometry.GetAbsolutePosition());
			const FVector2D Max = ScreenGeometry.AbsoluteToLocal(Geometry.GetAbsolutePositionAtCoordinates(FVector2D(1.0, 1.0)));
			if (Max.X > Min.X && Max.Y > Min.Y)
			{
				Rect = FBox2D(Min / ScreenSize, Max / ScreenSize);
			}
		}
		// Thin areas (the clue info bar) still need room for their label
		const double MinHeight = 22.0 / LayoutDiagramSize.Y;
		if (Rect.Max.Y - Rect.Min.Y < MinHeight)
		{
			Rect.Max.Y = Rect.Min.Y + MinHeight;
		}
		Rects.Add(Rect);
	}
	// The puzzle area (first) starts below the clue info bar (last), which may have grown into it
	Rects[0].Min.Y = FMath::Max(Rects[0].Min.Y, Rects.Last().Max.Y + 2.0 / LayoutDiagramSize.Y);

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Areas); Index++)
	{
		const FArea& Area = Areas[Index];
		const FBox2D& Rect = Rects[Index];
		UBorder* Box = NewObject<UBorder>(this);
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Area.Color.CopyWithNewOpacity(0.28f));
		Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(4.f, 4.f, 4.f, 4.f), FSlateColor(Area.Color), 2.f);
		Box->SetBrush(Brush);
		Box->SetHorizontalAlignment(HAlign_Center);
		Box->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Label = NewObject<UTextBlock>(this);
		Label->SetText(Area.Label);
		FSlateFontInfo LabelFont = EntryFont;
		LabelFont.Size = 12;
		Label->SetFont(LabelFont);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Label->SetShadowOffset(FVector2D(1.f, 1.f));
		Label->SetJustification(ETextJustify::Center);
		Label->SetAutoWrapText(true);
		Box->SetContent(Label);

		if (UCanvasPanelSlot* BoxSlot = Canvas->AddChildToCanvas(Box))
		{
			BoxSlot->SetAnchors(FAnchors(Rect.Min.X, Rect.Min.Y, Rect.Max.X, Rect.Max.Y));
			BoxSlot->SetOffsets(FMargin(1.f));
		}
	}
	IllustrationBox->AddChild(DiagramSize);
}

void UGameRulesWidget::AddButtonLegend()
{
	// A button panel of our own, to copy its buttons' look from (whatever the panel's art is now)
	UClass* PanelClass = ButtonPanelClass.LoadSynchronous();
	UUserWidget* Panel = PanelClass ? CreateWidget<UUserWidget>(this, PanelClass) : nullptr;
	if (!Panel || !Panel->WidgetTree)
	{
		return;
	}

	// What each button does, by its name in WBP_ButtonPanel
	const TMap<FString, FText> Meanings =
	{
		{ TEXT("Button_P"), LOCTEXT("ButtonPause", "Pause menu") },
		{ TEXT("Button_H"), LOCTEXT("ButtonHint", "Hint: shows a clue that helps, and why") },
		{ TEXT("Button_U"), LOCTEXT("ButtonUndo", "Undo your last move") },
		{ TEXT("Button_HC"), LOCTEXT("ButtonHide", "Hide the selected clue") },
		{ TEXT("Button_Reset"), LOCTEXT("ButtonReset", "Reset the puzzle") },
		{ TEXT("Button_Fix"), LOCTEXT("ButtonFix", "Fix mistakes") },
	};

	UVerticalBox* Legend = NewObject<UVerticalBox>(this);
	UTextBlock* Heading = NewObject<UTextBlock>(this);
	Heading->SetText(LOCTEXT("ButtonsHeading", "Buttons"));
	Heading->SetFont(SectionFont);
	Heading->SetColorAndOpacity(FSlateColor(SectionColor));
	Heading->SetShadowOffset(FVector2D(1.f, 1.f));
	if (UVerticalBoxSlot* HeadingSlot = Legend->AddChildToVerticalBox(Heading))
	{
		HeadingSlot->SetPadding(FMargin(0.f, 14.f, 0.f, 4.f));
	}

	Panel->WidgetTree->ForEachWidget([&](UWidget* Widget)
	{
		// Only the buttons players see: not ones inside a hidden or collapsed box
		const UButton* Source = Cast<UButton>(Widget);
		if (!Source)
		{
			return;
		}
		for (const UWidget* Shown = Source; Shown; Shown = Shown->GetParent())
		{
			if (Shown->GetVisibility() == ESlateVisibility::Collapsed || Shown->GetVisibility() == ESlateVisibility::Hidden)
			{
				return;
			}
		}

		// A small copy of the button, scaled from its size in the panel: its style (the art), and its icon or caption
		const USizeBox* SourceSize = Cast<USizeBox>(Source->GetParent());
		const float Height = SourceSize && SourceSize->GetHeightOverride() > 0.f ? SourceSize->GetHeightOverride() : LegendButtonSize;
		const float Width = SourceSize && SourceSize->GetWidthOverride() > 0.f ? SourceSize->GetWidthOverride() : Height;
		const float Scale = LegendButtonSize / Height;

		UButton* Copy = NewObject<UButton>(this);
		Copy->SetStyle(Source->GetStyle());
		Copy->SetVisibility(ESlateVisibility::HitTestInvisible);
		FText Label;
		const UWidget* Content = Source->GetContent();
		UWidget* ContentCopy = nullptr;
		if (const UImage* Image = Cast<UImage>(Content))
		{
			UImage* Icon = NewObject<UImage>(this);
			Icon->SetBrush(Image->GetBrush());
			ContentCopy = Icon;
		}
		else if (const UTextBlock* Text = Cast<UTextBlock>(Content))
		{
			Label = Text->GetText();
			UTextBlock* TextCopy = NewObject<UTextBlock>(this);
			TextCopy->SetText(Text->GetText());
			FSlateFontInfo Font = Text->GetFont();
			Font.Size = FMath::Max(1.f, Font.Size * Scale);
			TextCopy->SetFont(Font);
			TextCopy->SetColorAndOpacity(Text->GetColorAndOpacity());
			TextCopy->SetShadowOffset(Text->GetShadowOffset());
			TextCopy->SetShadowColorAndOpacity(Text->GetShadowColorAndOpacity());
			TextCopy->SetJustification(ETextJustify::Center);
			ContentCopy = TextCopy;
		}
		if (ContentCopy)
		{
			Copy->SetContent(ContentCopy);
			const UButtonSlot* SourceSlot = Cast<UButtonSlot>(Content->Slot);
			if (UButtonSlot* CopySlot = Cast<UButtonSlot>(ContentCopy->Slot); CopySlot && SourceSlot)
			{
				const FMargin Padding = SourceSlot->GetPadding();
				CopySlot->SetPadding(FMargin(Padding.Left * Scale, Padding.Top * Scale, Padding.Right * Scale, Padding.Bottom * Scale));
				CopySlot->SetHorizontalAlignment(SourceSlot->GetHorizontalAlignment());
				CopySlot->SetVerticalAlignment(SourceSlot->GetVerticalAlignment());
			}
		}
		USizeBox* ButtonSize = NewObject<USizeBox>(this);
		ButtonSize->SetWidthOverride(Width * Scale);
		ButtonSize->SetHeightOverride(LegendButtonSize);
		ButtonSize->SetContent(Copy);

		const FText* Meaning = Meanings.Find(Source->GetName());
		UTextBlock* Description = NewObject<UTextBlock>(this);
		Description->SetText(Meaning ? *Meaning : Label);
		Description->SetFont(EntryFont);
		Description->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Description->SetShadowOffset(FVector2D(1.f, 1.f));

		UHorizontalBox* Line = NewObject<UHorizontalBox>(this);
		Line->AddChildToHorizontalBox(ButtonSize);
		if (UHorizontalBoxSlot* TextSlot = Line->AddChildToHorizontalBox(Description))
		{
			TextSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UVerticalBoxSlot* LineSlot = Legend->AddChildToVerticalBox(Line))
		{
			LineSlot->SetPadding(FMargin(0.f, 3.f));
		}
	});
	IllustrationBox->AddChild(Legend);
}

bool UGameRulesWidget::FindExampleClue(ECampaignLesson Lesson, UPuzzle*& OutPuzzle, UClue*& OutClue)
{
	if (TObjectPtr<UClue>* Cached = ExampleClues.Find(Lesson))
	{
		OutClue = *Cached;
		OutPuzzle = ExamplePuzzles.FindRef(Lesson);
		return OutClue && OutPuzzle;
	}

	// A small Lessons puzzle for the clue type has one in it (a fixed seed: the same example every time)
	for (int32 Seed = 1; Seed <= ExampleSeedTries; Seed++)
	{
		UPuzzle* Puzzle = NewObject<UPuzzle>(this);
		if (!Puzzle->InitCampaign(Seed, ExamplePuzzleSize, 1, Lesson))
		{
			continue;
		}
		for (const TArray<UClue*>* Clues : { &Puzzle->VerticalClues(), &Puzzle->HorizontalClues() })
		{
			for (UClue* Clue : *Clues)
			{
				if (Clue && Clue->GetCampaignLesson() == Lesson)
				{
					ExamplePuzzles.Add(Lesson, Puzzle);
					ExampleClues.Add(Lesson, Clue);
					OutPuzzle = Puzzle;
					OutClue = Clue;
					return true;
				}
			}
		}
	}
	ExampleClues.Add(Lesson, nullptr);
	return false;
}

UUserWidget* UGameRulesWidget::MakeExample(ECampaignLesson Lesson)
{
	UPuzzle* Puzzle = nullptr;
	UClue* Clue = nullptr;
	if (!PC || !FindExampleClue(Lesson, Puzzle, Clue))
	{
		return nullptr;
	}

	// The game's icon sets (PC_Happiness's HappinessIconSets): the clue widget turns icon numbers into pictures with them
	const FArrayProperty* IconSetsProperty = FindFProperty<FArrayProperty>(PC->GetClass(), TEXT("HappinessIconSets"));
	if (!IconSetsProperty)
	{
		return nullptr;
	}
	FScriptArrayHelper IconSets(IconSetsProperty, IconSetsProperty->ContainerPtrToValuePtr<void>(PC));
	const FObjectPropertyBase* IconSetElement = CastField<FObjectPropertyBase>(IconSetsProperty->Inner);
	if (!IconSetElement || IconSets.Num() < Puzzle->m_iSize)
	{
		return nullptr;
	}

	UClass* WidgetClass = (Clue->m_Type == eClueType::Vertical ? VerticalClueClass : HorizontalClueClass).LoadSynchronous();
	UUserWidget* Widget = WidgetClass ? CreateWidget<UUserWidget>(this, WidgetClass) : nullptr;
	UFunction* Populate = Widget ? Widget->FindFunction(TEXT("Populate")) : nullptr;
	if (!Populate)
	{
		return nullptr;
	}

	// Populate(Clue, Puzzle, IconSets), the clue panels' call
	uint8* Params = static_cast<uint8*>(FMemory_Alloca(Populate->ParmsSize));
	FMemory::Memzero(Params, Populate->ParmsSize);
	for (TFieldIterator<FProperty> It(Populate); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		It->InitializeValue_InContainer(Params);
		if (const FObjectPropertyBase* Object = CastField<FObjectPropertyBase>(*It))
		{
			UObject* Value = Object->PropertyClass && Clue->IsA(Object->PropertyClass) ? static_cast<UObject*>(Clue)
				: Object->PropertyClass && Puzzle->IsA(Object->PropertyClass) ? static_cast<UObject*>(Puzzle) : nullptr;
			Object->SetObjectPropertyValue_InContainer(Params, Value);
		}
		else if (const FArrayProperty* Array = CastField<FArrayProperty>(*It))
		{
			const FObjectPropertyBase* Element = CastField<FObjectPropertyBase>(Array->Inner);
			FScriptArrayHelper Sets(Array, Array->ContainerPtrToValuePtr<void>(Params));
			Sets.Resize(IconSets.Num());
			for (int32 Set = 0; Element && Set < IconSets.Num(); Set++)
			{
				Element->SetObjectPropertyValue(Sets.GetRawPtr(Set), IconSetElement->GetObjectPropertyValue(IconSets.GetRawPtr(Set)));
			}
		}
	}
	Widget->ProcessEvent(Populate, Params);
	for (TFieldIterator<FProperty> It(Populate); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		It->DestroyValue_InContainer(Params);
	}
	return Widget;
}

#undef LOCTEXT_NAMESPACE
