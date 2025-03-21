// TimecodeSyncLogicTest.h (수정된 버전)
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TimecodeNetworkManager.h"
#include "TimecodeSyncLogicTest.generated.h"

/**
 * 타임코드 동기화 로직 테스트 클래스
 * 마스터/슬레이브 모드 동기화 테스트
 */
UCLASS(Blueprintable, BlueprintType)
class TIMECODESYNC_API UTimecodeSyncLogicTest : public UObject
{
    GENERATED_BODY()

public:
    UTimecodeSyncLogicTest();

    // 마스터/슬레이브 타임코드 동기화 테스트
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    bool TestMasterSlaveSync(float Duration = 5.0f);

    // 다양한 프레임 레이트 테스트
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    bool TestMultipleFrameRates();

    // 시스템 시간 동기화 테스트
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    bool TestSystemTimeSync();

private:
    // 타임코드 메시지 수신 핸들러 (마스터)
    UFUNCTION()
    void OnMasterMessageReceived(const FTimecodeNetworkMessage& Message);

    // 타임코드 메시지 수신 핸들러 (슬레이브)
    UFUNCTION()
    void OnSlaveMessageReceived(const FTimecodeNetworkMessage& Message);

    // 멀티 타임코드 테스트 수신 핸들러
    UFUNCTION()
    void OnTestTimecodeReceived(const FTimecodeNetworkMessage& Message);

    // 시스템 시간 테스트 수신 핸들러
    UFUNCTION()
    void OnSystemTimeMessageReceived(const FTimecodeNetworkMessage& Message);

    // 로그 헬퍼 함수
    void LogTestResult(const FString& TestName, bool bSuccess, const FString& Message = TEXT(""));

    // 테스트 과정에서 수신한 타임코드 저장 변수
    FString CurrentMasterTimecode;
    FString CurrentSlaveTimecode;
    FString TestReceivedTimecode;
    FString SystemTimeReceivedTimecode;
};