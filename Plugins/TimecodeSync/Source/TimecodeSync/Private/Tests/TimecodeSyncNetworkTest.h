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
 * Ÿ���ڵ� ����ȭ ��Ʈ��ũ �׽�Ʈ Ŭ����
 * UDP ���� ��� �� ��Ŷ ����ȭ/������ȭ �׽�Ʈ��
 */
UCLASS(Blueprintable, BlueprintType)
class TIMECODESYNC_API UTimecodeSyncNetworkTest : public UObject
{
    GENERATED_BODY()

public:
    UTimecodeSyncNetworkTest();

    // UDP ���� ���� �׽�Ʈ (����ȣ��Ʈ)
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    bool TestUDPConnection(int32 Port = 12345);

    // �޽��� ����ȭ/������ȭ �׽�Ʈ
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    bool TestMessageSerialization();

    // ��Ŷ �ս� �ùķ��̼� �׽�Ʈ
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    bool TestPacketLoss(float LossPercentage = 20.0f);

private:
    // �α� ���� �Լ�
    void LogTestResult(const FString& TestName, bool bSuccess, const FString& Message = TEXT(""));
};