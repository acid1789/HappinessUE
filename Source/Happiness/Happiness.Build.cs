// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Happiness : ModuleRules
{
	public Happiness(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		// Lets subfolders include each other as "HappinessClassic/..." and "UI/..."
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore" });

		// Ads (PloxTools AdMob, a Fab plugin installed in the engine)
		PrivateDependencyModuleNames.AddRange(new string[] { "PloxToolsAdMob" });
		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			// The ads code reads the screen's size and density from Java (FAndroidApplication)
			PrivateDependencyModuleNames.Add("Launch");
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
