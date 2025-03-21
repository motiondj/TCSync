using UnrealBuildTool;

public class TimecodeSync : ModuleRules
{
    public TimecodeSync(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "Networking",   // Add networking module
                "Sockets"       // Add socket module
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Projects",
                "Slate",
                "SlateCore",
                "DeveloperSettings"
            }
        );

        // Add test-related dependencies
        if (Target.bBuildDeveloperTools || Target.Configuration != UnrealTargetConfiguration.Shipping)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "AutomationTest"
                }
            );
        }

        // Add Tests folder to include paths
        PrivateIncludePaths.AddRange(
            new string[]
            {
                "TimecodeSync/Private",
                "TimecodeSync/Private/Tests"
            }
        );
    }
}