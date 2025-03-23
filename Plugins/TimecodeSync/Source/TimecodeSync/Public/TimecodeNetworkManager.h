#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Common/UdpSocketReceiver.h"
#include "Common/UdpSocketBuilder.h"
#include "Networking.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Serialization/ArrayReader.h"
#include "TimecodeNetworkTypes.h"
#include "TimecodePLL.h"  // 추가된 헤더
#include "TimecodeNetworkManager.generated.h"

class FSocket;
class FUdpSocketReceiver;

// Network connection state enum
UENUM(BlueprintType)
enum class ENetworkConnectionState : uint8
{
    Disconnected,  // Not connected
    Connecting,    // Connecting
    Connected      // Connected
};

// Timecode message receive delegate
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimecodeMessageReceived, const FTimecodeNetworkMessage&, Message);

// Network state change delegate
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNetworkStateChanged, ENetworkConnectionState, NewState);

// Message received delegate
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMessageReceived, const FTimecodeNetworkMessage&, Message);

/**
 * Timecode network manager class
 */
UCLASS(BlueprintType)
class TIMECODESYNC_API UTimecodeNetworkManager : public UObject
{
    GENERATED_BODY()

public:
    UTimecodeNetworkManager();
    virtual ~UTimecodeNetworkManager();

    // Network initialization (master/slave mode)
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool Initialize(bool bIsMaster, int32 Port);

    // Network shutdown
    UFUNCTION(BlueprintCallable, Category = "Network")
    void Shutdown();

    // Send timecode message
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool SendTimecodeMessage(const FString& Timecode, ETimecodeMessageType MessageType = ETimecodeMessageType::TimecodeSync);

    // Send event message
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool SendEventMessage(const FString& EventName, const FString& Timecode);

    // Set target IP
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SetTargetIP(const FString& IPAddress);

    // Join multicast group
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool JoinMulticastGroup(const FString& MulticastGroup);

    // Check network state
    UFUNCTION(BlueprintCallable, Category = "Network")
    ENetworkConnectionState GetConnectionState() const;

    // Timecode message receive delegate
    UPROPERTY(BlueprintAssignable, Category = "Network")
    FOnTimecodeMessageReceived OnTimecodeMessageReceived;

    // Network state change delegate
    UPROPERTY(BlueprintAssignable, Category = "Network")
    FOnNetworkStateChanged OnNetworkStateChanged;

    // Role mode set/get functions
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SetRoleMode(ETimecodeRoleMode NewMode);

    UFUNCTION(BlueprintCallable, Category = "Network")
    ETimecodeRoleMode GetRoleMode() const;

    // Manual master set/get functions
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SetManualMaster(bool bInIsManuallyMaster);

    UFUNCTION(BlueprintCallable, Category = "Network")
    bool GetIsManuallyMaster() const;

    // Master IP set/get functions (for manual slave mode)
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SetMasterIPAddress(const FString& InMasterIP);

    UFUNCTION(BlueprintCallable, Category = "Network")
    FString GetMasterIPAddress() const;

    // Role mode change delegate
    UPROPERTY(BlueprintAssignable, Category = "Network")
    FRoleModeChangedDelegate OnRoleModeChanged;

    // Network state functions
    bool HasReceivedValidMessage() const { return bHasReceivedValidMessage; }

    // Get current timecode
    UFUNCTION(BlueprintCallable, Category = "Network")
    FString GetCurrentTimecode() const;

    // Check if master
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool IsMaster() const;

    // Message received delegate
    UPROPERTY(BlueprintAssignable, Category = "Network")
    FOnMessageReceived OnMessageReceived;

    // 패킷 손실 처리 테스트 함수 추가
    UFUNCTION(BlueprintCallable, Category = "TimecodeSync|Tests")
    bool TestPacketLossHandling(float SimulatedPacketLossRate = 0.3f);

    // 대상 포트 설정 함수
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SetTargetPort(int32 Port);

    // 대상 포트 가져오기 함수
    UFUNCTION(BlueprintCallable, Category = "Network")
    int32 GetTargetPort() const;

    // -------------------------
    // PLL 관련 함수들 추가
    // -------------------------

    /**
     * Initialize the PLL component with specified parameters
     * @param InBandwidth PLL bandwidth (0.01-1.0)
     * @param InDamping PLL damping factor (0.1-2.0)
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|PLL")
    void InitializePLL(float InBandwidth = 0.1f, float InDamping = 0.7f);

    /**
     * Update the PLL with a received timecode message
     * @param MasterTimecode The timecode received from the master
     * @param ReceiveTime The local time when the message was received
     */
    void UpdatePLL(const FString& MasterTimecode, double ReceiveTime);

    /**
     * Get the PLL-corrected local time
     * @return The corrected local time
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|PLL")
    double GetPLLCorrectedTime() const;

    /**
     * Convert a timecode string to seconds
     * @param Timecode The timecode string (format: HH:MM:SS:FF)
     * @param FrameRate The frame rate to use for conversion
     * @return The timecode value in seconds
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|PLL")
    double TimecodeToSeconds(const FString& Timecode, float FrameRate) const;

    /**
     * Convert seconds to a timecode string
     * @param Seconds The time value in seconds
     * @param FrameRate The frame rate to use for conversion
     * @param bDropFrame Whether to use drop frame format
     * @return The formatted timecode string
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|PLL")
    FString SecondsToTimecode(double Seconds, float FrameRate, bool bDropFrame) const;

    /**
     * Check if the PLL is currently locked (stable synchronization)
     * @return True if PLL is locked, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|PLL")
    bool IsPLLLocked() const;

    /**
     * Get the current phase error (time difference between master and slave)
     * @return The phase error in seconds
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|PLL")
    double GetPLLPhaseError() const;

    /**
     * Get the current frequency ratio (for debugging)
     * @return The frequency ratio
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|PLL")
    double GetPLLFrequencyRatio() const;

    /**
     * Toggle PLL usage
     * @param bEnable Whether to enable PLL correction
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|PLL")
    void SetUsePLL(bool bEnable);

    /**
     * Check if PLL is currently enabled
     * @return True if PLL is enabled, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|PLL")
    bool IsUsingPLL() const;

private:
    // UDP socket
    FSocket* Socket;

    // UDP receiver
    FUdpSocketReceiver* Receiver;

    // Connection state
    ENetworkConnectionState ConnectionState;

    // Sender ID (unique identifier)
    FString InstanceID;

    // Port number
    int32 PortNumber;

    // Target IP address
    FString TargetIPAddress;

    // Multicast group address
    FString MulticastGroupAddress;

    // Master mode flag
    bool bIsMasterMode;

    // UDP receive callback
    void OnUDPReceived(const FArrayReaderPtr& DataPtr, const FIPv4Endpoint& Endpoint);

    // Socket creation function
    bool CreateSocket();

    // Message processing function
    void ProcessMessage(const FTimecodeNetworkMessage& Message);

    // Connection state set function
    void SetConnectionState(ENetworkConnectionState NewState);

    // Send heartbeat message
    void SendHeartbeat();

    // Role mode setting
    ETimecodeRoleMode RoleMode;

    // Manual mode master flag
    bool bIsManuallyMaster;

    // Manual master IP address (for slave mode)
    FString MasterIPAddress;

    // Auto role detection function
    bool AutoDetectRole();

    // Auto role detection result flag
    bool bRoleAutomaticallyDetermined;

    // Network state tracking
    bool bHasReceivedValidMessage = false;

    // 대상 포트 번호
    int32 TargetPortNumber;

    // -------------------------
    // PLL 관련 변수들 추가
    // -------------------------

    // PLL instance
    UPROPERTY()
    UTimecodePLL* PLL;

    // Whether to use PLL correction
    bool bUsePLL;

    // Last master timecode received (for logging)
    FString LastMasterTimecode;

    // Last local time when a master message was received
    double LastReceiveTime;

    // Store incoming message timestamps for PLL updates
    TQueue<TPair<double, double>> TimestampQueue;
};