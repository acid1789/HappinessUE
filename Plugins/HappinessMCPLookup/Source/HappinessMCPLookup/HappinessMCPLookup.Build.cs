using UnrealBuildTool;

public class HappinessMCPLookup : ModuleRules
{
	public HappinessMCPLookup(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"ToolsetRegistry"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Json",
			"UnrealEd"
		});
	}
}
