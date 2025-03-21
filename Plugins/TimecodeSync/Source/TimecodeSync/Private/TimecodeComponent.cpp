#include "TimecodeComponent.h"
#include "TimecodeUtils.h"
#include "TimecodeSettings.h"
#include "Engine/World.h"

// nDisplay 모듈 포함 여부 확인
#if !defined(WITH_DISPLAYCLUSTER)
#define WITH_DISPLAYCLUSTER 0
#endif

#if WITH_DISPLAYCLUSTER
#include "DisplayClusterConfigurationTypes.h"
#include "DisplayClusterRootComponent.h"
#include "IDisplayCluster.h"
#include "IDisplayClusterClusterManager.h"
#endif

// 로그 카테고리 정의
DEFINE_LOG_CATEGORY_STATIC(LogTimecodeComponent, Log, All);

UTimecodeComponent::UTimecodeComponent()
{
    // 컴포넌트 설정
    PrimaryComponentTick.bCanEverTick = true;

    // 기본값 설정
    const UTimecodeSettings* Settings = GetDefault<UTimecodeSettings>();

    // 역할 관련 설정 초기화
    RoleMode = Settings ? Settings->RoleMode : ETimecodeRoleMode::Automatic;
    bIsMaster = false;
    bIsManuallyMaster = Settings ? Settings->bIsManualMaster : false;
    MasterIPAddress = Settings ? Settings->MasterIPAddress : TEXT("");
    bUseNDisplay = Settings ? Settings->bEnableNDisplayIntegration : false;

    // 타임코드 관련 설정 초기화
    FrameRate = Settings ? Settings->FrameRate : 30.0f;
    bUseDropFrameTimecode = Settings ? Settings->bUseDropFrameTimecode : false;
    bAutoStart = Settings ? Settings->bAutoStartTimecode : true;

    // 네트워크 설정 초기화
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
    ConnectionState = ENetworkConnectionState::Disconnected;
}

void UTimecodeComponent::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Timecode component initialized"), *GetOwner()->GetName());

    // 설정에 따라 역할 결정
    if (RoleMode == ETimecodeRoleMode::Automatic)
    {
        if (bUseNDisplay)
        {
            bIsMaster = CheckNDisplayRole();
            UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Using nDisplay role: %s"),
                *GetOwner()->GetName(), bIsMaster ? TEXT("MASTER") : TEXT("SLAVE"));
        }
    }
    else // Manual
    {
        bIsMaster = bIsManuallyMaster;
        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Using manual role: %s"),
            *GetOwner()->GetName(), bIsMaster ? TEXT("MASTER") : TEXT("SLAVE"));
    }

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

    UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Timecode component shutdown"), *GetOwner()->GetName());

    Super::EndPlay(EndPlayReason);
}

void UTimecodeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 타임코드 실행 중인 경우 업데이트
    if (bIsRunning)
    {
        // 마스터 모드에서만 타임코드 업데이트
        if (bIsMaster)
        {
            UpdateTimecode(DeltaTime);
        }

        // 이벤트 확인은 마스터/슬레이브 모두 수행
        CheckTimecodeEvents();

        // 네트워크 동기화 타이머 업데이트 (마스터만)
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
    if (!bIsRunning)
    {
        bIsRunning = true;
        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Timecode started"), *GetOwner()->GetName());
    }
}

void UTimecodeComponent::StopTimecode()
{
    if (bIsRunning)
    {
        bIsRunning = false;
        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Timecode stopped"), *GetOwner()->GetName());
    }
}

void UTimecodeComponent::ResetTimecode()
{
    ElapsedTimeSeconds = 0.0f;

    // 타임코드 문자열 업데이트
    CurrentTimecode = UTimecodeUtils::SecondsToTimecode(ElapsedTimeSeconds, FrameRate, bUseDropFrameTimecode);

    // 이벤트 트리거 상태 초기화
    TriggeredEvents.Empty();

    // 타임코드 변경 이벤트 발생
    OnTimecodeChanged.Broadcast(CurrentTimecode);

    UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Timecode reset"), *GetOwner()->GetName());
}

FString UTimecodeComponent::GetCurrentTimecode() const
{
    return CurrentTimecode;
}

float UTimecodeComponent::GetCurrentTimeInSeconds() const
{
    return ElapsedTimeSeconds;
}

void UTimecodeComponent::RegisterTimecodeEvent(const FString& EventName, float EventTimeInSeconds)
{
    if (EventTimeInSeconds >= 0.0f)
    {
        TimecodeEvents.Add(EventName, EventTimeInSeconds);
        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Timecode event registered: %s at %f seconds"),
            *GetOwner()->GetName(), *EventName, EventTimeInSeconds);
    }
    else
    {
        UE_LOG(LogTimecodeComponent, Warning, TEXT("[%s] Invalid event time: %f. Event times must be >= 0"),
            *GetOwner()->GetName(), EventTimeInSeconds);
    }
}

void UTimecodeComponent::UnregisterTimecodeEvent(const FString& EventName)
{
    if (TimecodeEvents.Remove(EventName) > 0)
    {
        TriggeredEvents.Remove(EventName);
        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Timecode event unregistered: %s"),
            *GetOwner()->GetName(), *EventName);
    }
    else
    {
        UE_LOG(LogTimecodeComponent, Warning, TEXT("[%s] Event not found: %s"),
            *GetOwner()->GetName(), *EventName);
    }
}

void UTimecodeComponent::ClearAllTimecodeEvents()
{
    int32 EventCount = TimecodeEvents.Num();
    TimecodeEvents.Empty();
    TriggeredEvents.Empty();

    UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Cleared %d timecode events"),
        *GetOwner()->GetName(), EventCount);
}

void UTimecodeComponent::ResetEventTriggers()
{
    int32 TriggerCount = TriggeredEvents.Num();
    TriggeredEvents.Empty();

    UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Reset %d event triggers"),
        *GetOwner()->GetName(), TriggerCount);
}

ENetworkConnectionState UTimecodeComponent::GetNetworkConnectionState() const
{
    return ConnectionState;
}

bool UTimecodeComponent::SetupNetwork()
{
    // 기존 네트워크 매니저 종료
    ShutdownNetwork();

    // 네트워크 매니저 생성
    NetworkManager = NewObject<UTimecodeNetworkManager>(this);
    if (NetworkManager)
    {
        // 역할 관련 설정 먼저 적용
        NetworkManager->SetRoleMode(RoleMode);

        if (RoleMode == ETimecodeRoleMode::Manual)
        {
            NetworkManager->SetManualMaster(bIsManuallyMaster);

            if (!bIsManuallyMaster && !MasterIPAddress.IsEmpty())
            {
                NetworkManager->SetMasterIPAddress(MasterIPAddress);
            }
        }

        // 콜백 설정
        NetworkManager->OnMessageReceived.AddDynamic(this, &UTimecodeComponent::OnTimecodeMessageReceived);
        NetworkManager->OnNetworkStateChanged.AddDynamic(this, &UTimecodeComponent::OnNetworkStateChanged);
        NetworkManager->OnRoleModeChanged.AddDynamic(this, &UTimecodeComponent::OnNetworkRoleModeChanged);

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

        // 연결 상태 업데이트
        ConnectionState = NetworkManager->GetConnectionState();

        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Network setup %s"),
            *GetOwner()->GetName(), bSuccess ? TEXT("successful") : TEXT("failed"));

        return bSuccess;
    }

    UE_LOG(LogTimecodeComponent, Error, TEXT("[%s] Failed to create network manager"),
        *GetOwner()->GetName());
    return false;
}

void UTimecodeComponent::ShutdownNetwork()
{
    if (NetworkManager)
    {
        NetworkManager->Shutdown();
        NetworkManager = nullptr;
        ConnectionState = ENetworkConnectionState::Disconnected;

        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Network shutdown"), *GetOwner()->GetName());
    }
}

bool UTimecodeComponent::JoinMulticastGroup(const FString& InMulticastGroup)
{
    if (NetworkManager)
    {
        bool bSuccess = NetworkManager->JoinMulticastGroup(InMulticastGroup);
        if (bSuccess)
        {
            MulticastGroup = InMulticastGroup;
        }
        return bSuccess;
    }
    return false;
}

void UTimecodeComponent::SetTargetIP(const FString& InTargetIP)
{
    if (TargetIP != InTargetIP)
    {
        TargetIP = InTargetIP;

        if (NetworkManager)
        {
            NetworkManager->SetTargetIP(TargetIP);
        }

        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Target IP set to: %s"),
            *GetOwner()->GetName(), *TargetIP);
    }
}

void UTimecodeComponent::SetRoleMode(ETimecodeRoleMode NewMode)
{
    if (RoleMode != NewMode)
    {
        ETimecodeRoleMode OldMode = RoleMode;
        RoleMode = NewMode;

        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Role mode changed from %s to %s"),
            *GetOwner()->GetName(),
            (OldMode == ETimecodeRoleMode::Automatic) ? TEXT("Automatic") : TEXT("Manual"),
            (RoleMode == ETimecodeRoleMode::Automatic) ? TEXT("Automatic") : TEXT("Manual"));

        // 네트워크 매니저에 역할 모드 설정
        if (NetworkManager)
        {
            NetworkManager->SetRoleMode(RoleMode);
        }
        else
        {
            // 네트워크 매니저가 없으면 직접 역할 결정
            bool bOldMaster = bIsMaster;

            if (RoleMode == ETimecodeRoleMode::Automatic)
            {
                if (bUseNDisplay)
                {
                    bIsMaster = CheckNDisplayRole();
                }
                else
                {
                    // 간단한 자동 역할 결정 (구현에 따라 달라짐)
                    bIsMaster = false; // 기본값은 슬레이브
                }
            }
            else // Manual
            {
                bIsMaster = bIsManuallyMaster;
            }

            // 역할이 변경되었으면 이벤트 발생
            if (bOldMaster != bIsMaster)
            {
                OnRoleStateChanged(bIsMaster);
            }
        }

        // 역할 모드 변경 이벤트 발생
        OnRoleModeChanged.Broadcast(RoleMode);
    }
}

ETimecodeRoleMode UTimecodeComponent::GetRoleMode() const
{
    return RoleMode;
}

void UTimecodeComponent::SetManualMaster(bool bInIsManuallyMaster)
{
    if (bIsManuallyMaster != bInIsManuallyMaster)
    {
        bIsManuallyMaster = bInIsManuallyMaster;

        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Manual master setting changed to: %s"),
            *GetOwner()->GetName(), bIsManuallyMaster ? TEXT("MASTER") : TEXT("SLAVE"));

        // 네트워크 매니저에 수동 마스터 설정
        if (NetworkManager)
        {
            NetworkManager->SetManualMaster(bIsManuallyMaster);
        }
        else if (RoleMode == ETimecodeRoleMode::Manual)
        {
            // 네트워크 매니저가 없고 수동 모드인 경우 직접 역할 설정
            bool bOldMaster = bIsMaster;
            bIsMaster = bIsManuallyMaster;

            // 역할이 변경되었으면 이벤트 발생
            if (bOldMaster != bIsMaster)
            {
                OnRoleStateChanged(bIsMaster);
            }
        }
    }
}

bool UTimecodeComponent::GetIsManuallyMaster() const
{
    return bIsManuallyMaster;
}

void UTimecodeComponent::SetMasterIPAddress(const FString& InMasterIP)
{
    if (MasterIPAddress != InMasterIP)
    {
        MasterIPAddress = InMasterIP;

        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Master IP address changed to: %s"),
            *GetOwner()->GetName(), *MasterIPAddress);

        // 네트워크 매니저에 마스터 IP 설정
        if (NetworkManager)
        {
            NetworkManager->SetMasterIPAddress(MasterIPAddress);
        }
    }
}

FString UTimecodeComponent::GetMasterIPAddress() const
{
    return MasterIPAddress;
}

bool UTimecodeComponent::GetIsMaster() const
{
    return bIsMaster;
}

void UTimecodeComponent::UpdateTimecode(float DeltaTime)
{
    // 경과 시간 업데이트
    ElapsedTimeSeconds += DeltaTime;

    // 타임코드 생성
    FString NewTimecode = UTimecodeUtils::SecondsToTimecode(ElapsedTimeSeconds, FrameRate, bUseDropFrameTimecode);

    // 타임코드가 변경된 경우 이벤트 발생
    if (NewTimecode != CurrentTimecode)
    {
        CurrentTimecode = NewTimecode;
        OnTimecodeChanged.Broadcast(CurrentTimecode);

        UE_LOG(LogTimecodeComponent, Verbose, TEXT("[%s] Timecode updated: %s"),
            *GetOwner()->GetName(), *CurrentTimecode);
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

            UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Timecode event triggered: %s at time %s"),
                *GetOwner()->GetName(), *EventName, *CurrentTimecode);
        }
    }
}

void UTimecodeComponent::SyncOverNetwork()
{
    if (NetworkManager && bIsMaster)
    {
        // 현재 타임코드 네트워크로 전송
        NetworkManager->SendTimecodeMessage(CurrentTimecode, ETimecodeMessageType::TimecodeSync);

        UE_LOG(LogTimecodeComponent, Verbose, TEXT("[%s] Timecode synced over network: %s"),
            *GetOwner()->GetName(), *CurrentTimecode);
    }
}

void UTimecodeComponent::OnRoleStateChanged(bool bNewIsMaster)
{
    // 역할이 변경된 경우 처리
    bIsMaster = bNewIsMaster;

    // 역할 변경 이벤트 발생
    OnRoleChanged.Broadcast(bIsMaster);

    UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Role changed to: %s"),
        *GetOwner()->GetName(), bIsMaster ? TEXT("MASTER") : TEXT("SLAVE"));

    // 마스터 변경 시 타임코드 관련 처리
    if (bIsRunning)
    {
        if (bIsMaster)
        {
            // 마스터가 된 경우 타임코드 즉시 동기화
            SyncOverNetwork();
        }
        else
        {
            // 슬레이브가 된 경우 로컬 변경 중지 (마스터에서 수신한 타임코드 사용)
        }
    }
}

bool UTimecodeComponent::CheckNDisplayRole()
{
#if WITH_DISPLAYCLUSTER
    // nDisplay 모듈이 있는 경우
    if (IDisplayCluster::IsAvailable())
    {
        // nDisplay 클러스터 매니저 가져오기
        IDisplayClusterClusterManager* ClusterManager = IDisplayCluster::Get().GetClusterMgr();
        if (ClusterManager)
        {
            // 현재 노드가 마스터인지 확인
            bool bIsNDisplayMaster = ClusterManager->IsMaster();

            UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] nDisplay role: %s"),
                *GetOwner()->GetName(), bIsNDisplayMaster ? TEXT("MASTER") : TEXT("SLAVE"));

            return bIsNDisplayMaster;
        }
    }
#endif

    // nDisplay가 없거나 사용할 수 없는 경우 기본값 반환
    UE_LOG(LogTimecodeComponent, Warning, TEXT("[%s] Cannot determine nDisplay role, defaulting to SLAVE"),
        *GetOwner()->GetName());
    return false;
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

                // 타임코드 문자열을 시간(초)으로 변환
                ElapsedTimeSeconds = UTimecodeUtils::TimecodeToSeconds(CurrentTimecode, FrameRate, bUseDropFrameTimecode);

                // 타임코드 변경 이벤트 발생
                OnTimecodeChanged.Broadcast(CurrentTimecode);

                UE_LOG(LogTimecodeComponent, Verbose, TEXT("[%s] Received timecode sync: %s"),
                    *GetOwner()->GetName(), *CurrentTimecode);
            }
            break;

        case ETimecodeMessageType::Event:
            // 이벤트 메시지 처리
            if (!Message.Data.IsEmpty())
            {
                // 이벤트 이름으로 이벤트 시간 찾기
                float* EventTimePtr = TimecodeEvents.Find(Message.Data);
                float EventTime = EventTimePtr ? *EventTimePtr : ElapsedTimeSeconds;

                // 이벤트 트리거
                OnTimecodeEventTriggered.Broadcast(Message.Data, EventTime);

                // 트리거된 이벤트로 표시
                TriggeredEvents.Add(Message.Data);

                UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Received timecode event: %s"),
                    *GetOwner()->GetName(), *Message.Data);
            }
            break;

        case ETimecodeMessageType::RoleAssignment:
            // 역할 할당 메시지 처리 (자동 모드에서만)
            if (RoleMode == ETimecodeRoleMode::Automatic)
            {
                if (Message.Data.Equals(TEXT("MASTER"), ESearchCase::IgnoreCase))
                {
                    if (!bIsMaster)
                    {
                        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Role changed to MASTER by network message"),
                            *GetOwner()->GetName());
                        OnRoleStateChanged(true);
                    }
                }
                else if (Message.Data.Equals(TEXT("SLAVE"), ESearchCase::IgnoreCase))
                {
                    if (bIsMaster)
                    {
                        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Role changed to SLAVE by network message"),
                            *GetOwner()->GetName());
                        OnRoleStateChanged(false);
                    }
                }
            }
            break;

        default:
            // 기타 메시지 타입 무시
            break;
        }
    }
}

void UTimecodeComponent::OnNetworkStateChanged(ENetworkConnectionState NewState)
{
    // 네트워크 상태 업데이트
    ConnectionState = NewState;

    // 네트워크 상태 변경 이벤트 발생
    OnNetworkConnectionChanged.Broadcast(ConnectionState);

    UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Network connection state changed: %s"),
        *GetOwner()->GetName(),
        ConnectionState == ENetworkConnectionState::Connected ? TEXT("Connected") :
        ConnectionState == ENetworkConnectionState::Connecting ? TEXT("Connecting") :
        TEXT("Disconnected"));

    // 연결이 끊어진 경우 처리
    if (ConnectionState == ENetworkConnectionState::Disconnected)
    {
        if (!bIsMaster && bIsRunning)
        {
            UE_LOG(LogTimecodeComponent, Warning,
                TEXT("[%s] Network disconnected in slave mode, timecode sync may be affected"),
                *GetOwner()->GetName());
        }
    }
}

void UTimecodeComponent::OnNetworkRoleModeChanged(ETimecodeRoleMode NewMode)
{
    // 네트워크 매니저의 역할 모드가 변경된 경우 컴포넌트에도 적용
    if (RoleMode != NewMode)
    {
        RoleMode = NewMode;

        // 역할 모드 변경 이벤트 발생
        OnRoleModeChanged.Broadcast(RoleMode);

        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Role mode changed by network: %s"),
            *GetOwner()->GetName(),
            (RoleMode == ETimecodeRoleMode::Automatic) ? TEXT("Automatic") : TEXT("Manual"));
    }
}