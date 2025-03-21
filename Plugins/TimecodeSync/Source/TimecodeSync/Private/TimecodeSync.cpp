#include "TimecodeSync.h"
#include "TimecodeSettings.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FTimecodeSyncModule"

void FTimecodeSyncModule::StartupModule()
{
    // Module initialization code
    UE_LOG(LogTemp, Log, TEXT("TimecodeSync Module Started"));

    // Load settings
    LoadSettings();

    UE_LOG(LogTemp, Warning, TEXT("TimecodeSync module started, checking for tests..."));

    // Temporary code for test validation
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
    // Module shutdown code
    UE_LOG(LogTemp, Log, TEXT("TimecodeSync Module Shutdown"));
}

void FTimecodeSyncModule::LoadSettings()
{
    // Load and apply settings
    const UTimecodeSettings* Settings = GetDefault<UTimecodeSettings>();
    if (Settings)
    {
        UE_LOG(LogTemp, Log, TEXT("TimecodeSync Settings Loaded - FrameRate: %f"), Settings->FrameRate);
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTimecodeSyncModule, TimecodeSync)