#include "TimecodeComponent.h"
#include "TimecodeUtils.h"
#include "TimecodeSettings.h"
#include "Engine/World.h"

UTimecodeComponent::UTimecodeComponent()
{
    // 컴포넌트 설정
    PrimaryComponentTick.bCanEverTick = true;

    // 기본값 설정
    const UTimecodeSettings* Settings = GetDefault<UTimecodeSettings>();
    bIsMaster = false;
    bUseNDisplay = false;
    bAutoStart = true;
    FrameRate = Settings ? Settings->FrameRate : 30.0f;
    UDPPort = Settings ? Settings->DefaultUDPPort : 10000;
    TargetIP = TEXT("");
    MulticastGroup = Settings ? Settings->MulticastGroupAddress : TEXT("239.0.0.1");
    SyncInterval = Settings ? Settings->BroadcastInterval : 0.033f; // 약 30Hz

    // 내부 변수 초기화
    bIsRunning = false;
    ElapsedTimeSeconds = 0.0f;
    CurrentTimecode = TEXT("00:00:00:00");
    SyncTimer = 0.0f;
    NetworkManager = nullptr;
}

void UTimecodeComponent::BeginPlay()
{
    Super::BeginPlay();

    // 네트워크 설정
    SetupNetwork();

    // 자동 시작 설정
    if (bAutoStart)
    {
        StartTimecode();
    }
}

void UTimecodeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 네트워크 종료
    ShutdownNetwork();

    Super::EndPlay(EndPlayReason);
}

void UTimecodeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 타임코드 실행 중인 경우 업데이트
    if (bIsRunning)
    {
        UpdateTimecode(DeltaTime);
        CheckTimecodeEvents();

        // 네트워크 동기화 타이머 업데이트
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

    // 타임코드 변경 이벤트 발생
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
    // 기존 네트워크 매니저 종료
    ShutdownNetwork();

    // 네트워크 매니저 생성
    NetworkManager = NewObject<UTimecodeNetworkManager>(this);
    if (NetworkManager)
    {
        // 콜백 설정
        NetworkManager->OnMessageReceived.AddDynamic(this, &UTimecodeComponent::OnTimecodeMessageReceived);
        NetworkManager->OnNetworkStateChanged.AddDynamic(this, &UTimecodeComponent::OnNetworkStateChanged);

        // 네트워크 초기화
        bool bSuccess = NetworkManager->Initialize(bIsMaster, UDPPort);

        // 타겟 IP 설정
        if (!TargetIP.IsEmpty())
        {
            NetworkManager->SetTargetIP(TargetIP);
        }

        // 멀티캐스트 그룹 참가
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
    // 마스터 모드에서만 타임코드 업데이트
    if (bIsMaster)
    {
        // 경과 시간 업데이트
        ElapsedTimeSeconds += DeltaTime;

        // 타임코드 생성
        FString NewTimecode = UTimecodeUtils::SecondsToTimecode(ElapsedTimeSeconds, FrameRate, false);

        // 타임코드가 변경된 경우 이벤트 발생
        if (NewTimecode != CurrentTimecode)
        {
            CurrentTimecode = NewTimecode;
            OnTimecodeChanged.Broadcast(CurrentTimecode);
        }
    }
}

void UTimecodeComponent::CheckTimecodeEvents()
{
    // 등록된 모든 이벤트 확인
    for (const auto& Event : TimecodeEvents)
    {
        const FString& EventName = Event.Key;
        float EventTime = Event.Value;

        // 아직 트리거되지 않은 이벤트이고, 현재 시간이 이벤트 시간을 지났는지 확인
        if (!TriggeredEvents.Contains(EventName) && ElapsedTimeSeconds >= EventTime)
        {
            // 이벤트 트리거
            OnTimecodeEventTriggered.Broadcast(EventName, EventTime);

            // 트리거된 이벤트로 표시
            TriggeredEvents.Add(EventName);

            // 이벤트 네트워크 브로드캐스트 (마스터 모드에서만)
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
        // 현재 타임코드 네트워크로 전송
        NetworkManager->SendTimecodeMessage(CurrentTimecode, ETimecodeMessageType::TimecodeSync);
    }
}

void UTimecodeComponent::OnTimecodeMessageReceived(const FTimecodeNetworkMessage& Message)
{
    // 슬레이브 모드에서만 메시지 처리
    if (!bIsMaster)
    {
        switch (Message.MessageType)
        {
        case ETimecodeMessageType::TimecodeSync:
            // 타임코드 동기화 메시지 처리
            if (Message.Timecode != CurrentTimecode)
            {
                CurrentTimecode = Message.Timecode;
                ElapsedTimeSeconds = UTimecodeUtils::TimecodeToSeconds(CurrentTimecode, FrameRate, false);
                OnTimecodeChanged.Broadcast(CurrentTimecode);

                UE_LOG(LogTemp, Verbose, TEXT("Received timecode sync: %s"), *CurrentTimecode);
            }
            break;

        case ETimecodeMessageType::Event:
            // 이벤트 메시지 처리
            OnTimecodeEventTriggered.Broadcast(Message.Data, ElapsedTimeSeconds);

            UE_LOG(LogTemp, Log, TEXT("Received timecode event: %s"), *Message.Data);
            break;

        default:
            // 기타 메시지 타입 무시
            break;
        }
    }
}

void UTimecodeComponent::OnNetworkStateChanged(ENetworkConnectionState NewState)
{
    // 네트워크 상태 변경 이벤트 발생
    OnNetworkConnectionChanged.Broadcast(NewState);

    UE_LOG(LogTemp, Log, TEXT("Network connection state changed: %d"), (int32)NewState);
}