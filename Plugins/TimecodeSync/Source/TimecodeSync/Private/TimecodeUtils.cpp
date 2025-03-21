#include "TimecodeUtils.h"
#include "Misc/DateTime.h"
#include "Misc/Parse.h"

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
        // 드롭 프레임 타임코드 계산은 29.97fps 또는 59.94fps에서만 의미가 있음
        if (FMath::IsNearlyEqual(FrameRate, 29.97f) || FMath::IsNearlyEqual(FrameRate, 59.94f))
        {
            // SMPTE 드롭 프레임 계산
            // 29.97fps의 경우: 첫 2프레임 건너뜀 (매 분마다, 10분의 배수 제외)
            // 59.94fps의 경우: 첫 4프레임 건너뜀 (매 분마다, 10분의 배수 제외)

            // 전체 프레임 수 계산
            int32 TotalFrames = FMath::RoundToInt(TimeInSeconds * FrameRate);

            // 프레임 드롭 수 계산
            int32 DropFrames = FMath::IsNearlyEqual(FrameRate, 29.97f) ? 2 : 4;

            // 30fps 기준으로 시, 분, 초, 프레임 계산
            int32 NominalRate = FMath::RoundToInt(FrameRate);
            int32 FramesPerMinute = NominalRate * 60; // 1분당 프레임 수
            int32 FramesPerTenMinutes = FramesPerMinute * 10; // 10분당 프레임 수

            // 10분 단위로 나누어 계산
            int32 TenMinutePeriods = TotalFrames / FramesPerTenMinutes;
            int32 RemainingFrames = TotalFrames % FramesPerTenMinutes;

            // 남은 프레임에서 첫 1분 이후마다 프레임 드롭 적용
            int32 AdditionalDropFrames = 0;
            if (RemainingFrames >= FramesPerMinute) {
                int32 RemainingMinutes = (RemainingFrames - FramesPerMinute) / NominalRate / 60 + 1;
                // 10분 이내에서 첫 1분 이후부터 9분까지(최대 9회) 드롭 가능
                AdditionalDropFrames = DropFrames * FMath::Min(9, RemainingMinutes);
            }

            // 총 드롭 프레임 = 10분 단위 드롭 + 나머지 시간 드롭
            int32 TotalDropFrames = DropFrames * 9 * TenMinutePeriods + AdditionalDropFrames;

            // 드롭 프레임을 보정한 총 프레임
            int32 AdjustedFrames = TotalFrames + TotalDropFrames;

            // 시, 분, 초, 프레임으로 변환
            int32 FramesPerHour = FramesPerMinute * 60;
            Hours = AdjustedFrames / FramesPerHour;
            AdjustedFrames %= FramesPerHour;

            Minutes = AdjustedFrames / FramesPerMinute;
            AdjustedFrames %= FramesPerMinute;

            Seconds = AdjustedFrames / NominalRate;
            Frames = AdjustedFrames % NominalRate;

            // 매 분(10분 배수 제외)의 시작에서는 첫 프레임 번호가 드롭됨
            // 따라서 00:01:00;00 대신 00:01:00;02가 됨 (29.97fps)
            if (Seconds == 0 && Frames < DropFrames && Minutes % 10 != 0) {
                Frames = DropFrames;
            }
        }
    }

    // 타임코드 문자열 생성 (HH:MM:SS:FF 또는 HH:MM:SS;FF 형식)
    FString Delimiter = bUseDropFrame ? ";" : ":";
    return FString::Printf(TEXT("%02d:%02d:%02d%s%02d"), Hours, Minutes, Seconds, *Delimiter, Frames);
}

float UTimecodeUtils::TimecodeToSeconds(const FString& Timecode, float FrameRate, bool bUseDropFrame)
{
    // 프레임 레이트 유효성 검증
    if (FrameRate <= 0.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid frame rate: %f"), FrameRate);
        FrameRate = 30.0f; // 기본값으로 대체
    }

    // 공백 제거 및 타임코드 정규화
    FString CleanTimecode = Timecode.TrimStartAndEnd();

    // 타임코드 포맷 검증
    TArray<FString> TimeParts;
    if (CleanTimecode.ParseIntoArray(TimeParts, TEXT(":;"), true) != 4)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid timecode format: %s"), *CleanTimecode);

        // 수동 파싱 시도
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

            // 시간(초) 계산
            float FrameSeconds = (FrameRate > 0.0f) ? ((float)Frames / FrameRate) : 0.0f;
            float TotalSeconds = Hours * 3600.0f + Minutes * 60.0f + Seconds + FrameSeconds;

            UE_LOG(LogTemp, Log, TEXT("Calculated seconds: H:%f + M:%f + S:%f + F:%f = Total:%f"),
                Hours * 3600.0f, Minutes * 60.0f, (float)Seconds, FrameSeconds, TotalSeconds);

            return TotalSeconds;
        }

        return 0.0f;
    }

    // 시, 분, 초, 프레임 파싱
    int32 Hours = FCString::Atoi(*TimeParts[0]);
    int32 Minutes = FCString::Atoi(*TimeParts[1]);
    int32 Seconds = FCString::Atoi(*TimeParts[2]);
    int32 Frames = FCString::Atoi(*TimeParts[3]);

    // 파싱된 값 로그 출력
    UE_LOG(LogTemp, Log, TEXT("Parsed timecode %s into H:%d M:%d S:%d F:%d"),
        *CleanTimecode, Hours, Minutes, Seconds, Frames);

    // 드롭 프레임 타임코드 처리
    if (bUseDropFrame && (FMath::IsNearlyEqual(FrameRate, 29.97f) || FMath::IsNearlyEqual(FrameRate, 59.94f)))
    {
        int32 TotalMinutes = Hours * 60 + Minutes;
        int32 FramesToAdd = 0;

        // 10분마다의 예외를 제외하고 매 분마다 첫 2개(또는 4개) 프레임 보정
        if (FMath::IsNearlyEqual(FrameRate, 29.97f))
        {
            FramesToAdd = 2 * (TotalMinutes - TotalMinutes / 10);
        }
        else if (FMath::IsNearlyEqual(FrameRate, 59.94f))
        {
            FramesToAdd = 4 * (TotalMinutes - TotalMinutes / 10);
        }

        // 총 프레임 수에서 보정할 프레임 수 차감
        int32 TotalFrames = Hours * 3600 * (int32)FrameRate
            + Minutes * 60 * (int32)FrameRate
            + Seconds * (int32)FrameRate
            + Frames
            - FramesToAdd;

        // 초로 변환
        return TotalFrames / FrameRate;
    }
    else
    {
        // 시간(초) 계산 - 안전한 나눗셈 연산
        float FrameSeconds = (FrameRate > 0.0f) ? ((float)Frames / FrameRate) : 0.0f;
        float TotalSeconds = Hours * 3600.0f + Minutes * 60.0f + Seconds + FrameSeconds;

        UE_LOG(LogTemp, Log, TEXT("Calculated seconds: H:%f + M:%f + S:%f + F:%f = Total:%f"),
            Hours * 3600.0f, Minutes * 60.0f, (float)Seconds, FrameSeconds, TotalSeconds);

        return TotalSeconds;
    }
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