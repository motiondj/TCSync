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

        // 테스트 관련 의존성 추가
        if (Target.bBuildDeveloperTools || Target.Configuration != UnrealTargetConfiguration.Shipping)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "AutomationTest"
                }
            );
        }

        // Tests 폴더를 포함 경로에 추가
        PrivateIncludePaths.AddRange(
            new string[]
            {
                "TimecodeSync/Private",
                "TimecodeSync/Private/Tests"
            }
        );
    }
}