#include "TimecodeComponent.h"
#include "TimecodeUtils.h"
#include "TimecodeSettings.h"
#include "Engine/World.h"

UTimecodeComponent::UTimecodeComponent()
{
    // ������Ʈ ����
    PrimaryComponentTick.bCanEverTick = true;

    // �⺻�� ����
    const UTimecodeSettings* Settings = GetDefault<UTimecodeSettings>();
    bIsMaster = false;
    bUseNDisplay = false;
    bAutoStart = true;
    FrameRate = Settings ? Settings->FrameRate : 30.0f;
    UDPPort = Settings ? Settings->DefaultUDPPort : 10000;
    TargetIP = TEXT("");
    MulticastGroup = Settings ? Settings->MulticastGroupAddress : TEXT("239.0.0.1");
    SyncInterval = Settings ? Settings->BroadcastInterval : 0.033f; // �� 30Hz

    // ���� ���� �ʱ�ȭ
    bIsRunning = false;
    ElapsedTimeSeconds = 0.0f;
    CurrentTimecode = TEXT("00:00:00:00");
    SyncTimer = 0.0f;
    NetworkManager = nullptr;
}

void UTimecodeComponent::BeginPlay()
{
    Super::BeginPlay();

    // ��Ʈ��ũ ����
    SetupNetwork();

    // �ڵ� ���� ����
    if (bAutoStart)
    {
        StartTimecode();
    }
}

void UTimecodeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // ��Ʈ��ũ ����
    ShutdownNetwork();

    Super::EndPlay(EndPlayReason);
}

void UTimecodeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Ÿ���ڵ� ���� ���� ��� ������Ʈ
    if (bIsRunning)
    {
        UpdateTimecode(DeltaTime);
        CheckTimecodeEvents();

        // ��Ʈ��ũ ����ȭ Ÿ�̸� ������Ʈ
        if (bIsMaster && NetworkManager)
        {
            SyncTimer += DeltaTime;
            if (SyncTimer >= SyncInterval)
            {
                SyncOverNetwork();
                SyncTimer = 0.0f;
            }
        }
    }
}

void UTimecodeComponent::StartTimecode()
{
    bIsRunning = true;

    UE_LOG(LogTemp, Log, TEXT("Timecode started"));
}

void UTimecodeComponent::StopTimecode()
{
    bIsRunning = false;

    UE_LOG(LogTemp, Log, TEXT("Timecode stopped"));
}

void UTimecodeComponent::ResetTimecode()
{
    ElapsedTimeSeconds = 0.0f;
    CurrentTimecode = TEXT("00:00:00:00");
    TriggeredEvents.Empty();

    // Ÿ���ڵ� ���� �̺�Ʈ �߻�
    OnTimecodeChanged.Broadcast(CurrentTimecode);

    UE_LOG(LogTemp, Log, TEXT("Timecode reset"));
}

void UTimecodeComponent::RegisterTimecodeEvent(const FString& EventName, float EventTimeInSeconds)
{
    if (EventTimeInSeconds >= 0.0f)
    {
        TimecodeEvents.Add(EventName, EventTimeInSeconds);
        UE_LOG(LogTemp, Log, TEXT("Timecode event registered: %s at %f seconds"), *EventName, EventTimeInSeconds);
    }
}

void UTimecodeComponent::UnregisterTimecodeEvent(const FString& EventName)
{
    TimecodeEvents.Remove(EventName);
    TriggeredEvents.Remove(EventName);

    UE_LOG(LogTemp, Log, TEXT("Timecode event unregistered: %s"), *EventName);
}

ENetworkConnectionState UTimecodeComponent::GetNetworkConnectionState() const
{
    if (NetworkManager)
    {
        return NetworkManager->GetConnectionState();
    }

    return ENetworkConnectionState::Disconnected;
}

bool UTimecodeComponent::SetupNetwork()
{
    // ���� ��Ʈ��ũ �Ŵ��� ����
    ShutdownNetwork();

    // ��Ʈ��ũ �Ŵ��� ����
    NetworkManager = NewObject<UTimecodeNetworkManager>(this);
    if (NetworkManager)
    {
        // �ݹ� ����
        NetworkManager->OnMessageReceived.AddDynamic(this, &UTimecodeComponent::OnTimecodeMessageReceived);
        NetworkManager->OnNetworkStateChanged.AddDynamic(this, &UTimecodeComponent::OnNetworkStateChanged);

        // ��Ʈ��ũ �ʱ�ȭ
        bool bSuccess = NetworkManager->Initialize(bIsMaster, UDPPort);

        // Ÿ�� IP ����
        if (!TargetIP.IsEmpty())
        {
            NetworkManager->SetTargetIP(TargetIP);
        }

        // ��Ƽĳ��Ʈ �׷� ����
        if (!MulticastGroup.IsEmpty())
        {
            NetworkManager->JoinMulticastGroup(MulticastGroup);
        }

        UE_LOG(LogTemp, Log, TEXT("Network setup %s"), bSuccess ? TEXT("successful") : TEXT("failed"));
        return bSuccess;
    }

    UE_LOG(LogTemp, Error, TEXT("Failed to create network manager"));
    return false;
}

void UTimecodeComponent::ShutdownNetwork()
{
    if (NetworkManager)
    {
        NetworkManager->Shutdown();
        NetworkManager = nullptr;

        UE_LOG(LogTemp, Log, TEXT("Network shutdown"));
    }
}

void UTimecodeComponent::UpdateTimecode(float DeltaTime)
{
    // ������ ��忡���� Ÿ���ڵ� ������Ʈ
    if (bIsMaster)
    {
        // ��� �ð� ������Ʈ
        ElapsedTimeSeconds += DeltaTime;

        // Ÿ���ڵ� ����
        FString NewTimecode = UTimecodeUtils::SecondsToTimecode(ElapsedTimeSeconds, FrameRate, false);

        // Ÿ���ڵ尡 ����� ��� �̺�Ʈ �߻�
        if (NewTimecode != CurrentTimecode)
        {
            CurrentTimecode = NewTimecode;
            OnTimecodeChanged.Broadcast(CurrentTimecode);
        }
    }
}

void UTimecodeComponent::CheckTimecodeEvents()
{
    // ��ϵ� ��� �̺�Ʈ Ȯ��
    for (const auto& Event : TimecodeEvents)
    {
        const FString& EventName = Event.Key;
        float EventTime = Event.Value;

        // ���� Ʈ���ŵ��� ���� �̺�Ʈ�̰�, ���� �ð��� �̺�Ʈ �ð��� �������� Ȯ��
        if (!TriggeredEvents.Contains(EventName) && ElapsedTimeSeconds >= EventTime)
        {
            // �̺�Ʈ Ʈ����
            OnTimecodeEventTriggered.Broadcast(EventName, EventTime);

            // Ʈ���ŵ� �̺�Ʈ�� ǥ��
            TriggeredEvents.Add(EventName);

            // �̺�Ʈ ��Ʈ��ũ ��ε�ĳ��Ʈ (������ ��忡����)
            if (bIsMaster && NetworkManager)
            {
                NetworkManager->SendEventMessage(EventName, CurrentTimecode);
            }

            UE_LOG(LogTemp, Log, TEXT("Timecode event triggered: %s at time %s"), *EventName, *CurrentTimecode);
        }
    }
}

void UTimecodeComponent::SyncOverNetwork()
{
    if (NetworkManager && bIsMaster)
    {
        // ���� Ÿ���ڵ� ��Ʈ��ũ�� ����
        NetworkManager->SendTimecodeMessage(CurrentTimecode, ETimecodeMessageType::TimecodeSync);
    }
}

void UTimecodeComponent::OnTimecodeMessageReceived(const FTimecodeNetworkMessage& Message)
{
    // �����̺� ��忡���� �޽��� ó��
    if (!bIsMaster)
    {
        switch (Message.MessageType)
        {
        case ETimecodeMessageType::TimecodeSync:
            // Ÿ���ڵ� ����ȭ �޽��� ó��
            if (Message.Timecode != CurrentTimecode)
            {
                CurrentTimecode = Message.Timecode;
                ElapsedTimeSeconds = UTimecodeUtils::TimecodeToSeconds(CurrentTimecode, FrameRate, false);
                OnTimecodeChanged.Broadcast(CurrentTimecode);

                UE_LOG(LogTemp, Verbose, TEXT("Received timecode sync: %s"), *CurrentTimecode);
            }
            break;

        case ETimecodeMessageType::Event:
            // �̺�Ʈ �޽��� ó��
            OnTimecodeEventTriggered.Broadcast(Message.Data, ElapsedTimeSeconds);

            UE_LOG(LogTemp, Log, TEXT("Received timecode event: %s"), *Message.Data);
            break;

        default:
            // ��Ÿ �޽��� Ÿ�� ����
            break;
        }
    }
}

void UTimecodeComponent::OnNetworkStateChanged(ENetworkConnectionState NewState)
{
    // ��Ʈ��ũ ���� ���� �̺�Ʈ �߻�
    OnNetworkConnectionChanged.Broadcast(NewState);

    UE_LOG(LogTemp, Log, TEXT("Network connection state changed: %d"), (int32)NewState);
}