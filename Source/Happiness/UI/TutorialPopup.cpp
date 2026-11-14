#include "UI/TutorialPopup.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/RichTextBlock.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Space between the panel's sides and its text */
	constexpr float TutorialPopupSidePadding = 22.f;
}

UTutorialPopup::UTutorialPopup(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Happiness/UI/Fonts/LessonsSerif_Font.LessonsSerif_Font"));
	TextFont = Font.Object;
	// Only the Next button takes taps
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

TSharedRef<SWidget> UTutorialPopup::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UTutorialPopup::BuildLayout()
{
	Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PopupBox"));
	Box->SetWidthOverride(PendingWidth);
	WidgetTree->RootWidget = Box;

	// The lessons' cream and gold (the lessons-complete cards' colors), a little more opaque over the dim
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrush(FSlateRoundedBoxBrush(FLinearColor(1.f, .97f, .86f, .97f), 26.f, FLinearColor(.78f, .6f, .25f, 1.f), 3.f));
	Panel->SetPadding(FMargin(TutorialPopupSidePadding, 28.f, TutorialPopupSidePadding, 22.f));
	Panel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Box->AddChild(Panel);
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
	Content->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Panel->SetContent(Content);

	const FLinearColor Ink(.17f, .085f, .035f), Ivory(1.f, .96f, .86f);
	Text = WidgetTree->ConstructWidget<URichTextBlock>();
	FTextBlockStyle TextStyle;
	TextStyle.SetFont(FSlateFontInfo(TextFont, FontSize));
	TextStyle.SetColorAndOpacity(FSlateColor(Ink));
	Text->SetDefaultTextStyle(TextStyle);
	Text->SetDecorators({ UTutorialIconDecorator::StaticClass() });
	Text->SetJustification(ETextJustify::Center);
	// A fixed wrap width (the panel's inside): auto wrap can keep wrapping to an older, narrower width
	Text->SetWrapTextAt(PendingWidth - TutorialPopupSidePadding * 2.f);
	Text->SetText(PendingMessage);
	Text->SetVisibility(ESlateVisibility::HitTestInvisible);
	Content->AddChildToVerticalBox(Text);

	// Next (and SecondLabel's and ThirdLabel's buttons, left of it): sage buttons with ivory text (the lessons' green), bottom right.
	// Hidden, not collapsed, until they're due, so the panel doesn't change size when they appear
	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
	Buttons->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UVerticalBoxSlot* ButtonsSlot = Content->AddChildToVerticalBox(Buttons))
	{
		ButtonsSlot->SetHorizontalAlignment(HAlign_Right);
		ButtonsSlot->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
	}
	if (!ThirdLabel.IsEmpty())
	{
		ThirdButton = MakeButton(ThirdLabel);
		ThirdButton->OnClicked.AddUniqueDynamic(this, &UTutorialPopup::HandleThirdClicked);
		if (UHorizontalBoxSlot* ThirdSlot = Buttons->AddChildToHorizontalBox(ThirdButton))
		{
			ThirdSlot->SetPadding(FMargin(0.f, 0.f, 18.f, 0.f));
		}
	}
	if (!SecondLabel.IsEmpty())
	{
		SecondButton = MakeButton(SecondLabel);
		SecondButton->OnClicked.AddUniqueDynamic(this, &UTutorialPopup::HandleSecondClicked);
		if (UHorizontalBoxSlot* SecondSlot = Buttons->AddChildToHorizontalBox(SecondButton))
		{
			SecondSlot->SetPadding(FMargin(0.f, 0.f, 18.f, 0.f));
		}
	}
	NextButton = MakeButton(NextLabel.IsEmpty() ? NSLOCTEXT("Tutorial", "Next", "Next") : NextLabel);
	NextButton->OnClicked.AddUniqueDynamic(this, &UTutorialPopup::HandleNextClicked);
	Buttons->AddChildToHorizontalBox(NextButton);
}

UButton* UTutorialPopup::MakeButton(const FText& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style;
	const FLinearColor Gold(.78f, .6f, .25f, 1.f), Ivory(1.f, .96f, .86f);
	Style.SetNormal(FSlateRoundedBoxBrush(FLinearColor(.30f, .43f, .22f), 18.f, Gold, 2.f));
	Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor(.36f, .50f, .27f), 18.f, Gold, 2.f));
	Style.SetPressed(FSlateRoundedBoxBrush(FLinearColor(.24f, .35f, .17f), 18.f, Gold, 2.f));
	Style.SetDisabled(FSlateRoundedBoxBrush(FLinearColor(.30f, .43f, .22f, .5f), 18.f, Gold, 2.f));
	Style.SetNormalPadding(FMargin(34.f, 8.f));
	Style.SetPressedPadding(FMargin(34.f, 10.f, 34.f, 6.f));
	Button->SetStyle(Style);
	Button->SetVisibility(ESlateVisibility::Hidden);
	UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>();
	ButtonText->SetText(Label);
	ButtonText->SetFont(FSlateFontInfo(TextFont, FontSize - 2));
	ButtonText->SetColorAndOpacity(FSlateColor(Ivory));
	Button->SetContent(ButtonText);
	return Button;
}

void UTutorialPopup::HandleNextClicked()
{
	OnNext.Broadcast();
}

void UTutorialPopup::HandleSecondClicked()
{
	OnSecond.Broadcast();
}

void UTutorialPopup::HandleThirdClicked()
{
	OnThird.Broadcast();
}

void UTutorialPopup::NativeConstruct()
{
	Super::NativeConstruct();
	FadeTime = 0.f;
	SetRenderOpacity(FadeSeconds > 0.f ? 0.f : 1.f);
	for (UButton* Button : { NextButton.Get(), SecondButton.Get(), ThirdButton.Get() })
	{
		if (Button && !bShowNext && !bReserveNext)
		{
			Button->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UTutorialPopup::ShowNextNow()
{
	bShowNext = true;
	// Due from now, and fully shown already (the tick's fade-in is past its end)
	NextDelay = FadeTime - FadeSeconds;
	for (UButton* Button : { NextButton.Get(), SecondButton.Get(), ThirdButton.Get() })
	{
		if (Button)
		{
			Button->SetVisibility(ESlateVisibility::Visible);
			Button->SetRenderOpacity(1.f);
		}
	}
}

void UTutorialPopup::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	FadeTime += InDeltaTime;
	if (FadeSeconds > 0.f && FadeTime - InDeltaTime < FadeSeconds)
	{
		SetRenderOpacity(FMath::SmoothStep(0.f, 1.f, FMath::Clamp(FadeTime / FadeSeconds, 0.f, 1.f)));
	}
	// The Next button (and the second) fades in over the same time, once due
	if (bShowNext && FadeTime >= NextDelay)
	{
		const float Shown = FadeSeconds > 0.f ? FMath::Clamp((FadeTime - NextDelay) / FadeSeconds, 0.f, 1.f) : 1.f;
		for (UButton* Button : { NextButton.Get(), SecondButton.Get(), ThirdButton.Get() })
		{
			if (!Button)
			{
				continue;
			}
			if (!Button->IsVisible())
			{
				Button->SetVisibility(ESlateVisibility::Visible);
			}
			if (Button->GetRenderOpacity() != Shown)
			{
				Button->SetRenderOpacity(FMath::SmoothStep(0.f, 1.f, Shown));
			}
		}
	}
}

void UTutorialPopup::SetMessage(const FText& Message)
{
	PendingMessage = Message;
	if (Text)
	{
		Text->SetText(Message);
	}
}

void UTutorialPopup::SetIcon(FName Id, UTexture2D* Texture)
{
	if (!Texture)
	{
		return;
	}
	IconTextures.AddUnique(Texture);
	TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
	Brush->SetResourceObject(Texture);
	Brush->DrawAs = ESlateBrushDrawType::Image;
	IconBrushes.Add(Id, Brush);
}

const FSlateBrush* UTutorialPopup::FindIcon(FName Id) const
{
	const TSharedPtr<FSlateBrush>* Brush = IconBrushes.Find(Id);
	return Brush ? Brush->Get() : nullptr;
}

namespace
{
	/** <icon id="Name"/>: the owning popup's icon, a square a little taller than the text's font */
	class FTutorialIconDecorator : public FRichTextDecorator
	{
	public:
		explicit FTutorialIconDecorator(URichTextBlock* InOwner) : FRichTextDecorator(InOwner) {}

		virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const override
		{
			return RunParseResult.Name == TEXT("icon") && RunParseResult.MetaData.Contains(TEXT("id"));
		}

	protected:
		virtual TSharedPtr<SWidget> CreateDecoratorWidget(const FTextRunInfo& RunInfo, const FTextBlockStyle& DefaultTextStyle) const override
		{
			const UTutorialPopup* Popup = Owner ? Owner->GetTypedOuter<UTutorialPopup>() : nullptr;
			const FString* Id = RunInfo.MetaData.Find(TEXT("id"));
			const FSlateBrush* Brush = Popup && Id ? Popup->FindIcon(FName(**Id)) : nullptr;
			// Font sizes are points (at 96 dpi a point is 4/3 of a unit): a square a little over the line's height, with
			// a space either side
			const float Size = DefaultTextStyle.Font.Size * 1.9f;
			return SNew(SBox).Padding(FMargin(6.f, 0.f))
				[
					SNew(SBox).WidthOverride(Size).HeightOverride(Size)
					[
						SNew(SImage).Image(Brush)
					]
				];
		}
	};
}

TSharedPtr<ITextDecorator> UTutorialIconDecorator::CreateDecorator(URichTextBlock* InOwner)
{
	return MakeShareable(new FTutorialIconDecorator(InOwner));
}

void UTutorialPopup::SetWidth(float Width)
{
	PendingWidth = Width;
	if (Box && !FMath::IsNearlyEqual(Box->GetWidthOverride(), Width))
	{
		Box->SetWidthOverride(Width);
		if (Text)
		{
			Text->SetWrapTextAt(Width - TutorialPopupSidePadding * 2.f);
		}
	}
}
