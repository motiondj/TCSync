#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimecodeNetworkManager.h"
#include "TimecodeNetworkTypes.h"
#include "TimecodePLL.h"  // 추가됨
#include "TimecodeComponent.generated.h"

// Delegate declaration for timecode change event
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimecodeChanged, const FString&, NewTimecode);

// Delegate declaration for timecode event trigger
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimecodeEventTriggered, const FString&, EventName, float, EventTime);

// Delegate declaration for network connection state change
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNetworkConnectionChanged, ENetworkConnectionState, NewState);

// Delegate declaration for role change event
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoleChanged, bool, IsMaster);

/**
 * Component that provides timecode functionality when attached to an actor
 */
UCLASS(ClassGroup = "Timecode", meta = (BlueprintSpawnableComponent))
class TIMECODESYNC_API UTimecodeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTimecodeComponent();

    /** Role Settings */

    // Role determination mode setting
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode Role")
    ETimecodeRoleMode RoleMode;

    // Master flag in automatic mode (read-only)
    UPROPERTY(BlueprintReadOnly, Category = "Timecode Role")
    bool bIsMaster;

    // Master flag setting in manual mode
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode Role", meta = (EditCondition = "RoleMode==ETimecodeRoleMode::Manual", EditConditionHides))
    bool bIsManuallyMaster;

    // Master IP setting (used in manual slave mode)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode Role", meta = (EditCondition = "RoleMode==ETimecodeRoleMode::Manual && !bIsManuallyMaster", EditConditionHides))
    FString MasterIPAddress;

    // Whether to use nDisplay (only in automatic mode)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode Role", meta = (EditCondition = "RoleMode==ETimecodeRoleMode::Automatic", EditConditionHides))
    bool bUseNDisplay;

    /** Timecode Settings */

    // Frame rate setting
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode", meta = (ClampMin = "1.0", ClampMax = "240.0"))
    float FrameRate;

    // Whether to use drop frame timecode
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode")
    bool bUseDropFrameTimecode;

    // Auto start setting
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode")
    bool bAutoStart;

    /** Network Settings */

    // UDP port setting (for receiving messages)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network", meta = (DisplayName = "Receive Port", ClampMin = "1024", ClampMax = "65535"))
    int32 UDPPort;

    // UDP port for sending messages (target port)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network", meta = (DisplayName = "Send Port", ClampMin = "1024", ClampMax = "65535"))
    int32 TargetPortNumber;

    // Target IP setting (for unicast transmission in master mode) - 현재 사용 안함
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network", meta = (EditConditionHides, EditCondition = "false"))
    FString TargetIP;

    // Multicast group setting
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
    FString MulticastGroup;

    // Network synchronization interval (in seconds)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network", meta = (ClampMin = "0.001", ClampMax = "1.0"))
    float SyncInterval;

    /** PLL Settings */

    // PLL 사용 여부 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode|PLL", meta = (AdvancedDisplay))
    bool bUsePLL;

    // PLL 대역폭 설정 (반응성 조절)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode|PLL", meta = (AdvancedDisplay, EditCondition = "bUsePLL", EditConditionHides, ClampMin = "0.01", ClampMax = "1.0"))
    float PLLBandwidth;

    // PLL 감쇠 계수 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode|PLL", meta = (AdvancedDisplay, EditCondition = "bUsePLL", EditConditionHides, ClampMin = "0.1", ClampMax = "2.0"))
    float PLLDamping;

    /** Status and Statistics (Read-only) */

    // Current timecode (read-only)
    UPROPERTY(BlueprintReadOnly, Category = "Timecode")
    FString CurrentTimecode;

    // Network connection state (read-only)
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    ENetworkConnectionState ConnectionState;

    // Whether timecode is running (read-only)
    UPROPERTY(BlueprintReadOnly, Category = "Timecode")
    bool bIsRunning;

    /** Event Delegates */

    // Timecode change event
    UPROPERTY(BlueprintAssignable, Category = "Timecode")
    FOnTimecodeChanged OnTimecodeChanged;

    // Timecode event trigger
    UPROPERTY(BlueprintAssignable, Category = "Timecode")
    FOnTimecodeEventTriggered OnTimecodeEventTriggered;

    // Network connection state change event
    UPROPERTY(BlueprintAssignable, Category = "Network")
    FOnNetworkConnectionChanged OnNetworkConnectionChanged;

    // Role change event
    UPROPERTY(BlueprintAssignable, Category = "Timecode Role")
    FOnRoleChanged OnRoleChanged;

    // Role mode change event
    UPROPERTY(BlueprintAssignable, Category = "Timecode Role")
    FRoleModeChangedDelegate OnRoleModeChanged;

protected:
    // Called when component is initialized
    virtual void BeginPlay() override;

    // Called when component is removed
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Called every frame
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    /** Timecode Control Functions */

    // Start timecode
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void StartTimecode();

    // Stop timecode
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void StopTimecode();

    // Reset timecode
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void ResetTimecode();

    // Get current timecode
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    FString GetCurrentTimecode() const;

    // Get current time in seconds
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    float GetCurrentTimeInSeconds() const;

    /** Timecode Event Functions */

    // Register timecode event
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void RegisterTimecodeEvent(const FString& EventName, float EventTimeInSeconds);

    // Unregister timecode event
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void UnregisterTimecodeEvent(const FString& EventName);

    // Clear all timecode events
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void ClearAllTimecodeEvents();

    // Reset event triggers
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void ResetEventTriggers();

    /** Network Functions */

    // Get network connection state
    UFUNCTION(BlueprintCallable, Category = "Network")
    ENetworkConnectionState GetNetworkConnectionState() const;

    // Setup network connection
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool SetupNetwork();

    // Shutdown network connection
    UFUNCTION(BlueprintCallable, Category = "Network")
    void ShutdownNetwork();

    // Join multicast group
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool JoinMulticastGroup(const FString& InMulticastGroup);

    // Set target IP
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SetTargetIP(const FString& InTargetIP);

    /** Role Functions */

    // Set role mode
    UFUNCTION(BlueprintCallable, Category = "Timecode Role")
    void SetRoleMode(ETimecodeRoleMode NewMode);

    // Get current role mode
    UFUNCTION(BlueprintCallable, Category = "Timecode Role")
    ETimecodeRoleMode GetRoleMode() const;

    // Set manual master flag
    UFUNCTION(BlueprintCallable, Category = "Timecode Role")
    void SetManualMaster(bool bInIsManuallyMaster);

    // Get manual master flag
    UFUNCTION(BlueprintCallable, Category = "Timecode Role")
    bool GetIsManuallyMaster() const;

    // Set master IP address
    UFUNCTION(BlueprintCallable, Category = "Timecode Role")
    void SetMasterIPAddress(const FString& InMasterIP);

    // Get master IP address
    UFUNCTION(BlueprintCallable, Category = "Timecode Role")
    FString GetMasterIPAddress() const;

    // Get current master flag
    UFUNCTION(BlueprintPure, Category = "Timecode Role")
    bool GetIsMaster() const;

    /** PLL Functions */

    // PLL 사용 설정
    UFUNCTION(BlueprintCallable, Category = "Timecode|PLL")
    void SetUsePLL(bool bEnable);

    // PLL 잠금 상태 확인
    UFUNCTION(BlueprintPure, Category = "Timecode|PLL")
    bool IsPLLLocked() const;

    // PLL 위상 오차 확인
    UFUNCTION(BlueprintPure, Category = "Timecode|PLL")
    double GetPLLPhaseError() const;

    // PLL 주파수 비율 확인
    UFUNCTION(BlueprintPure, Category = "Timecode|PLL")
    double GetPLLFrequencyRatio() const;

    // PLL 대역폭 설정
    UFUNCTION(BlueprintCallable, Category = "Timecode|PLL")
    void SetPLLBandwidth(float Bandwidth);

    // PLL 감쇠 계수 설정
    UFUNCTION(BlueprintCallable, Category = "Timecode|PLL")
    void SetPLLDamping(float Damping);

    // Log detailed debug information about the timecode component
    UFUNCTION(BlueprintCallable, Category = "Timecode|Debug")
    void LogDebugInfo();

private:
    // Elapsed time in seconds
    float ElapsedTimeSeconds;

    // Timecode event map (event name -> trigger time)
    TMap<FString, float> TimecodeEvents;

    // Track triggered events
    TSet<FString> TriggeredEvents;

    // Network synchronization timer
    float SyncTimer;

    // Network manager
    UPROPERTY()
    UTimecodeNetworkManager* NetworkManager;

    // Internal timecode update function
    void UpdateTimecode(float DeltaTime);

    // Timecode event check function
    void CheckTimecodeEvents();

    // Network synchronization function
    void SyncOverNetwork();

    // Role state change callback
    void OnRoleStateChanged(bool bNewIsMaster);

    // nDisplay-based role determination
    bool CheckNDisplayRole();

    // PLL 초기화 함수
    void InitializePLLSettings();

    // Network message reception callback
    UFUNCTION()
    void OnTimecodeMessageReceived(const FTimecodeNetworkMessage& Message);

    // Network state change callback
    UFUNCTION()
    void OnNetworkStateChanged(ENetworkConnectionState NewState);

    // Network role mode change callback
    UFUNCTION()
    void OnNetworkRoleModeChanged(ETimecodeRoleMode NewMode);
};