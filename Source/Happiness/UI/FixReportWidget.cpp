#include "UI/FixReportWidget.h"

#include "HappinessClassic/Clue.h"
#include "HappinessClassic/Hint.h"
#include "HappinessClassic/IconSets.h"
#include "HappinessClassic/Puzzle.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScaleBox.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "FixReport"

namespace
{
	const FLinearColor ReportInk(.10f, .05f, .02f);
	const FLinearColor ReportSage(.16f, .29f, .075f);
	// The panel's width, and its text's (the panel less its padding)
	constexpr float ReportWidth = 980.f;
	constexpr float ReportPadding = 56.f;

	/** The puzzle screen's (WBP_Happiness) icon sets, one per row (DA_IconSet) */
	TArray<UObject*> ReportIconSets(const UObject* Screen)
	{
		TArray<UObject*> Sets;
		const FArrayProperty* Property = Screen ? FindFProperty<FArrayProperty>(Screen->GetClass(), TEXT("IconSets")) : nullptr;
		const FObjectPropertyBase* Inner = Property ? CastField<FObjectPropertyBase>(Property->Inner) : nullptr;
		if (!Inner)
		{
			return Sets;
		}
		FScriptArrayHelper Array(Property, Property->ContainerPtrToValuePtr<void>(Screen));
		for (int32 Index = 0; Index < Array.Num(); ++Index)
		{
			Sets.Add(Inner->GetObjectPropertyValue(Array.GetRawPtr(Index)));
		}
		return Sets;
	}

	/**
	 * A mistake's reason speaks of "this cell" and "here", as the board's hint highlights the cell; the report names
	 * the column instead
	 */
	FClueHelp ReportInColumn(const FClueHelp& Help, int32 Col)
	{
		const FString Column = FString::Printf(TEXT("column %d"), Col + 1);
		FClueHelp Named = Help;
		for (FClueHelpSegment& Segment : Named.Segments)
		{
			if (Segment.Type != EClueHelpSegementType::Text)
			{
				continue;
			}
			Segment.Text.ReplaceInline(TEXT("this cell"), *Column);
			Segment.Text.ReplaceInline(TEXT("this column"), *Column);
			Segment.Text.ReplaceInline(TEXT("be here"), *(TEXT("be in ") + Column));
			Segment.Text.ReplaceInline(TEXT("go here"), *(TEXT("go in ") + Column));
			Segment.Text.ReplaceInline(TEXT("goes here"), *(TEXT("goes in ") + Column));
		}
		return Named;
	}
}

UFixReportWidget::UFixReportWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> Panel(TEXT("/Game/Happiness/UI/Textures/T_HintReadingPanel.T_HintReadingPanel"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Buttons(TEXT("/Game/General/Textures/T_LessonNotebookButtons.T_LessonNotebookButtons"));
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Accept(TEXT("/Game/General/Audio/Cues/MenuAccept.MenuAccept"));
	PanelTexture = Panel.Object;
	ButtonTexture = Buttons.Object;
	ReportFont = Font.Object;
	ClickSound = Accept.Object;
}

void UFixReportWidget::Open(UUserWidget* Screen, UHint* Mistake, const FText& Message, TFunction<void()> OnClosed)
{
	UFixReportWidget* Report = Screen ? CreateWidget<UFixReportWidget>(Screen, UFixReportWidget::StaticClass()) : nullptr;
	if (!Report)
	{
		if (OnClosed)
		{
			OnClosed();
		}
		return;
	}
	Report->OnClosed = MoveTemp(OnClosed);
	Report->AddToViewport(100);
	Report->Fill(Mistake, Message, ReportIconSets(Screen));
}

TSharedRef<SWidget> UFixReportWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UFixReportWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (!GetGameInstance() && Content && Content->GetChildrenCount() == 0)
	{
		Fill(nullptr, LOCTEXT("SampleMessage", "Undid 4 moves. Puzzle is in a good state again"), {});
		Content->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		FClueHelp What;
		What.AddText(TEXT("You placed [icon] in column 3, but it doesn't go there."));
		Content->AddChildToVerticalBox(MakeLine(What, {}, 30, ReportInk))->SetPadding(FMargin(0, 0, 0, 18));
		FClueHelp Why;
		Why.AddText(TEXT("[icon] is in column 3, so [icon] can't be in column 3."));
		Content->AddChildToVerticalBox(MakeLine(Why, {}, 28, ReportInk));
	}
}

void UFixReportWidget::BuildLayout()
{
	// A clear blocker over the whole screen, with the panel in the middle
	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ClickBlocker"));
	Root->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, .25f));
	Root->SetHorizontalAlignment(HAlign_Center);
	Root->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Root;

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(ReportWidth);
	Root->SetContent(Size);

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	FSlateBrush PanelBrush;
	PanelBrush.SetResourceObject(PanelTexture);
	PanelBrush.DrawAs = ESlateBrushDrawType::Box;
	PanelBrush.Margin = FMargin(.07f, .14f);
	Panel->SetBrush(PanelBrush);
	Panel->SetPadding(FMargin(ReportPadding, 40.f));
	Size->SetContent(Panel);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Column);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>();
	Title->SetText(LOCTEXT("Title", "Where It Went Wrong"));
	Title->SetFont(FSlateFontInfo(ReportFont, 40));
	Title->SetColorAndOpacity(ReportInk);
	Title->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(Title)->SetPadding(FMargin(0, 0, 0, 24));

	Content = WidgetTree->ConstructWidget<UVerticalBox>();
	Column->AddChildToVerticalBox(Content);

	UImage* Rule = WidgetTree->ConstructWidget<UImage>();
	Rule->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.44f, .28f, .1f, .35f), 0.f));
	Rule->SetDesiredSizeOverride(FVector2D(ReportWidth - 2.f * ReportPadding, 1.f));
	Column->AddChildToVerticalBox(Rule)->SetPadding(FMargin(0, 26, 0, 20));

	MessageText = WidgetTree->ConstructWidget<UTextBlock>();
	MessageText->SetFont(FSlateFontInfo(ReportFont, 30));
	MessageText->SetColorAndOpacity(ReportSage);
	MessageText->SetJustification(ETextJustify::Center);
	MessageText->SetAutoWrapText(true);
	Column->AddChildToVerticalBox(MessageText)->SetPadding(FMargin(0, 0, 0, 26));

	OkButton = WidgetTree->ConstructWidget<UButton>();
	FSlateBrush Face;
	Face.SetResourceObject(ButtonTexture);
	Face.DrawAs = ESlateBrushDrawType::Image;
	Face.SetUVRegion(FBox2f(FVector2f(.03f, .06f), FVector2f(.98f, .485f)));
	FButtonStyle Style;
	Style.Normal = Style.Hovered = Style.Pressed = Style.Disabled = Face;
	Style.Hovered.TintColor = FLinearColor(1.08f, 1.08f, 1.04f);
	Style.Pressed.TintColor = FLinearColor(.83f, .88f, .78f);
	Style.NormalPadding = Style.PressedPadding = FMargin(0.f);
	OkButton->SetStyle(Style);
	OkButton->SetCursor(EMouseCursor::Hand);
	UTextBlock* Caption = WidgetTree->ConstructWidget<UTextBlock>();
	Caption->SetText(LOCTEXT("Ok", "OK"));
	Caption->SetFont(FSlateFontInfo(ReportFont, 30));
	Caption->SetColorAndOpacity(ReportInk);
	UButtonSlot* CaptionSlot = CastChecked<UButtonSlot>(OkButton->AddChild(Caption));
	CaptionSlot->SetHorizontalAlignment(HAlign_Center);
	CaptionSlot->SetVerticalAlignment(VAlign_Center);
	USizeBox* OkSize = WidgetTree->ConstructWidget<USizeBox>();
	OkSize->SetWidthOverride(220.f);
	OkSize->SetHeightOverride(62.f);
	OkSize->SetContent(OkButton);
	Column->AddChildToVerticalBox(OkSize)->SetHorizontalAlignment(HAlign_Center);
	OkButton->OnClicked.AddUniqueDynamic(this, &UFixReportWidget::HandleOk);
}

void UFixReportWidget::Fill(UHint* Mistake, const FText& Message, const TArray<UObject*>& IconSets)
{
	if (!Content)
	{
		BuildLayout();
	}
	Content->ClearChildren();
	MessageText->SetText(Message);
	if (!Mistake)
	{
		// Nothing to point at (no mistakes, or none left in the undo list): just the message
		Content->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	Content->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// What the player did: an error hint that brings an icon back was a wrong ruling out, else a wrong placement
	const bool bPlaced = !Mistake->bSetFinalIcon;
	FClueHelp What;
	What.AddText(bPlaced ? TEXT("You placed") : TEXT("You ruled"));
	What.AddIcon(Mistake->Row, Mistake->Icon);
	What.AddText(bPlaced ? FString::Printf(TEXT("in column %d, but it doesn't go there."), Mistake->Col + 1)
		: FString::Printf(TEXT("out of column %d, but that's where it goes."), Mistake->Col + 1));
	Content->AddChildToVerticalBox(MakeLine(What, IconSets, 30, ReportInk))->SetPadding(FMargin(0, 0, 0, 22));

	if (Mistake->bErrorGiven)
	{
		// It broke one of the puzzle's givens
		FClueHelp Given;
		Given.AddText(TEXT("That was a given."));
		Content->AddChildToVerticalBox(MakeLine(Given, IconSets, 28, ReportSage))->SetPadding(FMargin(0, 0, 0, 10));
		Content->AddChildToVerticalBox(MakeLine(ReportInColumn(Mistake->ErrorReason, Mistake->Col), IconSets, 28, ReportInk));
		return;
	}
	if (Mistake->bErrorWasPlaced && (!Mistake->bErrorProvedByClue || !Mistake->TheClue || Mistake->ErrorReason.Segments.Num() == 0))
	{
		// It took off an icon already placed there, and rightly; no single clue shows it from the board
		FClueHelp Placed;
		Placed.AddIcon(Mistake->Row, Mistake->Icon);
		Placed.AddText(FString::Printf(TEXT("was already placed in column %d, and that was right."), Mistake->Col + 1));
		Content->AddChildToVerticalBox(MakeLine(Placed, IconSets, 28, ReportInk));
		return;
	}
	if (!Mistake->bErrorProvedByClue || !Mistake->TheClue || Mistake->ErrorReason.Segments.Num() == 0)
	{
		// No clue showed it from the board the player had: it was a guess
		FClueHelp Guess;
		Guess.AddText(bPlaced ? TEXT("No clue put it there yet, so that was a guess.") : TEXT("No clue ruled it out yet, so that was a guess."));
		Content->AddChildToVerticalBox(MakeLine(Guess, IconSets, 28, ReportInk));
		return;
	}

	// "This clue shows why:" and the clue, drawn as on the board (else as the help line describes it), then why
	UHorizontalBox* ClueRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	UTextBlock* Heading = WidgetTree->ConstructWidget<UTextBlock>();
	Heading->SetText(LOCTEXT("ClueHeading", "This clue shows why:"));
	Heading->SetFont(FSlateFontInfo(ReportFont, 28));
	Heading->SetColorAndOpacity(ReportSage);
	ClueRow->AddChildToHorizontalBox(Heading)->SetVerticalAlignment(VAlign_Center);
	UWidget* Clue = MakeClue(Mistake->TheClue, Mistake->GetTypedOuter<UPuzzle>(), IconSets);
	if (!Clue)
	{
		UBorder* ClueBox = WidgetTree->ConstructWidget<UBorder>();
		ClueBox->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.44f, .28f, .1f, .12f), 14.f));
		ClueBox->SetPadding(FMargin(22, 12));
		ClueBox->SetContent(MakeLine(Mistake->TheClue->ClueHelp, IconSets, 28, ReportInk));
		Clue = ClueBox;
	}
	UHorizontalBoxSlot* ClueSlot = ClueRow->AddChildToHorizontalBox(Clue);
	ClueSlot->SetPadding(FMargin(20, 0, 0, 0));
	ClueSlot->SetVerticalAlignment(VAlign_Center);
	Content->AddChildToVerticalBox(ClueRow)->SetHorizontalAlignment(HAlign_Center);
	UVerticalBoxSlot* ReasonSlot = Content->AddChildToVerticalBox(MakeLine(ReportInColumn(Mistake->ErrorReason, Mistake->Col), IconSets, 28, ReportInk));
	ReasonSlot->SetPadding(FMargin(0, 16, 0, 0));
}

UWidget* UFixReportWidget::MakeClue(UClue* Clue, UPuzzle* Puzzle, const TArray<UObject*>& IconSets)
{
	// The board's clue widgets (cooked: the clue panels create them), filled by their Populate(Clue, Puzzle,
	// IconSets) as the panels do
	if (!Clue || !Puzzle || (Clue->m_Type != eClueType::Vertical && Clue->m_Type != eClueType::Horizontal))
	{
		return nullptr;
	}
	const bool bVertical = Clue->m_Type == eClueType::Vertical;
	TSoftClassPtr<UUserWidget> ClueClass(FSoftObjectPath(bVertical ? TEXT("/Game/Happiness/UI/WBP_VerticalClue.WBP_VerticalClue_C")
		: TEXT("/Game/Happiness/UI/WBP_HorizontalClue.WBP_HorizontalClue_C")));
	UClass* Class = ClueClass.LoadSynchronous();
	UUserWidget* Widget = Class ? CreateWidget<UUserWidget>(this, Class) : nullptr;
	UFunction* Populate = Widget ? Widget->FindFunction(TEXT("Populate")) : nullptr;
	if (!Populate)
	{
		return nullptr;
	}
	uint8* Params = static_cast<uint8*>(FMemory_Alloca(Populate->ParmsSize));
	FMemory::Memzero(Params, Populate->ParmsSize);
	for (TFieldIterator<FProperty> It(Populate); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		It->InitializeValue_InContainer(Params);
		const FString Name = It->GetAuthoredName();
		if (const FObjectPropertyBase* Object = CastField<FObjectPropertyBase>(*It))
		{
			Object->SetObjectPropertyValue_InContainer(Params, Name == TEXT("Clue") ? static_cast<UObject*>(Clue) : Name == TEXT("Puzzle") ? Puzzle : nullptr);
		}
		else if (const FArrayProperty* Array = CastField<FArrayProperty>(*It); Array && Name == TEXT("IconSets"))
		{
			if (const FObjectPropertyBase* Inner = CastField<FObjectPropertyBase>(Array->Inner))
			{
				FScriptArrayHelper Sets(Array, Array->ContainerPtrToValuePtr<void>(Params));
				Sets.Resize(IconSets.Num());
				for (int32 Index = 0; Index < IconSets.Num(); ++Index)
				{
					Inner->SetObjectPropertyValue(Sets.GetRawPtr(Index), IconSets[Index]);
				}
			}
		}
	}
	Widget->ProcessEvent(Populate, Params);
	for (TFieldIterator<FProperty> It(Populate); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		It->DestroyValue_InContainer(Params);
	}

	// Shown, not tapped; scaled down to fit beside the heading
	Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
	UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>();
	Fit->SetStretch(EStretch::ScaleToFit);
	Fit->SetStretchDirection(EStretchDirection::DownOnly);
	Fit->SetContent(Widget);
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetMaxDesiredHeight(bVertical ? 230.f : 100.f);
	Size->SetMaxDesiredWidth(ReportWidth - 2.f * ReportPadding - 380.f);
	Size->SetContent(Fit);
	return Size;
}

UWidget* UFixReportWidget::MakeLine(const FClueHelp& Help, const TArray<UObject*>& IconSets, int32 FontSize, const FLinearColor& Color)
{
	// Word-sized pieces, so the text wraps around the inline icons (as the hint info panel does)
	UWrapBox* Line = WidgetTree->ConstructWidget<UWrapBox>();
	Line->SetHorizontalAlignment(HAlign_Center);
	Line->SetExplicitWrapSize(true);
	Line->SetWrapSize(ReportWidth - 2.f * ReportPadding - 50.f);
	const float IconSize = FontSize * 1.5f;
	for (const FClueHelpSegment& Segment : Help.Segments)
	{
		if (Segment.Type == EClueHelpSegementType::Text)
		{
			TArray<FString> Words;
			Segment.Text.ParseIntoArrayWS(Words);
			for (const FString& Word : Words)
			{
				UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
				Text->SetText(FText::FromString(Word));
				Text->SetFont(FSlateFontInfo(ReportFont, FontSize));
				Text->SetColorAndOpacity(Color);
				UWrapBoxSlot* WordSlot = CastChecked<UWrapBoxSlot>(Line->AddChild(Text));
				const bool bPunctuation = Word.StartsWith(TEXT(",")) || Word.StartsWith(TEXT("."));
				WordSlot->SetPadding(FMargin(bPunctuation ? -6.f : 0.f, 0, 8, 4));
				WordSlot->SetVerticalAlignment(VAlign_Center);
			}
			continue;
		}
		const TArray<UTexture2D*> Icons = IconSets.IsValidIndex(Segment.IconRow) ? UIconSetSubsystem::GetIconTextures(IconSets[Segment.IconRow]) : TArray<UTexture2D*>();
		if (!Icons.IsValidIndex(Segment.IconColumn) || !Icons[Segment.IconColumn])
		{
			continue;
		}
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(IconSize);
		Size->SetHeightOverride(IconSize);
		UImage* Image = WidgetTree->ConstructWidget<UImage>();
		Image->SetBrushFromTexture(Icons[Segment.IconColumn]);
		Size->AddChild(Image);
		UWrapBoxSlot* IconSlot = CastChecked<UWrapBoxSlot>(Line->AddChild(Size));
		IconSlot->SetPadding(FMargin(0, 0, 8, 4));
		IconSlot->SetVerticalAlignment(VAlign_Center);
	}
	return Line;
}

void UFixReportWidget::HandleOk()
{
	if (ClickSound)
	{
		UGameplayStatics::PlaySound2D(this, ClickSound);
	}
	RemoveFromParent();
	if (OnClosed)
	{
		TFunction<void()> Closed = MoveTemp(OnClosed);
		Closed();
	}
}

#undef LOCTEXT_NAMESPACE
