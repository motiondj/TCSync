// SMPTETimecodeConverter.cpp

#include "SMPTETimecodeConverter.h"
#include "Misc/DateTime.h"

// Define log category
DEFINE_LOG_CATEGORY_STATIC(LogSMPTEConverter, Log, All);

USMPTETimecodeConverter::USMPTETimecodeConverter()
{
    // No specific initialization needed
}

FString USMPTETimecodeConverter::SecondsToTimecode(float TimeInSeconds, float FrameRate, bool bUseDropFrame)
{
    // Handle negative time
    TimeInSeconds = FMath::Max(0.0f, TimeInSeconds);

    // Drop frame is only applicable for 29.97fps and 59.94fps
    bool bIsDropFrame = bUseDropFrame &&
        (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f) ||
            FMath::IsNearlyEqual(FrameRate, 59.94f, 0.01f));

    // Standard non-drop frame calculation
    if (!bIsDropFrame)
    {
        int32 Hours = FMath::FloorToInt(TimeInSeconds / 3600.0f);
        int32 Minutes = FMath::FloorToInt((TimeInSeconds - Hours * 3600.0f) / 60.0f);
        int32 Seconds = FMath::FloorToInt(TimeInSeconds - Hours * 3600.0f - Minutes * 60.0f);

        float FrameTime = TimeInSeconds - FMath::FloorToInt(TimeInSeconds);
        int32 Frames = FMath::FloorToInt(FrameTime * FrameRate);

        return FString::Printf(TEXT("%02d:%02d:%02d:%02d"), Hours, Minutes, Seconds, Frames);
    }

    // SMPTE drop frame timecode calculation - complete reimplementation
    double ActualFrameRate;
    int32 DropFrames;

    if (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f))
    {
        ActualFrameRate = FRAMERATE_29_97; // Exact 29.97fps
        DropFrames = 2;
    }
    else // 59.94fps
    {
        ActualFrameRate = FRAMERATE_59_94; // Exact 59.94fps
        DropFrames = 4;
    }

    // Calculate total frames
    double TotalSecondsD = static_cast<double>(TimeInSeconds);
    int64 TotalFrames = static_cast<int64>(TotalSecondsD * ActualFrameRate + 0.5); // Round
    int64 FramesPerSecond = static_cast<int64>(ActualFrameRate + 0.5);

    // Debug log
    UE_LOG(LogSMPTEConverter, Verbose, TEXT("Input time: %.3f sec, Frame rate: %.3f, Total frames: %lld"),
        TimeInSeconds, ActualFrameRate, TotalFrames);

    // SMPTE standard calculation
    // Skip first 2 (or 4) frame numbers every minute except every 10th minute
    int64 D = TotalFrames / (FramesPerSecond * 60 * 10); // Number of 10-minute blocks
    int64 M = TotalFrames % (FramesPerSecond * 60 * 10); // Frames within the 10-minute block

    // Check if past 1-minute boundary
    if (M >= FramesPerSecond * 60)
    {
        // Number of minutes within the 10-minute block (excluding first minute)
        int64 E = (M - FramesPerSecond * 60) / (FramesPerSecond * 60 - DropFrames);
        // Apply drop frame adjustment
        TotalFrames = TotalFrames - (DropFrames * (D * 9 + E));
    }
    else
    {
        // Within first minute of 10-minute block, no adjustment needed for this minute
        TotalFrames = TotalFrames - (DropFrames * D * 9);
    }

    // Debug log
    UE_LOG(LogSMPTEConverter, Verbose, TEXT("10-min blocks: %lld, Remaining frames: %lld, Final adjusted frames: %lld"),
        D, M, TotalFrames);

    // Calculate final hours, minutes, seconds, frames
    int32 Hours = static_cast<int32>(TotalFrames / (FramesPerSecond * 3600));
    TotalFrames %= (FramesPerSecond * 3600);

    int32 Minutes = static_cast<int32>(TotalFrames / (FramesPerSecond * 60));
    TotalFrames %= (FramesPerSecond * 60);

    int32 Seconds = static_cast<int32>(TotalFrames / FramesPerSecond);
    int32 Frames = static_cast<int32>(TotalFrames % FramesPerSecond);

    // Special case: exactly at 10-minute boundary should be 00 frames
    if (TimeInSeconds >= 600.0 && FMath::IsNearlyEqual(TimeInSeconds, 600.0, 0.034))
    {
        UE_LOG(LogSMPTEConverter, Verbose, TEXT("Exact 10-minute boundary detected: %02d:%02d:%02d;%02d"),
            Hours, Minutes, Seconds, Frames);
    }

    // Drop frame timecode uses semicolon (;)
    return FString::Printf(TEXT("%02d:%02d:%02d;%02d"), Hours, Minutes, Seconds, Frames);
}

float USMPTETimecodeConverter::TimecodeToSeconds(const FString& Timecode, float FrameRate, bool bUseDropFrame)
{
    // Validate frame rate
    if (FrameRate <= 0.0f)
    {
        UE_LOG(LogSMPTEConverter, Error, TEXT("Invalid frame rate: %f, using default 30fps"), FrameRate);
        FrameRate = 30.0f;
    }

    // Trim and normalize timecode
    FString CleanTimecode = Timecode.TrimStartAndEnd();

    // Check for semicolon to detect drop frame
    bool bHasSemicolon = CleanTimecode.Contains(TEXT(";"));
    bool bIsDropFrame = bUseDropFrame || bHasSemicolon;

    // Check consistency of drop frame flag and frame rate
    if (bIsDropFrame)
    {
        // Drop frame is only applicable for 29.97fps and 59.94fps
        if (!FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f) && !FMath::IsNearlyEqual(FrameRate, 59.94f, 0.01f))
        {
            // Adjust to closest appropriate frame rate for drop frame
            FrameRate = 29.97f;
            UE_LOG(LogSMPTEConverter, Warning, TEXT("Adjusted frame rate to 29.97fps for drop frame timecode"));
        }
    }

    // Normalize timecode for parsing
    FString NormalizedTimecode = CleanTimecode;
    if (bHasSemicolon)
    {
        NormalizedTimecode = CleanTimecode.Replace(TEXT(";"), TEXT(":"));
    }

    // Parse timecode
    int32 Hours = 0, Minutes = 0, Seconds = 0, Frames = 0;

    // Parse timecode format (HH:MM:SS:FF)
    if (NormalizedTimecode.Len() >= 11 &&
        NormalizedTimecode[2] == ':' &&
        NormalizedTimecode[5] == ':' &&
        (NormalizedTimecode[8] == ':' || CleanTimecode[8] == ';'))
    {
        Hours = FCString::Atoi(*NormalizedTimecode.Mid(0, 2));
        Minutes = FCString::Atoi(*NormalizedTimecode.Mid(3, 2));
        Seconds = FCString::Atoi(*NormalizedTimecode.Mid(6, 2));
        Frames = FCString::Atoi(*NormalizedTimecode.Mid(9, 2));
    }
    else
    {
        TArray<FString> TimeParts;
        if (NormalizedTimecode.ParseIntoArray(TimeParts, TEXT(":"), false) == 4)
        {
            Hours = FCString::Atoi(*TimeParts[0]);
            Minutes = FCString::Atoi(*TimeParts[1]);
            Seconds = FCString::Atoi(*TimeParts[2]);
            Frames = FCString::Atoi(*TimeParts[3]);
        }
        else
        {
            UE_LOG(LogSMPTEConverter, Warning, TEXT("Invalid timecode format: %s"), *CleanTimecode);
            return 0.0f;
        }
    }

    // Log parsing results
    UE_LOG(LogSMPTEConverter, Verbose, TEXT("Parsed timecode %s into H:%d M:%d S:%d F:%d"),
        *CleanTimecode, Hours, Minutes, Seconds, Frames);

    if (bIsDropFrame && (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f) || FMath::IsNearlyEqual(FrameRate, 59.94f, 0.01f)))
    {
        // Convert drop frame timecode to seconds
        double ActualFrameRate = (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f)) ?
            FRAMERATE_29_97 : FRAMERATE_59_94;
        int32 DropFrames = (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f)) ? 2 : 4;

        // Calculate total minutes and adjust for drop frame
        int64 TotalMinutes = Hours * 60 + Minutes;
        int64 NonDropMinutes = TotalMinutes - TotalMinutes / 10;

        // Calculate total frames
        int64 TotalFrames = Hours * static_cast<int64>(ActualFrameRate * 3600.0) +
            Minutes * static_cast<int64>(ActualFrameRate * 60.0) +
            Seconds * static_cast<int64>(ActualFrameRate) +
            Frames;

        // Apply drop frame adjustment
        TotalFrames += NonDropMinutes * DropFrames;

        // Convert frames to seconds
        double TotalSeconds = static_cast<double>(TotalFrames) / ActualFrameRate;

        UE_LOG(LogSMPTEConverter, Verbose, TEXT("Calculated seconds for drop frame timecode %s: %.6f"),
            *CleanTimecode, TotalSeconds);

        return static_cast<float>(TotalSeconds);
    }
    else
    {
        // Standard timecode calculation
        double FrameSeconds = static_cast<double>(Frames) / FrameRate;
        double TotalSeconds = Hours * 3600.0 + Minutes * 60.0 + Seconds + FrameSeconds;

        UE_LOG(LogSMPTEConverter, Verbose, TEXT("Calculated seconds for %s: %.6f"), *CleanTimecode, TotalSeconds);

        return static_cast<float>(TotalSeconds);
    }
}

FString USMPTETimecodeConverter::GetCurrentSystemTimecode(float FrameRate, bool bUseDropFrame)
{
    // Get current system time
    FDateTime CurrentTime = FDateTime::Now();

    // Calculate seconds since midnight
    float SecondsSinceMidnight =
        CurrentTime.GetHour() * 3600.0f +
        CurrentTime.GetMinute() * 60.0f +
        CurrentTime.GetSecond() +
        CurrentTime.GetMillisecond() / 1000.0f;

    // Convert to timecode
    return SecondsToTimecode(SecondsSinceMidnight, FrameRate, bUseDropFrame);
}

bool USMPTETimecodeConverter::IsTimecodeFormatValid(const FString& Timecode) const
{
    // Trim and normalize timecode
    FString CleanTimecode = Timecode.TrimStartAndEnd();

    // Check basic format (length and separators)
    if (CleanTimecode.Len() < 11) // Minimum length for HH:MM:SS:FF
    {
        return false;
    }

    // Check for proper separators (: or ;)
    if (CleanTimecode[2] != ':' || CleanTimecode[5] != ':' ||
        (CleanTimecode[8] != ':' && CleanTimecode[8] != ';'))
    {
        return false;
    }

    // Try to parse the parts
    bool bIsValid = true;

    // Hours
    int32 Hours = FCString::Atoi(*CleanTimecode.Mid(0, 2));
    bIsValid = bIsValid && (Hours >= 0 && Hours <= 23);

    // Minutes
    int32 Minutes = FCString::Atoi(*CleanTimecode.Mid(3, 2));
    bIsValid = bIsValid && (Minutes >= 0 && Minutes <= 59);

    // Seconds
    int32 Seconds = FCString::Atoi(*CleanTimecode.Mid(6, 2));
    bIsValid = bIsValid && (Seconds >= 0 && Seconds <= 59);

    // Frames (reasonable range check only)
    int32 Frames = FCString::Atoi(*CleanTimecode.Mid(9, 2));
    bIsValid = bIsValid && (Frames >= 0 && Frames <= 99);

    return bIsValid;
}

bool USMPTETimecodeConverter::IsDropFrameTimecode(const FString& Timecode) const
{
    // Check if timecode contains semicolon
    return Timecode.Contains(TEXT(";"));
}

int64 USMPTETimecodeConverter::CalculateDropFrameAdjustment(int64 TotalFrames, float FrameRate)
{
    // Helper function to calculate drop frame adjustment
    double ActualFrameRate;
    int32 DropFrames;

    if (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f))
    {
        ActualFrameRate = FRAMERATE_29_97;
        DropFrames = 2;
    }
    else // 59.94fps
    {
        ActualFrameRate = FRAMERATE_59_94;
        DropFrames = 4;
    }

    int64 FramesPerSecond = static_cast<int64>(ActualFrameRate + 0.5);
    int64 FramesPerMinute = FramesPerSecond * 60;
    int64 FramesPer10Minutes = FramesPerMinute * 10;

    int64 TenMinBlocks = TotalFrames / FramesPer10Minutes;
    int64 RemainingFrames = TotalFrames % FramesPer10Minutes;

    int64 MinutesInRemainder = 0;
    if (RemainingFrames >= FramesPerMinute)
    {
        MinutesInRemainder = (RemainingFrames - FramesPerMinute) / (FramesPerMinute - DropFrames) + 1;
    }

    return DropFrames * (TenMinBlocks * 9 + MinutesInRemainder);
}