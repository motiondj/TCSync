#include "TimecodeSync.h"
#include "TimecodeSettings.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FTimecodeSyncModule"

void FTimecodeSyncModule::StartupModule()
{
    // 모듈 초기화 코드
    UE_LOG(LogTemp, Log, TEXT("TimecodeSync Module Started"));

    // 설정 로드
    LoadSettings();

    UE_LOG(LogTemp, Warning, TEXT("TimecodeSync module started, checking for tests..."));

    // 테스트 검증을 위한 임시 코드
    TArray<FAutomationTestInfo> TestInfos;
    FAutomationTestFramework::Get().GetValidTestNames(TestInfos);
    for (const FAutomationTestInfo& TestInfo : TestInfos)
    {
        if (TestInfo.GetDisplayName().Contains(TEXT("TimecodeSync")))
        {
            UE_LOG(LogTemp, Warning, TEXT("Found test: %s"), *TestInfo.GetDisplayName());
        }
    }
}

void FTimecodeSyncModule::ShutdownModule()
{
    // 모듈 종료 코드
    UE_LOG(LogTemp, Log, TEXT("TimecodeSync Module Shutdown"));
}

void FTimecodeSyncModule::LoadSettings()
{
    // 설정 로드 및 적용
    const UTimecodeSettings* Settings = GetDefault<UTimecodeSettings>();
    if (Settings)
    {
        UE_LOG(LogTemp, Log, TEXT("TimecodeSync Settings Loaded - FrameRate: %f"), Settings->FrameRate);
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTimecodeSyncModule, TimecodeSync)