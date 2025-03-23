// TimecodeNetworkManager.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Networking.h"  // 네트워킹 관련 헤더들을 한 번에 포함
#include "TimecodeNetworkTypes.h"  // 이미 정의된 타입 사용
#include "TimecodePLL.h"
#include "TimecodeNetworkManager.generated.h"

class FSocket;
class FUdpSocketReceiver;
struct FIPv4Endpoint;

/**
 * Network manager for timecode synchronization
 */
UCLASS()
class TIMECODESYNC_API UTimecodeNetworkManager : public UObject
{
    GENERATED_BODY()

public:
    UTimecodeNetworkManager();

    virtual void BeginDestroy() override;

    /**
     * Initialize the network manager
     * @param bInIsMaster True to initialize as master, false as slave
     * @param InPort UDP port to listen on
     * @return True if initialization succeeded
     */
    bool Initialize(bool bInIsMaster, int32 InPort);

    /**
     * Shut down the network manager
     */
    void Shutdown();

    /**
     * Send a timecode message
     * @param Timecode The timecode string to send
     * @param MessageType The type of message to send
     * @return True if the message was sent successfully
     */
    bool SendTimecodeMessage(const FString& Timecode, ETimecodeMessageType MessageType);

    /**
     * Send an event message
     * @param EventName The name of the event
     * @param Timecode The timecode associated with the event
     * @return True if the message was sent successfully
     */
    bool SendEventMessage(const FString& EventName, const FString& Timecode);

    /**
     * Set the role mode
     * @param NewMode The new role mode
     */
    void SetRoleMode(ETimecodeRoleMode NewMode);

    /**
     * Get the current role mode
     * @return The current role mode
     */
    ETimecodeRoleMode GetRoleMode() const;

    /**
     * Set the manual master flag
     * @param bInIsManuallyMaster True to set as manual master
     */
    void SetManualMaster(bool bInIsManuallyMaster);

    /**
     * Get the manual master flag
     * @return True if manually set as master
     */
    bool GetIsManuallyMaster() const;

    /**
     * Set the master IP address
     * @param InMasterIP The master IP address
     */
    void SetMasterIPAddress(const FString& InMasterIP);

    /**
     * Get the master IP address
     * @return The master IP address
     */
    FString GetMasterIPAddress() const;

    /**
     * Check if this instance is the master
     * @return True if this instance is the master
     */
    bool IsMaster() const;

    /**
     * Join a multicast group
     * @param MulticastGroup The multicast group address
     * @return True if join was successful
     */
    bool JoinMulticastGroup(const FString& MulticastGroup);

    /**
     * Get the connection state
     * @return The current connection state
     */
    ENetworkConnectionState GetConnectionState() const;

    /**
     * Get the current timecode
     * @return The current timecode string
     */
    FString GetCurrentTimecode() const;

    /**
     * Check if the network manager has received a valid message
     * @return True if a valid message has been received
     */
    bool HasReceivedValidMessage() const;

    /**
     * Set the target IP address
     * @param IPAddress The target IP address
     */
    void SetTargetIP(const FString& IPAddress);

    /**
     * Set the target port
     * @param Port The target port number
     */
    void SetTargetPort(int32 Port);

    /**
     * Get the target port
     * @return The target port number
     */
    int32 GetTargetPort() const;

    /**
     * Initialize the PLL component with specified parameters
     * @param InBandwidth PLL bandwidth (0.01-1.0)
     * @param InDamping PLL damping factor (0.1-2.0)
     */
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
    double GetPLLCorrectedTime() const;

    /**
     * Convert a timecode string to seconds
     * @param Timecode The timecode string (format: HH:MM:SS:FF)
     * @param FrameRate The frame rate to use for conversion
     * @return The timecode value in seconds
     */
    double TimecodeToSeconds(const FString& Timecode, float FrameRate) const;

    /**
     * Convert seconds to a timecode string
     * @param Seconds The time value in seconds
     * @param FrameRate The frame rate to use for conversion
     * @param bDropFrame Whether to use drop frame format
     * @return The formatted timecode string
     */
    FString SecondsToTimecode(double Seconds, float FrameRate, bool bDropFrame) const;

    /**
     * Check if the PLL is currently locked (stable synchronization)
     * @return True if PLL is locked, false otherwise
     */
    bool IsPLLLocked() const;

    /**
     * Get the current phase error (time difference between master and slave)
     * @return The phase error in seconds
     */
    double GetPLLPhaseError() const;

    /**
     * Get the current frequency ratio (for debugging)
     * @return The frequency ratio
     */
    double GetPLLFrequencyRatio() const;

    /**
     * Toggle PLL usage
     * @param bEnable Whether to enable PLL correction
     */
    void SetUsePLL(bool bEnable);

    /**
     * Check if PLL is currently enabled
     * @return True if PLL is enabled, false otherwise
     */
    bool IsUsingPLL() const;

    // Delegate for received messages (두 이름 모두 지원)
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimecodeMessageReceived, const FTimecodeNetworkMessage&, Message);
    UPROPERTY(BlueprintAssignable, Category = "Timecode Network")
    FOnTimecodeMessageReceived OnTimecodeMessageReceived;

    // 이전 코드와의 호환성을 위한 별칭
    UPROPERTY(BlueprintAssignable, Category = "Timecode Network")
    FOnTimecodeMessageReceived OnMessageReceived;

    // Delegate for network state changes
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNetworkStateChanged, ENetworkConnectionState, NewState);
    UPROPERTY(BlueprintAssignable, Category = "Timecode Network")
    FOnNetworkStateChanged OnNetworkStateChanged;

    // Delegate for role mode changes
    UPROPERTY(BlueprintAssignable, Category = "Timecode Network")
    FRoleModeChangedDelegate OnRoleModeChanged;

private:
    // Socket for network communication
    FSocket* Socket;

    // Socket receiver for incoming data
    FUdpSocketReceiver* Receiver;

    // Connection state
    ENetworkConnectionState ConnectionState;

    // Instance ID for identifying this network manager
    FString InstanceID;

    // UDP port for receiving messages
    int32 PortNumber;

    // Target IP address for sending messages
    FString TargetIPAddress;

    // Target port for sending messages
    int32 TargetPortNumber;

    // Multicast group address
    FString MulticastGroupAddress;

    // Whether this instance is in master mode
    bool bIsMasterMode;

    // Role mode (automatic or manual)
    ETimecodeRoleMode RoleMode;

    // Manual master flag
    bool bIsManuallyMaster;

    // Master IP address (for manual slave mode)
    FString MasterIPAddress;

    // Whether the role was automatically determined
    bool bRoleAutomaticallyDetermined;

    // Whether a valid message has been received
    bool bHasReceivedValidMessage;

    // Helper function to create socket
    bool CreateSocket();

    // Callback for UDP received data
    void OnUDPReceived(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt);

    // Process received message
    void ProcessMessage(const FTimecodeNetworkMessage& Message);

    // Set connection state
    void SetConnectionState(ENetworkConnectionState NewState);

    // Send heartbeat message
    void SendHeartbeat();

    // Auto-detect role based on IP address
    bool AutoDetectRole();

    // Test packet loss handling (for testing purposes)
    bool TestPacketLossHandling(float SimulatedPacketLossRate = 0.2f);

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