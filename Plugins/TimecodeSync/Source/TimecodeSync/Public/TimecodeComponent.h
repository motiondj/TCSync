#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimecodeNetworkManager.h"
#include "TimecodeComponent.generated.h"

// Ÿ���ڵ� ���� �̺�Ʈ�� ���� ��������Ʈ ����
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimecodeChanged, const FString&, NewTimecode);

// Ÿ���ڵ� �̺�Ʈ Ʈ���Ÿ� ���� ��������Ʈ ����
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimecodeEventTriggered, const FString&, EventName, float, EventTime);

// ��Ʈ��ũ ���� ���� ������ ���� ��������Ʈ ����
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNetworkConnectionChanged, ENetworkConnectionState, NewState);

/**
 * ���Ϳ� �����Ͽ� Ÿ���ڵ� ����� �����ϴ� ������Ʈ
 */
UCLASS(ClassGroup = "Timecode", meta = (BlueprintSpawnableComponent))
class TIMECODESYNC_API UTimecodeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTimecodeComponent();

    // Ÿ���ڵ� ���� ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode")
    bool bIsMaster;

    // nDisplay ��� ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode")
    bool bUseNDisplay;

    // ������ ����Ʈ ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode", meta = (ClampMin = "1.0", ClampMax = "240.0"))
    float FrameRate;

    // UDP ��Ʈ ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network", meta = (ClampMin = "1024", ClampMax = "65535"))
    int32 UDPPort;

    // Ÿ�� IP ���� (������ ��忡�� ����ĳ��Ʈ ���� ��)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
    FString TargetIP;

    // ��Ƽĳ��Ʈ �׷� ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
    FString MulticastGroup;

    // �ڵ� ���� ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode")
    bool bAutoStart;

    // ��Ʈ��ũ ����ȭ ���� (��)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network", meta = (ClampMin = "0.001", ClampMax = "1.0"))
    float SyncInterval;

    // ���� Ÿ���ڵ� (�б� ����)
    UPROPERTY(BlueprintReadOnly, Category = "Timecode")
    FString CurrentTimecode;

    // Ÿ���ڵ� ���� �̺�Ʈ
    UPROPERTY(BlueprintAssignable, Category = "Timecode")
    FOnTimecodeChanged OnTimecodeChanged;

    // Ÿ���ڵ� �̺�Ʈ Ʈ����
    UPROPERTY(BlueprintAssignable, Category = "Timecode")
    FOnTimecodeEventTriggered OnTimecodeEventTriggered;

    // ��Ʈ��ũ ���� ���� ���� �̺�Ʈ
    UPROPERTY(BlueprintAssignable, Category = "Network")
    FOnNetworkConnectionChanged OnNetworkConnectionChanged;

protected:
    // ������Ʈ �ʱ�ȭ �� ȣ��
    virtual void BeginPlay() override;

    // ������Ʈ ���� �� ȣ��
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // �� ������ ȣ��
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // Ÿ���ڵ� ����
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void StartTimecode();

    // Ÿ���ڵ� ����
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void StopTimecode();

    // Ÿ���ڵ� �缳��
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void ResetTimecode();

    // Ÿ���ڵ� �̺�Ʈ ���
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void RegisterTimecodeEvent(const FString& EventName, float EventTimeInSeconds);

    // Ÿ���ڵ� �̺�Ʈ ����
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void UnregisterTimecodeEvent(const FString& EventName);

    // ��Ʈ��ũ ���� ���� ��������
    UFUNCTION(BlueprintCallable, Category = "Network")
    ENetworkConnectionState GetNetworkConnectionState() const;

    // ��Ʈ��ũ ���� ����
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool SetupNetwork();

    // ��Ʈ��ũ ���� ����
    UFUNCTION(BlueprintCallable, Category = "Network")
    void ShutdownNetwork();

private:
    // Ÿ���ڵ� ���� �� ����
    bool bIsRunning;

    // ��� �ð� (��)
    float ElapsedTimeSeconds;

    // Ÿ���ڵ� �̺�Ʈ �� (�̺�Ʈ �̸� -> Ʈ���� �ð�)
    TMap<FString, float> TimecodeEvents;

    // �̹� Ʈ���ŵ� �̺�Ʈ ����
    TSet<FString> TriggeredEvents;

    // ��Ʈ��ũ ����ȭ Ÿ�̸�
    float SyncTimer;

    // ��Ʈ��ũ ������
    UPROPERTY()
    UTimecodeNetworkManager* NetworkManager;

    // ���� Ÿ���ڵ� ������Ʈ �Լ�
    void UpdateTimecode(float DeltaTime);

    // Ÿ���ڵ� �̺�Ʈ Ȯ�� �Լ�
    void CheckTimecodeEvents();

    // ��Ʈ��ũ ����ȭ �Լ�
    void SyncOverNetwork();

    // ��Ʈ��ũ �޽��� ���� �ݹ�
    UFUNCTION()
    void OnTimecodeMessageReceived(const FTimecodeNetworkMessage& Message);

    // ��Ʈ��ũ ���� ���� �ݹ�
    UFUNCTION()
    void OnNetworkStateChanged(ENetworkConnectionState NewState);
};