// TimecodeComponent.cpp

#include "TimecodeComponent.h"
#include "TimecodeNetworkManager.h"  // 이 헤더 추가
#include "TimecodeUtils.h"
#include "TimecodeSettings.h"
#include "PLLSynchronizer.h"
#include "SMPTETimecodeConverter.h"
#include "Engine/World.h"

// Check if nDisplay module is included
#if !defined(WITH_DISPLAYCLUSTER)
#define WITH_DISPLAYCLUSTER 0
#endif

#if WITH_DISPLAYCLUSTER
#include "DisplayClusterConfigurationTypes.h"
#include "DisplayClusterRootComponent.h"
#include "IDisplayCluster.h"
#include "IDisplayClusterClusterManager.h"
#endif

// Define log category
DEFINE_LOG_CATEGORY_STATIC(LogTimecodeComponent, Log, All);

UTimecodeComponent::UTimecodeComponent()
{
    // Component settings
    PrimaryComponentTick.bCanEverTick = true;

    // Set default values
    const UTimecodeSettings* Settings = GetDefault<UTimecodeSettings>();

    // Initialize role-related settings
    RoleMode = Settings ? Settings->RoleMode : ETimecodeRoleMode::Automatic;
    bIsMaster = false;
    bIsManuallyMaster = Settings ? Settings->bIsManualMaster : false;
    MasterIPAddress = Settings ? Settings->MasterIPAddress : TEXT("");
    bUseNDisplay = Settings ? Settings->bEnableNDisplayIntegration : false;
    bIsDedicatedMaster = Settings ? Settings->bIsDedicatedMaster : false;

    // Initialize timecode-related settings
    FrameRate = Settings ? Settings->FrameRate : 30.0f;
    bUseDropFrameTimecode = Settings ? Settings->bUseDropFrameTimecode : false;
    bAutoStart = Settings ? Settings->bAutoStartTimecode : true;

    // Initialize network settings
    UDPPort = Settings ? Settings->DefaultUDPPort : 10000;
    TargetIP = TEXT("");
    MulticastGroup = Settings ? Settings->MulticastGroupAddress : TEXT("239.0.0.1");
    SyncInterval = Settings ? Settings->BroadcastInterval : 0.033f; // Approximately 30Hz
    TargetPortNumber = Settings ? (Settings->DefaultUDPPort + 1) : 10001; // 기본값은 UDPPort + 1

    // PLL 설정 초기화
    bUsePLL = true;                // PLL 기본적으로 활성화
    PLLBandwidth = 0.1f;           // 기본 대역폭
    PLLDamping = 1.0f;             // 기본 감쇠 계수

    // Initialize internal variables
    bIsRunning = false;
    ElapsedTimeSeconds = 0.0f;
    CurrentTimecode = TEXT("00:00:00:00");
    SyncTimer = 0.0f;
    NetworkManager = nullptr;
    ConnectionState = ENetworkConnectionState::Disconnected;

    // 타임코드 모드 기본 설정
    TimecodeMode = ETimecodeMode::Integrated;

    // 서브모듈 생성
    PLLSynchronizer = CreateDefaultSubobject<UPLLSynchronizer>(TEXT("PLLSynchronizer"));
    SMPTEConverter = CreateDefaultSubobject<USMPTETimecodeConverter>(TEXT("SMPTEConverter"));
}

void UTimecodeComponent::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Timecode component initialized"), *GetOwner()->GetName());

    // 서브모듈 초기화
    if (PLLSynchronizer)
    {
        PLLSynchronizer->Initialize();
    }

    // Determine role based on settings
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

    // Setup network
    SetupNetwork();

    // 타임코드 모드 적용
    ApplyTimecodeMode();

    // Auto start setting
    if (bAutoStart)
    {
        StartTimecode();
    }
}

void UTimecodeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Shutdown network
    ShutdownNetwork();

    UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Timecode component shutdown"), *GetOwner()->GetName());

    Super::EndPlay(EndPlayReason);
}

void UTimecodeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 타임코드가 실행 중일 때만 업데이트
    if (bIsRunning)
    {
        // 현재 모드에 따라 적절한 업데이트 함수 호출
        if (bIsMaster)
        {
            switch (TimecodeMode)
            {
            case ETimecodeMode::Raw:
                UpdateRawTimecode(DeltaTime);
                break;
            case ETimecodeMode::PLL_Only:
                UpdatePLLTimecode(DeltaTime);
                break;
            case ETimecodeMode::SMPTE_Only:
                UpdateSMPTETimecode(DeltaTime);
                break;
            case ETimecodeMode::Integrated:
            default:
                UpdateIntegratedTimecode(DeltaTime);
                break;
            }
        }
        else if (bUsePLL && PLLSynchronizer)
        {
            // 슬레이브 모드에서는 PLL 업데이트 (모드에 상관없이 PLL이 활성화된 경우)
            PLLSynchronizer->Update(DeltaTime);
        }

        // 이벤트 확인 (모든 모드 공통)
        CheckTimecodeEvents();

        // 네트워크 동기화 (마스터 모드일 때만)
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

    // Update timecode string
    if (SMPTEConverter)
    {
        CurrentTimecode = SMPTEConverter->SecondsToTimecode(ElapsedTimeSeconds, FrameRate, bUseDropFrameTimecode);
    }
    else
    {
        CurrentTimecode = UTimecodeUtils::SecondsToTimecode(ElapsedTimeSeconds, FrameRate, bUseDropFrameTimecode);
    }

    // Reset event trigger states
    TriggeredEvents.Empty();

    // Trigger timecode change event
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
    // Shutdown existing network manager
    ShutdownNetwork();

    // Create network manager
    NetworkManager = NewObject<UTimecodeNetworkManager>(this);
    if (NetworkManager)
    {
        // Apply role settings first
        NetworkManager->SetRoleMode(RoleMode);

        if (RoleMode == ETimecodeRoleMode::Manual)
        {
            NetworkManager->SetManualMaster(bIsManuallyMaster);

            if (!bIsManuallyMaster && !MasterIPAddress.IsEmpty())
            {
                NetworkManager->SetMasterIPAddress(MasterIPAddress);
            }
        }

        // 여기에 추가: 전용 마스터 설정 적용
        NetworkManager->SetDedicatedMaster(bIsDedicatedMaster);

        // Apply PLL settings
        NetworkManager->SetUsePLL(bUsePLL);
        NetworkManager->SetPLLParameters(PLLBandwidth, PLLDamping);

        // Setup callbacks
        NetworkManager->OnMessageReceived.AddDynamic(this, &UTimecodeComponent::OnTimecodeMessageReceived);
        NetworkManager->OnNetworkStateChanged.AddDynamic(this, &UTimecodeComponent::OnNetworkStateChanged);
        NetworkManager->OnRoleModeChanged.AddDynamic(this, &UTimecodeComponent::OnNetworkRoleModeChanged);

        // Initialize network
        bool bSuccess = NetworkManager->Initialize(bIsMaster, UDPPort);

        // Set target port
        NetworkManager->SetTargetPort(TargetPortNumber);

        // Join multicast group
        if (!MulticastGroup.IsEmpty())
        {
            NetworkManager->JoinMulticastGroup(MulticastGroup);
        }

        // Update connection state
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

        // Set role mode in network manager
        if (NetworkManager)
        {
            NetworkManager->SetRoleMode(RoleMode);
        }
        else
        {
            // Determine role directly if network manager is not available
            bool bOldMaster = bIsMaster;

            if (RoleMode == ETimecodeRoleMode::Automatic)
            {
                if (bUseNDisplay)
                {
                    bIsMaster = CheckNDisplayRole();
                }
                else
                {
                    // Simple automatic role determination (implementation may vary)
                    bIsMaster = false; // Default to slave
                }
            }
            else // Manual
            {
                bIsMaster = bIsManuallyMaster;
            }

            // Trigger role change event if role changed
            if (bOldMaster != bIsMaster)
            {
                OnRoleStateChanged(bIsMaster);
            }
        }

        // Trigger role mode change event
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

        // Set role mode in network manager
        if (NetworkManager)
        {
            NetworkManager->SetManualMaster(bIsManuallyMaster);
        }
        else if (RoleMode == ETimecodeRoleMode::Manual)
        {
            // Determine role directly if network manager is not available
            bool bOldMaster = bIsMaster;
            bIsMaster = bIsManuallyMaster;

            // Trigger role change event if role changed
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

        // Set master IP address in network manager
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
    // Update elapsed time
    ElapsedTimeSeconds += DeltaTime;

    // Generate timecode using SMPTE converter module
    FString NewTimecode;

    if (SMPTEConverter)
    {
        NewTimecode = SMPTEConverter->SecondsToTimecode(ElapsedTimeSeconds, FrameRate, bUseDropFrameTimecode);
    }
    else
    {
        // Fallback to direct function call
        NewTimecode = UTimecodeUtils::SecondsToTimecode(ElapsedTimeSeconds, FrameRate, bUseDropFrameTimecode);
    }

    // Trigger event if timecode changed
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
    // Check all registered events
    for (const auto& Event : TimecodeEvents)
    {
        const FString& EventName = Event.Key;
        float EventTime = Event.Value;

        // Check if event is not triggered and current time has passed event time
        if (!TriggeredEvents.Contains(EventName) && ElapsedTimeSeconds >= EventTime)
        {
            // Trigger event
            OnTimecodeEventTriggered.Broadcast(EventName, EventTime);

            // Mark as triggered
            TriggeredEvents.Add(EventName);

            // Broadcast event over network (master mode only)
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
        // Send current timecode over network
        NetworkManager->SendTimecodeMessage(CurrentTimecode, ETimecodeMessageType::TimecodeSync);

        UE_LOG(LogTimecodeComponent, Verbose, TEXT("[%s] Timecode synced over network: %s"),
            *GetOwner()->GetName(), *CurrentTimecode);
    }
}

void UTimecodeComponent::OnRoleStateChanged(bool bNewIsMaster)
{
    // Handle role change
    bIsMaster = bNewIsMaster;

    // Trigger role change event
    OnRoleChanged.Broadcast(bIsMaster);

    UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Role changed to: %s"),
        *GetOwner()->GetName(), bIsMaster ? TEXT("MASTER") : TEXT("SLAVE"));

    // Handle timecode-related tasks on master change
    if (bIsRunning)
    {
        if (bIsMaster)
        {
            // Sync timecode immediately when becoming master
            SyncOverNetwork();
        }
        else
        {
            // Stop local changes when becoming slave (use timecode received from master)
        }
    }
}

bool UTimecodeComponent::CheckNDisplayRole()
{
#if WITH_DISPLAYCLUSTER
    // If nDisplay module is available
    if (IDisplayCluster::IsAvailable())
    {
        // Get nDisplay cluster manager
        IDisplayClusterClusterManager* ClusterManager = IDisplayCluster::Get().GetClusterMgr();
        if (ClusterManager)
        {
            // Check if current node is master
            bool bIsNDisplayMaster = ClusterManager->IsMaster();

            UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] nDisplay role: %s"),
                *GetOwner()->GetName(), bIsNDisplayMaster ? TEXT("MASTER") : TEXT("SLAVE"));

            return bIsNDisplayMaster;
        }
    }
#endif

    // Return default value if nDisplay is not available or cannot be used
    UE_LOG(LogTimecodeComponent, Warning, TEXT("[%s] Cannot determine nDisplay role, defaulting to SLAVE"),
        *GetOwner()->GetName());
    return false;
}

void UTimecodeComponent::OnTimecodeMessageReceived(const FTimecodeNetworkMessage& Message)
{
    // Process messages only in slave mode
    if (!bIsMaster)
    {
        switch (Message.MessageType)
        {
        case ETimecodeMessageType::TimecodeSync:
            // Process timecode sync message
            if (Message.Timecode != CurrentTimecode)
            {
                CurrentTimecode = Message.Timecode;

                // Convert timecode string to seconds using SMPTE module
                float ReceivedTimeSeconds;

                if (SMPTEConverter)
                {
                    ReceivedTimeSeconds = SMPTEConverter->TimecodeToSeconds(CurrentTimecode, FrameRate, bUseDropFrameTimecode);
                }
                else
                {
                    // Fallback to direct function call
                    ReceivedTimeSeconds = UTimecodeUtils::TimecodeToSeconds(CurrentTimecode, FrameRate, bUseDropFrameTimecode);
                }

                // Apply PLL correction to local time if enabled
                if (bUsePLL && PLLSynchronizer)
                {
                    // Process local time through PLL
                    ElapsedTimeSeconds = PLLSynchronizer->ProcessTime(ElapsedTimeSeconds, ReceivedTimeSeconds, GetWorld()->GetDeltaSeconds());

                    // Get PLL status for logging
                    double Phase, Frequency, Offset;
                    PLLSynchronizer->GetStatus(Phase, Frequency, Offset);

                    UE_LOG(LogTimecodeComponent, Verbose, TEXT("[%s] PLL Correction - Freq: %.6f, Offset: %.3fms"),
                        *GetOwner()->GetName(), Frequency, Offset * 1000.0);
                }
                else
                {
                    // Without PLL, directly use the received time
                    ElapsedTimeSeconds = ReceivedTimeSeconds;
                }

                // Trigger timecode change event
                OnTimecodeChanged.Broadcast(CurrentTimecode);

                UE_LOG(LogTimecodeComponent, Verbose, TEXT("[%s] Received timecode sync: %s"),
                    *GetOwner()->GetName(), *CurrentTimecode);
            }
            break;

        case ETimecodeMessageType::Event:
            // Process event message
            if (!Message.Data.IsEmpty())
            {
                // Find event time by event name
                float* EventTimePtr = TimecodeEvents.Find(Message.Data);
                float EventTime = EventTimePtr ? *EventTimePtr : ElapsedTimeSeconds;

                // Trigger event
                OnTimecodeEventTriggered.Broadcast(Message.Data, EventTime);

                // Mark as triggered
                TriggeredEvents.Add(Message.Data);

                UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Received timecode event: %s"),
                    *GetOwner()->GetName(), *Message.Data);
            }
            break;

        case ETimecodeMessageType::RoleAssignment:
            // Process role assignment message (automatic mode only)
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
            // Ignore other message types
            break;
        }
    }
}

void UTimecodeComponent::OnNetworkStateChanged(ENetworkConnectionState NewState)
{
    // Update network state
    ConnectionState = NewState;

    // Trigger network state change event
    OnNetworkConnectionChanged.Broadcast(ConnectionState);

    UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Network connection state changed: %s"),
        *GetOwner()->GetName(),
        ConnectionState == ENetworkConnectionState::Connected ? TEXT("Connected") :
        ConnectionState == ENetworkConnectionState::Connecting ? TEXT("Connecting") :
        TEXT("Disconnected"));

    // Handle disconnection
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
    // Apply network manager's role mode change to component
    if (RoleMode != NewMode)
    {
        RoleMode = NewMode;

        // Trigger role mode change event
        OnRoleModeChanged.Broadcast(RoleMode);

        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Role mode changed by network: %s"),
            *GetOwner()->GetName(),
            (RoleMode == ETimecodeRoleMode::Automatic) ? TEXT("Automatic") : TEXT("Manual"));
    }
}

void UTimecodeComponent::LogDebugInfo()
{
    // Basic component info
    UE_LOG(LogTimecodeComponent, Display, TEXT("===== Timecode Component Debug Info ====="));
    UE_LOG(LogTimecodeComponent, Display, TEXT("Owner: %s"), *GetOwner()->GetName());
    UE_LOG(LogTimecodeComponent, Display, TEXT("Current Timecode: %s"), *CurrentTimecode);
    UE_LOG(LogTimecodeComponent, Display, TEXT("Elapsed Time: %.3f seconds"), ElapsedTimeSeconds);

    // 타임코드 모드 정보 추가
    UE_LOG(LogTimecodeComponent, Display, TEXT("Timecode Mode: %s"),
        TimecodeMode == ETimecodeMode::PLL_Only ? TEXT("PLL Only") :
        TimecodeMode == ETimecodeMode::SMPTE_Only ? TEXT("SMPTE Only") :
        TimecodeMode == ETimecodeMode::Integrated ? TEXT("Integrated") :
        TEXT("Raw"));

    // Role info
    UE_LOG(LogTimecodeComponent, Display, TEXT("Role Mode: %s"),
        (RoleMode == ETimecodeRoleMode::Automatic) ? TEXT("Automatic") : TEXT("Manual"));
    UE_LOG(LogTimecodeComponent, Display, TEXT("Current Role: %s"), bIsMaster ? TEXT("MASTER") : TEXT("SLAVE"));
    UE_LOG(LogTimecodeComponent, Display, TEXT("Manual Master Setting: %s"), bIsManuallyMaster ? TEXT("MASTER") : TEXT("SLAVE"));

    if (!bIsMaster && RoleMode == ETimecodeRoleMode::Manual)
    {
        UE_LOG(LogTimecodeComponent, Display, TEXT("Master IP: %s"), *MasterIPAddress);
    }

    // Settings info
    UE_LOG(LogTimecodeComponent, Display, TEXT("Frame Rate: %.2f fps"), FrameRate);
    UE_LOG(LogTimecodeComponent, Display, TEXT("Drop Frame: %s"), bUseDropFrameTimecode ? TEXT("Yes") : TEXT("No"));
    UE_LOG(LogTimecodeComponent, Display, TEXT("Running: %s"), bIsRunning ? TEXT("Yes") : TEXT("No"));

    // Network info
    UE_LOG(LogTimecodeComponent, Display, TEXT("UDP Port: %d"), UDPPort);
    UE_LOG(LogTimecodeComponent, Display, TEXT("Target IP: %s"), *TargetIP);
    UE_LOG(LogTimecodeComponent, Display, TEXT("Multicast Group: %s"), *MulticastGroup);
    UE_LOG(LogTimecodeComponent, Display, TEXT("Sync Interval: %.3f seconds"), SyncInterval);

    // Connection state
    UE_LOG(LogTimecodeComponent, Display, TEXT("Connection State: %s"),
        ConnectionState == ENetworkConnectionState::Connected ? TEXT("Connected") :
        ConnectionState == ENetworkConnectionState::Connecting ? TEXT("Connecting") :
        TEXT("Disconnected"));

    // Event info
    UE_LOG(LogTimecodeComponent, Display, TEXT("Registered Events: %d"), TimecodeEvents.Num());
    UE_LOG(LogTimecodeComponent, Display, TEXT("Triggered Events: %d"), TriggeredEvents.Num());

    // Network manager status
    if (NetworkManager)
    {
        UE_LOG(LogTimecodeComponent, Display, TEXT("Network Manager: Valid"));
        UE_LOG(LogTimecodeComponent, Display, TEXT("Network Manager Connection: %s"),
            NetworkManager->HasReceivedValidMessage() ? TEXT("Active") : TEXT("Inactive"));
    }
    else
    {
        UE_LOG(LogTimecodeComponent, Display, TEXT("Network Manager: Invalid/Null"));
    }

    // 모듈 상태 정보 추가
    UE_LOG(LogTimecodeComponent, Display, TEXT("=========================================="));
    UE_LOG(LogTimecodeComponent, Display, TEXT("Module Status:"));

    if (PLLSynchronizer)
    {
        double Phase, Frequency, Offset;
        PLLSynchronizer->GetStatus(Phase, Frequency, Offset);

        UE_LOG(LogTimecodeComponent, Display, TEXT("PLL Module: Valid"));
        UE_LOG(LogTimecodeComponent, Display, TEXT("  Enabled: %s"), bUsePLL ? TEXT("Yes") : TEXT("No"));
        UE_LOG(LogTimecodeComponent, Display, TEXT("  Bandwidth: %.3f, Damping: %.3f"), PLLBandwidth, PLLDamping);
        UE_LOG(LogTimecodeComponent, Display, TEXT("  Status - Frequency: %.6f, Offset: %.3fms, Error: %.3fms"),
            Frequency, Offset * 1000.0, Phase * 1000.0);
    }
    else
    {
        UE_LOG(LogTimecodeComponent, Display, TEXT("PLL Module: Invalid/Null"));
    }

    if (SMPTEConverter)
    {
        UE_LOG(LogTimecodeComponent, Display, TEXT("SMPTE Module: Valid"));
    }
    else
    {
        UE_LOG(LogTimecodeComponent, Display, TEXT("SMPTE Module: Invalid/Null"));
    }

    UE_LOG(LogTimecodeComponent, Display, TEXT("=========================================="));

    // PLL 정보 추가
    UE_LOG(LogTimecodeComponent, Display, TEXT("PLL Enabled: %s"), bUsePLL ? TEXT("Yes") : TEXT("No"));
    UE_LOG(LogTimecodeComponent, Display, TEXT("PLL Bandwidth: %.3f, Damping: %.3f"), PLLBandwidth, PLLDamping);

    if (NetworkManager && bUsePLL)
    {
        double Phase, Frequency, Offset;
        NetworkManager->GetPLLStatus(Phase, Frequency, Offset);

        UE_LOG(LogTimecodeComponent, Display, TEXT("PLL Status - Frequency: %.6f, Offset: %.3fms"),
            Frequency, Offset * 1000.0);
    }

    UE_LOG(LogTimecodeComponent, Display, TEXT("========================================="));
}

void UTimecodeComponent::SetTimecodeMode(ETimecodeMode NewMode)
{
    if (TimecodeMode != NewMode)
    {
        ETimecodeMode OldMode = TimecodeMode;
        TimecodeMode = NewMode;

        // 로그 메시지에 모드 이름 표시
        const UEnum* TimecodeEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("ETimecodeMode"), true);
        FString OldModeName = TimecodeEnum ? TimecodeEnum->GetNameStringByValue((int64)OldMode) : TEXT("Unknown");
        FString NewModeName = TimecodeEnum ? TimecodeEnum->GetNameStringByValue((int64)TimecodeMode) : TEXT("Unknown");

        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Timecode mode changed from %s to %s"),
            *GetOwner()->GetName(), *OldModeName, *NewModeName);

        // 새 모드 적용
        ApplyTimecodeMode();

        // 모드 변경 이벤트 발생
        OnTimecodeModeChanged.Broadcast(TimecodeMode);
    }
}

ETimecodeMode UTimecodeComponent::GetTimecodeMode() const
{
    return TimecodeMode;
}

void UTimecodeComponent::ApplyTimecodeMode()
{
    // 모드에 따라 컴포넌트 설정 변경
    switch (TimecodeMode)
    {
    case ETimecodeMode::PLL_Only:
        bUsePLL = true;
        bUseDropFrameTimecode = false;
        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] PLL Only mode: PLL enabled, Drop Frame disabled"),
            *GetOwner()->GetName());
        break;

    case ETimecodeMode::SMPTE_Only:
        bUsePLL = false;
        bUseDropFrameTimecode = true;
        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] SMPTE Only mode: PLL disabled, Drop Frame enabled"),
            *GetOwner()->GetName());
        break;

    case ETimecodeMode::Integrated:
        bUsePLL = true;
        bUseDropFrameTimecode = true;
        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Integrated mode: PLL enabled, Drop Frame enabled"),
            *GetOwner()->GetName());
        break;

    case ETimecodeMode::Raw:
        bUsePLL = false;
        bUseDropFrameTimecode = false;
        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Raw mode: PLL disabled, Drop Frame disabled"),
            *GetOwner()->GetName());
        break;
    }

    // 네트워크 매니저에 PLL 설정 적용
    if (NetworkManager)
    {
        NetworkManager->SetUsePLL(bUsePLL);
    }

    // PLL 모듈 직접 설정 - 추가됨
    if (PLLSynchronizer)
    {
        // PLL 모듈 상태 초기화 (모드가 변경되면 이전 상태 초기화)
        if (bUsePLL)
        {
            PLLSynchronizer->Initialize();
        }
        else
        {
            PLLSynchronizer->Reset();
        }
    }
}

void UTimecodeComponent::SetUsePLL(bool bInUsePLL)
{
    if (bUsePLL != bInUsePLL)
    {
        bUsePLL = bInUsePLL;

        if (NetworkManager)
        {
            NetworkManager->SetUsePLL(bUsePLL);
        }

        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] PLL %s"),
            *GetOwner()->GetName(), bUsePLL ? TEXT("enabled") : TEXT("disabled"));
    }
}

bool UTimecodeComponent::GetUsePLL() const
{
    return bUsePLL;
}

void UTimecodeComponent::SetPLLParameters(float Bandwidth, float Damping)
{
    // 유효 범위 클램핑
    PLLBandwidth = FMath::Clamp(Bandwidth, 0.01f, 1.0f);
    PLLDamping = FMath::Clamp(Damping, 0.1f, 2.0f);

    // PLL 모듈에 설정 적용
    if (PLLSynchronizer)
    {
        PLLSynchronizer->SetParameters(PLLBandwidth, PLLDamping);
    }

    // 네트워크 매니저에도 설정 적용
    if (NetworkManager)
    {
        NetworkManager->SetPLLParameters(PLLBandwidth, PLLDamping);
    }

    UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] PLL parameters set - Bandwidth: %.3f, Damping: %.3f"),
        *GetOwner()->GetName(), PLLBandwidth, PLLDamping);
}

void UTimecodeComponent::GetPLLParameters(float& OutBandwidth, float& OutDamping) const
{
    OutBandwidth = PLLBandwidth;
    OutDamping = PLLDamping;
}

void UTimecodeComponent::GetPLLStatus(float& OutFrequency, float& OutOffset) const
{
    if (PLLSynchronizer)
    {
        double Phase, Frequency, Offset;
        PLLSynchronizer->GetStatus(Phase, Frequency, Offset);

        OutFrequency = (float)Frequency;
        OutOffset = (float)Offset;
    }
    else if (NetworkManager)
    {
        // Fallback to network manager
        double Phase, Frequency, Offset;
        NetworkManager->GetPLLStatus(Phase, Frequency, Offset);

        OutFrequency = (float)Frequency;
        OutOffset = (float)Offset;
    }
    else
    {
        OutFrequency = 1.0f;
        OutOffset = 0.0f;
    }
}

void UTimecodeComponent::SetDedicatedMaster(bool bInIsDedicatedMaster)
{
    if (bIsDedicatedMaster != bInIsDedicatedMaster)
    {
        bIsDedicatedMaster = bInIsDedicatedMaster;

        UE_LOG(LogTimecodeComponent, Log, TEXT("[%s] Dedicated master mode %s"),
            *GetOwner()->GetName(), bIsDedicatedMaster ? TEXT("enabled") : TEXT("disabled"));

        // 네트워크 매니저에 설정 적용
        if (NetworkManager)
        {
            NetworkManager->SetDedicatedMaster(bIsDedicatedMaster);
        }

        // 전용 마스터 모드가 활성화되면 역할 설정 업데이트
        if (bIsDedicatedMaster)
        {
            // 수동 모드로 변경하고 마스터로 설정
            SetRoleMode(ETimecodeRoleMode::Manual);
            SetManualMaster(true);
        }
    }
}

bool UTimecodeComponent::GetIsDedicatedMaster() const
{
    return bIsDedicatedMaster;
}

void UTimecodeComponent::UpdateRawTimecode(float DeltaTime)
{
    // Raw 모드: 단순히 경과 시간을 증가시키고 기본 형식의 타임코드 생성
    ElapsedTimeSeconds += DeltaTime;

    // 기본 형식의 타임코드 생성 (HH:MM:SS:FF)
    int32 Hours = FMath::FloorToInt(ElapsedTimeSeconds / 3600.0f);
    int32 Minutes = FMath::FloorToInt(FMath::Fmod(ElapsedTimeSeconds / 60.0f, 60.0f));
    int32 Seconds = FMath::FloorToInt(FMath::Fmod(ElapsedTimeSeconds, 60.0f));
    int32 Frames = FMath::FloorToInt(FMath::Fmod(ElapsedTimeSeconds * FrameRate, FrameRate));

    FString NewTimecode = FString::Printf(TEXT("%02d:%02d:%02d:%02d"), Hours, Minutes, Seconds, Frames);

    if (NewTimecode != CurrentTimecode)
    {
        CurrentTimecode = NewTimecode;
        OnTimecodeChanged.Broadcast(CurrentTimecode);

        UE_LOG(LogTimecodeComponent, Verbose, TEXT("[%s] Raw timecode updated: %s"),
            *GetOwner()->GetName(), *CurrentTimecode);
    }
}

void UTimecodeComponent::UpdatePLLTimecode(float DeltaTime)
{
    // PLL 모드: PLL 기반 시간 조정 후 기본 타임코드 생성 (드롭 프레임 없음)

    // PLL 모듈이 초기화되지 않았다면 초기화
    if (!PLLSynchronizer)
    {
        UE_LOG(LogTimecodeComponent, Warning, TEXT("[%s] PLL Synchronizer not available in PLL mode"),
            *GetOwner()->GetName());

        // PLL이 없으면 원시 시간 업데이트로 대체
        UpdateRawTimecode(DeltaTime);
        return;
    }

    // 시간 업데이트
    ElapsedTimeSeconds += DeltaTime;

    // PLL 처리를 통한 시간 미세 조정 (마스터 모드에서도 자체 안정화를 위해 PLL 적용)
    float AdjustedTime = PLLSynchronizer->ProcessTime(ElapsedTimeSeconds, ElapsedTimeSeconds, DeltaTime);

    // 기본 타임코드 형식 생성
    int32 Hours = FMath::FloorToInt(AdjustedTime / 3600.0f);
    int32 Minutes = FMath::FloorToInt(FMath::Fmod(AdjustedTime / 60.0f, 60.0f));
    int32 Seconds = FMath::FloorToInt(FMath::Fmod(AdjustedTime, 60.0f));
    int32 Frames = FMath::FloorToInt(FMath::Fmod(AdjustedTime * FrameRate, FrameRate));

    FString NewTimecode = FString::Printf(TEXT("%02d:%02d:%02d:%02d"), Hours, Minutes, Seconds, Frames);

    if (NewTimecode != CurrentTimecode)
    {
        CurrentTimecode = NewTimecode;
        ElapsedTimeSeconds = AdjustedTime; // 조정된 시간으로 업데이트
        OnTimecodeChanged.Broadcast(CurrentTimecode);

        UE_LOG(LogTimecodeComponent, Verbose, TEXT("[%s] PLL timecode updated: %s"),
            *GetOwner()->GetName(), *CurrentTimecode);
    }
}

void UTimecodeComponent::UpdateSMPTETimecode(float DeltaTime)
{
    // SMPTE 모드: SMPTE 타임코드 변환 (드롭 프레임 적용)

    // 시간 업데이트
    ElapsedTimeSeconds += DeltaTime;

    // SMPTE 컨버터 적용
    FString NewTimecode;
    if (SMPTEConverter)
    {
        NewTimecode = SMPTEConverter->SecondsToTimecode(ElapsedTimeSeconds, FrameRate, true);
    }
    else
    {
        // 컨버터가 없으면 유틸리티 함수 사용
        NewTimecode = UTimecodeUtils::SecondsToTimecode(ElapsedTimeSeconds, FrameRate, true);
    }

    if (NewTimecode != CurrentTimecode)
    {
        CurrentTimecode = NewTimecode;
        OnTimecodeChanged.Broadcast(CurrentTimecode);

        UE_LOG(LogTimecodeComponent, Verbose, TEXT("[%s] SMPTE timecode updated: %s"),
            *GetOwner()->GetName(), *CurrentTimecode);
    }
}

void UTimecodeComponent::UpdateIntegratedTimecode(float DeltaTime)
{
    // 통합 모드: PLL로 시간 조정 후 SMPTE 드롭 프레임 적용

    // PLL 모듈이 초기화되지 않았다면 초기화
    if (!PLLSynchronizer)
    {
        UE_LOG(LogTimecodeComponent, Warning, TEXT("[%s] PLL Synchronizer not available in Integrated mode"),
            *GetOwner()->GetName());

        // PLL이 없으면 SMPTE 시간 업데이트로 대체
        UpdateSMPTETimecode(DeltaTime);
        return;
    }

    // 시간 업데이트
    ElapsedTimeSeconds += DeltaTime;

    // PLL 처리를 통한 시간 미세 조정
    float AdjustedTime = PLLSynchronizer->ProcessTime(ElapsedTimeSeconds, ElapsedTimeSeconds, DeltaTime);

    // SMPTE 컨버터로 타임코드 생성
    FString NewTimecode;
    if (SMPTEConverter)
    {
        NewTimecode = SMPTEConverter->SecondsToTimecode(AdjustedTime, FrameRate, true);
    }
    else
    {
        // 컨버터가 없으면 유틸리티 함수 사용
        NewTimecode = UTimecodeUtils::SecondsToTimecode(AdjustedTime, FrameRate, true);
    }

    if (NewTimecode != CurrentTimecode)
    {
        CurrentTimecode = NewTimecode;
        ElapsedTimeSeconds = AdjustedTime; // 조정된 시간으로 업데이트
        OnTimecodeChanged.Broadcast(CurrentTimecode);

        UE_LOG(LogTimecodeComponent, Verbose, TEXT("[%s] Integrated timecode updated: %s"),
            *GetOwner()->GetName(), *CurrentTimecode);
    }
}