// SMPTETimecodeConverter.h
// SMPTE Timecode conversion utilities

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SMPTETimecodeConverter.generated.h"

/**
 * SMPTE Timecode converter for transforming between seconds and timecode formats
 * Supports both drop frame and non-drop frame timecodes
 */
UCLASS(BlueprintType, Blueprintable)
class TIMECODESYNC_API USMPTETimecodeConverter : public UObject
{
    GENERATED_BODY()

public:
    USMPTETimecodeConverter();

    /**
     * Convert seconds to SMPTE timecode
     * @param TimeInSeconds - Time in seconds to convert
     * @param FrameRate - Frame rate to use for conversion
     * @param bUseDropFrame - Whether to use drop frame timecode
     * @return Formatted timecode string (HH:MM:SS:FF or HH:MM:SS;FF for drop frame)
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|SMPTE")
    FString SecondsToTimecode(float TimeInSeconds, float FrameRate, bool bUseDropFrame);

    /**
     * Convert SMPTE timecode to seconds
     * @param Timecode - Timecode string to convert (HH:MM:SS:FF or HH:MM:SS;FF)
     * @param FrameRate - Frame rate to use for conversion
     * @param bUseDropFrame - Whether input uses drop frame timecode
     * @return Time in seconds
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|SMPTE")
    float TimecodeToSeconds(const FString& Timecode, float FrameRate, bool bUseDropFrame);

    /**
     * Get current system time as SMPTE timecode
     * @param FrameRate - Frame rate to use for conversion
     * @param bUseDropFrame - Whether to use drop frame timecode
     * @return Formatted timecode string
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|SMPTE")
    FString GetCurrentSystemTimecode(float FrameRate, bool bUseDropFrame);

    /**
     * Check if timecode format is valid
     * @param Timecode - Timecode string to check
     * @return True if timecode format is valid
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|SMPTE")
    bool IsTimecodeFormatValid(const FString& Timecode) const;

    /**
     * Check if timecode is drop frame format
     * @param Timecode - Timecode string to check
     * @return True if timecode is drop frame format
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|SMPTE")
    bool IsDropFrameTimecode(const FString& Timecode) const;

private:
    // Helper functions
    int64 CalculateDropFrameAdjustment(int64 TotalFrames, float FrameRate);

    // More accurate SMPTE drop frame constants
    const double FRAMERATE_29_97 = 30.0 * 1000.0 / 1001.0;
    const double FRAMERATE_59_94 = 60.0 * 1000.0 / 1001.0;
};