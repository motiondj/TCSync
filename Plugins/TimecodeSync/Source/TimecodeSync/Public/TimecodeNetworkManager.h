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
 * 타임코드 네트워크 관리자 클래스
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

    // 멀티캐스트 그룹 참가
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool JoinMulticastGroup(const FString& MulticastGroup);

    // 네트워크 상태 확인
    UFUNCTION(BlueprintCallable, Category = "Network")
    ENetworkConnectionState GetConnectionState() const;

    // 타임코드 메시지 수신 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "Network")
    FOnTimecodeMessageReceived OnMessageReceived;

    // 네트워크 상태 변경 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "Network")
    FOnNetworkStateChanged OnNetworkStateChanged;

    // 역할 모드 설정/조회 함수
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SetRoleMode(ETimecodeRoleMode NewMode);

    UFUNCTION(BlueprintCallable, Category = "Network")
    ETimecodeRoleMode GetRoleMode() const;

    // 수동 마스터 설정 함수
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SetManualMaster(bool bInIsManuallyMaster);

    UFUNCTION(BlueprintCallable, Category = "Network")
    bool GetIsManuallyMaster() const;

    // 마스터 IP 설정 함수 (수동 슬레이브 모드용)
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SetMasterIPAddress(const FString& InMasterIP);

    UFUNCTION(BlueprintCallable, Category = "Network")
    FString GetMasterIPAddress() const;

    // 역할 모드 변경 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "Network")
    FRoleModeChangedDelegate OnRoleModeChanged;

private:
    // UDP 소켓
    FSocket* Socket;

    // UDP 리시버
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

    // 연결 상태 설정 함수
    void SetConnectionState(ENetworkConnectionState NewState);

    // 하트비트 메시지 전송
    void SendHeartbeat();

    // 역할 모드 설정
    ETimecodeRoleMode RoleMode;

    // 수동 모드 마스터 여부
    bool bIsManuallyMaster;

    // 수동 마스터 IP 주소 (슬레이브 모드용)
    FString MasterIPAddress;

    // 자동 역할 감지 함수
    bool AutoDetectRole();

    // 자동 역할 감지 결과 플래그
    bool bRoleAutomaticallyDetermined;
};
