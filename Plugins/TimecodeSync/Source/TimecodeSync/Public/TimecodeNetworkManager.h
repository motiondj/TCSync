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
#include "TimecodeNetworkTypes.h"       // 공유 타입 정의를 포함
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

    // 타임코드 모드 변경 명령 전송
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool SendModeChangeCommand(ETimecodeMode NewMode);

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

    // 전용 마스터 기능 설정/조회
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SetDedicatedMaster(bool bInIsDedicatedMaster);

    UFUNCTION(BlueprintCallable, Category = "Network")
    bool IsDedicatedMaster() const;

    // PLL 설정 메서드
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SetUsePLL(bool bInUsePLL);

    UFUNCTION(BlueprintCallable, Category = "Network")
    void SetPLLParameters(float Bandwidth, float Damping);

    UFUNCTION(BlueprintCallable, Category = "Network")
    bool GetUsePLL() const;

    UFUNCTION(BlueprintCallable, Category = "Network")
    void GetPLLParameters(float& OutBandwidth, float& OutDamping) const;

    // PLL 상태 정보 메서드
    UFUNCTION(BlueprintCallable, Category = "Network")
    void GetPLLStatus(double& OutPhase, double& OutFrequency, double& OutOffset) const;

    /**
     * 주기적 업데이트 (연결 상태 체크용)
     * @param DeltaTime - 마지막 업데이트 이후 경과 시간
     */
    UFUNCTION(BlueprintCallable, Category = "Network")
    void Tick(float DeltaTime);

    // 안전한 종료를 위한 플래그
    bool bIsShuttingDown;

private:
    // UDP socket
    FSocket* Socket;

    // UDP receiver
    FUdpSocketReceiver* Receiver;

    // Connection state
    ENetworkConnectionState ConnectionState;

    // Sender ID (unique identifier)
    FString InstanceID;

    /** Port used for receiving incoming messages from network */
    int32 ReceivePortNumber;

    /** Port used when sending outgoing messages to target devices */
    int32 SendPortNumber; // 또는 TargetPortNumber

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

    // 전용 마스터 서버 관련 변수들
    bool bIsDedicatedMaster;   // 전용 마스터 서버 여부

    // PLL 설정
    bool bUsePLL;
    float PLLBandwidth;     // PLL 반응성 (0.01-1.0)
    float PLLDamping;       // PLL 안정성 (0.1-2.0)

    // PLL 상태 변수
    double PLLPhase;        // 위상 (현재 상태)
    double PLLFrequency;    // 주파수 (변화율)
    double PLLOffset;       // 오프셋 (보정값)

    // 마지막 수신 시간 추적
    double LastMasterTimestamp;
    double LastLocalTimestamp;

    // 타임코드 보정 및 PLL 상태 업데이트
    void UpdatePLL(double MasterTime, double LocalTime);
    double GetPLLCorrectedTime(double LocalTime) const;
    void InitializePLL();

    // 멀티캐스트 활성화 상태 추적
    bool bMulticastEnabled;

    // 특정 IP로 메시지 전송 헬퍼 함수
    bool SendToSpecificIP(const TArray<uint8>& MessageData, const FString& IPAddress,
        int32& BytesSent, const FString& TargetName);

    // 멀티캐스트 그룹으로 메시지 전송 헬퍼 함수
    bool SendToMulticastGroup(const TArray<uint8>& MessageData, int32& BytesSent);

    // 연결 관리 변수
    float ConnectionCheckTimer;
    int32 ConnectionRetryCount;
    float ConnectionRetryInterval;
    const float MAX_RETRY_INTERVAL = 5.0f;
    const int32 MAX_RETRY_COUNT = 5;
    bool bConnectionLost;
    FDateTime LastMessageTime;

    // 연결 상태 확인 함수
    void CheckConnectionStatus(float DeltaTime);
    bool AttemptReconnection();
    void ResetConnectionStatus();

};