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

    // 테스트 종류 선택
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Settings")
    int32 TestType;

    // UDP 연결 테스트 포트
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Settings")
    int32 UDPPort;

    // 마스터/슬레이브 테스트 실행 시간
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Settings")
    float SyncDuration;

protected:
    virtual void BeginPlay() override;

public:
    // 테스트 수동 실행 함수
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    void RunSelectedTest();

    // 모든 테스트 실행
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    void RunAllTests();
};