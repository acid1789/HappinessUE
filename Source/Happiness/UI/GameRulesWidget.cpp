#include "UI/GameRulesWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "UObject/EnumProperty.h"
#include "UObject/UnrealType.h"
#include "HappinessClassic/Clue.h"
#include "HappinessClassic/ClueExample.h"
#include "HappinessClassic/Puzzle.h"
#include "HappinessClassic/PuzzleCell.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/SlateBrush.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "GameRules"

namespace
{
	// How many seeds to try for a puzzle with the clue type in it
	const int32 ExampleSeedTries = 40;
	const FLinearColor RulesInk(0.025f, 0.012f, 0.005f, 1.f);
	const FLinearColor RulesCream(0.97f, 0.94f, 0.84f, 1.f);
	const FLinearColor RulesGold(0.83f, 0.63f, 0.28f, 1.f);
	const FLinearColor RulesSage(0.025f, 0.10f, 0.025f, 1.f);
	const FLinearColor RulesNavy(0.001f, 0.012f, 0.025f, 1.f);

	FSlateBrush MakeRulesSurface(const FLinearColor& Fill, const FLinearColor& Outline, float Radius = 9.f, float Width = 2.f)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(Radius), FSlateColor(Outline), Width);
		return Brush;
	}

	FButtonStyle MakeRulesEntryStyle(bool bSelected, UTexture2D* Texture)
	{
		FButtonStyle Style;
		if (Texture)
		{
			auto Face = [Texture, bSelected](const FLinearColor& Tint)
			{
				FSlateBrush Brush;
				Brush.SetResourceObject(Texture);
				Brush.DrawAs = ESlateBrushDrawType::Image;
				Brush.ImageSize = FVector2D(405.f, 42.f);
				Brush.TintColor = FSlateColor(Tint);
				Brush.SetUVRegion(FBox2f(
					FVector2f(.001f, bSelected ? .502f : .002f),
					FVector2f(.999f, bSelected ? .998f : .498f)));
				return Brush;
			};
			Style.SetNormal(Face(FLinearColor::White));
			Style.SetHovered(Face(FLinearColor(1.f, 1.f, .94f, 1.f)));
			Style.SetPressed(Face(FLinearColor(.88f, .90f, .83f, 1.f)));
		}
		else
		{
			Style.SetNormal(MakeRulesSurface(bSelected ? RulesNavy : RulesCream, RulesGold, 24.f, 2.f));
			Style.SetHovered(MakeRulesSurface(RulesCream, RulesGold, 24.f, 2.f));
			Style.SetPressed(MakeRulesSurface(RulesSage, RulesGold, 24.f, 2.f));
		}
		Style.SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f));
		return Style;
	}

	FText GetRulesLessonDetails(ECampaignLesson Lesson)
	{
		switch (Lesson)
		{
		case ECampaignLesson::VerticalTwo: return LOCTEXT("DetailsSameColumn", "Both icons belong in one column, even though they are in different rows. Once you place either icon, you know the other icon's column too.");
		case ECampaignLesson::VerticalThree: return LOCTEXT("DetailsThreeColumn", "All three icons line up in a single column. Placing any one of them identifies the column for the other two.");
		case ECampaignLesson::TwoNot: return LOCTEXT("DetailsTwoNot", "The crossed-out icon cannot be in the other icon's column. If you place one of them, rule that column out for the other.");
		case ECampaignLesson::ThreeNot: return LOCTEXT("DetailsThreeNot", "The two uncrossed icons share a column. The crossed-out icon cannot be in that column, even if its own row still has several possibilities.");
		case ECampaignLesson::EitherOr: return LOCTEXT("DetailsEitherOr", "The top icon shares a column with exactly one of the two choices below. Whichever choice shares it, the other choice must be in a different column.");
		case ECampaignLesson::NextTo: return LOCTEXT("DetailsNextTo", "There is no column between these icons. Either icon may be on the left; placing one narrows the other to its immediate neighboring columns.");
		case ECampaignLesson::NotNextTo: return LOCTEXT("DetailsNotNextTo", "The crossed-out icon cannot occupy either column immediately beside the other icon. The clue does not by itself say where else it must be.");
		case ECampaignLesson::DirectlyLeftOf: return LOCTEXT("DetailsDirectLeft", "The first icon is exactly one column to the left of the second. Their order is fixed, and there is no column between them.");
		case ECampaignLesson::LeftOf: return LOCTEXT("DetailsLeftOf", "The first icon appears somewhere to the left of the second. They may be adjacent or have other columns between them, but their order cannot reverse.");
		case ECampaignLesson::NotLeftOf: return LOCTEXT("DetailsNotLeft", "The first icon cannot be to the left of the second. Use this to remove placements that would put the icons in that forbidden order.");
		case ECampaignLesson::Edge: return LOCTEXT("DetailsEdge", "An edge icon belongs in the first or last column. When crossed out, those two edge columns are ruled out instead.");
		case ECampaignLesson::Span: return LOCTEXT("DetailsSpan", "The middle icon has one of the other icons somewhere to its left and the other somewhere to its right. The outer icons may appear in either order.");
		case ECampaignLesson::SpanNotSide: return LOCTEXT("DetailsSpanNotSide", "One icon is beside the middle icon. The crossed-out icon cannot be on the middle icon's opposite side, which helps decide the arrangement.");
		case ECampaignLesson::Gap: return LOCTEXT("DetailsGap", "The icons have exactly one column between them. The gap may face either way: one icon can be two columns left or two columns right of the other.");
		case ECampaignLesson::SpanNotMid: return LOCTEXT("DetailsSpanNotMid", "The two outer icons have one column between them. The crossed-out icon cannot occupy that middle column.");
		case ECampaignLesson::Between: return LOCTEXT("DetailsBetween", "The middle icon lies somewhere between the other two icons. The outer icons may appear in either order, and gaps are allowed.");
		case ECampaignLesson::Chain: return LOCTEXT("DetailsChain", "Read this clue from left to right. The icons must appear in the shown order, although one or more columns may separate them.");
		case ECampaignLesson::NextToEitherOr: return LOCTEXT("DetailsNextEither", "The middle icon is beside exactly one of the two other icons. The remaining icon is not beside it, so testing one neighbor can eliminate the other choice.");
		case ECampaignLesson::AllApart: return LOCTEXT("DetailsAllApart", "Each of the three icons occupies a different column. If you place one icon, its column is ruled out for both of the others.");
		default: return FText();
		}
	}

	/** Icon Index of an icon set (DA_IconSet): its Icons array holds the textures, directly or in a struct. As UHintInfoWidget's. */
	UTexture2D* GetRulesIconTexture(UObject* IconSet, int32 Index)
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
	CellDialogClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/Happiness/UI/WBP_CellDialog.WBP_CellDialog_C")));
	GridCellClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/Happiness/UI/WBP_Cell.WBP_Cell_C")));
	HorizontalClueClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/Happiness/UI/WBP_HorizontalClue.WBP_HorizontalClue_C")));
	ButtonPanelClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/Happiness/UI/WBP_ButtonPanel.WBP_ButtonPanel_C")));

	static ConstructorHelpers::FObjectFinder<UFont> Serif(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	EntryFont = FSlateFontInfo(Serif.Object, 18);
	SectionFont = FSlateFontInfo(Serif.Object, 21);
	EntryColor = RulesInk;
	SelectedColor = RulesCream;
	SectionColor = RulesCream;
	EntryStyle = MakeRulesEntryStyle(false, nullptr);
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
		Subject.Details = Description;
		Subject.PictureNote = PictureNote;
		Subject.BasicIndex = Result.Num() - 1;
	};
	auto Clue = [&Result](const FText& Section, ECampaignLesson Lesson, const FText& Description, const FText& Title = FText())
	{
		FRulesSubject& Subject = Result.AddDefaulted_GetRef();
		Subject.Section = Section;
		Subject.Title = Title.IsEmpty() ? UCampaignTree::GetLessonDisplayName(Lesson) : Title;
		Subject.Description = Description;
		Subject.Details = GetRulesLessonDetails(Lesson);
		Subject.bClue = true;
		Subject.Lesson = Lesson;
	};

	const FText Basics = LOCTEXT("Basics", "Basics");
	Basic(Basics, LOCTEXT("Objective", "Objective"),
		LOCTEXT("ObjectiveDesc", "Use the clues to determine where each icon belongs in the grid.\nEach icon belongs in exactly one spot in the grid."),		
		LOCTEXT("ObjectivePic", "A solved puzzle grid, with one row and one column highlighted"));
	Result.Last().Details = LOCTEXT("ObjectiveDetails", "");
	Basic(FText(), LOCTEXT("Layout", "Game Layout"),
		LOCTEXT("LayoutDesc", "The puzzle grid, clues, and controls each have their own area."),
		FText());
	// Below the diagram and button legend (scrolling with them): what each area is for
	Result.Last().Details = LOCTEXT("LayoutDetails",
		"Puzzle Area: The grid you solve. Each row is one set of icons, and each cell starts with every icon that might belong there.\n\n"
		"Clue Info: Explains the selected clue in words. When you ask for a hint, its explanation appears here.\n\n"
		"Vertical Clues: Clues about which icons share a column, or never do.\n\n"
		"Horizontal Clues: Clues about the left-to-right order of the columns: which icons are next to, left of, or between others.");
	Result.Last().bLayoutDiagram = true;
	Basic(FText(), LOCTEXT("Controls", "Controls"),
		LOCTEXT("ControlsMouseDesc", "Left-click an icon to place it. Right-click an icon to eliminate it. Middle-click to restore it."),
		LOCTEXT("ControlsPic", "A cell with all its icons, a cell with one ruled out, a cell with its answer placed"));
	Result.Last().Details = LOCTEXT("ControlsMouseDetails", "Each cell begins with the icons that might belong there. Right-click an icon the clues rule out to eliminate it, and middle-click its spot to bring it back. When you are sure of a cell's answer, left-click that icon to place it. On a placed icon, right-click rules it out and middle-click returns the cell to its icons.");
	Result.Last().TouchDescription = LOCTEXT("ControlsTouchDesc", "Tap a cell to open the cell window.\n Here you can eliminate, restore, or place icons.");
	Result.Last().TouchDetails = LOCTEXT("ControlsTouchDetails", "Tap the icon you want to interact with to select it.\nEliminate/Restore:  Determines if this icon should still be a candidate in this cell.\nConfirm:  Indicates this icon should be placed in this cell.\nReset All:  Restores all icons in the cell.\nCancel:  Returns to the puzzle without making changes\nCommit:  Applies the changes to the puzzle.");
	Result.Last().bControls = true;
	Basic(FText(), LOCTEXT("Clues", "Clues"),
		LOCTEXT("CluesDesc", "Tap a clue to see what it means. Hide clues you're done with (HC); Unhide Clues in the pause menu brings them back."),
		LOCTEXT("CluesPic", "A selected clue, with its explanation above the grid"));
	Result.Last().Details = LOCTEXT("CluesDetails", "Vertical clues describe icons that share a column or cannot share one. Horizontal clues describe their left-to-right positions. Select a clue to read its meaning above the puzzle, and hide it when you no longer need it.");
	Basic(FText(), LOCTEXT("Hints", "Hints"),
		LOCTEXT("HintsDesc", "Stuck? H points to a clue that helps, and explains why. Each hint adds 10 seconds."),
		LOCTEXT("HintsPic", "The hint panel pointing at a clue and the cell it helps with"));
	Result.Last().Details = LOCTEXT("HintsDetails", "A hint identifies a useful clue and highlights the part of the puzzle it can help with. Read the explanation and make the deduction yourself. Each hint adds 10 seconds to your time.");

	const FText Vertical = LOCTEXT("Vertical", "Vertical Clues");
	Clue(Vertical, ECampaignLesson::VerticalTwo, LOCTEXT("SameColumnDesc", "These icons are in the same column."), LOCTEXT("SameColumn", "Same Column"));
	Clue(FText(), ECampaignLesson::VerticalThree, LOCTEXT("VerticalThreeDesc", "All three icons are in the same column."));
	Clue(FText(), ECampaignLesson::TwoNot, LOCTEXT("TwoNotDesc", "The crossed-out icon is never in the same column as the other."));
	Clue(FText(), ECampaignLesson::ThreeNot, LOCTEXT("ThreeNotDesc", "Two icons share a column; the crossed-out one isn't in it."));
	Clue(FText(), ECampaignLesson::EitherOr, LOCTEXT("EitherOrDesc", "The top icon shares a column with exactly one of the two below it."));

	const FText Horizontal = LOCTEXT("Horizontal", "Horizontal Clues");
	Clue(Horizontal, ECampaignLesson::NextTo, LOCTEXT("NextToDesc", "These two icons are in adjacent columns, next to each other in either order. They must be directly beside one another, with no column between them."));
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
	ScrollBelowSummary();
	FieldGuideTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/General/Textures/T_GameRulesFieldGuide.T_GameRulesFieldGuide"));
	if (FieldGuideTexture)
	{
		FSlateBrush Background;
		Background.SetResourceObject(FieldGuideTexture);
		Background.DrawAs = ESlateBrushDrawType::Image;
		Background.TintColor = FSlateColor(FLinearColor::White);
		Backdrop->SetBrush(Background);
		Backdrop->SetBrushColor(FLinearColor::White);
		FSlateBrush NoFrame;
		NoFrame.DrawAs = ESlateBrushDrawType::NoDrawType;
		IndexPanel->SetBrush(NoFrame);
		DetailPaper->SetBrush(NoFrame);
	}
	CloseButtonTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/General/Textures/T_LessonNotebookButtons.T_LessonNotebookButtons"));
	TopicPillTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/General/Textures/T_GameRulesTopicPillDark.T_GameRulesTopicPillDark"));
	EntryStyle = MakeRulesEntryStyle(false, TopicPillTexture);
	auto CloseFace = [this](const FLinearColor& Tint)
	{
		if (!CloseButtonTexture)
		{
			return MakeRulesSurface(RulesCream, RulesGold, 14.f, 3.f);
		}
		FSlateBrush Face;
		Face.SetResourceObject(CloseButtonTexture);
		Face.DrawAs = ESlateBrushDrawType::Image;
		Face.ImageSize = FVector2D(190.f, 58.f);
		Face.TintColor = FSlateColor(Tint);
		Face.SetUVRegion(FBox2f(FVector2f(.03f, .06f), FVector2f(.98f, .485f)));
		return Face;
	};
	FButtonStyle BackStyle;
	BackStyle.SetNormal(CloseFace(FLinearColor::White));
	BackStyle.SetHovered(CloseFace(FLinearColor(1.f, 1.f, 0.94f, 1.f)));
	BackStyle.SetPressed(CloseFace(FLinearColor(0.87f, 0.89f, 0.79f, 1.f)));
	BackStyle.SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f));
	CloseButton->SetStyle(BackStyle);
	CloseButton->OnClicked.AddUniqueDynamic(this, &UGameRulesWidget::HandleClose);
	BuildNextButton(BackStyle);
	if (Subjects.Num() == 0)
	{
		Subjects = MakeSubjects();
		BuildIndex();
	}
	if (Selected == INDEX_NONE)
	{
		Select(PC ? 0 : FMath::Clamp(PreviewSubject, 0, Subjects.Num() - 1));
	}
	else
	{
		// Opened (Show) before it was constructed: again, now its textures and the Next button are in place
		Select(Selected);
	}
}

void UGameRulesWidget::ScrollBelowSummary()
{
	// The title and summary stay put; the example, the picture and the details below them scroll together, in the
	// details' scroll box (DescriptionScroll). Moved once: a second construct finds them already in it
	UScrollBox* Scroll = Cast<UScrollBox>(DescriptionText->GetParent());
	UWidget* ExampleArea = ExampleBox->GetParent();
	if (!Scroll || !ExampleArea || ExampleArea->GetParent() == Scroll)
	{
		return;
	}
	// The scroll box takes all the room below the summary
	if (UVerticalBoxSlot* ScrollSlot = Cast<UVerticalBoxSlot>(Scroll->Slot))
	{
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	// Pictures only: a drag or the wheel over them (the button legend's buttons too) scrolls the page
	IllustrationBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	for (UWidget* Widget : { ExampleArea, static_cast<UWidget*>(IllustrationBox), static_cast<UWidget*>(DescriptionText) })
	{
		FMargin SlotPadding;
		EHorizontalAlignment Horizontal = HAlign_Fill;
		if (const UVerticalBoxSlot* OldSlot = Cast<UVerticalBoxSlot>(Widget->Slot))
		{
			SlotPadding = OldSlot->GetPadding();
			Horizontal = OldSlot->GetHorizontalAlignment();
		}
		else if (const UScrollBoxSlot* OldScrollSlot = Cast<UScrollBoxSlot>(Widget->Slot))
		{
			SlotPadding = OldScrollSlot->GetPadding();
			Horizontal = OldScrollSlot->GetHorizontalAlignment();
		}
		Widget->RemoveFromParent();
		if (UScrollBoxSlot* NewSlot = Cast<UScrollBoxSlot>(Scroll->AddChild(Widget)))
		{
			NewSlot->SetPadding(SlotPadding);
			NewSlot->SetHorizontalAlignment(Horizontal);
		}
	}
}

bool UGameRulesWidget::IsTouchInput() const
{
	// PC_Happiness.InputMode, a Blueprint enum: NewEnumerator0 touch, NewEnumerator1 mouse
	if (PC)
	{
		if (const FProperty* Property = PC->GetClass()->FindPropertyByName(TEXT("InputMode")))
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
	}
	// No player controller (the editor preview): as BeginPlay sets it
	return PLATFORM_ANDROID || PLATFORM_IOS;
}

void UGameRulesWidget::Show(APlayerController* PlayerController)
{
	// Always from the start, whatever was read last time
	Open(PlayerController, 0);
}

void UGameRulesWidget::ShowLesson(APlayerController* PlayerController, ECampaignLesson Lesson)
{
	if (Subjects.Num() == 0)
	{
		Subjects = MakeSubjects();
		BuildIndex();
	}
	// The first lesson (Game Basics) teaches the game itself, so it opens at the Basics' Objective, not its clue
	const int32 Subject = Lesson == ECampaignLesson::VerticalTwo ? INDEX_NONE : FindLessonSubject(Lesson);
	Open(PlayerController, Subject == INDEX_NONE ? 0 : Subject);
}

void UGameRulesWidget::Open(APlayerController* PlayerController, int32 Subject)
{
	PC = PlayerController;
	if (Subjects.Num() == 0)
	{
		Subjects = MakeSubjects();
		BuildIndex();
	}
	SetVisibility(ESlateVisibility::Visible);
	// Selected now, with the player controller: the examples need its icon sets
	Select(FMath::Clamp(Subject, 0, Subjects.Num() - 1));
	if (UScrollBox* Index = Cast<UScrollBox>(SubjectList->GetParent()))
	{
		if (Selected == 0)
		{
			Index->ScrollToStart();
		}
		else if (EntryButtons.IsValidIndex(Selected))
		{
			// The section header above the first clue of a section scrolls in with it
			Index->ScrollWidgetIntoView(EntryButtons[Selected], false, EDescendantScrollDestination::Center);
		}
	}
}

int32 UGameRulesWidget::FindLessonSubject(ECampaignLesson Lesson) const
{
	return Subjects.IndexOfByPredicate([Lesson](const FRulesSubject& Subject) { return Subject.bClue && Subject.Lesson == Lesson; });
}

void UGameRulesWidget::HandleClose()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UGameRulesWidget::BuildNextButton(const FButtonStyle& Style)
{
	UCanvasPanel* Canvas = Cast<UCanvasPanel>(GetRootWidget());
	if (NextButton || !Canvas)
	{
		return;
	}
	// Close's twin in the bottom right corner: same face and size, same label font
	NextButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("NextButton"));
	NextButton->SetStyle(Style);
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NextText"));
	Label->SetText(LOCTEXT("Next", "Next  >"));
	if (const UTextBlock* CloseText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("CloseText"))))
	{
		Label->SetFont(CloseText->GetFont());
		Label->SetColorAndOpacity(CloseText->GetColorAndOpacity());
	}
	if (UButtonSlot* LabelSlot = Cast<UButtonSlot>(NextButton->AddChild(Label)))
	{
		LabelSlot->SetHorizontalAlignment(HAlign_Center);
		LabelSlot->SetVerticalAlignment(VAlign_Center);
	}
	const UCanvasPanelSlot* CloseSlot = CloseButton->GetParent() ? Cast<UCanvasPanelSlot>(CloseButton->GetParent()->Slot) : nullptr;
	const FVector2D Size = CloseSlot ? FVector2D(CloseSlot->GetSize()) : FVector2D(205.f, 62.f);
	const float Inset = CloseSlot ? -CloseSlot->GetPosition().X : 29.f;
	if (UCanvasPanelSlot* NextSlot = Canvas->AddChildToCanvas(NextButton))
	{
		NextSlot->SetAnchors(FAnchors(1.f, 1.f));
		NextSlot->SetAlignment(FVector2D(1.f, 1.f));
		NextSlot->SetPosition(FVector2D(-Inset, -Inset));
		NextSlot->SetSize(Size);
		NextSlot->SetZOrder(10);
	}
	// The detail column ends above it, so its text scrolls clear of the button
	if (UWidget* Detail = WidgetTree->FindWidget(TEXT("Detail")))
	{
		if (UCanvasPanelSlot* DetailSlot = Cast<UCanvasPanelSlot>(Detail->Slot))
		{
			FMargin Offsets = DetailSlot->GetOffsets();
			Offsets.Bottom = FMath::Max(Offsets.Bottom, Inset + Size.Y + 12.f);
			DetailSlot->SetOffsets(Offsets);
		}
	}
	NextButton->OnClicked.AddUniqueDynamic(this, &UGameRulesWidget::HandleNext);
}

void UGameRulesWidget::HandleNext()
{
	if (!Subjects.IsValidIndex(Selected + 1))
	{
		return;
	}
	Select(Selected + 1);
	// Keep the index in step: the new subject's entry comes into view
	if (UScrollBox* Index = Cast<UScrollBox>(SubjectList->GetParent()); Index && EntryButtons.IsValidIndex(Selected))
	{
		Index->ScrollWidgetIntoView(EntryButtons[Selected], true, EDescendantScrollDestination::IntoView);
	}
}

void UGameRulesWidget::BuildIndex()
{
	SubjectList->ClearChildren();
	EntryButtons.Reset();
	EntryTexts.Reset();
	EntryHandlers.Reset();
	const UFont* Serif = LoadObject<UFont>(nullptr, TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	const FSlateFontInfo RowFont(Serif, 19);
	const FSlateFontInfo HeaderFont(Serif, 24);

	for (int32 Index = 0; Index < Subjects.Num(); Index++)
	{
		const FRulesSubject& Subject = Subjects[Index];
		if (!Subject.Section.IsEmpty())
		{
			UTextBlock* Header = NewObject<UTextBlock>(this);
			Header->SetText(Subject.Section);
			Header->SetFont(HeaderFont);
			Header->SetColorAndOpacity(FSlateColor(RulesCream));
			Header->SetJustification(ETextJustify::Center);
			if (UVerticalBoxSlot* HeaderSlot = Cast<UVerticalBoxSlot>(SubjectList->AddChild(Header)))
			{
				HeaderSlot->SetPadding(FMargin(0.f, Index == 0 ? 5.f : 10.f, 0.f, 5.f));
			}
		}

		UButton* Button = NewObject<UButton>(this);
		Button->SetStyle(MakeRulesEntryStyle(false, TopicPillTexture));
		UTextBlock* Text = NewObject<UTextBlock>(this);
		Text->SetText(Subject.Title);
		Text->SetFont(RowFont);
		Text->SetColorAndOpacity(FSlateColor(RulesInk));
		UHorizontalBox* EntryContent = NewObject<UHorizontalBox>(this);
		if (UHorizontalBoxSlot* LabelSlot = EntryContent->AddChildToHorizontalBox(Text))
		{
			LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			LabelSlot->SetPadding(FMargin(20.f, 2.f, 0.f, 2.f));
		}
		UTextBlock* Arrow = NewObject<UTextBlock>(this);
		Arrow->SetText(FText::FromString(TEXT(">")));
		Arrow->SetFont(RowFont);
		Arrow->SetColorAndOpacity(FSlateColor(RulesGold));
		if (UHorizontalBoxSlot* ArrowSlot = EntryContent->AddChildToHorizontalBox(Arrow))
		{
			ArrowSlot->SetPadding(FMargin(0.f, 1.f, 17.f, 1.f));
		}
		if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Button->AddChild(EntryContent)))
		{
			ContentSlot->SetHorizontalAlignment(HAlign_Fill);
			ContentSlot->SetVerticalAlignment(VAlign_Fill);
		}
		if (UVerticalBoxSlot* ButtonSlot = Cast<UVerticalBoxSlot>(SubjectList->AddChild(Button)))
		{
			ButtonSlot->SetPadding(FMargin(4.f, 1.f));
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
	if (NextButton)
	{
		NextButton->SetVisibility(Subjects.IsValidIndex(Index + 1) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	const FRulesSubject& Subject = Subjects[Index];
	const bool bTouchText = Subject.bControls && IsTouchInput();
	const FText& Details = bTouchText ? Subject.TouchDetails : Subject.Details;
	TitleText->SetText(Subject.Title);
	if (SummaryText)
	{
		SummaryText->SetText(bTouchText ? Subject.TouchDescription : Subject.Description);
	}
	DescriptionText->SetText(Details);
	// No details (Game Layout): no gap for them below the pictures
	DescriptionText->SetVisibility(Details.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	// A new subject reads from its top
	if (UScrollBox* Scroll = Cast<UScrollBox>(DescriptionText->GetParent()))
	{
		Scroll->ScrollToStart();
	}

	for (int32 Entry = 0; Entry < EntryTexts.Num(); Entry++)
	{
		const bool bSelected = Entry == Index;
		EntryTexts[Entry]->SetColorAndOpacity(FSlateColor(bSelected ? RulesCream : RulesInk));
		EntryButtons[Entry]->SetStyle(MakeRulesEntryStyle(bSelected, TopicPillTexture));
	}

	ExampleBox->ClearChildren();
	IllustrationBox->ClearChildren();
	UWidget* Example = nullptr;
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
		const UFont* Serif = LoadObject<UFont>(nullptr, TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
		USizeBox* CardSize = NewObject<USizeBox>(this);
		CardSize->SetWidthOverride(1190.f);
		CardSize->SetHeightOverride(180.f);
		UBorder* Card = NewObject<UBorder>(this);
		Card->SetBrush(MakeRulesSurface(FLinearColor(1.f, 0.985f, 0.94f, 1.f), RulesGold, 18.f, 3.f));
		Card->SetPadding(FMargin(20.f, 12.f));
		UHorizontalBox* Row = NewObject<UHorizontalBox>(this);
		auto AddArea = [this, Row](UWidget* Content, float Width)
		{
			USizeBox* Area = NewObject<USizeBox>(this);
			Area->SetWidthOverride(Width);
			Area->SetContent(Content);
			if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Area))
			{
				Slot->SetVerticalAlignment(VAlign_Center);
			}
		};
		UTextBlock* Lead = NewObject<UTextBlock>(this);
		Lead->SetText(LOCTEXT("ExampleClue", "Example Clue:"));
		Lead->SetFont(FSlateFontInfo(Serif, 28));
		Lead->SetColorAndOpacity(FSlateColor(FLinearColor(0.02f, 0.09f, 0.02f, 1.f)));
		AddArea(Lead, 265.f);
		UScaleBox* ClueFit = NewObject<UScaleBox>(this);
		ClueFit->SetStretch(EStretch::ScaleToFit);
		ClueFit->AddChild(Example);
		AddArea(ClueFit, 470.f);
		UTextBlock* Explanation = NewObject<UTextBlock>(this);
		Explanation->SetText(Subject.Lesson == ECampaignLesson::NextTo
			? LOCTEXT("NextToExampleCaption", "The icons must sit side by side. Either one may be on the left.")
			: Subject.Description);
		Explanation->SetFont(FSlateFontInfo(Serif, 23));
		Explanation->SetColorAndOpacity(FSlateColor(RulesInk));
		Explanation->SetAutoWrapText(true);
		AddArea(Explanation, 410.f);
		Card->SetContent(Row);
		CardSize->SetContent(Card);
		ExampleBox->AddChild(CardSize);
	}
	ExampleBox->GetParent()->SetVisibility(Example ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

TArray<UObject*> UGameRulesWidget::GetIconSets() const
{
	TArray<UObject*> Result;
	const FArrayProperty* Property = PC ? FindFProperty<FArrayProperty>(PC->GetClass(), TEXT("HappinessIconSets")) : nullptr;
	const FObjectPropertyBase* Element = Property ? CastField<FObjectPropertyBase>(Property->Inner) : nullptr;
	if (Element)
	{
		FScriptArrayHelper Array(Property, Property->ContainerPtrToValuePtr<void>(PC));
		for (int32 Set = 0; Set < Array.Num(); Set++)
		{
			Result.Add(Element->GetObjectPropertyValue(Array.GetRawPtr(Set)));
		}
	}
	if (Result.Num() == 0)
	{
		for (const TCHAR* Path : {
			TEXT("/Game/Happiness/IconSets/IS_Flowers.IS_Flowers"),
			TEXT("/Game/Happiness/IconSets/IS_Fruits.IS_Fruits"),
			TEXT("/Game/Happiness/IconSets/IS_Balls.IS_Balls"),
			TEXT("/Game/Happiness/IconSets/IS_Toys.IS_Toys") })
		{
			Result.Add(LoadObject<UObject>(nullptr, Path));
		}
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
			BoardSlot->SetPadding(FMargin(0.f, 0.f, 24.f, 0.f));
			BoardSlot->SetVerticalAlignment(VAlign_Top);
		}
	};
	AddBoard(MakeBoard(Example, Example.SolutionColumns(), INDEX_NONE, IconSets, LOCTEXT("Fits", "Works"), RulesSage));
	TArray<int32> Broken;
	int32 Moved = INDEX_NONE;
	if (Example.FindBroken(Broken, Moved))
	{
		AddBoard(MakeBoard(Example, Broken, Moved, IconSets, LOCTEXT("Breaks", "Does not work"), FLinearColor(0.34f, 0.025f, 0.015f, 1.f)));
	}
	if (UVerticalBoxSlot* RowSlot = Cast<UVerticalBoxSlot>(IllustrationBox->AddChild(Row)))
	{
		RowSlot->SetHorizontalAlignment(HAlign_Center);
		RowSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 12.f));
	}
}

UWidget* UGameRulesWidget::MakeBoard(const FClueExample& Example, const TArray<int32>& Columns, int32 Highlight,
	const TArray<UObject*>& IconSets, const FText& Caption, const FLinearColor& Color)
{
	UVerticalBox* Board = NewObject<UVerticalBox>(this);
	const UFont* Serif = LoadObject<UFont>(nullptr, TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));

	UTextBlock* Label = NewObject<UTextBlock>(this);
	Label->SetText(Caption);
	Label->SetFont(FSlateFontInfo(Serif, 28));
	Label->SetColorAndOpacity(FSlateColor(Color));
	UHorizontalBox* Heading = NewObject<UHorizontalBox>(this);
	USizeBox* MarkSize = NewObject<USizeBox>(this);
	MarkSize->SetWidthOverride(40.f);
	MarkSize->SetHeightOverride(40.f);
	UBorder* MarkDisc = NewObject<UBorder>(this);
	MarkDisc->SetBrush(MakeRulesSurface(Color, Color, 20.f, 0.f));
	UCanvasPanel* MarkCanvas = NewObject<UCanvasPanel>(this);
	MarkDisc->SetContent(MarkCanvas);
	auto AddStroke = [this, MarkCanvas](FVector2D Position, FVector2D Size, float Angle)
	{
		UBorder* Stroke = NewObject<UBorder>(this);
		Stroke->SetBrushColor(RulesCream);
		if (UCanvasPanelSlot* Slot = MarkCanvas->AddChildToCanvas(Stroke))
		{
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
		}
		Stroke->SetRenderTransformPivot(FVector2D(0.5f, 0.f));
		Stroke->SetRenderTransformAngle(Angle);
	};
	if (Highlight == INDEX_NONE)
	{
		AddStroke(FVector2D(11.f, 17.f), FVector2D(6.f, 12.f), -45.f);
		AddStroke(FVector2D(29.f, 8.f), FVector2D(6.f, 24.f), 37.f);
	}
	else
	{
		AddStroke(FVector2D(12.f, 10.f), FVector2D(6.f, 25.f), -45.f);
		AddStroke(FVector2D(29.f, 10.f), FVector2D(6.f, 25.f), 45.f);
	}
	MarkDisc->SetHorizontalAlignment(HAlign_Center);
	MarkDisc->SetVerticalAlignment(VAlign_Center);
	MarkSize->SetContent(MarkDisc);
	Heading->AddChildToHorizontalBox(MarkSize);
	if (UHorizontalBoxSlot* LabelSlot = Heading->AddChildToHorizontalBox(Label))
	{
		LabelSlot->SetPadding(FMargin(14.f, 0.f, 0.f, 0.f));
		LabelSlot->SetVerticalAlignment(VAlign_Center);
	}
	if (UVerticalBoxSlot* HeadingSlot = Board->AddChildToVerticalBox(Heading))
	{
		HeadingSlot->SetPadding(FMargin(10.f, 4.f, 0.f, 12.f));
	}

	// The clue's rows (in the clue's order), every column
	TArray<int32> Rows;
	for (const FClueExamplePiece& Piece : Example.Pieces)
	{
		Rows.AddUnique(Piece.Row);
	}

	UBorder* Frame = NewObject<UBorder>(this);
	Frame->SetBrush(MakeRulesSurface(FLinearColor(1.f, 0.98f, 0.90f, 1.f), Color, 8.f, 2.f));
	Frame->SetPadding(FMargin(7.f));
	UUniformGridPanel* Grid = NewObject<UUniformGridPanel>(this);
	Grid->SetSlotPadding(FMargin(3.f));
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
			CellSize->SetWidthOverride(FMath::Max(BoardCellSize, 78.f));
			CellSize->SetHeightOverride(FMath::Max(BoardCellSize, 78.f));
			UBorder* Cell = NewObject<UBorder>(this);
			Cell->SetBrush(MakeRulesSurface(
				PieceHere != INDEX_NONE && PieceHere == Highlight ? FLinearColor(1.f, 0.84f, 0.78f, 1.f) : RulesCream,
				FLinearColor(0.70f, 0.62f, 0.45f, 1.f), 3.f, 1.f));
			Cell->SetPadding(FMargin(3.f));
			if (PieceHere != INDEX_NONE)
			{
				const FClueExamplePiece& Piece = Example.Pieces[PieceHere];
				UTexture2D* Texture = IconSets.IsValidIndex(Piece.Row) ? GetRulesIconTexture(IconSets[Piece.Row], Piece.Icon) : nullptr;
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
	if (UVerticalBoxSlot* GridSlot = Board->AddChildToVerticalBox(Frame))
	{
		GridSlot->SetHorizontalAlignment(HAlign_Center);
	}
	UTextBlock* Note = NewObject<UTextBlock>(this);
	Note->SetText(Highlight == INDEX_NONE
		? LOCTEXT("ExampleWorksNote", "These positions follow the clue.")
		: LOCTEXT("ExampleBreaksNote", "These positions contradict the clue."));
	Note->SetFont(FSlateFontInfo(Serif, 18));
	Note->SetColorAndOpacity(FSlateColor(RulesInk));
	Note->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* NoteSlot = Board->AddChildToVerticalBox(Note))
	{
		NoteSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}
	USizeBox* CardSize = NewObject<USizeBox>(this);
	CardSize->SetWidthOverride(540.f);
	CardSize->SetHeightOverride(140.f + Rows.Num() * FMath::Max(BoardCellSize, 78.f));
	UBorder* Card = NewObject<UBorder>(this);
	Card->SetBrush(MakeRulesSurface(Highlight == INDEX_NONE
		? FLinearColor(0.94f, 0.97f, 0.87f, 1.f)
		: FLinearColor(1.f, 0.90f, 0.86f, 1.f), Color, 20.f, 2.f));
	Card->SetPadding(FMargin(20.f, 12.f));
	Card->SetContent(Board);
	CardSize->SetContent(Card);
	return CardSize;
}

void UGameRulesWidget::AddPicture(const FRulesSubject& Subject)
{
	USizeBox* PictureSize = NewObject<USizeBox>(this);
	PictureSize->SetWidthOverride(760.f);
	PictureSize->SetHeightOverride(300.f);
	// Controls: the cell dialog itself (touch), scaled to the picture's width, or a grid cell (mouse), enlarged to
	// the picture's height
	const bool bTouchMock = Subject.bControls && IsTouchInput();
	UWidget* ControlsMock = !Subject.bControls ? nullptr : bTouchMock ? MakeCellDialogMock() : MakeGridCellMock();
	if (ControlsMock)
	{
		const FVector2D MockSize = bTouchMock ? CellDialogSize : GridCellSize;
		if (bTouchMock)
		{
			PictureSize->SetHeightOverride(760.f * MockSize.Y / MockSize.X);
		}
		USizeBox* MockBox = NewObject<USizeBox>(this);
		MockBox->SetWidthOverride(MockSize.X);
		MockBox->SetHeightOverride(MockSize.Y);
		MockBox->SetContent(ControlsMock);
		UScaleBox* MockFit = NewObject<UScaleBox>(this);
		MockFit->SetStretch(EStretch::ScaleToFit);
		MockFit->AddChild(MockBox);
		PictureSize->SetContent(MockFit);
	}
	else if (UTexture2D* Picture = Pictures.FindRef(Subject.Title.ToString()))
	{
		UImage* Image = NewObject<UImage>(this);
		Image->SetBrushFromTexture(Picture, false);
		PictureSize->SetContent(Image);
	}
	else
	{
		const UFont* Serif = LoadObject<UFont>(nullptr, TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
		auto MakeTile = [this, Serif](int32 Row, int32 IconIndex, bool bSelected, bool bEliminated) -> UWidget*
		{
			USizeBox* Size = NewObject<USizeBox>(this);
			Size->SetWidthOverride(74.f);
			Size->SetHeightOverride(74.f);
			UBorder* Tile = NewObject<UBorder>(this);
			Tile->SetBrush(MakeRulesSurface(RulesCream, bEliminated ? FLinearColor(0.48f, 0.11f, 0.08f, 1.f) : bSelected ? RulesSage : RulesGold, 5.f, bSelected ? 3.f : 1.f));
			Tile->SetPadding(FMargin(3.f));
			// Abstract symbols make the row and column rule readable at any icon set.
			{
				UOverlay* Layers = NewObject<UOverlay>(this);
				UTextBlock* Symbol = NewObject<UTextBlock>(this);
				Symbol->SetText(FText::FromString(FString::Chr(TEXT('A') + IconIndex)));
				Symbol->SetFont(FSlateFontInfo(Serif, 35));
				Symbol->SetColorAndOpacity(FSlateColor(Row == 0 ? RulesSage : Row == 1 ? RulesNavy : FLinearColor(0.59f, 0.22f, 0.14f, 1.f)));
				if (UOverlaySlot* SymbolSlot = Layers->AddChildToOverlay(Symbol))
				{
					SymbolSlot->SetHorizontalAlignment(HAlign_Center);
					SymbolSlot->SetVerticalAlignment(VAlign_Center);
				}
				if (bEliminated)
				{
					UTextBlock* Cross = NewObject<UTextBlock>(this);
					Cross->SetText(FText::FromString(TEXT("×")));
					Cross->SetFont(FSlateFontInfo(Serif, 47));
					Cross->SetColorAndOpacity(FSlateColor(FLinearColor(0.50f, 0.07f, 0.05f, 1.f)));
					if (UOverlaySlot* CrossSlot = Layers->AddChildToOverlay(Cross))
					{
						CrossSlot->SetHorizontalAlignment(HAlign_Center);
						CrossSlot->SetVerticalAlignment(VAlign_Center);
					}
				}
				Tile->SetContent(Layers);
			}
			Size->SetContent(Tile);
			return Size;
		};

		UBorder* Frame = NewObject<UBorder>(this);
		Frame->SetBrush(MakeRulesSurface(FLinearColor(1.f, 0.98f, 0.91f, 1.f), RulesGold));
		Frame->SetPadding(FMargin(14.f));
		UHorizontalBox* Content = NewObject<UHorizontalBox>(this);
		if (Subject.BasicIndex == 0)
		{
			UUniformGridPanel* Grid = NewObject<UUniformGridPanel>(this);
			Grid->SetSlotPadding(FMargin(3.f));
			for (int32 Row = 0; Row < 3; ++Row)
			{
				for (int32 Column = 0; Column < 4; ++Column)
				{
					Grid->AddChildToUniformGrid(MakeTile(Row, (Column + Row) % 4, Column == 1, false), Row, Column);
				}
			}
			Content->AddChildToHorizontalBox(Grid);
			UTextBlock* Note = NewObject<UTextBlock>(this);
			Note->SetText(LOCTEXT("ObjectiveVisual", "Each row uses every icon once.\n\nA column has one icon from each row."));
			Note->SetFont(FSlateFontInfo(Serif, 21));
			Note->SetColorAndOpacity(FSlateColor(RulesInk));
			Note->SetAutoWrapText(true);
			if (UHorizontalBoxSlot* NoteSlot = Content->AddChildToHorizontalBox(Note))
			{
				NoteSlot->SetPadding(FMargin(26.f, 0.f, 0.f, 0.f));
				NoteSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				NoteSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
		else
		{
			// Controls: what does each, for the player's input
			const bool bTouch = IsTouchInput();
			const FText Captions[3] = {
				Subject.bControls ? (bTouch ? LOCTEXT("ControlsTouchOpen", "Tap a cell") : LOCTEXT("ControlsMouseRestore", "Middle-click: restore"))
					: Subject.BasicIndex == 3 ? LOCTEXT("ClueSelect", "Select a clue") : LOCTEXT("HintAsk", "Ask for a hint"),
				Subject.bControls ? (bTouch ? LOCTEXT("ControlsTouchEliminate", "Select an icon, Eliminate") : LOCTEXT("ControlsMouseEliminate", "Right-click: eliminate"))
					: Subject.BasicIndex == 3 ? LOCTEXT("ClueRead", "Read its meaning") : LOCTEXT("HintLook", "See the highlight"),
				Subject.bControls ? (bTouch ? LOCTEXT("ControlsTouchPlace", "Select an icon, Confirm") : LOCTEXT("ControlsMousePlace", "Left-click: place"))
					: Subject.BasicIndex == 3 ? LOCTEXT("ClueHide", "Hide when done") : LOCTEXT("HintDeduce", "Make the deduction")
			};
			for (int32 Step = 0; Step < 3; ++Step)
			{
				UVerticalBox* Card = NewObject<UVerticalBox>(this);
				if (UVerticalBoxSlot* TileSlot = Card->AddChildToVerticalBox(MakeTile(Step % 3, Step, Step == 2, Subject.BasicIndex == 2 && Step == 1)))
				{
					TileSlot->SetHorizontalAlignment(HAlign_Center);
					TileSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 12.f));
				}
				UTextBlock* Label = NewObject<UTextBlock>(this);
				Label->SetText(Captions[Step]);
				Label->SetFont(FSlateFontInfo(Serif, 20));
				Label->SetColorAndOpacity(FSlateColor(RulesInk));
				Label->SetJustification(ETextJustify::Center);
				Label->SetAutoWrapText(true);
				Card->AddChildToVerticalBox(Label);
				USizeBox* CardSize = NewObject<USizeBox>(this);
				CardSize->SetWidthOverride(220.f);
				CardSize->SetContent(Card);
				if (UHorizontalBoxSlot* CardSlot = Content->AddChildToHorizontalBox(CardSize))
				{
					CardSlot->SetPadding(FMargin(6.f));
				}
			}
		}
		Frame->SetContent(Content);
		PictureSize->SetContent(Frame);
	}
	if (UVerticalBoxSlot* PictureSlot = Cast<UVerticalBoxSlot>(IllustrationBox->AddChild(PictureSize)))
	{
		PictureSlot->SetHorizontalAlignment(HAlign_Center);
		PictureSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 12.f));
	}
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
		{ TEXT("GamePanel"), LOCTEXT("AreaPuzzle", "Puzzle Area"), FLinearColor(0.37f, 0.57f, 0.59f), FBox2D(FVector2D(0.061, 0.056), FVector2D(0.875, 0.722)) },
		{ TEXT("ButtonPanel"), LOCTEXT("AreaButtons", "Buttons"), FLinearColor(0.72f, 0.55f, 0.30f), FBox2D(FVector2D(0.0, 0.0), FVector2D(0.061, 0.743)) },
		{ TEXT("VerticalCluePanel"), LOCTEXT("AreaVertical", "Vertical Clues"), FLinearColor(0.43f, 0.62f, 0.43f), FBox2D(FVector2D(0.006, 0.725), FVector2D(0.872, 0.994)) },
		{ TEXT("HorizontalCluePanel"), LOCTEXT("AreaHorizontal", "Horizontal Clues"), FLinearColor(0.59f, 0.52f, 0.65f), FBox2D(FVector2D(0.875, 0.007), FVector2D(1.0, 1.0)) },
		{ TEXT("HelpPanel"), LOCTEXT("AreaInfo", "Clue Info"), FLinearColor(0.77f, 0.67f, 0.43f), FBox2D(FVector2D(0.061, 0.007), FVector2D(0.875, 0.049)) },
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
	const float DiagramHeight = FMath::Min(LayoutDiagramSize.Y, 225.f);
	DiagramSize->SetHeightOverride(DiagramHeight);
	// The canvas inside the frame's padding: what the areas' fractions are of
	const double CanvasHeight = DiagramHeight - 10.0;
	UBorder* Frame = NewObject<UBorder>(this);
	Frame->SetBrush(MakeRulesSurface(FLinearColor(1.f, 0.98f, 0.90f, 1.f), RulesGold));
	Frame->SetPadding(FMargin(5.f));
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
		const double MinHeight = 24.0 / CanvasHeight;
		if (Rect.Max.Y - Rect.Min.Y < MinHeight)
		{
			Rect.Max.Y = Rect.Min.Y + MinHeight;
		}
		Rects.Add(Rect);
	}
	// The puzzle area (first) starts below the clue info bar (last), which may have grown into it
	Rects[0].Min.Y = FMath::Max(Rects[0].Min.Y, Rects.Last().Max.Y + 2.0 / CanvasHeight);
	// The vertical clues span from the buttons' left edge to the puzzle area's right edge
	Rects[2].Min.X = Rects[1].Min.X;
	Rects[2].Max.X = Rects[0].Max.X;

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Areas); Index++)
	{
		const FArea& Area = Areas[Index];
		const FBox2D& Rect = Rects[Index];
		UBorder* Box = NewObject<UBorder>(this);
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Area.Color.CopyWithNewOpacity(0.36f));
		Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(4.f, 4.f, 4.f, 4.f), FSlateColor(Area.Color), 2.f);
		Box->SetBrush(Brush);
		Box->SetHorizontalAlignment(HAlign_Center);
		Box->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Label = NewObject<UTextBlock>(this);
		Label->SetText(Area.Label);
		FSlateFontInfo LabelFont = EntryFont;
		LabelFont.Size = 16;
		Label->SetFont(LabelFont);
		Label->SetColorAndOpacity(FSlateColor(RulesInk));
		Label->SetJustification(ETextJustify::Center);
		const bool bButtons = FCString::Strcmp(Area.Widget, TEXT("ButtonPanel")) == 0;
		const bool bInfo = FCString::Strcmp(Area.Widget, TEXT("HelpPanel")) == 0;
		if (bButtons)
		{
			// Turned to read upward along the narrow strip: on a canvas, so it keeps its full length while turned
			UCanvasPanel* Turn = NewObject<UCanvasPanel>(this);
			if (UCanvasPanelSlot* LabelSlot = Turn->AddChildToCanvas(Label))
			{
				LabelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
				LabelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				LabelSlot->SetAutoSize(true);
			}
			Label->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			Label->SetRenderTransformAngle(-90.f);
			Box->SetPadding(FMargin(0.f));
			Box->SetHorizontalAlignment(HAlign_Fill);
			Box->SetVerticalAlignment(VAlign_Fill);
			Box->SetContent(Turn);
		}
		else if (bInfo)
		{
			// The thin clue info bar: one line, shrunk to fit inside it
			UScaleBox* Fit = NewObject<UScaleBox>(this);
			Fit->SetStretch(EStretch::ScaleToFit);
			Fit->SetStretchDirection(EStretchDirection::DownOnly);
			Fit->SetContent(Label);
			Box->SetPadding(FMargin(4.f, 1.f));
			Box->SetContent(Fit);
		}
		else
		{
			Label->SetAutoWrapText(true);
			Box->SetContent(Label);
		}

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
	const UFont* Serif = LoadObject<UFont>(nullptr, TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	struct FLegendButton
	{
		const TCHAR* Widget;	// in WBP_ButtonPanel
		const TCHAR* Name;		// if the panel can't be read
		FText Meaning;
	};
	// Two columns: Pause and Hint on the left, Undo and Hide on the right
	const FLegendButton Buttons[] =
	{
		{ TEXT("Button_P"), TEXT("Pause"), LOCTEXT("ButtonPause", "Pause the puzzle") },
		{ TEXT("Button_H"), TEXT("Hint"), LOCTEXT("ButtonHint", "Find a useful clue and learn why it helps") },
		{ TEXT("Button_U"), TEXT("Undo"), LOCTEXT("ButtonUndo", "Undo your last move") },
		{ TEXT("Button_HC"), TEXT("Hide Clue"), LOCTEXT("ButtonHide", "Hide the selected clue") },
	};
	// Each button as it looks on the puzzle screen: its face and label, copied from a button panel made just for that
	const UUserWidget* Panel = nullptr;
	if (UClass* PanelClass = ButtonPanelClass.LoadSynchronous())
	{
		Panel = CreateWidget<UUserWidget>(this, PanelClass);
	}
	constexpr float PanelButtonSize = 94.f;	// WBP_ButtonPanel's buttons
	constexpr float TileSize = 80.f;

	UVerticalBox* Legend = NewObject<UVerticalBox>(this);
	UTextBlock* Heading = NewObject<UTextBlock>(this);
	Heading->SetText(LOCTEXT("ButtonsHeading", "Buttons"));
	Heading->SetFont(FSlateFontInfo(Serif, 24));
	Heading->SetColorAndOpacity(FSlateColor(RulesSage));
	if (UVerticalBoxSlot* HeadingSlot = Legend->AddChildToVerticalBox(Heading))
	{
		HeadingSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 3.f));
	}
	UHorizontalBox* Columns = NewObject<UHorizontalBox>(this);
	UVerticalBox* Column = nullptr;

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Buttons); ++Index)
	{
		if (Index % 2 == 0)
		{
			Column = NewObject<UVerticalBox>(this);
			if (UHorizontalBoxSlot* ColumnSlot = Columns->AddChildToHorizontalBox(Column))
			{
				ColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				ColumnSlot->SetPadding(FMargin(Index == 0 ? 0.f : 24.f, 0.f, 0.f, 0.f));
			}
		}
		const UButton* Real = Panel ? Cast<UButton>(Panel->GetWidgetFromName(Buttons[Index].Widget)) : nullptr;
		const UTextBlock* RealLabel = Real ? Cast<UTextBlock>(Real->GetChildAt(0)) : nullptr;

		UBorder* ButtonTile = NewObject<UBorder>(this);
		if (Real)
		{
			ButtonTile->SetBrush(Real->GetStyle().Normal);
		}
		else
		{
			ButtonTile->SetBrush(MakeRulesSurface(RulesSage, RulesGold, 8.f, 2.f));
		}
		// The label where the real one sits on the tile (bottom centre, just above the edge), scaled with it
		const UButtonSlot* RealSlot = RealLabel ? Cast<UButtonSlot>(RealLabel->Slot) : nullptr;
		const float Scale = TileSize / PanelButtonSize;
		if (RealSlot)
		{
			const FMargin LabelPadding = RealSlot->GetPadding();
			const FMargin ButtonPadding = Real->GetStyle().NormalPadding;
			ButtonTile->SetPadding(FMargin((LabelPadding.Left + ButtonPadding.Left) * Scale, (LabelPadding.Top + ButtonPadding.Top) * Scale,
				(LabelPadding.Right + ButtonPadding.Right) * Scale, (LabelPadding.Bottom + ButtonPadding.Bottom) * Scale));
			ButtonTile->SetHorizontalAlignment(RealSlot->GetHorizontalAlignment());
			ButtonTile->SetVerticalAlignment(RealSlot->GetVerticalAlignment());
		}
		else
		{
			ButtonTile->SetPadding(FMargin(4.f));
			ButtonTile->SetHorizontalAlignment(HAlign_Center);
			ButtonTile->SetVerticalAlignment(VAlign_Center);
		}
		UTextBlock* ButtonName = NewObject<UTextBlock>(this);
		if (RealLabel)
		{
			// Its own text and font, scaled with the tile
			ButtonName->SetText(RealLabel->GetText());
			FSlateFontInfo Font = RealLabel->GetFont();
			Font.Size = FMath::Max(1.f, FMath::RoundToFloat(Font.Size * Scale));
			ButtonName->SetFont(Font);
			ButtonName->SetColorAndOpacity(RealLabel->GetColorAndOpacity());
		}
		else
		{
			ButtonName->SetText(FText::FromString(Buttons[Index].Name));
			ButtonName->SetFont(FSlateFontInfo(Serif, 14));
			ButtonName->SetColorAndOpacity(FSlateColor(RulesCream));
		}
		ButtonName->SetJustification(ETextJustify::Center);
		ButtonTile->SetContent(ButtonName);
		USizeBox* ButtonSize = NewObject<USizeBox>(this);
		ButtonSize->SetWidthOverride(TileSize);
		ButtonSize->SetHeightOverride(TileSize);
		ButtonSize->SetContent(ButtonTile);
		UTextBlock* Description = NewObject<UTextBlock>(this);
		Description->SetText(Buttons[Index].Meaning);
		Description->SetFont(FSlateFontInfo(Serif, 19));
		Description->SetColorAndOpacity(FSlateColor(RulesInk));
		Description->SetAutoWrapText(true);

		UHorizontalBox* Line = NewObject<UHorizontalBox>(this);
		if (UHorizontalBoxSlot* TileSlot = Line->AddChildToHorizontalBox(ButtonSize))
		{
			TileSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UHorizontalBoxSlot* TextSlot = Line->AddChildToHorizontalBox(Description))
		{
			TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			TextSlot->SetPadding(FMargin(14.f, 0.f, 0.f, 0.f));
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UVerticalBoxSlot* LineSlot = Column->AddChildToVerticalBox(Line))
		{
			LineSlot->SetPadding(FMargin(0.f, 4.f));
		}
	}
	Legend->AddChildToVerticalBox(Columns);
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

UWidget* UGameRulesWidget::MakeExample(ECampaignLesson Lesson)
{
	UPuzzle* Puzzle = nullptr;
	UClue* Clue = nullptr;
	if (!FindExampleClue(Lesson, Puzzle, Clue))
	{
		return nullptr;
	}

	// The current player's icon sets, or sample sets when rendering this screen without a player.
	const TArray<UObject*> IconSets = GetIconSets();
	if (IconSets.Num() < Puzzle->m_iSize)
	{
		return nullptr;
	}
	if (!PC)
	{
		const TCHAR* PreviewRelation = TEXT("-");
		switch (Lesson)
		{
		case ECampaignLesson::NextTo:
		case ECampaignLesson::Gap:
		case ECampaignLesson::Span:
		case ECampaignLesson::Between:
			PreviewRelation = TEXT("<->");
			break;
		case ECampaignLesson::DirectlyLeftOf:
		case ECampaignLesson::LeftOf:
		case ECampaignLesson::Chain:
			PreviewRelation = TEXT("->");
			break;
		default:
			break;
		}
		FClueExample Sample;
		if (!Sample.Build(*Clue, *Puzzle))
		{
			return nullptr;
		}
		UHorizontalBox* Strip = NewObject<UHorizontalBox>(this);
		const UFont* Serif = LoadObject<UFont>(nullptr, TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
		for (int32 PieceIndex = 0; PieceIndex < Sample.Pieces.Num(); ++PieceIndex)
		{
			const FClueExamplePiece& Piece = Sample.Pieces[PieceIndex];
			UTexture2D* Texture = IconSets.IsValidIndex(Piece.Row) ? GetRulesIconTexture(IconSets[Piece.Row], Piece.Icon) : nullptr;
			USizeBox* TileSize = NewObject<USizeBox>(this);
			TileSize->SetWidthOverride(102.f);
			TileSize->SetHeightOverride(102.f);
			UBorder* TileFrame = NewObject<UBorder>(this);
			TileFrame->SetBrush(MakeRulesSurface(RulesCream, RulesGold, 8.f, 2.f));
			TileFrame->SetPadding(FMargin(5.f));
			if (Texture)
			{
				UImage* TileArt = NewObject<UImage>(this);
				TileArt->SetBrushFromTexture(Texture, false);
				TileFrame->SetContent(TileArt);
			}
			TileSize->SetContent(TileFrame);
			if (UHorizontalBoxSlot* TileSlot = Strip->AddChildToHorizontalBox(TileSize))
			{
				TileSlot->SetVerticalAlignment(VAlign_Center);
			}
			if (PieceIndex + 1 < Sample.Pieces.Num())
			{
				UTextBlock* Relation = NewObject<UTextBlock>(this);
				Relation->SetText(FText::FromString(Clue->m_Type == eClueType::Vertical ? TEXT("+") : PreviewRelation));
				Relation->SetFont(FSlateFontInfo(Serif, 44));
				Relation->SetColorAndOpacity(FSlateColor(RulesSage));
				if (UHorizontalBoxSlot* RelationSlot = Strip->AddChildToHorizontalBox(Relation))
				{
					RelationSlot->SetPadding(FMargin(18.f, 0.f));
					RelationSlot->SetVerticalAlignment(VAlign_Center);
				}
			}
		}
		return Strip;
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
				Element->SetObjectPropertyValue(Sets.GetRawPtr(Set), IconSets[Set]);
			}
		}
	}
	// The clue's widget tree must exist before Populate assigns its icon images.
	Widget->TakeWidget();
	Widget->ProcessEvent(Populate, Params);
	for (TFieldIterator<FProperty> It(Populate); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		It->DestroyValue_InContainer(Params);
	}
	return Widget;
}

namespace
{
	/** Calls a Blueprint function on Target, Fill giving each parameter its value (left at its default otherwise) */
	bool CallBlueprintFunction(UObject* Target, const TCHAR* Name, TFunctionRef<void(FProperty*, uint8*)> Fill)
	{
		UFunction* Function = Target ? Target->FindFunction(Name) : nullptr;
		if (!Function)
		{
			return false;
		}
		uint8* Params = static_cast<uint8*>(FMemory_Alloca(Function->ParmsSize));
		FMemory::Memzero(Params, Function->ParmsSize);
		for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			It->InitializeValue_InContainer(Params);
			Fill(*It, Params);
		}
		Target->ProcessEvent(Function, Params);
		for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			It->DestroyValue_InContainer(Params);
		}
		return true;
	}

	void SetCellParam(FProperty* Property, uint8* Params, const FPuzzleCell& Cell)
	{
		if (const FStructProperty* Struct = CastField<FStructProperty>(Property); Struct && Struct->Struct == FPuzzleCell::StaticStruct())
		{
			*Struct->ContainerPtrToValuePtr<FPuzzleCell>(Params) = Cell;
		}
	}
}

UWidget* UGameRulesWidget::MakeCellDialogMock()
{
	// Always the Balls, whatever the player's sets
	UObject* IconSet = LoadObject<UObject>(nullptr, TEXT("/Game/Happiness/IconSets/IS_Balls.IS_Balls"));
	UClass* DialogClass = CellDialogClass.LoadSynchronous();
	UUserWidget* Dialog = DialogClass && IconSet ? CreateWidget<UUserWidget>(this, DialogClass) : nullptr;
	if (!Dialog)
	{
		return nullptr;
	}
	// A six-icon cell: two ruled out, the rest still possible
	constexpr int32 IconCount = 6;
	constexpr int32 SelectedIcon = 3;
	FPuzzleCell Cell;
	Cell.m_bValues = { true, false, true, true, false, true };
	// Its widget tree must exist before Populate fills its cell
	Dialog->TakeWidget();
	// Populate(PuzzleSize, IconSet, Row, Col, Puzzle, Cell), as the puzzle screen opens it; no puzzle, so no
	// wrong-icon marks
	const bool bPopulated = CallBlueprintFunction(Dialog, TEXT("Populate"), [&](FProperty* Property, uint8* Params)
	{
		if (const FIntProperty* Int = CastField<FIntProperty>(Property); Int && Property->GetFName() == TEXT("PuzzleSize"))
		{
			Int->SetPropertyValue_InContainer(Params, IconCount);
		}
		else if (const FObjectPropertyBase* Object = CastField<FObjectPropertyBase>(Property);
			Object && Object->PropertyClass && IconSet->IsA(Object->PropertyClass) && !Object->PropertyClass->IsChildOf(UPuzzle::StaticClass()))
		{
			Object->SetObjectPropertyValue_InContainer(Params, IconSet);
		}
		SetCellParam(Property, Params, Cell);
	});
	if (!bPopulated)
	{
		return nullptr;
	}
	// Select one of the possible icons, as a tap on it does: its highlight, Eliminate and Confirm ready
	const FObjectPropertyBase* CellProperty = FindFProperty<FObjectPropertyBase>(Dialog->GetClass(), TEXT("Cell"));
	UObject* CellWidget = CellProperty ? CellProperty->GetObjectPropertyValue_InContainer(Dialog) : nullptr;
	const FArrayProperty* IconsProperty = CellWidget ? FindFProperty<FArrayProperty>(CellWidget->GetClass(), TEXT("AllIcons")) : nullptr;
	const FObjectPropertyBase* IconElement = IconsProperty ? CastField<FObjectPropertyBase>(IconsProperty->Inner) : nullptr;
	if (IconElement)
	{
		FScriptArrayHelper Icons(IconsProperty, IconsProperty->ContainerPtrToValuePtr<void>(CellWidget));
		if (UObject* Icon = Icons.IsValidIndex(SelectedIcon) ? IconElement->GetObjectPropertyValue(Icons.GetRawPtr(SelectedIcon)) : nullptr)
		{
			CallBlueprintFunction(Dialog, TEXT("IconPress"), [&](FProperty* Property, uint8* Params)
			{
				if (const FObjectPropertyBase* Object = CastField<FObjectPropertyBase>(Property); Object && Object->PropertyClass && Icon->IsA(Object->PropertyClass))
				{
					Object->SetObjectPropertyValue_InContainer(Params, Icon);
				}
				SetCellParam(Property, Params, Cell);
			});
		}
	}
	// A picture: it takes no taps, and a drag over it scrolls the page
	Dialog->SetVisibility(ESlateVisibility::HitTestInvisible);
	return Dialog;
}

UWidget* UGameRulesWidget::MakeGridCellMock()
{
	UObject* IconSet = LoadObject<UObject>(nullptr, TEXT("/Game/Happiness/IconSets/IS_Balls.IS_Balls"));
	UClass* CellClass = GridCellClass.LoadSynchronous();
	UUserWidget* CellWidget = CellClass && IconSet ? CreateWidget<UUserWidget>(this, CellClass) : nullptr;
	if (!CellWidget)
	{
		return nullptr;
	}
	// The touch page's sample: six Balls, two ruled out
	constexpr int32 IconCount = 6;
	FPuzzleCell Cell;
	Cell.m_bValues = { true, false, true, true, false, true };
	CellWidget->TakeWidget();
	// SetupCell(PuzzleSize, IconSet, CellSize, Row, Col), then ApplyPuzzleCell(PuzzleCell, Puzzle) as the board does;
	// no puzzle, so no wrong-icon marks
	const bool bSetUp = CallBlueprintFunction(CellWidget, TEXT("SetupCell"), [&](FProperty* Property, uint8* Params)
	{
		if (const FIntProperty* Int = CastField<FIntProperty>(Property); Int && Property->GetFName() == TEXT("PuzzleSize"))
		{
			Int->SetPropertyValue_InContainer(Params, IconCount);
		}
		else if (const FObjectPropertyBase* Object = CastField<FObjectPropertyBase>(Property);
			Object && Object->PropertyClass && IconSet->IsA(Object->PropertyClass))
		{
			Object->SetObjectPropertyValue_InContainer(Params, IconSet);
		}
		else if (const FStructProperty* Struct = CastField<FStructProperty>(Property); Struct && Struct->Struct == TBaseStructure<FVector2D>::Get())
		{
			*Struct->ContainerPtrToValuePtr<FVector2D>(Params) = GridCellSize;
		}
	});
	if (!bSetUp || !CallBlueprintFunction(CellWidget, TEXT("ApplyPuzzleCell"), [&](FProperty* Property, uint8* Params)
	{
		SetCellParam(Property, Params, Cell);
	}))
	{
		return nullptr;
	}
	CellWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	return CellWidget;
}

#undef LOCTEXT_NAMESPACE
