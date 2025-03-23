#include "TimecodeNetworkManager.h"
#include "TimecodeUtils.h"

// 로그 카테고리 정의
DEFINE_LOG_CATEGORY_STATIC(LogTimecodeNetwork, Log, All);

// PLL 초기화 메서드 구현
void UTimecodeNetworkManager::InitializePLL(float InBandwidth, float InDamping)
{
    if (!PLL)
    {
        PLL = NewObject<UTimecodePLL>(this);
        PLL->AddToRoot();
    }

    PLL->Initialize(InBandwidth, InDamping);
    bUsePLL = true;

    UE_LOG(LogTimecodeNetwork, Display, TEXT("PLL initialized with Bandwidth=%.3f, Damping=%.3f"),
        InBandwidth, InDamping);
}

// PLL 업데이트 메서드 구현
void UTimecodeNetworkManager::UpdatePLL(const FString& MasterTimecode, double ReceiveTime)
{
    if (!PLL || !bUsePLL)
    {
        return;
    }

    // 마스터 타임코드를 초 단위로 변환
    float FrameRate = 30.0f; // 기본값, 실제로는 메시지에서 프레임 레이트 추출 필요
    bool bIsDropFrame = MasterTimecode.Contains(TEXT(";"));

    if (bIsDropFrame)
    {
        FrameRate = 29.97f; // 드롭 프레임 타임코드면 29.97fps로 가정
    }

    double MasterTimeSeconds = TimecodeToSeconds(MasterTimecode, FrameRate);

    // 로컬 시간과 마스터 시간 차이로 PLL 업데이트
    PLL->Update(MasterTimeSeconds, ReceiveTime, 0.033f); // 0.033 = 약 30fps 가정

    // 상태 정보 저장
    LastMasterTimecode = MasterTimecode;
    LastReceiveTime = ReceiveTime;

    // 디버깅을 위한 로깅
    if (FMath::RandBool() && FMath::RandRange(0, 20) == 0) // 때때로만 로그 출력 (과도한 로그 방지)
    {
        UE_LOG(LogTimecodeNetwork, Verbose, TEXT("PLL Update: Master=%s, Error=%.6fs, Ratio=%.6f, Locked=%s"),
            *MasterTimecode,
            PLL->GetPhaseError(),
            PLL->GetFrequencyRatio(),
            PLL->IsLocked() ? TEXT("True") : TEXT("False"));
    }
}

// PLL 보정 시간 가져오기
double UTimecodeNetworkManager::GetPLLCorrectedTime() const
{
    if (!PLL || !bUsePLL)
    {
        return FPlatformTime::Seconds();
    }

    return PLL->GetCorrectedTime(FPlatformTime::Seconds());
}

// 타임코드를 초 단위로 변환
double UTimecodeNetworkManager::TimecodeToSeconds(const FString& Timecode, float FrameRate) const
{
    // 타임코드 포맷: HH:MM:SS:FF 또는 HH:MM:SS;FF (드롭 프레임)
    FString CleanTimecode = Timecode;
    TArray<FString> TimeParts;

    // 드롭 프레임 세미콜론을 콜론으로 변환하여 처리
    if (CleanTimecode.Replace(TEXT(";"), TEXT(":")).ParseIntoArray(TimeParts, TEXT(":"), false) != 4)
    {
        UE_LOG(LogTimecodeNetwork, Warning, TEXT("Invalid timecode format: %s"), *Timecode);
        return 0.0;
    }

    int32 Hours = FCString::Atoi(*TimeParts[0]);
    int32 Minutes = FCString::Atoi(*TimeParts[1]);
    int32 Seconds = FCString::Atoi(*TimeParts[2]);
    int32 Frames = FCString::Atoi(*TimeParts[3]);

    // 프레임 수 검증
    int32 MaxFrames = FMath::RoundToInt(FrameRate);
    if (Frames >= MaxFrames)
    {
        Frames = MaxFrames - 1;
    }

    // 시, 분, 초, 프레임을 초 단위로 변환
    double TotalSeconds = Hours * 3600.0 + Minutes * 60.0 + Seconds + Frames / FrameRate;

    // 드롭 프레임 보정 (29.97fps 드롭 프레임인 경우)
    bool bIsDropFrame = Timecode.Contains(TEXT(";"));
    if (bIsDropFrame && FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f))
    {
        // 매 분마다 2프레임 삭제 (10분 단위 제외)
        int32 TotalMinutes = Hours * 60 + Minutes;
        int32 DroppedFrames = TotalMinutes - TotalMinutes / 10;
        TotalSeconds -= DroppedFrames / FrameRate;
    }

    return TotalSeconds;
}

// 초를 타임코드 문자열로 변환
FString UTimecodeNetworkManager::SecondsToTimecode(double Seconds, float FrameRate, bool bDropFrame) const
{
    // 음수 시간 처리
    Seconds = FMath::Max(0.0, Seconds);

    // 드롭 프레임 보정 (29.97fps인 경우)
    if (bDropFrame && FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f))
    {
        // 실제 경과 시간에서 드롭 프레임 보정
        int32 TotalFrames = FMath::RoundToInt(Seconds * FrameRate);
        int32 D = TotalFrames / (17982); // 10분당 프레임 수 (29.97 * 60 * 10 - 18)
        int32 M = TotalFrames % (17982);

        if (M < 2)
        {
            M = 0;
        }
        else if (M >= 2)
        {
            M = M - 2 * ((M - 2) / 1798 + 1);
        }

        TotalFrames = 17982 * D + M;

        // 다시 시간으로 변환
        Seconds = TotalFrames / FrameRate;
    }

    // 시, 분, 초, 프레임 계산
    int32 Hours = FMath::FloorToInt(Seconds / 3600.0);
    Seconds -= Hours * 3600.0;

    int32 Minutes = FMath::FloorToInt(Seconds / 60.0);
    Seconds -= Minutes * 60.0;

    int32 Secs = FMath::FloorToInt(Seconds);
    Seconds -= Secs;

    int32 Frames = FMath::RoundToInt(Seconds * FrameRate);

    // 프레임 오버플로우 처리
    int32 MaxFrames = FMath::RoundToInt(FrameRate);
    if (Frames >= MaxFrames)
    {
        Frames = 0;
        Secs++;

        // 초, 분, 시 오버플로우 처리
        if (Secs >= 60)
        {
            Secs = 0;
            Minutes++;

            if (Minutes >= 60)
            {
                Minutes = 0;
                Hours++;
            }
        }
    }

    // 타임코드 문자열 포맷팅
    FString Separator = bDropFrame ? TEXT(";") : TEXT(":");
    return FString::Printf(TEXT("%02d:%02d:%02d%s%02d"), Hours, Minutes, Secs, *Separator, Frames);
}

// PLL 잠금 상태 확인
bool UTimecodeNetworkManager::IsPLLLocked() const
{
    if (!PLL || !bUsePLL)
    {
        return false;
    }

    return PLL->IsLocked();
}

// PLL 위상 오차 가져오기
double UTimecodeNetworkManager::GetPLLPhaseError() const
{
    if (!PLL || !bUsePLL)
    {
        return 0.0;
    }

    return PLL->GetPhaseError();
}

// PLL 주파수 비율 가져오기
double UTimecodeNetworkManager::GetPLLFrequencyRatio() const
{
    if (!PLL || !bUsePLL)
    {
        return 1.0;
    }

    return PLL->GetFrequencyRatio();
}

// PLL 사용 설정
void UTimecodeNetworkManager::SetUsePLL(bool bEnable)
{
    bUsePLL = bEnable;

    if (bEnable && PLL)
    {
        PLL->Reset(); // PLL 상태 재설정
    }

    UE_LOG(LogTimecodeNetwork, Display, TEXT("PLL %s"), bEnable ? TEXT("Enabled") : TEXT("Disabled"));
}

// PLL 사용 여부 확인
bool UTimecodeNetworkManager::IsUsingPLL() const
{
    return bUsePLL && PLL != nullptr;
}