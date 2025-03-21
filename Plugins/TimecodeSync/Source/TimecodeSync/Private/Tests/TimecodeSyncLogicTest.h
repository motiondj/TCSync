// TimecodeSyncLogicTest.h (������ ����)
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TimecodeNetworkManager.h"
#include "TimecodeSyncLogicTest.generated.h"

/**
 * Ÿ���ڵ� ����ȭ ���� �׽�Ʈ Ŭ����
 * ������/�����̺� ��� ����ȭ �׽�Ʈ
 */
UCLASS(Blueprintable, BlueprintType)
class TIMECODESYNC_API UTimecodeSyncLogicTest : public UObject
{
    GENERATED_BODY()

public:
    UTimecodeSyncLogicTest();

    // ������/�����̺� Ÿ���ڵ� ����ȭ �׽�Ʈ
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    bool TestMasterSlaveSync(float Duration = 5.0f);

    // �پ��� ������ ����Ʈ �׽�Ʈ
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    bool TestMultipleFrameRates();

    // �ý��� �ð� ����ȭ �׽�Ʈ
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    bool TestSystemTimeSync();

private:
    // Ÿ���ڵ� �޽��� ���� �ڵ鷯 (������)
    UFUNCTION()
    void OnMasterMessageReceived(const FTimecodeNetworkMessage& Message);

    // Ÿ���ڵ� �޽��� ���� �ڵ鷯 (�����̺�)
    UFUNCTION()
    void OnSlaveMessageReceived(const FTimecodeNetworkMessage& Message);

    // ��Ƽ Ÿ���ڵ� �׽�Ʈ ���� �ڵ鷯
    UFUNCTION()
    void OnTestTimecodeReceived(const FTimecodeNetworkMessage& Message);

    // �ý��� �ð� �׽�Ʈ ���� �ڵ鷯
    UFUNCTION()
    void OnSystemTimeMessageReceived(const FTimecodeNetworkMessage& Message);

    // �α� ���� �Լ�
    void LogTestResult(const FString& TestName, bool bSuccess, const FString& Message = TEXT(""));

    // �׽�Ʈ �������� ������ Ÿ���ڵ� ���� ����
    FString CurrentMasterTimecode;
    FString CurrentSlaveTimecode;
    FString TestReceivedTimecode;
    FString SystemTimeReceivedTimecode;
};