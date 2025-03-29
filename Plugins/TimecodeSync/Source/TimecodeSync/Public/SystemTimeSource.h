// SystemTimeSource.h
// 경로: Plugins/TimecodeSync/Source/TimecodeSync/Public/SystemTimeSource.h

#pragma once

#include "CoreMinimal.h"
#include "TimeSourceInterface.h"
#include "SystemTimeSource.generated.h"

/**
 * System time source implementation
 * Uses FPlatformTime::Seconds() as time source
 */
UCLASS(BlueprintType, Blueprintable)
class TIMECODESYNC_API USystemTimeSource : public UObject, public ITimeSourceInterface
{
    GENERATED_BODY()

public:
    USystemTimeSource();

    // ITimeSourceInterface implementation
    virtual float GetCurrentTimeInSeconds_Implementation() override;
    virtual FString GetTimeSourceName_Implementation() override;
    virtual bool IsAvailable_Implementation() override;

private:
    // Starting time reference
    double StartTimeSeconds;
};