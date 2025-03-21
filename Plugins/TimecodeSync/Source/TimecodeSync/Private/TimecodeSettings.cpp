#include "TimecodeSettings.h"

UTimecodeSettings::UTimecodeSettings()
{
    // 기본값 설정
    FrameRate = 30.0f;
    bUseDropFrameTimecode = false;
    DefaultUDPPort = 10000;
    MulticastGroupAddress = "239.0.0.1";
    BroadcastInterval = 0.033f; // 약 30Hz
    bAutoDetectRole = true;
    bIsMaster = false;
    bEnableNDisplayIntegration = false;
}

FName UTimecodeSettings::GetCategoryName() const
{
    return FName("Plugins");
}

FText UTimecodeSettings::GetSectionDescription() const
{
    return FText::FromString("Timecode Sync");
}