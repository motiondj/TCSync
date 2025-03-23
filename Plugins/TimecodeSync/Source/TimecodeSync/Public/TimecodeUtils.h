#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TimecodeUtils.generated.h"

/**
 * Timecode generation and processing utility class
 */
UCLASS()
class TIMECODESYNC_API UTimecodeUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Convert time in seconds to SMPTE timecode string
     * @param TimeInSeconds - Time in seconds to convert
     * @param FrameRate - Frame rate
     * @param bUseDropFrame - Whether to use drop frame timecode
     * @return Formatted timecode string (HH:MM:SS:FF)
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    static FString SecondsToTimecode(float TimeInSeconds, float FrameRate, bool bUseDropFrame = false);

    /**
     * Convert SMPTE timecode string to time in seconds
     * @param Timecode - Timecode string to convert (HH:MM:SS:FF)
     * @param FrameRate - Frame rate
     * @param bUseDropFrame - Whether to use drop frame timecode
     * @return Time in seconds
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    static float TimecodeToSeconds(const FString& Timecode, float FrameRate, bool bUseDropFrame = false);

    /**
     * Convert current system time to timecode
     * @param FrameRate - Frame rate
     * @param bUseDropFrame - Whether to use drop frame timecode
     * @return Current time as timecode string
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    static FString GetCurrentSystemTimecode(float FrameRate, bool bUseDropFrame = false);
};
