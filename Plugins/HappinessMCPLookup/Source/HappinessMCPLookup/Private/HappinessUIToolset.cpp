#include "HappinessUIToolset.h"

#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/GridPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Editor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"
#include "Slate/WidgetRenderer.h"
#include "TextureResource.h"

namespace
{
	// Populate only the render instance, using the real cell and icon widgets without changing any assets.
	bool PopulateBoardPreview(UUserWidget* Widget, int32 Size)
	{
		if (UUserWidget* Screen = Cast<UUserWidget>(Widget->GetWidgetFromName(TEXT("HappinessWidget"))))
		{
			Screen->SetVisibility(ESlateVisibility::Visible);
			if (UWidget* Logo = Widget->GetWidgetFromName(TEXT("SizeBox_0")))
			{
				Logo->SetVisibility(ESlateVisibility::Collapsed);
			}
			return PopulateBoardPreview(Screen, Size);
		}
		if (UUserWidget* Panel = Cast<UUserWidget>(Widget->GetWidgetFromName(TEXT("GamePanel"))))
		{
			return PopulateBoardPreview(Panel, Size);
		}
		UCanvasPanel* Canvas = Cast<UCanvasPanel>(Widget->GetWidgetFromName(TEXT("Border_44")));
		UClass* CellClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/Happiness/UI/WBP_Cell.WBP_Cell_C"));
		UClass* IconClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/Happiness/UI/WBP_Icon.WBP_Icon_C"));
		if (!Canvas || !CellClass || !IconClass)
		{
			return false;
		}
		UGridPanel* Grid = NewObject<UGridPanel>(Widget);
		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Grid);
		CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		CanvasSlot->SetOffsets(FMargin(0.f));
		for (int32 Row = 0; Row < Size; ++Row)
		{
			Grid->SetRowFill(Row, 1.f);
			Grid->SetColumnFill(Row, 1.f);
			for (int32 Column = 0; Column < Size; ++Column)
			{
				UUserWidget* Cell = CreateWidget<UUserWidget>(Widget->GetWorld(), CellClass);
				Grid->AddChildToGrid(Cell, Row, Column);
				UHorizontalBox* Top = Cast<UHorizontalBox>(Cell->GetWidgetFromName(TEXT("TopRowIcons")));
				UHorizontalBox* Bottom = Cast<UHorizontalBox>(Cell->GetWidgetFromName(TEXT("BottomRowIcons")));
				if (!Top || !Bottom)
				{
					return false;
				}
				const TCHAR* Icons[] = {TEXT("Bear"), TEXT("Doll"), TEXT("Ducky"), TEXT("Elephant"), TEXT("Horse"), TEXT("Jacks"), TEXT("Top"), TEXT("Train")};
				for (int32 Index = 0; Index < Size; ++Index)
				{
					UUserWidget* Icon = CreateWidget<UUserWidget>(Widget->GetWorld(), IconClass);
					if (UImage* Image = Cast<UImage>(Icon->GetWidgetFromName(TEXT("TheIcon"))))
					{
						const TCHAR* Name = Icons[(Index + Row) % UE_ARRAY_COUNT(Icons)];
						const FString Path = FString::Printf(TEXT("/Game/Happiness/IconSets/Icons/Toys/%s.%s"), Name, Name);
						Image->SetBrushFromTexture(LoadObject<UTexture2D>(nullptr, *Path), false);
					}
					(Index < FMath::DivideAndRoundUp(Size, 2) ? Top : Bottom)->AddChildToHorizontalBox(Icon);
				}
			}
		}
		return true;
	}
}

FString UHappinessUIToolset::RenderWidget(const FString& WidgetBlueprintPath, int32 Width, int32 Height, const FString& OutputFile, int32 PreviewPuzzleSize)
{
	if (!GEditor)
	{
		return TEXT("Error: editor not available.");
	}

	Width = FMath::Clamp(Width, 64, 7680);
	Height = FMath::Clamp(Height, 64, 4320);

	// "/Game/Path/WBP_Name" -> generated class "/Game/Path/WBP_Name.WBP_Name_C"
	FString PackagePath = WidgetBlueprintPath;
	PackagePath.RemoveFromEnd(TEXT("_C"));
	int32 Dot;
	if (PackagePath.FindChar('.', Dot))
	{
		PackagePath.LeftInline(Dot);
	}
	const FString AssetName = FPaths::GetBaseFilename(PackagePath);
	const FString ClassPath = FString::Printf(TEXT("%s.%s_C"), *PackagePath, *AssetName);

	UClass* WidgetClass = LoadClass<UUserWidget>(nullptr, *ClassPath);
	if (!WidgetClass)
	{
		return FString::Printf(TEXT("Error: could not load widget class '%s'."), *ClassPath);
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	UUserWidget* Widget = World ? CreateWidget<UUserWidget>(World, WidgetClass) : nullptr;
	if (!Widget)
	{
		return TEXT("Error: could not create the widget.");
	}
	if (PreviewPuzzleSize != 0 && !PopulateBoardPreview(Widget, FMath::Clamp(PreviewPuzzleSize, 3, 8)))
	{
		return TEXT("Error: widget does not contain a puzzle board suitable for preview.");
	}

	const FVector2D DrawSize(Width, Height);
	UTextureRenderTarget2D* Target = FWidgetRenderer::CreateTargetFor(DrawSize, TF_Bilinear, true);
	if (!Target)
	{
		return TEXT("Error: could not create a render target.");
	}

	// Draw a few frames: the first lays the widget out, later ones can use cached child geometry
	// (the campaign tree places its connector lines from its nodes' cached geometry)
	TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
	FWidgetRenderer Renderer(true, false);
	for (int32 Pass = 0; Pass < (PreviewPuzzleSize != 0 ? 5 : 3); Pass++)
	{
		Renderer.DrawWidget(Target, SlateWidget, DrawSize, 0.016f);
	}
	FlushRenderingCommands();

	TArray<FColor> Pixels;
	FTextureRenderTargetResource* Resource = Target->GameThread_GetRenderTargetResource();
	if (!Resource || !Resource->ReadPixels(Pixels) || Pixels.Num() != Width * Height)
	{
		return TEXT("Error: could not read back the rendered pixels.");
	}
	for (FColor& Pixel : Pixels)
	{
		Pixel.A = 255;
	}

	TArray64<uint8> Png;
	FImageUtils::PNGCompressImageArray(Width, Height, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Png);

	const FString Path = OutputFile.IsEmpty()
		? FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("WidgetRenders") / (AssetName + TEXT(".png")))
		: OutputFile;
	if (!FFileHelper::SaveArrayToFile(Png, *Path))
	{
		return FString::Printf(TEXT("Error: could not write '%s'."), *Path);
	}

	Widget->RemoveFromParent();
	return Path;
}
