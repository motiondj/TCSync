// TimecodeSyncTestActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimecodeSyncTestActor.generated.h"

UCLASS(Blueprintable)
class TIMECODESYNC_API ATimecodeSyncTestActor : public AActor
{
    GENERATED_BODY()

public:
    ATimecodeSyncTestActor();

    // Select test type
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Settings")
    int32 TestType;

    // UDP connection test port
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Settings")
    int32 UDPPort;

    // Master/Slave test execution duration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Settings")
    float SyncDuration;

    // 통합 테스트 실행 함수
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    void RunIntegratedTest();

protected:
    virtual void BeginPlay() override;

public:
    // Manual test execution function
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    void RunSelectedTest();

    // Run all tests
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    void RunAllTests();
};