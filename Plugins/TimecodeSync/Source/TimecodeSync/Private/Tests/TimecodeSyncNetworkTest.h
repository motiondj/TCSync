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
 * Timecode synchronization network test class
 * For testing UDP socket communication and packet serialization/deserialization
 */
UCLASS(Blueprintable, BlueprintType)
class TIMECODESYNC_API UTimecodeSyncNetworkTest : public UObject
{
    GENERATED_BODY()

public:
    UTimecodeSyncNetworkTest();

    // UDP socket connection test (localhost)
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    bool TestUDPConnection(int32 Port = 12345);

    // Message serialization/deserialization test
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    bool TestMessageSerialization();

    // Packet loss simulation test
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    bool TestPacketLoss(float LossPercentage = 20.0f);

private:
    // Log helper function
    void LogTestResult(const FString& TestName, bool bSuccess, const FString& Message = TEXT(""));
};