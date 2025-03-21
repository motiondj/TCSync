#include "TimecodeUtils.h"
#include "Misc/DateTime.h"
#include "Misc/Parse.h"

FString UTimecodeUtils::SecondsToTimecode(float TimeInSeconds, float FrameRate, bool bUseDropFrame)
{
    // Handle negative time
    TimeInSeconds = FMath::Max(0.0f, TimeInSeconds);

    // Calculate hours, minutes, seconds
    int32 Hours = FMath::FloorToInt(TimeInSeconds / 3600.0f);
    int32 Minutes = FMath::FloorToInt((TimeInSeconds - Hours * 3600.0f) / 60.0f);
    int32 Seconds = FMath::FloorToInt(TimeInSeconds - Hours * 3600.0f - Minutes * 60.0f);

    // Calculate frames
    float FrameTime = TimeInSeconds - FMath::FloorToInt(TimeInSeconds);
    int32 Frames = FMath::FloorToInt(FrameTime * FrameRate);

    // Calculate drop frame timecode (used for 29.97fps, 59.94fps, etc.)
    if (bUseDropFrame)
    {
        // Drop frame timecode calculation is only meaningful at 29.97fps or 59.94fps
        if (FMath::IsNearlyEqual(FrameRate, 29.97f) || FMath::IsNearlyEqual(FrameRate, 59.94f))
        {
            // SMPTE drop frame calculation
            // For 29.97fps: Skip first 2 frames (every minute, except multiples of 10)
            // For 59.94fps: Skip first 4 frames (every minute, except multiples of 10)

            // Calculate total frames
            int32 TotalFrames = FMath::RoundToInt(TimeInSeconds * FrameRate);

            // Calculate number of frames to drop
            int32 DropFrames = FMath::IsNearlyEqual(FrameRate, 29.97f) ? 2 : 4;

            // Calculate hours, minutes, seconds, frames based on 30fps
            int32 NominalRate = FMath::RoundToInt(FrameRate);
            int32 FramesPerMinute = NominalRate * 60; // Frames per minute
            int32 FramesPerTenMinutes = FramesPerMinute * 10; // Frames per 10 minutes

            // Calculate by 10-minute periods
            int32 TenMinutePeriods = TotalFrames / FramesPerTenMinutes;
            int32 RemainingFrames = TotalFrames % FramesPerTenMinutes;

            // Apply frame drops after first minute in remaining frames
            int32 AdditionalDropFrames = 0;
            if (RemainingFrames >= FramesPerMinute) {
                int32 RemainingMinutes = (RemainingFrames - FramesPerMinute) / NominalRate / 60 + 1;
                // Maximum 9 drops possible within 10 minutes (after first minute until 9th minute)
                AdditionalDropFrames = DropFrames * FMath::Min(9, RemainingMinutes);
            }

            // Total drop frames = 10-minute period drops + remaining time drops
            int32 TotalDropFrames = DropFrames * 9 * TenMinutePeriods + AdditionalDropFrames;

            // Total frames adjusted for drops
            int32 AdjustedFrames = TotalFrames + TotalDropFrames;

            // Convert to hours, minutes, seconds, frames
            int32 FramesPerHour = FramesPerMinute * 60;
            Hours = AdjustedFrames / FramesPerHour;
            AdjustedFrames %= FramesPerHour;

            Minutes = AdjustedFrames / FramesPerMinute;
            AdjustedFrames %= FramesPerMinute;

            Seconds = AdjustedFrames / NominalRate;
            Frames = AdjustedFrames % NominalRate;

            // First frame number is dropped at the start of each minute (except multiples of 10)
            // Therefore, 00:01:00;02 instead of 00:01:00;00 (29.97fps)
            if (Seconds == 0 && Frames < DropFrames && Minutes % 10 != 0) {
                Frames = DropFrames;
            }
        }
    }

    // Generate timecode string (HH:MM:SS:FF or HH:MM:SS;FF format)
    FString Delimiter = bUseDropFrame ? ";" : ":";
    return FString::Printf(TEXT("%02d:%02d:%02d%s%02d"), Hours, Minutes, Seconds, *Delimiter, Frames);
}

float UTimecodeUtils::TimecodeToSeconds(const FString& Timecode, float FrameRate, bool bUseDropFrame)
{
    // Validate frame rate
    if (FrameRate <= 0.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid frame rate: %f"), FrameRate);
        FrameRate = 30.0f; // Replace with default value
    }

    // Remove whitespace and normalize timecode
    FString CleanTimecode = Timecode.TrimStartAndEnd();

    // Validate timecode format
    TArray<FString> TimeParts;
    if (CleanTimecode.ParseIntoArray(TimeParts, TEXT(":;"), true) != 4)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid timecode format: %s"), *CleanTimecode);

        // Try manual parsing
        if (CleanTimecode.Len() >= 11 &&
            CleanTimecode[2] == ':' &&
            CleanTimecode[5] == ':' &&
            (CleanTimecode[8] == ':' || CleanTimecode[8] == ';'))
        {
            FString HourStr = CleanTimecode.Mid(0, 2);
            FString MinStr = CleanTimecode.Mid(3, 2);
            FString SecStr = CleanTimecode.Mid(6, 2);
            FString FrameStr = CleanTimecode.Mid(9, 2);

            int32 Hours = FCString::Atoi(*HourStr);
            int32 Minutes = FCString::Atoi(*MinStr);
            int32 Seconds = FCString::Atoi(*SecStr);
            int32 Frames = FCString::Atoi(*FrameStr);

            UE_LOG(LogTemp, Warning, TEXT("Manual parsing result - H:%d M:%d S:%d F:%d"),
                Hours, Minutes, Seconds, Frames);

            // Calculate time in seconds
            float FrameSeconds = (FrameRate > 0.0f) ? ((float)Frames / FrameRate) : 0.0f;
            float TotalSeconds = Hours * 3600.0f + Minutes * 60.0f + Seconds + FrameSeconds;

            UE_LOG(LogTemp, Log, TEXT("Calculated seconds: H:%f + M:%f + S:%f + F:%f = Total:%f"),
                Hours * 3600.0f, Minutes * 60.0f, (float)Seconds, FrameSeconds, TotalSeconds);

            return TotalSeconds;
        }

        return 0.0f;
    }

    // Parse hours, minutes, seconds, frames
    int32 Hours = FCString::Atoi(*TimeParts[0]);
    int32 Minutes = FCString::Atoi(*TimeParts[1]);
    int32 Seconds = FCString::Atoi(*TimeParts[2]);
    int32 Frames = FCString::Atoi(*TimeParts[3]);

    // Log parsed values
    UE_LOG(LogTemp, Log, TEXT("Parsed timecode %s into H:%d M:%d S:%d F:%d"),
        *CleanTimecode, Hours, Minutes, Seconds, Frames);

    // Handle drop frame timecode
    if (bUseDropFrame && (FMath::IsNearlyEqual(FrameRate, 29.97f) || FMath::IsNearlyEqual(FrameRate, 59.94f)))
    {
        int32 TotalMinutes = Hours * 60 + Minutes;
        int32 FramesToAdd = 0;

        // Adjust first 2 (or 4) frames every minute except multiples of 10
        if (FMath::IsNearlyEqual(FrameRate, 29.97f))
        {
            FramesToAdd = 2 * (TotalMinutes - TotalMinutes / 10);
        }
        else if (FMath::IsNearlyEqual(FrameRate, 59.94f))
        {
            FramesToAdd = 4 * (TotalMinutes - TotalMinutes / 10);
        }

        // Subtract adjustment frames from total frames
        int32 TotalFrames = Hours * 3600 * (int32)FrameRate
            + Minutes * 60 * (int32)FrameRate
            + Seconds * (int32)FrameRate
            + Frames
            - FramesToAdd;

        // Convert to seconds
        return TotalFrames / FrameRate;
    }
    else
    {
        // Calculate time in seconds - safe division operation
        float FrameSeconds = (FrameRate > 0.0f) ? ((float)Frames / FrameRate) : 0.0f;
        float TotalSeconds = Hours * 3600.0f + Minutes * 60.0f + Seconds + FrameSeconds;

        UE_LOG(LogTemp, Log, TEXT("Calculated seconds: H:%f + M:%f + S:%f + F:%f = Total:%f"),
            Hours * 3600.0f, Minutes * 60.0f, (float)Seconds, FrameSeconds, TotalSeconds);

        return TotalSeconds;
    }
}

FString UTimecodeUtils::GetCurrentSystemTimecode(float FrameRate, bool bUseDropFrame)
{
    // Get current system time
    FDateTime CurrentTime = FDateTime::Now();

    // Calculate elapsed time in seconds since midnight
    float SecondsSinceMidnight =
        CurrentTime.GetHour() * 3600.0f +
        CurrentTime.GetMinute() * 60.0f +
        CurrentTime.GetSecond() +
        CurrentTime.GetMillisecond() / 1000.0f;

    // Convert to timecode
    return SecondsToTimecode(SecondsSinceMidnight, FrameRate, bUseDropFrame);
}