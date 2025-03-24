#include "TimecodeUtils.h"
#include "Misc/DateTime.h"
#include "Misc/Parse.h"

// 더 정확한 SMPTE 드롭 프레임 계산을 위한 상수
// 29.97 = 30 * 1000/1001, 59.94 = 60 * 1000/1001
constexpr double FRAMERATE_29_97 = 30.0 * 1000.0 / 1001.0;
constexpr double FRAMERATE_59_94 = 60.0 * 1000.0 / 1001.0;

FString UTimecodeUtils::SecondsToTimecode(float TimeInSeconds, float FrameRate, bool bUseDropFrame)
{
    // 음수 시간 처리
    TimeInSeconds = FMath::Max(0.0f, TimeInSeconds);

    // 표준 드롭 프레임 계산 확인
    bool bIsDropFrame = bUseDropFrame &&
        (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f) ||
            FMath::IsNearlyEqual(FrameRate, 59.94f, 0.01f));

    // 로깅을 위한 원본 값 저장
    float OriginalTime = TimeInSeconds;

    // 표준 비-드롭 프레임 계산
    if (!bIsDropFrame)
    {
        // 시간, 분, 초, 프레임 계산
        int32 Hours = FMath::FloorToInt(TimeInSeconds / 3600.0f);
        int32 Minutes = FMath::FloorToInt((TimeInSeconds - Hours * 3600.0f) / 60.0f);
        int32 Seconds = FMath::FloorToInt(TimeInSeconds - Hours * 3600.0f - Minutes * 60.0f);

        float FrameTime = TimeInSeconds - FMath::FloorToInt(TimeInSeconds);
        int32 Frames = FMath::FloorToInt(FrameTime * FrameRate);

        // 표준 타임코드 문자열 생성 (HH:MM:SS:FF)
        return FString::Printf(TEXT("%02d:%02d:%02d:%02d"), Hours, Minutes, Seconds, Frames);
    }

    // 여기서부터 드롭 프레임 타임코드 계산
    double ActualFrameRate = (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f)) ? FRAMERATE_29_97 : FRAMERATE_59_94;
    int32 DropFrames = (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f)) ? 2 : 4;

    // 총 프레임 수 계산
    int64 TotalFrames = static_cast<int64>(TimeInSeconds * ActualFrameRate + 0.5); // 반올림

    // SMPTE 드롭 프레임 계산을 위한 상수
    const int64 FramesPerMinute = static_cast<int64>(ActualFrameRate * 60);
    const int64 FramesPerTenMinutes = static_cast<int64>(ActualFrameRate * 600) - (DropFrames * 9);
    const int64 FramesPerHour = FramesPerTenMinutes * 6;

    // 드롭 프레임 타임코드 계산
    int64 D = TotalFrames / FramesPerTenMinutes;
    int64 M = TotalFrames % FramesPerTenMinutes;

    // 드롭 프레임 조정
    TotalFrames += DropFrames * (D * 9 + (M - M / FramesPerMinute) / (FramesPerMinute / DropFrames));

    // 수정된 프레임에서 시간, 분, 초, 프레임 계산
    int32 Frames = static_cast<int32>(TotalFrames % static_cast<int64>(ActualFrameRate));
    int32 Seconds = static_cast<int32>((TotalFrames / static_cast<int64>(ActualFrameRate)) % 60);
    int32 Minutes = static_cast<int32>(((TotalFrames / static_cast<int64>(ActualFrameRate)) / 60) % 60);
    int32 Hours = static_cast<int32>((TotalFrames / static_cast<int64>(ActualFrameRate)) / 3600);

    // 드롭 프레임 타임코드 포맷 (세미콜론 사용)
    FString Result = FString::Printf(TEXT("%02d:%02d:%02d;%02d"), Hours, Minutes, Seconds, Frames);

    UE_LOG(LogTemp, Verbose, TEXT("SecondsToTimecode: %.6fs at %.2ffps (DropFrame=%d) -> %s"),
        OriginalTime, FrameRate, bIsDropFrame ? 1 : 0, *Result);

    return Result;
}

float UTimecodeUtils::TimecodeToSeconds(const FString& Timecode, float FrameRate, bool bUseDropFrame)
{
    // 유효한 프레임 레이트 확인
    if (FrameRate <= 0.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid frame rate: %f, using default 30fps"), FrameRate);
        FrameRate = 30.0f;
    }

    // 공백 제거 및 타임코드 정규화
    FString CleanTimecode = Timecode.TrimStartAndEnd();

    // 세미콜론 확인으로 드롭 프레임 여부 감지
    bool bHasSemicolon = CleanTimecode.Contains(TEXT(";"));
    bool bIsDropFrame = bUseDropFrame || bHasSemicolon;

    // 드롭 프레임 플래그와 프레임 레이트 일관성 확인
    if (bIsDropFrame)
    {
        // 드롭 프레임은 29.97fps와 59.94fps에만 적용
        if (!FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f) && !FMath::IsNearlyEqual(FrameRate, 59.94f, 0.01f))
        {
            // 드롭 프레임에 가장 가까운 적절한 프레임 레이트로 조정
            FrameRate = 29.97f;
            UE_LOG(LogTemp, Warning, TEXT("Adjusted frame rate to 29.97fps for drop frame timecode"));
        }
    }

    // 타임코드 구문 분석을 위한 표준화
    FString NormalizedTimecode = CleanTimecode;
    if (bHasSemicolon)
    {
        NormalizedTimecode = CleanTimecode.Replace(TEXT(";"), TEXT(":"));
    }

    // 타임코드 구분 및 파싱
    int32 Hours = 0, Minutes = 0, Seconds = 0, Frames = 0;

    // 타임코드 파싱 (00:00:00:00 형식)
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
        // 대체 파싱 시도
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
            UE_LOG(LogTemp, Warning, TEXT("Invalid timecode format: %s"), *CleanTimecode);
            return 0.0f;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Parsed timecode %s into H:%d M:%d S:%d F:%d"),
        *CleanTimecode, Hours, Minutes, Seconds, Frames);

    double TotalSeconds = 0.0;

    if (bIsDropFrame && (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f) || FMath::IsNearlyEqual(FrameRate, 59.94f, 0.01f)))
    {
        // 드롭 프레임 타임코드를 초로 변환
        double ActualFrameRate = (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f)) ? FRAMERATE_29_97 : FRAMERATE_59_94;
        int32 DropFrames = (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f)) ? 2 : 4;

        // SMPTE 드롭 프레임 계산을 위한 상수
        const int64 FramesPerMinute = static_cast<int64>(ActualFrameRate * 60);
        const int64 FramesPerTenMinutes = static_cast<int64>(ActualFrameRate * 600) - (DropFrames * 9);
        const int64 FramesPerHour = FramesPerTenMinutes * 6;

        // 총 프레임 계산
        int64 TotalFrames = 0;

        // 시간별 계산
        TotalFrames += Hours * FramesPerHour;

        // 10분 단위 계산
        int32 TenMinuteChunks = Minutes / 10;
        TotalFrames += TenMinuteChunks * FramesPerTenMinutes;

        // 나머지 분 계산 (1~9)
        int32 RemainingMinutes = Minutes % 10;
        if (RemainingMinutes > 0)
        {
            // 첫번째 분은 드롭하지 않음
            TotalFrames += FramesPerMinute;

            // 나머지 분은 각각 DropFrames씩 프레임 건너뜀
            if (RemainingMinutes > 1)
            {
                TotalFrames += (RemainingMinutes - 1) * (FramesPerMinute - DropFrames);
            }
        }

        // 초 및 프레임 추가
        TotalFrames += Seconds * static_cast<int64>(ActualFrameRate);
        TotalFrames += Frames;

        // 프레임 수를 초로 변환
        TotalSeconds = static_cast<double>(TotalFrames) / ActualFrameRate;
    }
    else
    {
        // 일반 타임코드 계산
        double FrameSeconds = static_cast<double>(Frames) / FrameRate;
        TotalSeconds = Hours * 3600.0 + Minutes * 60.0 + Seconds + FrameSeconds;
    }

    UE_LOG(LogTemp, Log, TEXT("Calculated seconds for %s: %.6f"), *CleanTimecode, TotalSeconds);

    return static_cast<float>(TotalSeconds);
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