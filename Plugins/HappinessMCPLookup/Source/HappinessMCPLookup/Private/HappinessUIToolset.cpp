#include "HappinessUIToolset.h"

#include "Blueprint/UserWidget.h"
#include "Editor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"
#include "Slate/WidgetRenderer.h"
#include "TextureResource.h"

FString UHappinessUIToolset::RenderWidget(const FString& WidgetBlueprintPath, int32 Width, int32 Height, const FString& OutputFile)
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
	for (int32 Pass = 0; Pass < 3; Pass++)
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
