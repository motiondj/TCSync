#include "TimecodeSettings.h"

UTimecodeSettings::UTimecodeSettings()
{
    // 기본값 설정
    FrameRate = 30.0f;
    bUseDropFrameTimecode = false;
    DefaultUDPPort = 10000;
    MulticastGroupAddress = "239.0.0.1";
    BroadcastInterval = 0.033f; // 약 30Hz

    // 역할 관련 기본 설정
    RoleMode = ETimecodeRoleMode::Automatic;
    bIsManualMaster = false;
    MasterIPAddress = "";

    // nDisplay 관련 기본 설정
    bEnableNDisplayIntegration = false;
    bUseNDisplayRoleAssignment = true;

    // 고급 설정 기본값
    bAutoStartTimecode = true;
    bEnablePacketLossCompensation = true;
    bEnableNetworkLatencyCompensation = true;
    ConnectionCheckInterval = 1.0f;
}

FName UTimecodeSettings::GetCategoryName() const
{
    return FName("Plugins");
}

FText UTimecodeSettings::GetSectionText() const
{
    return FText::FromString("Timecode Sync");
}

FText UTimecodeSettings::GetSectionDescription() const
{
    return FText::FromString("Configure settings for the Timecode Sync plugin.");
}

#if WITH_EDITOR
void UTimecodeSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // 속성 변경 시 유효성 검사 및 로직 처리
    const FName PropertyName = (PropertyChangedEvent.Property != nullptr)
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    // 프레임 레이트 유효성 검사
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UTimecodeSettings, FrameRate))
    {
        // 프레임 레이트가 너무 낮거나 높으면 경고
        if (FrameRate < 1.0f || FrameRate > 240.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("FrameRate should be between 1.0 and 240.0"));
            FrameRate = FMath::Clamp(FrameRate, 1.0f, 240.0f);
        }

        // 드롭 프레임 타임코드는 주로 29.97fps 또는 59.94fps에서 사용됨
        if (bUseDropFrameTimecode)
        {
            if (!FMath::IsNearlyEqual(FrameRate, 29.97f) && !FMath::IsNearlyEqual(FrameRate, 59.94f))
            {
                UE_LOG(LogTemp, Warning, TEXT("Drop frame timecode is typically used with 29.97fps or 59.94fps"));
            }
        }
    }

    // 드롭 프레임 타임코드 설정 변경 시
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTimecodeSettings, bUseDropFrameTimecode))
    {
        if (bUseDropFrameTimecode)
        {
            // 드롭 프레임은 주로 29.97fps 또는 59.94fps에서 사용됨
            if (!FMath::IsNearlyEqual(FrameRate, 29.97f) && !FMath::IsNearlyEqual(FrameRate, 59.94f))
            {
                UE_LOG(LogTemp, Warning, TEXT("Drop frame timecode is typically used with 29.97fps or 59.94fps"));
            }
        }
    }

    // 역할 모드 변경 시
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTimecodeSettings, RoleMode))
    {
        if (RoleMode == ETimecodeRoleMode::Manual)
        {
            UE_LOG(LogTemp, Log, TEXT("Switched to manual role mode"));

            // 수동 모드에서 마스터 IP가 비어 있고 슬레이브로 설정된 경우 경고
            if (!bIsManualMaster && MasterIPAddress.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("Manual slave mode requires a Master IP address"));
            }
        }
        else // Automatic
        {
            UE_LOG(LogTemp, Log, TEXT("Switched to automatic role mode"));
        }
    }

    // 수동 마스터 설정 변경 시
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTimecodeSettings, bIsManualMaster))
    {
        if (RoleMode == ETimecodeRoleMode::Manual)
        {
            UE_LOG(LogTemp, Log, TEXT("Manual master setting changed to: %s"),
                bIsManualMaster ? TEXT("MASTER") : TEXT("SLAVE"));

            // 슬레이브로 변경됐는데 마스터 IP가 비어 있으면 경고
            if (!bIsManualMaster && MasterIPAddress.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("Manual slave mode requires a Master IP address"));
            }
        }
    }

    // 마스터 IP 주소 변경 시
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTimecodeSettings, MasterIPAddress))
    {
        if (RoleMode == ETimecodeRoleMode::Manual && !bIsManualMaster)
        {
            if (MasterIPAddress.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("Master IP address cannot be empty in manual slave mode"));
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("Master IP address set to: %s"), *MasterIPAddress);

                // 간단한 IP 형식 검증 (더 엄격한 검증이 필요할 수 있음)
                TArray<FString> IPParts;
                MasterIPAddress.ParseIntoArray(IPParts, TEXT("."), true);

                if (IPParts.Num() != 4)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Invalid IP address format: %s"), *MasterIPAddress);
                }
            }
        }
    }
}
#endif