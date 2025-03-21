using UnrealBuildTool;

public class TimecodeSyncEditor : ModuleRules
{
	public TimecodeSyncEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"Slate",
				"SlateCore"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"EditorFramework",
				"UnrealEd",
				"ToolMenus",
				"LevelEditor",
				"PropertyEditor",
				"TimecodeSync" // 런타임 모듈에 대한 종속성 추가
            }
		);

        if (Target.bBuildDeveloperTools || Target.Configuration != UnrealTargetConfiguration.Shipping)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
            "AutomationTest"
                }
            );
        }
    }
}