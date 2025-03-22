// TimecodeNetworkManager.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TimecodeNetworkTypes.h"
#include "TimecodeNetworkManager.generated.h"

// Forward declare the packet loss handler
class UTimecodePacketLossHandler;

/**
 * Network message structure for timecode synchronization
 */
UENUM(BlueprintType)
enum class ETimecodeMessageType : uint8
{
    None = 0,
    TimecodeSync,      // 타임코드 동기화 메시지
    HeartBeat,         // 연결 유지 메시지
    Acknowledgment,    // 메시지 수신 확인용
    ResendRequest,     // 패킷 재전송 요청용
    RoleAssignment,    // 역할 할당 메시지
    Event,             // 이벤트 트리거 메시지
    Command            // 명령 메시지
};

USTRUCT(BlueprintType)
struct TIMECODESYNC_API FTimecodeNetworkMessage
{
    GENERATED_BODY()

public:
    // 기본 생성자
    FTimecodeNetworkMessage();

    // 메시지 타입
    UPROPERTY(BlueprintReadWrite, Category = "Network")
    ETimecodeMessageType MessageType;

    // 타임코드 문자열
    UPROPERTY(BlueprintReadWrite, Category = "Network")
    FString Timecode;

    // 시스템 시간
    UPROPERTY(BlueprintReadWrite, Category = "Network")
    double SystemTime;

    // 프레임 레이트
    UPROPERTY(BlueprintReadWrite, Category = "Network")
    int32 FrameRate;

    // 패킷 손실 처리를 위한 필드
    UPROPERTY(BlueprintReadWrite, Category = "Network")
    int32 SequenceNumber;

    UPROPERTY(BlueprintReadWrite, Category = "Network")
    int32 AcknowledgedSeqNum;

    UPROPERTY(BlueprintReadWrite, Category = "Network")
    double TimeStamp;

    // 추가 필드
    UPROPERTY(BlueprintReadWrite, Category = "Network")
    FString Data;

    UPROPERTY(BlueprintReadWrite, Category = "Network")
    FString SenderID;

    // 직렬화 및 역직렬화 함수
    bool Serialize(TArray<uint8>& OutData) const;
    bool Deserialize(const TArray<uint8>& InData);

    // 유틸리티 함수
    FTimecodeNetworkMessage CreateAcknowledgment() const;
    static FTimecodeNetworkMessage CreateResendRequest(int32 MissingSequenceNumber);
    bool IsValid() const;
    FString ToString() const;
};

// 네트워크 매니저 클래스 정의 추가
UCLASS()
class TIMECODESYNC_API UTimecodeNetworkManager : public UObject
{
    GENERATED_BODY()

public:
    UTimecodeNetworkManager();

    // 초기화 및 종료
    bool Initialize();
    void Shutdown();

    // 네트워크 상태 관리
    void SetConnectionState(ENetworkConnectionState NewState);
    ENetworkConnectionState GetConnectionState() const;

    // 타임코드 메시지 송신
    void SendTimecodeMessage(const FString& Timecode, double SystemTime, int32 FrameRate);

    // 네트워크 설정
    void SetUDPPort(int32 Port);
    void SetTargetIP(const FString& IP);
    bool JoinMulticastGroup(const FString& GroupIP);

    // 패킷 손실 처리 테스트
    UFUNCTION(BlueprintCallable, Category = "TimecodeSync|Tests")
    bool TestPacketLossHandling(float SimulatedPacketLossRate = 0.3f);

    // 이벤트 델리게이트 - 타임코드 메시지 수신 시 호출
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnTimecodeMessageReceived, const FTimecodeNetworkMessage&);
    FOnTimecodeMessageReceived OnTimecodeMessageReceived;

    // 이벤트 델리게이트 - 네트워크 상태 변경 시 호출
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnNetworkStateChanged, ENetworkConnectionState);
    FOnNetworkStateChanged OnNetworkStateChanged;

private:
    // 패킷 손실 처리 객체
    UPROPERTY()
    UTimecodePacketLossHandler* PacketLossHandler;

    // 네트워크 상태
    ENetworkConnectionState ConnectionState;

    // 네트워크 설정 값들
    int32 UDPPort;
    FString TargetIPAddress;
    FString MulticastGroupAddress;

    // 메시지 이력 및 패킷 손실 관리
    TArray<FTimecodeNetworkMessage> MessageHistory;

    // 메시지 이력 관리 함수
    void AddMessageToHistory(const FTimecodeNetworkMessage& Message);
    FTimecodeNetworkMessage* FindMessageInHistory(int32 SequenceNumber);
    void CleanupOldMessages();

    // 패킷 손실 처리 함수
    void ProcessResendRequest(int32 SequenceNumber);
    void ProcessPendingResends();

    // 내부 네트워크 함수
    void ReceiveData();
    void HandleIncomingMessage(const FTimecodeNetworkMessage& Message);
    void SendMessage(const FTimecodeNetworkMessage& Message);
};