#pragma once

#include "CoreMinimal.h"
#include "TimecodeNetworkTypes.generated.h"

// 타임코드 메시지 타입 열거형
UENUM(BlueprintType)
enum class ETimecodeMessageType : uint8
{
    Heartbeat,       // 연결 상태 확인용 하트비트
    TimecodeSync,    // 타임코드 동기화 메시지
    RoleAssignment,  // 역할 할당 메시지
    Event,           // 이벤트 트리거 메시지
    Command          // 명령 메시지
};

// 네트워크로 전송되는 타임코드 메시지 구조체
USTRUCT(BlueprintType)
struct FTimecodeNetworkMessage
{
    GENERATED_BODY()

    // 메시지 타입
    UPROPERTY(BlueprintReadWrite, Category = "Network")
    ETimecodeMessageType MessageType;

    // 타임코드 문자열
    UPROPERTY(BlueprintReadWrite, Category = "Network")
    FString Timecode;

    // 추가 데이터 (이벤트 이름, 명령 등)
    UPROPERTY(BlueprintReadWrite, Category = "Network")
    FString Data;

    // 메시지 생성 시간
    UPROPERTY(BlueprintReadWrite, Category = "Network")
    double Timestamp;

    // 송신자 ID
    UPROPERTY(BlueprintReadWrite, Category = "Network")
    FString SenderID;

    // 기본 생성자
    FTimecodeNetworkMessage()
        : MessageType(ETimecodeMessageType::Heartbeat)
        , Timecode(TEXT("00:00:00:00"))
        , Data(TEXT(""))
        , Timestamp(0.0)
        , SenderID(TEXT(""))
    {
    }

    // 메시지를 바이트 배열로 직렬화
    TArray<uint8> Serialize() const;

    // 바이트 배열에서 메시지 역직렬화
    bool Deserialize(const TArray<uint8>& Data);
};
