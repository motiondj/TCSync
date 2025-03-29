// SystemTimeSource.cpp
// 경로: Plugins/TimecodeSync/Source/TimecodeSync/Private/SystemTimeSource.cpp

#include "SystemTimeSource.h"

USystemTimeSource::USystemTimeSource()
{
    // Store starting time reference
    StartTimeSeconds = FPlatformTime::Seconds();
}

float USystemTimeSource::GetCurrentTimeInSeconds_Implementation()
{
    // Return time elapsed since creation
    return static_cast<float>(FPlatformTime::Seconds() - StartTimeSeconds);
}

FString USystemTimeSource::GetTimeSourceName_Implementation()
{
    return TEXT("System Time");
}

bool USystemTimeSource::IsAvailable_Implementation()
{
    return true; // System time is always available
}