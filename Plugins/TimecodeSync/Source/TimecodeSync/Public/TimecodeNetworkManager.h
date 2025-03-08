#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "TimecodeNetworkTypes.h"
#include "TimecodeNetworkManager.generated.h"

class FSocket;
class FUdpSocketReceiver;

// ��Ʈ��ũ ���� ���� ������
UENUM(BlueprintType)
enum class ENetworkConnectionState : uint8
{
    Disconnected,  // ������� ����
    Connecting,    // ���� ��
    Connected      // �����
};

// Ÿ���ڵ� �޽��� ���� ��������Ʈ
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimecodeMessageReceived, const FTimecodeNetworkMessage&, Message);

// ��Ʈ��ũ ���� ���� ��������Ʈ
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNetworkStateChanged, ENetworkConnectionState, NewState);

/**
 * Ÿ���ڵ� ��Ʈ��ũ ����� �����ϴ� Ŭ����
 */
UCLASS(BlueprintType)
class TIMECODESYNC_API UTimecodeNetworkManager : public UObject
{
    GENERATED_BODY()

public:
    UTimecodeNetworkManager();
    virtual ~UTimecodeNetworkManager();

    // ��Ʈ��ũ �ʱ�ȭ (������/�����̺� ���)
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool Initialize(bool bIsMaster, int32 Port);

    // ��Ʈ��ũ ����
    UFUNCTION(BlueprintCallable, Category = "Network")
    void Shutdown();

    // Ÿ���ڵ� �޽��� ����
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool SendTimecodeMessage(const FString& Timecode, ETimecodeMessageType MessageType = ETimecodeMessageType::TimecodeSync);

    // �̺�Ʈ �޽��� ����
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool SendEventMessage(const FString& EventName, const FString& Timecode);

    // Ÿ�� IP ����
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SetTargetIP(const FString& IPAddress);

    // ��Ƽĳ��Ʈ �׷� ����
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool JoinMulticastGroup(const FString& MulticastGroup);

    // ���� ���� Ȯ��
    UFUNCTION(BlueprintCallable, Category = "Network")
    ENetworkConnectionState GetConnectionState() const;

    // �޽��� ���� �̺�Ʈ
    UPROPERTY(BlueprintAssignable, Category = "Network")
    FOnTimecodeMessageReceived OnMessageReceived;

    // ��Ʈ��ũ ���� ���� �̺�Ʈ
    UPROPERTY(BlueprintAssignable, Category = "Network")
    FOnNetworkStateChanged OnNetworkStateChanged;

private:
    // UDP ����
    FSocket* Socket;

    // UDP ���ű�
    FUdpSocketReceiver* Receiver;

    // ���� ����
    ENetworkConnectionState ConnectionState;

    // �۽��� ID (���� �ĺ���)
    FString InstanceID;

    // ��Ʈ ��ȣ
    int32 PortNumber;

    // Ÿ�� IP �ּ�
    FString TargetIPAddress;

    // ��Ƽĳ��Ʈ �׷� �ּ�
    FString MulticastGroupAddress;

    // ������ ��� ����
    bool bIsMasterMode;

    // UDP ���� �ݹ�
    void OnUDPReceived(const FArrayReaderPtr& DataPtr, const FIPv4Endpoint& Endpoint);

    // ���� ���� �Լ�
    bool CreateSocket();

    // �޽��� ó�� �Լ�
    void ProcessMessage(const FTimecodeNetworkMessage& Message);

    // ���� ���� ���� �Լ�
    void SetConnectionState(ENetworkConnectionState NewState);

    // ��Ʈ��Ʈ �޽��� ����
    void SendHeartbeat();
};
