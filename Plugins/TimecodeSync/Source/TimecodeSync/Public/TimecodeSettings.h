#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TimecodeNetworkTypes.h"
#include "TimecodeSettings.generated.h"

/**
 * 타임코드 동기화 플러그인의 설정을 관리하는 클래스
 */
UCLASS(config = TimecodeSync, defaultconfig)
class TIMECODESYNC_API UTimecodeSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UTimecodeSettings();

    /** 타임코드 관련 설정 */

    // 프레임 레이트 설정
    UPROPERTY(config, EditAnywhere, Category = "Timecode", meta = (ClampMin = "1.0", ClampMax = "240.0"))
    float FrameRate;

    // 드롭 프레임 타임코드 사용 여부
    UPROPERTY(config, EditAnywhere, Category = "Timecode")
    bool bUseDropFrameTimecode;

    /** 네트워크 관련 설정 */

    // 기본 UDP 포트
    UPROPERTY(config, EditAnywhere, Category = "Network", meta = (ClampMin = "1024", ClampMax = "65535"))
    int32 DefaultUDPPort;

    // 멀티캐스트 그룹 IP
    UPROPERTY(config, EditAnywhere, Category = "Network")
    FString MulticastGroupAddress;

    // 타임코드 전송 간격 (초 단위)
    UPROPERTY(config, EditAnywhere, Category = "Network", meta = (ClampMin = "0.001", ClampMax = "1.0"))
    float BroadcastInterval;

    /** 역할 관련 설정 */

    // 역할 결정 모드
    UPROPERTY(config, EditAnywhere, Category = "Role")
    ETimecodeRoleMode RoleMode;

    // 수동 모드에서 마스터 역할 여부
    UPROPERTY(config, EditAnywhere, Category = "Role", meta = (EditCondition = "RoleMode==ETimecodeRoleMode::Manual"))
    bool bIsManualMaster;

    // 수동 슬레이브 모드에서 마스터 IP 주소
    UPROPERTY(config, EditAnywhere, Category = "Role", meta = (EditCondition = "RoleMode==ETimecodeRoleMode::Manual && !bIsManualMaster"))
    FString MasterIPAddress;

    /** nDisplay 관련 설정 */

    // nDisplay 통합 활성화 여부
    UPROPERTY(config, EditAnywhere, Category = "Integration")
    bool bEnableNDisplayIntegration;

    // nDisplay 노드ID 기반 역할 결정 (자동 모드에서만 사용)
    UPROPERTY(config, EditAnywhere, Category = "Integration", meta = (EditCondition = "bEnableNDisplayIntegration && RoleMode==ETimecodeRoleMode::Automatic"))
    bool bUseNDisplayRoleAssignment;

    /** 고급 설정 */

    // 자동 시작 여부
    UPROPERTY(config, EditAnywhere, Category = "Advanced")
    bool bAutoStartTimecode;

    // 패킷 손실 보정 활성화
    UPROPERTY(config, EditAnywhere, Category = "Advanced")
    bool bEnablePacketLossCompensation;

    // 네트워크 지연 보정 활성화
    UPROPERTY(config, EditAnywhere, Category = "Advanced")
    bool bEnableNetworkLatencyCompensation;

    // 연결 상태 확인 주기 (초 단위)
    UPROPERTY(config, EditAnywhere, Category = "Advanced", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float ConnectionCheckInterval;

public:
    // UDeveloperSettings 인터페이스
    virtual FName GetCategoryName() const override;
    virtual FText GetSectionText() const override;
    virtual FText GetSectionDescription() const override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};