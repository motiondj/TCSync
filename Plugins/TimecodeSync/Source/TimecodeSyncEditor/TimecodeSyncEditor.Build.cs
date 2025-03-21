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
                "SlateCore",
                "EditorStyle",
                "AppFramework"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Projects",
                "EditorFramework",
                "UnrealEd",
                "ToolMenus",
                "PropertyEditor",
                "LevelEditor",
                "TimecodeSync",
                "WorkspaceMenuStructure"
            }
        );
    }
}