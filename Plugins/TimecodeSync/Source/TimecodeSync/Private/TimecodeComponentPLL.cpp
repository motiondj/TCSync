// TimecodeComponentPLL.cpp
#include "TimecodeComponent.h"
#include "TimecodePLL.h"

// PLL 관련 로그 카테고리 정의
DEFINE_LOG_CATEGORY_STATIC(LogTimecodeComponentPLL, Log, All);

// PLL 초기화 함수
void UTimecodeComponent::InitializePLLSettings()
{
    if (NetworkManager)
    {
        NetworkManager->InitializePLL(PLLBandwidth, PLLDamping);
        NetworkManager->SetUsePLL(bUsePLL);

        UE_LOG(LogTimecodeComponentPLL, Log, TEXT("[%s] PLL initialized with Bandwidth=%.3f, Damping=%.3f, Enabled=%s"),
            *GetOwner()->GetName(), PLLBandwidth, PLLDamping, bUsePLL ? TEXT("True") : TEXT("False"));
    }
}

// PLL 사용 설정
void UTimecodeComponent::SetUsePLL(bool bEnable)
{
    if (bUsePLL != bEnable)
    {
        bUsePLL = bEnable;

        if (NetworkManager)
        {
            NetworkManager->SetUsePLL(bEnable);

            UE_LOG(LogTimecodeComponentPLL, Log, TEXT("[%s] PLL %s"),
                *GetOwner()->GetName(), bEnable ? TEXT("Enabled") : TEXT("Disabled"));
        }
    }
}

// PLL 잠금 상태 확인
bool UTimecodeComponent::IsPLLLocked() const
{
    if (NetworkManager)
    {
        return NetworkManager->IsPLLLocked();
    }
    return false;
}

// PLL 위상 오차 확인
double UTimecodeComponent::GetPLLPhaseError() const
{
    if (NetworkManager)
    {
        return NetworkManager->GetPLLPhaseError();
    }
    return 0.0;
}

// PLL 주파수 비율 확인
double UTimecodeComponent::GetPLLFrequencyRatio() const
{
    if (NetworkManager)
    {
        return NetworkManager->GetPLLFrequencyRatio();
    }
    return 1.0;
}

// PLL 대역폭 설정
void UTimecodeComponent::SetPLLBandwidth(float Bandwidth)
{
    PLLBandwidth = FMath::Clamp(Bandwidth, 0.01f, 1.0f);

    if (NetworkManager && bUsePLL)
    {
        NetworkManager->InitializePLL(PLLBandwidth, PLLDamping);

        UE_LOG(LogTimecodeComponentPLL, Log, TEXT("[%s] PLL Bandwidth set to: %.3f"),
            *GetOwner()->GetName(), PLLBandwidth);
    }
}

// PLL 감쇠 계수 설정
void UTimecodeComponent::SetPLLDamping(float Damping)
{
    PLLDamping = FMath::Clamp(Damping, 0.1f, 2.0f);

    if (NetworkManager && bUsePLL)
    {
        NetworkManager->InitializePLL(PLLBandwidth, PLLDamping);

        UE_LOG(LogTimecodeComponentPLL, Log, TEXT("[%s] PLL Damping set to: %.3f"),
            *GetOwner()->GetName(), PLLDamping);
    }
}