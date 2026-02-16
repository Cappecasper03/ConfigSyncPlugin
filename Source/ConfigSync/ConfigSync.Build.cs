using UnrealBuildTool;

public class ConfigSync : ModuleRules
{
	public ConfigSync(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
		[
			"Core"
		]);

		PrivateDependencyModuleNames.AddRange(
		[
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"InputCore",
			"UnrealEd",
			"ToolMenus",
			"EditorStyle",
			"EditorFramework",
			"Projects",
			"Json",
			"JsonUtilities",
			"DeveloperSettings",
			"DirectoryWatcher"
		]);
	}
}