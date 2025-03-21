// TimecodeSyncNetworkTest.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Common/UdpSocketBuilder.h"
#include "HAL/Runnable.h"
#include "TimecodeSyncNetworkTest.generated.h"

/**
 * 타임코드 동기화 네트워크 테스트 클래스
 * UDP 소켓 통신 및 패킷 직렬화/역직렬화 테스트용
 */
UCLASS(Blueprintable, BlueprintType)
class TIMECODESYNC_API UTimecodeSyncNetworkTest : public UObject
{
    GENERATED_BODY()

public:
    UTimecodeSyncNetworkTest();

    // UDP 소켓 연결 테스트 (로컬호스트)
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    bool TestUDPConnection(int32 Port = 12345);

    // 메시지 직렬화/역직렬화 테스트
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    bool TestMessageSerialization();

    // 패킷 손실 시뮬레이션 테스트
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    bool TestPacketLoss(float LossPercentage = 20.0f);

private:
    // 로그 헬퍼 함수
    void LogTestResult(const FString& TestName, bool bSuccess, const FString& Message = TEXT(""));
};