using UnrealBuildTool;

public class HappinessMCPInput : ModuleRules
{
	public HappinessMCPInput(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"ToolsetRegistry"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"LevelEditor",
			"Slate",
			"SlateCore"
		});
	}
}
