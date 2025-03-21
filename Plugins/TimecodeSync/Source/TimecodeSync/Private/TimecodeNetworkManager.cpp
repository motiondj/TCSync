#include "TimecodeNetworkManager.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "Common/UdpSocketReceiver.h"
#include "Common/UdpSocketBuilder.h"
#include "Networking.h"
#include "IPAddress.h"
#include "Misc/Guid.h"
#include "HAL/RunnableThread.h"
#include "Serialization/ArrayReader.h"

// Define log category
DEFINE_LOG_CATEGORY_STATIC(LogTimecodeNetwork, Log, All);

UTimecodeNetworkManager::UTimecodeNetworkManager()
    : Socket(nullptr)
    , Receiver(nullptr)
    , ConnectionState(ENetworkConnectionState::Disconnected)
    , InstanceID(FGuid::NewGuid().ToString())
    , PortNumber(10000)
    , TargetIPAddress(TEXT("127.0.0.1"))
    , MulticastGroupAddress(TEXT("239.0.0.1"))
    , bIsMasterMode(false)
    , RoleMode(ETimecodeRoleMode::Automatic)
    , bIsManuallyMaster(false)
    , MasterIPAddress(TEXT(""))
    , bRoleAutomaticallyDetermined(true)
{
    // Basic initialization complete
    UE_LOG(LogTimecodeNetwork, Verbose, TEXT("TimecodeNetworkManager created with ID: %s"), *InstanceID);
}

UTimecodeNetworkManager::~UTimecodeNetworkManager()
{
    Shutdown();
    UE_LOG(LogTimecodeNetwork, Verbose, TEXT("TimecodeNetworkManager destroyed"));
}

bool UTimecodeNetworkManager::Initialize(bool bIsMaster, int32 Port)
{
    // Shutdown if already initialized
    if (Socket != nullptr || Receiver != nullptr)
    {
        UE_LOG(LogTimecodeNetwork, Warning, TEXT("TimecodeNetworkManager already initialized, shutting down first"));
        Shutdown();
    }

    // Determine role
    if (RoleMode == ETimecodeRoleMode::Automatic)
    {
        bIsMasterMode = AutoDetectRole();
        bRoleAutomaticallyDetermined = true;
        UE_LOG(LogTimecodeNetwork, Log, TEXT("Automatic role detection: %s"), bIsMasterMode ? TEXT("MASTER") : TEXT("SLAVE"));
    }
    else // Manual mode
    {
        bIsMasterMode = bIsManuallyMaster;
        bRoleAutomaticallyDetermined = false;
        UE_LOG(LogTimecodeNetwork, Log, TEXT("Manual role setting: %s"), bIsMasterMode ? TEXT("MASTER") : TEXT("SLAVE"));
    }

    PortNumber = Port;

    // Create socket
    if (!CreateSocket())
    {
        UE_LOG(LogTimecodeNetwork, Error, TEXT("Failed to create UDP socket"));
        return false;
    }

    // Setup UDP reception
    FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(100);
    Receiver = new FUdpSocketReceiver(Socket, ThreadWaitTime, TEXT("TimecodeReceiver"));
    Receiver->OnDataReceived().BindUObject(this, &UTimecodeNetworkManager::OnUDPReceived);
    Receiver->Start();

    // Set connection state
    SetConnectionState(ENetworkConnectionState::Connected);

    UE_LOG(LogTimecodeNetwork, Log, TEXT("Network manager initialized in %s mode"), bIsMasterMode ? TEXT("Master") : TEXT("Slave"));
    return true;
}

void UTimecodeNetworkManager::Shutdown()
{
    // Stop reception
    if (Receiver != nullptr)
    {
        Receiver->Stop();
        delete Receiver;
        Receiver = nullptr;
        UE_LOG(LogTimecodeNetwork, Verbose, TEXT("UDP receiver stopped"));
    }

    // Clean up socket
    if (Socket != nullptr)
    {
        // Leave multicast group
        if (!MulticastGroupAddress.IsEmpty())
        {
            TSharedRef<FInternetAddr> MulticastAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
            bool bIsValid = false;
            MulticastAddr->SetIp(*MulticastGroupAddress, bIsValid);

            if (bIsValid)
            {
                Socket->LeaveMulticastGroup(*MulticastAddr);
                UE_LOG(LogTimecodeNetwork, Verbose, TEXT("Left multicast group: %s"), *MulticastGroupAddress);
            }
        }

        Socket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
        Socket = nullptr;
        UE_LOG(LogTimecodeNetwork, Verbose, TEXT("Socket closed and destroyed"));
    }

    // Set connection state
    SetConnectionState(ENetworkConnectionState::Disconnected);

    UE_LOG(LogTimecodeNetwork, Log, TEXT("Network manager shutdown"));
}

bool UTimecodeNetworkManager::SendTimecodeMessage(const FString& Timecode, ETimecodeMessageType MessageType)
{
    if (Socket == nullptr || ConnectionState != ENetworkConnectionState::Connected)
    {
        UE_LOG(LogTimecodeNetwork, Warning, TEXT("Cannot send message: Socket not connected"));
        return false;
    }

    // Serialize message
    FTimecodeNetworkMessage Message;
    Message.MessageType = MessageType;
    Message.Timecode = Timecode;
    Message.Timestamp = FPlatformTime::Seconds();
    Message.SenderID = InstanceID;
    Message.Data = TEXT(""); // Default data is empty string

    // Serialize message
    TArray<uint8> MessageData = Message.Serialize();

    // Send message
    int32 BytesSent = 0;

    // Send directly to master IP if in manual mode and slave
    if (RoleMode == ETimecodeRoleMode::Manual && !bIsMasterMode && !MasterIPAddress.IsEmpty())
    {
        TSharedRef<FInternetAddr> TargetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
        bool bIsValid = false;
        TargetAddr->SetIp(*MasterIPAddress, bIsValid);

        if (!bIsValid)
        {
            UE_LOG(LogTimecodeNetwork, Error, TEXT("Invalid Master IP: %s"), *MasterIPAddress);
            return false;
        }

        TargetAddr->SetPort(PortNumber);
        Socket->SendTo(MessageData.GetData(), MessageData.Num(), BytesSent, *TargetAddr);

        UE_LOG(LogTimecodeNetwork, Verbose, TEXT("Sent %s message directly to Master: %s"),
            *UEnum::GetValueAsString(MessageType), *MasterIPAddress);
    }
    // Use unicast if target IP is set
    else if (!TargetIPAddress.IsEmpty())
    {
        TSharedRef<FInternetAddr> TargetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
        bool bIsValid = false;
        TargetAddr->SetIp(*TargetIPAddress, bIsValid);

        if (!bIsValid)
        {
            UE_LOG(LogTimecodeNetwork, Error, TEXT("Invalid target IP: %s"), *TargetIPAddress);
            return false;
        }

        TargetAddr->SetPort(PortNumber);
        Socket->SendTo(MessageData.GetData(), MessageData.Num(), BytesSent, *TargetAddr);

        UE_LOG(LogTimecodeNetwork, Verbose, TEXT("Sent %s message to target: %s"),
            *UEnum::GetValueAsString(MessageType), *TargetIPAddress);
    }
    // Use multicast if multicast group is set
    else if (!MulticastGroupAddress.IsEmpty())
    {
        TSharedRef<FInternetAddr> MulticastAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
        bool bIsValid = false;
        MulticastAddr->SetIp(*MulticastGroupAddress, bIsValid);

        if (!bIsValid)
        {
            UE_LOG(LogTimecodeNetwork, Error, TEXT("Invalid multicast group: %s"), *MulticastGroupAddress);
            return false;
        }

        MulticastAddr->SetPort(PortNumber);
        Socket->SendTo(MessageData.GetData(), MessageData.Num(), BytesSent, *MulticastAddr);

        UE_LOG(LogTimecodeNetwork, Verbose, TEXT("Sent %s message to multicast group: %s"),
            *UEnum::GetValueAsString(MessageType), *MulticastGroupAddress);
    }
    else
    {
        UE_LOG(LogTimecodeNetwork, Error, TEXT("No target IP or multicast group specified"));
        return false;
    }

    return BytesSent == MessageData.Num();
}

bool UTimecodeNetworkManager::SendEventMessage(const FString& EventName, const FString& Timecode)
{
    if (Socket == nullptr || ConnectionState != ENetworkConnectionState::Connected)
    {
        UE_LOG(LogTimecodeNetwork, Warning, TEXT("Cannot send event: Socket not connected"));
        return false;
    }

    // Serialize message
    FTimecodeNetworkMessage Message;
    Message.MessageType = ETimecodeMessageType::Event;
    Message.Timecode = Timecode;
    Message.Data = EventName;
    Message.Timestamp = FPlatformTime::Seconds();
    Message.SenderID = InstanceID;

    // Serialize message
    TArray<uint8> MessageData = Message.Serialize();

    // Send message (choose between multicast group or single target IP)
    int32 BytesSent = 0;

    // Transmission priority: Multicast > TargetIP > MasterIP
    if (!MulticastGroupAddress.IsEmpty())
    {
        // Send to multicast group
        TSharedRef<FInternetAddr> MulticastAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
        bool bIsValid = false;
        MulticastAddr->SetIp(*MulticastGroupAddress, bIsValid);
        if (!bIsValid)
        {
            UE_LOG(LogTimecodeNetwork, Error, TEXT("Invalid multicast group: %s"), *MulticastGroupAddress);
            return false;
        }
        MulticastAddr->SetPort(PortNumber);

        Socket->SendTo(MessageData.GetData(), MessageData.Num(), BytesSent, *MulticastAddr);

        UE_LOG(LogTimecodeNetwork, Verbose, TEXT("Sent event '%s' to multicast group: %s"),
            *EventName, *MulticastGroupAddress);
    }
    else if (!TargetIPAddress.IsEmpty())
    {
        // Send to single target IP
        TSharedRef<FInternetAddr> TargetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
        bool bIsValid = false;
        TargetAddr->SetIp(*TargetIPAddress, bIsValid);
        if (!bIsValid)
        {
            UE_LOG(LogTimecodeNetwork, Error, TEXT("Invalid target IP: %s"), *TargetIPAddress);
            return false;
        }
        TargetAddr->SetPort(PortNumber);

        Socket->SendTo(MessageData.GetData(), MessageData.Num(), BytesSent, *TargetAddr);

        UE_LOG(LogTimecodeNetwork, Verbose, TEXT("Sent event '%s' to target: %s"),
            *EventName, *TargetIPAddress);
    }
    else if (RoleMode == ETimecodeRoleMode::Manual && !bIsMasterMode && !MasterIPAddress.IsEmpty())
    {
        // Send to master IP in manual slave mode
        TSharedRef<FInternetAddr> MasterAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
        bool bIsValid = false;
        MasterAddr->SetIp(*MasterIPAddress, bIsValid);
        if (!bIsValid)
        {
            UE_LOG(LogTimecodeNetwork, Error, TEXT("Invalid master IP: %s"), *MasterIPAddress);
            return false;
        }
        MasterAddr->SetPort(PortNumber);

        Socket->SendTo(MessageData.GetData(), MessageData.Num(), BytesSent, *MasterAddr);

        UE_LOG(LogTimecodeNetwork, Verbose, TEXT("Sent event '%s' to master: %s"),
            *EventName, *MasterIPAddress);
    }
    else
    {
        UE_LOG(LogTimecodeNetwork, Error, TEXT("No target IP or multicast group specified"));
        return false;
    }

    return BytesSent == MessageData.Num();
}

void UTimecodeNetworkManager::SetTargetIP(const FString& IPAddress)
{
    if (TargetIPAddress != IPAddress)
    {
        TargetIPAddress = IPAddress;
        UE_LOG(LogTimecodeNetwork, Log, TEXT("Target IP set to: %s"), *TargetIPAddress);
    }
}

void UTimecodeNetworkManager::SetRoleMode(ETimecodeRoleMode NewMode)
{
    if (RoleMode != NewMode)
    {
        ETimecodeRoleMode OldMode = RoleMode;
        RoleMode = NewMode;

        UE_LOG(LogTimecodeNetwork, Log, TEXT("Role mode changed from %s to %s"),
            (OldMode == ETimecodeRoleMode::Automatic) ? TEXT("Automatic") : TEXT("Manual"),
            (RoleMode == ETimecodeRoleMode::Automatic) ? TEXT("Automatic") : TEXT("Manual"));

        // Determine role again
        bool bOldMasterMode = bIsMasterMode;

        if (RoleMode == ETimecodeRoleMode::Automatic)
        {
            // Use IP-based or other automatic detection logic in automatic mode
            bIsMasterMode = AutoDetectRole();
            bRoleAutomaticallyDetermined = true;
        }
        else
        {
            // Use manual setting in manual mode
            bIsMasterMode = bIsManuallyMaster;
            bRoleAutomaticallyDetermined = false;
        }

        // Role changed and already initialized
        if (bOldMasterMode != bIsMasterMode && Socket != nullptr)
        {
            UE_LOG(LogTimecodeNetwork, Log, TEXT("Role changed, reinitializing network..."));
            int32 OldPort = PortNumber;
            Shutdown();
            Initialize(bIsMasterMode, OldPort);
        }

        // Event triggered
        OnRoleModeChanged.Broadcast(RoleMode);
    }
}

ETimecodeRoleMode UTimecodeNetworkManager::GetRoleMode() const
{
    return RoleMode;
}

void UTimecodeNetworkManager::SetManualMaster(bool bInIsManuallyMaster)
{
    if (bIsManuallyMaster != bInIsManuallyMaster)
    {
        bIsManuallyMaster = bInIsManuallyMaster;

        UE_LOG(LogTimecodeNetwork, Log, TEXT("Manual master setting changed to: %s"),
            bIsManuallyMaster ? TEXT("MASTER") : TEXT("SLAVE"));

        // Apply role change only if in manual mode
        if (RoleMode == ETimecodeRoleMode::Manual)
        {
            bool bOldMasterMode = bIsMasterMode;
            bIsMasterMode = bIsManuallyMaster;

            // Role changed and already initialized
            if (bOldMasterMode != bIsMasterMode && Socket != nullptr)
            {
                UE_LOG(LogTimecodeNetwork, Log, TEXT("Master role changed, reinitializing network..."));
                int32 OldPort = PortNumber;
                Shutdown();
                Initialize(bIsMasterMode, OldPort);
            }
        }
    }
}

bool UTimecodeNetworkManager::GetIsManuallyMaster() const
{
    return bIsManuallyMaster;
}

void UTimecodeNetworkManager::SetMasterIPAddress(const FString& InMasterIP)
{
    if (MasterIPAddress != InMasterIP)
    {
        MasterIPAddress = InMasterIP;

        UE_LOG(LogTimecodeNetwork, Log, TEXT("Master IP address changed to: %s"), *MasterIPAddress);

        // Set target IP if in manual slave mode
        if (RoleMode == ETimecodeRoleMode::Manual && !bIsManuallyMaster)
        {
            SetTargetIP(MasterIPAddress);
        }
    }
}

FString UTimecodeNetworkManager::GetMasterIPAddress() const
{
    return MasterIPAddress;
}

bool UTimecodeNetworkManager::JoinMulticastGroup(const FString& MulticastGroup)
{
    if (Socket == nullptr)
    {
        UE_LOG(LogTimecodeNetwork, Error, TEXT("Cannot join multicast group: Socket not created"));
        return false;
    }

    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    TSharedRef<FInternetAddr> GroupAddr = SocketSubsystem->CreateInternetAddr();

    bool bIsValid = false;
    GroupAddr->SetIp(*MulticastGroup, bIsValid);

    if (!bIsValid)
    {
        UE_LOG(LogTimecodeNetwork, Error, TEXT("Invalid multicast group address: %s"), *MulticastGroup);
        return false;
    }

    if (!Socket->JoinMulticastGroup(*GroupAddr))
    {
        UE_LOG(LogTimecodeNetwork, Error, TEXT("Failed to join multicast group: %s"), *MulticastGroup);
        return false;
    }

    MulticastGroupAddress = MulticastGroup;
    UE_LOG(LogTimecodeNetwork, Log, TEXT("Joined multicast group: %s"), *MulticastGroup);
    return true;
}

ENetworkConnectionState UTimecodeNetworkManager::GetConnectionState() const
{
    return ConnectionState;
}

bool UTimecodeNetworkManager::CreateSocket()
{
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (SocketSubsystem == nullptr)
    {
        UE_LOG(LogTimecodeNetwork, Error, TEXT("Socket subsystem not found"));
        return false;
    }

    // Create UDP socket
    Socket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("TimecodeSocket"), true);
    if (Socket == nullptr)
    {
        UE_LOG(LogTimecodeNetwork, Error, TEXT("Failed to create UDP socket"));
        return false;
    }

    // Set socket options
    Socket->SetReuseAddr();
    Socket->SetRecvErr();
    Socket->SetNonBlocking();

    // Set broadcast
    Socket->SetBroadcast();

    // Set local address
    TSharedRef<FInternetAddr> LocalAddr = SocketSubsystem->CreateInternetAddr();
    LocalAddr->SetAnyAddress();
    LocalAddr->SetPort(PortNumber);

    if (!Socket->Bind(*LocalAddr))
    {
        UE_LOG(LogTimecodeNetwork, Error, TEXT("Failed to bind socket to port %d"), PortNumber);
        SocketSubsystem->DestroySocket(Socket);
        Socket = nullptr;
        return false;
    }

    UE_LOG(LogTimecodeNetwork, Log, TEXT("UDP socket created and bound to port %d"), PortNumber);
    return true;
}

void UTimecodeNetworkManager::OnUDPReceived(const FArrayReaderPtr& DataPtr, const FIPv4Endpoint& Endpoint)
{
    if (!DataPtr.IsValid() || DataPtr->Num() == 0)
    {
        return;
    }

    // Convert data to TArray<uint8>
    TArray<uint8> ReceivedData;
    ReceivedData.Append(DataPtr->GetData(), DataPtr->Num());

    // Deserialize message
    FTimecodeNetworkMessage Message;
    if (Message.Deserialize(ReceivedData))
    {
        // Process message only if sender is not self
        if (Message.SenderID != InstanceID)
        {
            // Process message
            ProcessMessage(Message);
            // Message received notification
            OnMessageReceived.Broadcast(Message);
        }
    }
    else
    {
        UE_LOG(LogTimecodeNetwork, Warning, TEXT("Failed to deserialize received message from %s"), *Endpoint.ToString());
    }
}

void UTimecodeNetworkManager::ProcessMessage(const FTimecodeNetworkMessage& Message)
{
    // Convert message type to string
    FString MessageTypeStr = UEnum::GetValueAsString(Message.MessageType);

    switch (Message.MessageType)
    {
    case ETimecodeMessageType::Heartbeat:
        // Process heartbeat
        UE_LOG(LogTimecodeNetwork, Verbose, TEXT("Received heartbeat (%s) from %s"), *MessageTypeStr, *Message.SenderID);
        break;

    case ETimecodeMessageType::TimecodeSync:
        // Process timecode synchronization
        UE_LOG(LogTimecodeNetwork, Verbose, TEXT("Received timecode (%s): %s from %s"), *MessageTypeStr, *Message.Timecode, *Message.SenderID);
        break;

    case ETimecodeMessageType::RoleAssignment:
        // Process role assignment
        UE_LOG(LogTimecodeNetwork, Log, TEXT("Received role assignment (%s): %s from %s"), *MessageTypeStr, *Message.Data, *Message.SenderID);

        // Process external assignment only in automatic mode
        if (RoleMode == ETimecodeRoleMode::Automatic)
        {
            if (Message.Data.Equals(TEXT("MASTER"), ESearchCase::IgnoreCase))
            {
                if (!bIsMasterMode)
                {
                    UE_LOG(LogTimecodeNetwork, Log, TEXT("Role changed to MASTER by external assignment"));
                    bool bOldMasterMode = bIsMasterMode;
                    bIsMasterMode = true;

                    if (Socket != nullptr)
                    {
                        int32 OldPort = PortNumber;
                        Shutdown();
                        Initialize(bIsMasterMode, OldPort);
                    }
                }
            }
            else if (Message.Data.Equals(TEXT("SLAVE"), ESearchCase::IgnoreCase))
            {
                if (bIsMasterMode)
                {
                    UE_LOG(LogTimecodeNetwork, Log, TEXT("Role changed to SLAVE by external assignment"));
                    bool bOldMasterMode = bIsMasterMode;
                    bIsMasterMode = false;

                    if (Socket != nullptr)
                    {
                        int32 OldPort = PortNumber;
                        Shutdown();
                        Initialize(bIsMasterMode, OldPort);
                    }
                }
            }
        }
        break;

    case ETimecodeMessageType::Event:
        // Process event
        UE_LOG(LogTimecodeNetwork, Log, TEXT("Received event (%s): %s at %s from %s"), *MessageTypeStr, *Message.Data, *Message.Timecode, *Message.SenderID);
        break;

    case ETimecodeMessageType::Command:
        // Process command
        UE_LOG(LogTimecodeNetwork, Log, TEXT("Received command (%s): %s from %s"), *MessageTypeStr, *Message.Data, *Message.SenderID);
        break;

    default:
        UE_LOG(LogTimecodeNetwork, Warning, TEXT("Received unknown message type (%d) from %s"), static_cast<int32>(Message.MessageType), *Message.SenderID);
        break;
    }
}

void UTimecodeNetworkManager::SetConnectionState(ENetworkConnectionState NewState)
{
    if (ConnectionState != NewState)
    {
        ConnectionState = NewState;
        OnNetworkStateChanged.Broadcast(ConnectionState);

        UE_LOG(LogTimecodeNetwork, Log, TEXT("Network connection state changed: %s"),
            ConnectionState == ENetworkConnectionState::Connected ? TEXT("Connected") :
            ConnectionState == ENetworkConnectionState::Connecting ? TEXT("Connecting") :
            TEXT("Disconnected"));
    }
}

void UTimecodeNetworkManager::SendHeartbeat()
{
    if (Socket != nullptr && ConnectionState == ENetworkConnectionState::Connected)
    {
        SendTimecodeMessage(TEXT(""), ETimecodeMessageType::Heartbeat);
    }
}

bool UTimecodeNetworkManager::AutoDetectRole()
{
    // Get local IP address
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    bool bCanBindAll = false;
    TSharedPtr<FInternetAddr> LocalAddr = SocketSubsystem->GetLocalHostAddr(*GLog, bCanBindAll);

    if (LocalAddr.IsValid() && bCanBindAll)
    {
        // Print IP address
        FString LocalIP = LocalAddr->ToString(false);
        UE_LOG(LogTimecodeNetwork, Log, TEXT("Local IP: %s"), *LocalIP);

        // nDisplay integration code can be added here

        // IP-based simple decision logic
        // In actual implementation, you should search for other nodes using UDP broadcast
        // and compare IPs or use other methods to determine the master

        // Discover message broadcast to search for other nodes
        // Implement logic to wait for response and compare IPs

        // Temporary implementation: First node becomes master (more complex search logic needed)
        bool bHasLowerIP = false;

        // Implement search logic here
        // ...

        // Master if no node with lower IP
        bool bIsMaster = !bHasLowerIP;

        UE_LOG(LogTimecodeNetwork, Log, TEXT("Auto-detected role: %s"), bIsMaster ? TEXT("MASTER") : TEXT("SLAVE"));

        return bIsMaster;
    }

    // Default to SLAVE if IP cannot be obtained
    UE_LOG(LogTimecodeNetwork, Warning, TEXT("Could not determine local IP address, defaulting to SLAVE"));
    return false;
}