#include "TimecodeComponent.h"
#include "TimecodeUtils.h"
#include "TimecodeSettings.h"
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

    // Initialize timecode-related settings
    FrameRate = Settings ? Settings->FrameRate : 30.0f;
    bUseDropFrameTimecode = Settings ? Settings->bUseDropFrameTimecode : false;
    bAutoStart = Settings ? Settings->bAutoStartTimecode : true;

    // Initialize network settings
    UDPPort = Settings ? Settings->DefaultUDPPort : 10000;
    TargetIP = TEXT("");
    MulticastGroup = Settings ? Settings->MulticastGroupAddress : TEXT("239.0.0.1");
    SyncInterval = Settings ? Settings->BroadcastInterval : 0.033f; // Approximately 30Hz

    // Initialize internal variables
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

    // Update timecode if running
    if (bIsRunning)
    {
        // Update timecode only in master mode
        if (bIsMaster)
        {
            UpdateTimecode(DeltaTime);
        }

        // Check events for both master and slave
        CheckTimecodeEvents();

        // Update network sync timer (master only)
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
    CurrentTimecode = UTimecodeUtils::SecondsToTimecode(ElapsedTimeSeconds, FrameRate, bUseDropFrameTimecode);

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

        // Setup callbacks
        NetworkManager->OnMessageReceived.AddDynamic(this, &UTimecodeComponent::OnTimecodeMessageReceived);
        NetworkManager->OnNetworkStateChanged.AddDynamic(this, &UTimecodeComponent::OnNetworkStateChanged);
        NetworkManager->OnRoleModeChanged.AddDynamic(this, &UTimecodeComponent::OnNetworkRoleModeChanged);

        // Initialize network
        bool bSuccess = NetworkManager->Initialize(bIsMaster, UDPPort);

        // Set target IP
        if (!TargetIP.IsEmpty())
        {
            NetworkManager->SetTargetIP(TargetIP);
        }

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

    // Generate timecode
    FString NewTimecode = UTimecodeUtils::SecondsToTimecode(ElapsedTimeSeconds, FrameRate, bUseDropFrameTimecode);

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

                // Convert timecode string to seconds
                ElapsedTimeSeconds = UTimecodeUtils::TimecodeToSeconds(CurrentTimecode, FrameRate, bUseDropFrameTimecode);

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

    UE_LOG(LogTimecodeComponent, Display, TEXT("========================================="));
}