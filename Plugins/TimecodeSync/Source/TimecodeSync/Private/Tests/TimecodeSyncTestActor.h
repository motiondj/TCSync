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

    // �׽�Ʈ ���� ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Settings")
    int32 TestType;

    // UDP ���� �׽�Ʈ ��Ʈ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Settings")
    int32 UDPPort;

    // ������/�����̺� �׽�Ʈ ���� �ð�
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Settings")
    float SyncDuration;

protected:
    virtual void BeginPlay() override;

public:
    // �׽�Ʈ ���� ���� �Լ�
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    void RunSelectedTest();

    // ��� �׽�Ʈ ����
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    void RunAllTests();
};