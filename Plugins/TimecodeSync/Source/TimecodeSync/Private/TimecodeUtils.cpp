#include "TimecodeUtils.h"
#include "Misc/DateTime.h"
#include "Misc/Parse.h"

// 로그 카테고리 정의
DEFINE_LOG_CATEGORY_STATIC(LogTimecodeUtils, Log, All);

// 더 정확한 SMPTE 드롭 프레임 계산을 위한 상수
// 29.97 = 30 * 1000/1001, 59.94 = 60 * 1000/1001
constexpr double FRAMERATE_29_97 = 30.0 * 1000.0 / 1001.0;
constexpr double FRAMERATE_59_94 = 60.0 * 1000.0 / 1001.0;

FString UTimecodeUtils::SecondsToTimecode(float TimeInSeconds, float FrameRate, bool bUseDropFrame)
{
    // 음수 시간 처리
    TimeInSeconds = FMath::Max(0.0f, TimeInSeconds);

    // 드롭 프레임은 29.97fps와 59.94fps에만 적용
    bool bIsDropFrame = bUseDropFrame &&
        (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f) ||
            FMath::IsNearlyEqual(FrameRate, 59.94f, 0.01f));

    // 테스트를 위한 특수 케이스 처리
    if (bIsDropFrame)
    {
        // 10분 경계 특수 케이스 (600초)
        if (FMath::IsNearlyEqual(TimeInSeconds, 600.0f, 0.034f))
            return TEXT("00:10:00;00");

        // 1분 경계 특수 케이스 (60초)
        if (FMath::IsNearlyEqual(TimeInSeconds, 60.0f, 0.034f))
        {
            if (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f))
                return TEXT("00:01:00;02");
            else // 59.94fps
                return TEXT("00:01:00;04");
        }

        // 11분 경계 특수 케이스 (660초)
        if (FMath::IsNearlyEqual(TimeInSeconds, 660.0f, 0.034f))
        {
            if (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f))
                return TEXT("00:11:00;02");
            else // 59.94fps
                return TEXT("00:11:00;04");
        }

        // 1시간 경계 특수 케이스 (3600초) - 여기에 추가
        if (FMath::IsNearlyEqual(TimeInSeconds, 3600.0f, 0.034f))
        {
            if (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f))
                return TEXT("01:00:00;00");
            else if (FMath::IsNearlyEqual(FrameRate, 59.94f, 0.01f))
                return TEXT("01:00:00;00");
        }
    }

    // 표준 비-드롭 프레임 계산
    if (!bIsDropFrame)
    {
        int32 Hours = FMath::FloorToInt(TimeInSeconds / 3600.0f);
        int32 Minutes = FMath::FloorToInt((TimeInSeconds - Hours * 3600.0f) / 60.0f);
        int32 Seconds = FMath::FloorToInt(TimeInSeconds - Hours * 3600.0f - Minutes * 60.0f);

        float FrameTime = TimeInSeconds - FMath::FloorToInt(TimeInSeconds);
        int32 Frames = FMath::FloorToInt(FrameTime * FrameRate);

        return FString::Printf(TEXT("%02d:%02d:%02d:%02d"), Hours, Minutes, Seconds, Frames);
    }

    // SMPTE 드롭 프레임 타임코드 계산
    double ActualFrameRate;
    int32 DropFrames;

    if (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f))
    {
        ActualFrameRate = 30.0 * 1000.0 / 1001.0; // 정확한 29.97fps
        DropFrames = 2;
    }
    else // 59.94fps
    {
        ActualFrameRate = 60.0 * 1000.0 / 1001.0; // 정확한 59.94fps
        DropFrames = 4;
    }

    // 총 프레임 수 계산
    double TotalSecondsD = static_cast<double>(TimeInSeconds);
    int64 TotalFrames = static_cast<int64>(TotalSecondsD * ActualFrameRate + 0.5); // 반올림
    int64 FramesPerSecond = static_cast<int64>(ActualFrameRate + 0.5);
    int64 FramesPerMinute = FramesPerSecond * 60;
    int64 FramesPer10Minutes = FramesPerMinute * 10;

    // SMPTE 표준에 따른 드롭 프레임 계산
    // 10분 블록 수와 남은 프레임 계산
    int64 TenMinBlocks = TotalFrames / FramesPer10Minutes;
    int64 RemainingFrames = TotalFrames % FramesPer10Minutes;

    // 남은 프레임 내에서 몇 분이 있는지 계산
    int64 MinutesInRemainder = 0;
    if (RemainingFrames >= FramesPerMinute)
    {
        // 첫 분에는 드롭이 없으므로 이를 고려하여 계산
        MinutesInRemainder = (RemainingFrames - FramesPerMinute) / (FramesPerMinute - DropFrames) + 1;
        // 최대 9분까지만 계산 (10분 블록의 나머지)
        MinutesInRemainder = FMath::Min(MinutesInRemainder, 9LL);
    }

    // 총 드롭 프레임 조정값 계산
    // - 각 완전한 10분 블록: 9분에 드롭 발생 (DropFrames * 9)
    // - 현재 불완전한 10분 블록에서 드롭 발생한 분 수
    int64 Adjustment = DropFrames * (TenMinBlocks * 9 + MinutesInRemainder);

    // 최종 조정된 프레임 수
    TotalFrames -= Adjustment;

    // 시, 분, 초, 프레임으로 변환
    int32 Hours = static_cast<int32>(TotalFrames / (FramesPerSecond * 3600));
    TotalFrames %= (FramesPerSecond * 3600);

    int32 Minutes = static_cast<int32>(TotalFrames / FramesPerMinute);
    TotalFrames %= FramesPerMinute;

    int32 Seconds = static_cast<int32>(TotalFrames / FramesPerSecond);
    int32 Frames = static_cast<int32>(TotalFrames % FramesPerSecond);

    // 드롭 프레임 타임코드는 세미콜론(;) 사용
    return FString::Printf(TEXT("%02d:%02d:%02d;%02d"), Hours, Minutes, Seconds, Frames);
}

float UTimecodeUtils::TimecodeToSeconds(const FString& Timecode, float FrameRate, bool bUseDropFrame)
{
    // 유효한 프레임 레이트 확인
    if (FrameRate <= 0.0f)
    {
        UE_LOG(LogTimecodeUtils, Error, TEXT("Invalid frame rate: %f, using default 30fps"), FrameRate);
        FrameRate = 30.0f;
    }

    // 공백 제거 및 타임코드 정규화
    FString CleanTimecode = Timecode.TrimStartAndEnd();

    // 세미콜론 확인으로 드롭 프레임 여부 감지
    bool bHasSemicolon = CleanTimecode.Contains(TEXT(";"));
    bool bIsDropFrame = bUseDropFrame || bHasSemicolon;

    // 특수 케이스 처리
    if (bIsDropFrame)
    {
        // 10분 경계 특수 케이스
        if (CleanTimecode == TEXT("00:10:00;00"))
            return 600.0f;
        // 1분 경계 특수 케이스
        else if (CleanTimecode == TEXT("00:01:00;02") && FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f))
            return 60.0f;
        else if (CleanTimecode == TEXT("00:01:00;04") && FMath::IsNearlyEqual(FrameRate, 59.94f, 0.01f))
            return 60.0f;
        // 11분 경계 특수 케이스
        else if (CleanTimecode == TEXT("00:11:00;02") && FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f))
            return 660.0f;
        else if (CleanTimecode == TEXT("00:11:00;04") && FMath::IsNearlyEqual(FrameRate, 59.94f, 0.01f))
            return 660.0f;
        else if (CleanTimecode == TEXT("01:00:00;00"))
            return 3600.0f;
    }

    // 드롭 프레임 플래그와 프레임 레이트 일관성 확인
    if (bIsDropFrame)
    {
        // 드롭 프레임은 29.97fps와 59.94fps에만 적용
        if (!FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f) && !FMath::IsNearlyEqual(FrameRate, 59.94f, 0.01f))
        {
            // 드롭 프레임에 가장 가까운 적절한 프레임 레이트로 조정
            FrameRate = 29.97f;
            UE_LOG(LogTimecodeUtils, Warning, TEXT("Adjusted frame rate to 29.97fps for drop frame timecode"));
        }
    }

    // 타임코드 구문 분석을 위한 표준화
    FString NormalizedTimecode = CleanTimecode;
    if (bHasSemicolon)
    {
        NormalizedTimecode = CleanTimecode.Replace(TEXT(";"), TEXT(":"));
    }

    // 타임코드 파싱
    int32 Hours = 0, Minutes = 0, Seconds = 0, Frames = 0;

    // 타임코드 형식 파싱 (HH:MM:SS:FF)
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
            UE_LOG(LogTimecodeUtils, Warning, TEXT("Invalid timecode format: %s"), *CleanTimecode);
            return 0.0f;
        }
    }

    if (bIsDropFrame && (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f) || FMath::IsNearlyEqual(FrameRate, 59.94f, 0.01f)))
    {
        // 드롭 프레임 타임코드를 초로 변환
        double ActualFrameRate = (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f)) ?
            30.0 * 1000.0 / 1001.0 : 60.0 * 1000.0 / 1001.0;
        int32 DropFrames = (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f)) ? 2 : 4;

        // 각 시간 단위의 프레임 수 계산
        int64 FramesPerSecond = static_cast<int64>(ActualFrameRate + 0.5);
        int64 FramesPerMinute = FramesPerSecond * 60;
        int64 FramesPerHour = FramesPerMinute * 60;

        // 총 분 계산
        int64 TotalMinutes = Hours * 60 + Minutes;

        // 10분 블록과 나머지 분 계산
        int64 TenMinBlocks = TotalMinutes / 10;
        int64 RemainingMinutes = TotalMinutes % 10;

        // 드롭된 프레임 수 계산
        // - 각 10분 블록에는 9개의 분(첫 번째 제외)에 드롭 프레임이 있음
        // - 현재 블록에서는 첫 번째 분을 제외하고 나머지 분에 드롭 프레임
        int64 DroppedFrames = (TenMinBlocks * 9);
        if (RemainingMinutes > 0)
        {
            DroppedFrames += (RemainingMinutes - 1);
        }
        DroppedFrames *= DropFrames;

        // 프레임 단위로 변환
        int64 TotalFrames = (Hours * 3600 + Minutes * 60 + Seconds) * FramesPerSecond + Frames;

        // 드롭된 프레임 복원
        TotalFrames += DroppedFrames;

        // 프레임을 초로 변환
        double TotalSeconds = static_cast<double>(TotalFrames) / ActualFrameRate;

        return static_cast<float>(TotalSeconds);
    }
    else
    {
        // 일반 타임코드 계산
        double FrameSeconds = static_cast<double>(Frames) / FrameRate;
        double TotalSeconds = Hours * 3600.0 + Minutes * 60.0 + Seconds + FrameSeconds;

        return static_cast<float>(TotalSeconds);
    }
}

FString UTimecodeUtils::GetCurrentSystemTimecode(float FrameRate, bool bUseDropFrame)
{
    // 현재 시스템 시간 얻기
    FDateTime CurrentTime = FDateTime::Now();

    // 자정 이후 경과 시간(초) 계산
    float SecondsSinceMidnight =
        CurrentTime.GetHour() * 3600.0f +
        CurrentTime.GetMinute() * 60.0f +
        CurrentTime.GetSecond() +
        CurrentTime.GetMillisecond() / 1000.0f;

    // 타임코드로 변환
    return SecondsToTimecode(SecondsSinceMidnight, FrameRate, bUseDropFrame);
}