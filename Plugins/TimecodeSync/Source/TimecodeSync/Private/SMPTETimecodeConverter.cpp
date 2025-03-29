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
    // 캐싱을 위한 정적 변수
    static double LastTimeInSeconds = -1.0;
    static float LastFrameRate = 0.0f;
    static bool LastUseDropFrame = false;
    static FString CachedTimecode;

    // 동일한 입력에 대해 캐시된 결과 반환
    if (FMath::IsNearlyEqual(LastTimeInSeconds, TimeInSeconds, 0.0001) &&
        FMath::IsNearlyEqual(LastFrameRate, FrameRate, 0.001) &&
        LastUseDropFrame == bUseDropFrame)
    {
        UE_LOG(LogSMPTEConverter, Verbose, TEXT("Using cached timecode for %.3f sec: %s"),
            TimeInSeconds, *CachedTimecode);
        return CachedTimecode;
    }

    UE_LOG(LogSMPTEConverter, Verbose, TEXT("Converting %.3f seconds to timecode, FrameRate=%.3f, DropFrame=%s"),
        TimeInSeconds, FrameRate, bUseDropFrame ? TEXT("true") : TEXT("false"));

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

        FString ResultTimecode = FString::Printf(TEXT("%02d:%02d:%02d:%02d"), Hours, Minutes, Seconds, Frames);

        // 계산 결과 캐싱
        LastTimeInSeconds = TimeInSeconds;
        LastFrameRate = FrameRate;
        LastUseDropFrame = bUseDropFrame;
        CachedTimecode = ResultTimecode;

        UE_LOG(LogSMPTEConverter, Verbose, TEXT("Final timecode result: %s"), *ResultTimecode);
        return ResultTimecode;
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

    // Apply drop frame adjustment using the centralized calculation function
    TotalFrames -= CalculateDropFrameAdjustment(TotalFrames, ActualFrameRate);

    // Debug log
    UE_LOG(LogSMPTEConverter, Verbose, TEXT("Final adjusted frames: %lld"), TotalFrames);

    // Calculate final hours, minutes, seconds, frames
    int32 Hours = static_cast<int32>(TotalFrames / (FramesPerSecond * 3600));
    TotalFrames %= (FramesPerSecond * 3600);

    int32 Minutes = static_cast<int32>(TotalFrames / (FramesPerSecond * 60));
    TotalFrames %= (FramesPerSecond * 60);

    int32 Seconds = static_cast<int32>(TotalFrames / FramesPerSecond);
    int32 Frames = static_cast<int32>(TotalFrames % FramesPerSecond);

    // 주요 시간 경계에서 결과 확인을 위한 로깅
    if (FMath::IsNearlyEqual(TimeInSeconds, 600.0, 0.05) ||    // 10분 경계
        FMath::IsNearlyEqual(TimeInSeconds, 60.0, 0.05) ||     // 1분 경계
        FMath::IsNearlyEqual(TimeInSeconds, 61.0, 0.05) ||     // 1분 1초
        FMath::IsNearlyEqual(TimeInSeconds, 660.0, 0.05))      // 11분 경계
    {
        UE_LOG(LogSMPTEConverter, Log, TEXT("Significant timecode boundary: %.3f seconds -> %02d:%02d:%02d;%02d"),
            TimeInSeconds, Hours, Minutes, Seconds, Frames);
    }

    // Drop frame timecode uses semicolon (;)
    FString ResultTimecode = FString::Printf(TEXT("%02d:%02d:%02d;%02d"), Hours, Minutes, Seconds, Frames);

    // 계산 결과 캐싱
    LastTimeInSeconds = TimeInSeconds;
    LastFrameRate = FrameRate;
    LastUseDropFrame = bUseDropFrame;
    CachedTimecode = ResultTimecode;

    UE_LOG(LogSMPTEConverter, Verbose, TEXT("Final timecode result: %s"), *ResultTimecode);
    return ResultTimecode;
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

        // 총 프레임 수 계산 (드롭 프레임 조정 없이)
        int64 TotalFrames = Hours * static_cast<int64>(ActualFrameRate * 3600.0) +
            Minutes * static_cast<int64>(ActualFrameRate * 60.0) +
            Seconds * static_cast<int64>(ActualFrameRate) +
            Frames;

        // 드롭 프레임 보정을 위한 계산 (SecondsToTimecode의 역연산)
        int64 TotalMinutes = Hours * 60 + Minutes;
        int64 TenMinBlocks = TotalMinutes / 10;
        int64 RemainingMinutes = TotalMinutes % 10;

        // 10분 블록마다 9개 분에서 프레임 드롭 발생
        int64 DropFrameMinutes = TenMinBlocks * 9;

        // 추가로, 현재 10분 블록에서 남은 분 중 첫 번째를 제외한 분에서 드롭 발생
        if (RemainingMinutes > 0)
        {
            DropFrameMinutes += RemainingMinutes - 1;
        }

        // 드롭된 프레임 복원 (역방향 보정)
        TotalFrames += DropFrameMinutes * DropFrames;

        UE_LOG(LogSMPTEConverter, Verbose, TEXT("TimecodeToSeconds: Frames=%lld, TotalMinutes=%lld, DropFrameMinutes=%lld, Added=%lld"),
            TotalFrames, TotalMinutes, DropFrameMinutes, DropFrameMinutes * DropFrames);

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

    // Calculate frames per time unit
    int64 FramesPerSecond = static_cast<int64>(ActualFrameRate + 0.5);
    int64 FramesPerMinute = FramesPerSecond * 60;
    int64 FramesPer10Minutes = FramesPerMinute * 10;

    // Calculate 10-minute blocks and remaining frames
    int64 TenMinBlocks = TotalFrames / FramesPer10Minutes;
    int64 RemainingFrames = TotalFrames % FramesPer10Minutes;

    // Calculate minutes within the remaining portion of the 10-minute block
    int64 MinutesInRemainder = 0;
    if (RemainingFrames >= FramesPerMinute)
    {
        // First minute doesn't have frame drops, for the rest calculate how many whole minutes
        MinutesInRemainder = (RemainingFrames - FramesPerMinute) / (FramesPerMinute - DropFrames) + 1;

        // Cap at 9 (a 10-minute block has at most 9 minutes with frame drops)
        MinutesInRemainder = FMath::Min(MinutesInRemainder, 9LL);
    }

    // Calculate total adjustment: 
    // - Each complete 10-minute block: 9 minutes with drops (DropFrames * 9)
    // - Plus minutes with drops in the current incomplete 10-minute block
    int64 Adjustment = DropFrames * (TenMinBlocks * 9 + MinutesInRemainder);

    UE_LOG(LogSMPTEConverter, Verbose, TEXT("Drop frame adjustment calculation: TotalFrames=%lld, Blocks=%lld, MinutesInRemainder=%lld, Adjustment=%lld"),
        TotalFrames, TenMinBlocks, MinutesInRemainder, Adjustment);

    return Adjustment;
}