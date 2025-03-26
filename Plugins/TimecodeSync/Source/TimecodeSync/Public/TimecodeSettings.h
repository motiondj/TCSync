#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TimecodeNetworkTypes.h"
#include "TimecodeSettings.generated.h"

/**
 * Class managing settings for the timecode synchronization plugin
 */
UCLASS(config = TimecodeSync, defaultconfig)
class TIMECODESYNC_API UTimecodeSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UTimecodeSettings();

    /** Timecode Settings */

    // Frame rate setting
    UPROPERTY(config, EditAnywhere, Category = "Timecode", meta = (ClampMin = "1.0", ClampMax = "240.0"))
    float FrameRate;

    // Whether to use drop frame timecode
    UPROPERTY(config, EditAnywhere, Category = "Timecode")
    bool bUseDropFrameTimecode;

    /** Network Settings */

    // Default UDP port
    UPROPERTY(config, EditAnywhere, Category = "Network", meta = (ClampMin = "1024", ClampMax = "65535"))
    int32 DefaultUDPPort;

    // Multicast group IP
    UPROPERTY(config, EditAnywhere, Category = "Network")
    FString MulticastGroupAddress;

    // Timecode transmission interval (in seconds)
    UPROPERTY(config, EditAnywhere, Category = "Network", meta = (ClampMin = "0.001", ClampMax = "1.0"))
    float BroadcastInterval;

    /** Role Settings */

    // Role determination mode
    UPROPERTY(config, EditAnywhere, Category = "Role")
    ETimecodeRoleMode RoleMode;

    // Master role flag in manual mode
    UPROPERTY(config, EditAnywhere, Category = "Role", meta = (EditCondition = "RoleMode==ETimecodeRoleMode::Manual"))
    bool bIsManualMaster;

    // Master IP address in manual slave mode
    UPROPERTY(config, EditAnywhere, Category = "Role", meta = (EditCondition = "RoleMode==ETimecodeRoleMode::Manual && !bIsManualMaster"))
    FString MasterIPAddress;

    /** nDisplay Settings */

    // Enable nDisplay integration
    UPROPERTY(config, EditAnywhere, Category = "Integration")
    bool bEnableNDisplayIntegration;

    // Use nDisplay node ID for role assignment (only in automatic mode)
    UPROPERTY(config, EditAnywhere, Category = "Integration", meta = (EditCondition = "bEnableNDisplayIntegration && RoleMode==ETimecodeRoleMode::Automatic"))
    bool bUseNDisplayRoleAssignment;

    /** Advanced Settings */

    // Auto start flag
    UPROPERTY(config, EditAnywhere, Category = "Advanced")
    bool bAutoStartTimecode;

    // Enable packet loss compensation
    UPROPERTY(config, EditAnywhere, Category = "Advanced")
    bool bEnablePacketLossCompensation;

    // Enable network latency compensation
    UPROPERTY(config, EditAnywhere, Category = "Advanced")
    bool bEnableNetworkLatencyCompensation;

    // 전용 타임코드 마스터 서버 설정
    UPROPERTY(config, EditAnywhere, Category = "Advanced", meta = (DisplayName = "Dedicated Master Server"))
    bool bIsDedicatedMaster;

    // Connection status check interval (in seconds)
    UPROPERTY(config, EditAnywhere, Category = "Advanced", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float ConnectionCheckInterval;

    /** PLL Settings */

    // Enable PLL for timecode synchronization
    UPROPERTY(config, EditAnywhere, Category = "Advanced")
    bool bEnablePLL;

    // PLL bandwidth (responsiveness)
    UPROPERTY(config, EditAnywhere, Category = "Advanced", meta = (EditCondition = "bEnablePLL", ClampMin = "0.01", ClampMax = "1.0"))
    float PLLBandwidth;

    // PLL damping factor (stability)
    UPROPERTY(config, EditAnywhere, Category = "Advanced", meta = (EditCondition = "bEnablePLL", ClampMin = "0.1", ClampMax = "2.0"))
    float PLLDamping;

public:
    // UDeveloperSettings interface
    virtual FName GetCategoryName() const override;
    virtual FText GetSectionText() const;
    virtual FText GetSectionDescription() const override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};