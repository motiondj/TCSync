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
};
