using UnrealBuildTool;

public class GlobalSettingSyncer : ModuleRules
{
	public GlobalSettingSyncer(ReadOnlyTargetRules Target) : base(Target)
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