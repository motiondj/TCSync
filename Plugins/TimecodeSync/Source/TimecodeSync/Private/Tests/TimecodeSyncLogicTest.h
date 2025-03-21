// TimecodeSyncLogicTest.h (Modified version)
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TimecodeNetworkManager.h"
#include "TimecodeSyncLogicTest.generated.h"

/**
 * Timecode synchronization logic test class
 * For testing master/slave mode synchronization
 */
UCLASS(Blueprintable, BlueprintType)
class TIMECODESYNC_API UTimecodeSyncLogicTest : public UObject
{
    GENERATED_BODY()

public:
    UTimecodeSyncLogicTest();

    // Master/Slave timecode synchronization test
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    bool TestMasterSlaveSync(float Duration = 5.0f);

    // Multiple frame rates test
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    bool TestMultipleFrameRates();

    // System time synchronization test
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    bool TestSystemTimeSync();

private:
    // Timecode message reception handler (Master)
    UFUNCTION()
    void OnMasterMessageReceived(const FString& Timecode, ETimecodeMessageType MessageType);

    // Timecode message reception handler (Slave)
    UFUNCTION()
    void OnSlaveMessageReceived(const FString& Timecode, ETimecodeMessageType MessageType);

    // Multi-timecode test reception handler
    UFUNCTION()
    void OnTestTimecodeReceived(const FString& Timecode, ETimecodeMessageType MessageType);

    // System time test reception handler
    UFUNCTION()
    void OnSystemTimeReceived(const FString& Timecode, ETimecodeMessageType MessageType);

    // Log helper function
    void LogTestResult(const FString& TestName, bool bSuccess, const FString& Message = TEXT(""));

    // Variables to store received timecodes during testing
    FString CurrentMasterTimecode;
    FString CurrentSlaveTimecode;
    FString TestReceivedTimecode;
    FString SystemTimeReceivedTimecode;
};