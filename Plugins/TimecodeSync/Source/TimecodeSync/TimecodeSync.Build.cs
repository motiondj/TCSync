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
                "Networking",   // 네트워킹 모듈 추가
                "Sockets"       // 소켓 모듈 추가
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
    }
}