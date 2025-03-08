#include "TimecodeUtils.h"
#include "Misc/DateTime.h"

FString UTimecodeUtils::SecondsToTimecode(float TimeInSeconds, float FrameRate, bool bUseDropFrame)
{
    // 음수 시간 처리
    TimeInSeconds = FMath::Max(0.0f, TimeInSeconds);

    // 시, 분, 초 계산
    int32 Hours = FMath::FloorToInt(TimeInSeconds / 3600.0f);
    int32 Minutes = FMath::FloorToInt((TimeInSeconds - Hours * 3600.0f) / 60.0f);
    int32 Seconds = FMath::FloorToInt(TimeInSeconds - Hours * 3600.0f - Minutes * 60.0f);

    // 프레임 계산
    float FrameTime = TimeInSeconds - FMath::FloorToInt(TimeInSeconds);
    int32 Frames = FMath::FloorToInt(FrameTime * FrameRate);

    // 드롭 프레임 타임코드 계산 (29.97fps, 59.94fps 등에서 사용)
    if (bUseDropFrame)
    {
        // 드롭 프레임 타임코드 계산 로직 (간소화됨)
        // 실제 구현에서는 더 복잡한 드롭 프레임 계산 로직이 필요
    }

    // 타임코드 문자열 생성 (HH:MM:SS:FF 형식)
    FString Delimiter = bUseDropFrame ? ";" : ":";
    return FString::Printf(TEXT("%02d:%02d:%02d%s%02d"), Hours, Minutes, Seconds, *Delimiter, Frames);
}

float UTimecodeUtils::TimecodeToSeconds(const FString& Timecode, float FrameRate, bool bUseDropFrame)
{
    // 타임코드 포맷 검증
    TArray<FString> TimeParts;
    if (Timecode.ParseIntoArray(TimeParts, TEXT(":;"), true) != 4)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid timecode format: %s"), *Timecode);
        return 0.0f;
    }

    // 시, 분, 초, 프레임 파싱
    int32 Hours = FCString::Atoi(*TimeParts[0]);
    int32 Minutes = FCString::Atoi(*TimeParts[1]);
    int32 Seconds = FCString::Atoi(*TimeParts[2]);
    int32 Frames = FCString::Atoi(*TimeParts[3]);

    // 시간(초) 계산
    float TotalSeconds = Hours * 3600.0f + Minutes * 60.0f + Seconds + (Frames / FrameRate);

    // 드롭 프레임 보정 (필요한 경우)
    if (bUseDropFrame)
    {
        // 드롭 프레임 보정 로직 (간소화됨)
    }

    return TotalSeconds;
}

FString UTimecodeUtils::GetCurrentSystemTimecode(float FrameRate, bool bUseDropFrame)
{
    // 현재 시스템 시간 가져오기
    FDateTime CurrentTime = FDateTime::Now();

    // 자정 기준 경과 시간(초) 계산
    float SecondsSinceMidnight =
        CurrentTime.GetHour() * 3600.0f +
        CurrentTime.GetMinute() * 60.0f +
        CurrentTime.GetSecond() +
        CurrentTime.GetMillisecond() / 1000.0f;

    // 타임코드 변환
    return SecondsToTimecode(SecondsSinceMidnight, FrameRate, bUseDropFrame);
}