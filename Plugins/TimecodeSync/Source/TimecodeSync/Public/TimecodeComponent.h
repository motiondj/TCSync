#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimecodeNetworkManager.h"
#include "TimecodeNetworkTypes.h"
#include "PLLSynchronizer.h"
#include "SMPTETimecodeConverter.h"
#include "TimecodeComponent.generated.h"

class UPLLSynchronizer;
class USMPTETimecodeConverter;

// Delegate declaration for timecode change event
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimecodeChanged, const FString&, NewTimecode);

// Delegate declaration for timecode event trigger
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimecodeEventTriggered, const FString&, EventName, float, EventTime);

// Delegate declaration for network connection state change
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNetworkConnectionChanged, ENetworkConnectionState, NewState);

// Delegate declaration for role change event
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoleChanged, bool, IsMaster);

// Define timecode operation modes
UENUM(BlueprintType)
enum class ETimecodeMode : uint8
{
    PLL_Only UMETA(DisplayName = "PLL Synchronization Only"),
    SMPTE_Only UMETA(DisplayName = "SMPTE Timecode Only"),
    Integrated UMETA(DisplayName = "PLL and SMPTE Integrated"),
    Raw UMETA(DisplayName = "Raw Time (No Processing)")
};

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

    // PLL 사용 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode Sync", meta = (DisplayName = "Use PLL"))
    bool bUsePLL;

    // PLL 대역폭 (반응성 조절)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode Sync", meta = (EditCondition = "bUsePLL", EditConditionHides, ClampMin = "0.01", ClampMax = "1.0"))
    float PLLBandwidth;

    // PLL 감쇠 계수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode Sync", meta = (EditCondition = "bUsePLL", EditConditionHides, ClampMin = "0.1", ClampMax = "2.0"))
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

    // Timecode operation mode
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode", meta = (DisplayName = "Timecode Mode"))
    ETimecodeMode TimecodeMode = ETimecodeMode::Integrated;

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

    // Log detailed debug information about the timecode component
    UFUNCTION(BlueprintCallable, Category = "Timecode|Debug")
    void LogDebugInfo();

    // PLL 설정 메서드
    UFUNCTION(BlueprintCallable, Category = "Timecode Sync")
    void SetUsePLL(bool bInUsePLL);

    UFUNCTION(BlueprintCallable, Category = "Timecode Sync")
    bool GetUsePLL() const;

    UFUNCTION(BlueprintCallable, Category = "Timecode Sync")
    void SetPLLParameters(float Bandwidth, float Damping);

    UFUNCTION(BlueprintCallable, Category = "Timecode Sync")
    void GetPLLParameters(float& OutBandwidth, float& OutDamping) const;

    // PLL 상태 조회
    UFUNCTION(BlueprintCallable, Category = "Timecode Sync")
    void GetPLLStatus(float& OutFrequency, float& OutOffset) const;

    // Set timecode operation mode
    UFUNCTION(BlueprintCallable, Category = "Timecode Mode")
    void SetTimecodeMode(ETimecodeMode NewMode);

    // Get current timecode operation mode
    UFUNCTION(BlueprintCallable, Category = "Timecode Mode")
    ETimecodeMode GetTimecodeMode() const;

    // Apply timecode mode settings
    UFUNCTION(BlueprintCallable, Category = "Timecode Mode")
    void ApplyTimecodeMode();

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

    // Network message reception callback
    UFUNCTION()
    void OnTimecodeMessageReceived(const FTimecodeNetworkMessage& Message);

    // Network state change callback
    UFUNCTION()
    void OnNetworkStateChanged(ENetworkConnectionState NewState);

    // Network role mode change callback
    UFUNCTION()
    void OnNetworkRoleModeChanged(ETimecodeRoleMode NewMode);

    // PLL Synchronizer sub-module
    UPROPERTY()
    UPLLSynchronizer* PLLSynchronizer;

    // SMPTE Converter sub-module
    UPROPERTY()
    USMPTETimecodeConverter* SMPTEConverter;
};