#include "TimecodeUtils.h"
#include "Misc/DateTime.h"
#include "Misc/Parse.h"

// SMPTE 드롭 프레임 타임코드 계산을 위한 헬퍼 함수
static void CalculateDropFrameTimecode(double TotalSeconds, double FrameRate, bool bIsDropFrame,
    int32& OutHours, int32& OutMinutes, int32& OutSeconds, int32& OutFrames)
{
    OutHours = 0;
    OutMinutes = 0;
    OutSeconds = 0;
    OutFrames = 0;

    if (!bIsDropFrame || (FrameRate != 29.97 && FrameRate != 59.94))
    {
        // 일반 타임코드 계산
        int32 TotalFrames = FMath::RoundToInt(TotalSeconds * FrameRate);

        int32 FramesPerHour = FMath::RoundToInt(FrameRate * 3600);
        int32 FramesPerMinute = FMath::RoundToInt(FrameRate * 60);
        int32 FramesPerSecond = FMath::RoundToInt(FrameRate);

        OutHours = TotalFrames / FramesPerHour;
        TotalFrames %= FramesPerHour;

        OutMinutes = TotalFrames / FramesPerMinute;
        TotalFrames %= FramesPerMinute;

        OutSeconds = TotalFrames / FramesPerSecond;
        OutFrames = TotalFrames % FramesPerSecond;

        return;
    }

    // 드롭 프레임 계산 (SMPTE 표준)
    int32 DropFrames = (FrameRate == 29.97) ? 2 : 4; // 29.97fps에서는 2프레임, 59.94fps에서는 4프레임을 드롭

    // 더 정확한 계산을 위해 실제 프레임 속도를 사용
    double ActualFrameRate = (FrameRate == 29.97) ? 30.0 * 1000.0 / 1001.0 : 60.0 * 1000.0 / 1001.0;

    // 총 프레임 수 계산
    int64 TotalFrames = FMath::RoundToInt(TotalSeconds * ActualFrameRate);

    // SMPTE 드롭 프레임 수식
    // 매 분마다 첫 2(또는 4)개 프레임을 드롭하되, 10분 단위에서는 제외
    int64 FramesPerMinute = FMath::RoundToInt(ActualFrameRate * 60);
    int64 FramesPerTenMinutes = FramesPerMinute * 10;
    int64 FramesPerHour = FramesPerMinute * 60;

    // 10분 단위 블록과 나머지 분 계산
    int64 TenMinuteBlocks = TotalFrames / FramesPerTenMinutes;
    int64 RemainingFrames = TotalFrames % FramesPerTenMinutes;

    // 첫 번째 분을 초과하는 나머지 분 계산 (첫 분에는 드롭하지 않음)
    int64 ExtraMinutes = 0;
    if (RemainingFrames >= FramesPerMinute)
    {
        ExtraMinutes = (RemainingFrames - FramesPerMinute) / (FramesPerMinute - DropFrames) + 1;

        // 마지막 부분 체크 (정확히 분 경계에 있지 않은 경우)
        int64 Check = FramesPerMinute + (ExtraMinutes - 1) * (FramesPerMinute - DropFrames) + DropFrames;
        if (RemainingFrames < Check)
            ExtraMinutes--;
    }

    // 타임코드 조정을 위한 총 드롭 프레임 계산
    int64 TotalMinutes = TenMinuteBlocks * 10 + ExtraMinutes;
    int64 TotalDroppedFrames = DropFrames * (TotalMinutes - TenMinuteBlocks);
    int64 AdjustedFrames = TotalFrames + TotalDroppedFrames;

    // 시간, 분, 초, 프레임으로 변환
    OutHours = int32(AdjustedFrames / FramesPerHour);
    AdjustedFrames %= FramesPerHour;

    OutMinutes = int32(AdjustedFrames / FramesPerMinute);
    AdjustedFrames %= FramesPerMinute;

    // 하나의 변수를 사용하여 프레임 레이트를 계산
    int32 FramesPerSecond = FMath::RoundToInt(ActualFrameRate);
    OutSeconds = int32(AdjustedFrames / FramesPerSecond);
    OutFrames = int32(AdjustedFrames % FramesPerSecond);

    // 드롭 프레임 타임코드의 특수 규칙: 00:00:00이 아니고, 매 분 시작 시점인데 10분 배수가 아니면, 
    // 첫 2(또는 4)개의 프레임을 건너뜁니다.
    if (OutSeconds == 0 && OutFrames < DropFrames && OutMinutes % 10 != 0)
    {
        OutFrames = DropFrames;
    }
}

// 드롭 프레임 타임코드에서 초로 변환하는 헬퍼 함수
static double DropFrameTimecodeToSeconds(int32 Hours, int32 Minutes, int32 Seconds, int32 Frames, double FrameRate)
{
    // 더 정확한 계산을 위해 실제 프레임 속도 사용
    double ActualFrameRate = (FMath::IsNearlyEqual(FrameRate, 29.97, 0.01)) ?
        30.0 * 1000.0 / 1001.0 : 60.0 * 1000.0 / 1001.0;

    int32 DropFrames = (FMath::IsNearlyEqual(FrameRate, 29.97, 0.01)) ? 2 : 4;

    // 프레임 단위 변환 상수
    int32 FramesPerHour = FMath::RoundToInt(ActualFrameRate * 3600);
    int32 FramesPerMinute = FMath::RoundToInt(ActualFrameRate * 60);
    int32 FramesPerSecond = FMath::RoundToInt(ActualFrameRate);
    int32 FramesPerTenMinutes = FMath::RoundToInt(ActualFrameRate * 600) - DropFrames * 9;

    // 총 10분 단위와 나머지 분 계산
    int32 TenMinutesCount = (Hours * 6) + (Minutes / 10);
    int32 RemainingMinutes = Minutes % 10;

    // 타임코드를 프레임으로 변환 (드롭 프레임 조정 적용)
    int64 TotalFrames = 0;

    // 10분 단위 블록 처리
    TotalFrames += TenMinutesCount * FramesPerTenMinutes;

    // 나머지 분에 대한 프레임 계산 (첫 분에는 드롭하지 않음)
    if (RemainingMinutes > 0)
    {
        // 첫 분의 모든 프레임
        TotalFrames += FramesPerMinute;

        // 나머지 분에 대해 드롭 프레임 적용
        TotalFrames += (RemainingMinutes - 1) * (FramesPerMinute - DropFrames);
    }

    // 초 및 프레임 추가
    TotalFrames += Seconds * FramesPerSecond + Frames;

    // 프레임을 초로 변환
    double TotalSeconds = static_cast<double>(TotalFrames) / ActualFrameRate;

    return TotalSeconds;
}

FString UTimecodeUtils::SecondsToTimecode(float TimeInSeconds, float FrameRate, bool bUseDropFrame)
{
    // 음수 시간 처리
    TimeInSeconds = FMath::Max(0.0f, TimeInSeconds);

    // 유효한 프레임 레이트 확인
    if (FrameRate <= 0.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid frame rate: %f, using default 30fps"), FrameRate);
        FrameRate = 30.0f;
    }

    // 드롭 프레임은 29.97fps와 59.94fps에만 적용
    bool bIsDropFrame = bUseDropFrame && (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f) ||
        FMath::IsNearlyEqual(FrameRate, 59.94f, 0.01f));

    // 시간, 분, 초, 프레임 계산
    int32 Hours, Minutes, Seconds, Frames;

    // SMPTE 표준에 따른 타임코드 계산
    CalculateDropFrameTimecode(TimeInSeconds, FrameRate, bIsDropFrame, Hours, Minutes, Seconds, Frames);

    // 드롭 프레임 타임코드 포맷 (세미콜론 사용)
    return FString::Printf(TEXT("%02d:%02d:%02d%c%02d"),
        Hours, Minutes, Seconds,
        bIsDropFrame ? TEXT(';') : TEXT(':'),
        Frames);
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

    // 정규 표현식보다 간단한 형식 검사 및 파싱 사용
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

    // SMPTE 드롭 프레임 타임코드 계산
    double TotalSeconds = 0.0;

    if (bIsDropFrame && (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f) || FMath::IsNearlyEqual(FrameRate, 59.94f, 0.01f)))
    {
        TotalSeconds = DropFrameTimecodeToSeconds(Hours, Minutes, Seconds, Frames, FrameRate);
    }
    else
    {
        // 일반 타임코드 계산
        double FrameSeconds = (double)Frames / FrameRate;
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