#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "TimecodeNetworkTypes.h"
#include "TimecodeNetworkManager.generated.h"

class FSocket;
class FUdpSocketReceiver;

// 네트워크 연결 상태 열거형
UENUM(BlueprintType)
enum class ENetworkConnectionState : uint8
{
    Disconnected,  // 연결되지 않음
    Connecting,    // 연결 중
    Connected      // 연결됨
};

// 타임코드 메시지 수신 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimecodeMessageReceived, const FTimecodeNetworkMessage&, Message);

// 네트워크 상태 변경 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNetworkStateChanged, ENetworkConnectionState, NewState);

/**
 * 타임코드 네트워크 통신을 관리하는 클래스
 */
UCLASS(BlueprintType)
class TIMECODESYNC_API UTimecodeNetworkManager : public UObject
{
    GENERATED_BODY()

public:
    UTimecodeNetworkManager();
    virtual ~UTimecodeNetworkManager();

    // 네트워크 초기화 (마스터/슬레이브 모드)
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool Initialize(bool bIsMaster, int32 Port);

    // 네트워크 종료
    UFUNCTION(BlueprintCallable, Category = "Network")
    void Shutdown();

    // 타임코드 메시지 전송
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool SendTimecodeMessage(const FString& Timecode, ETimecodeMessageType MessageType = ETimecodeMessageType::TimecodeSync);

    // 이벤트 메시지 전송
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool SendEventMessage(const FString& EventName, const FString& Timecode);

    // 타겟 IP 설정
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SetTargetIP(const FString& IPAddress);

    // 멀티캐스트 그룹 설정
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool JoinMulticastGroup(const FString& MulticastGroup);

    // 연결 상태 확인
    UFUNCTION(BlueprintCallable, Category = "Network")
    ENetworkConnectionState GetConnectionState() const;

    // 메시지 수신 이벤트
    UPROPERTY(BlueprintAssignable, Category = "Network")
    FOnTimecodeMessageReceived OnMessageReceived;

    // 네트워크 상태 변경 이벤트
    UPROPERTY(BlueprintAssignable, Category = "Network")
    FOnNetworkStateChanged OnNetworkStateChanged;

private:
    // UDP 소켓
    FSocket* Socket;

    // UDP 수신기
    FUdpSocketReceiver* Receiver;

    // 연결 상태
    ENetworkConnectionState ConnectionState;

    // 송신자 ID (고유 식별자)
    FString InstanceID;

    // 포트 번호
    int32 PortNumber;

    // 타겟 IP 주소
    FString TargetIPAddress;

    // 멀티캐스트 그룹 주소
    FString MulticastGroupAddress;

    // 마스터 모드 여부
    bool bIsMasterMode;

    // UDP 수신 콜백
    void OnUDPReceived(const FArrayReaderPtr& DataPtr, const FIPv4Endpoint& Endpoint);

    // 소켓 생성 함수
    bool CreateSocket();

    // 메시지 처리 함수
    void ProcessMessage(const FTimecodeNetworkMessage& Message);

    // 연결 상태 변경 함수
    void SetConnectionState(ENetworkConnectionState NewState);

    // 하트비트 메시지 전송
    void SendHeartbeat();
};
