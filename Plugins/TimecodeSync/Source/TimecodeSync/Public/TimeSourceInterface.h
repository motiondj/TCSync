// TimeSourceInterface.h
// 경로: Plugins/TimecodeSync/Source/TimecodeSync/Public/TimeSourceInterface.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TimeSourceInterface.generated.h"

// Interface for time source providers
UINTERFACE(MinimalAPI, Blueprintable)
class UTimeSourceInterface : public UInterface
{
    GENERATED_BODY()
};

class TIMECODESYNC_API ITimeSourceInterface
{
    GENERATED_BODY()

public:
    // Get current time in seconds
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Timecode|TimeSource")
    float GetCurrentTimeInSeconds();

    // Get time source name
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Timecode|TimeSource")
    FString GetTimeSourceName();

    // Check if time source is available
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Timecode|TimeSource")
    bool IsAvailable();
};