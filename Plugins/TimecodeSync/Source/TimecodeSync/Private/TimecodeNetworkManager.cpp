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
    , bHasReceivedValidMessage(false)
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

    // 먼저 포트 번호 설정 (이 부분이 중요합니다)
    PortNumber = Port;


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
    // 연결 상태 초기화
    bHasReceivedValidMessage = false;

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

void UTimecodeNetworkManager::OnUDPReceived(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt)
{
    if (!ArrayReaderPtr.IsValid())
        return;

    // 메시지 역직렬화
    FTimecodeNetworkMessage ReceivedMessage;
    if (ReceivedMessage.Deserialize(*ArrayReaderPtr))
    {
        // 유효한 메시지를 받았음을 표시
        bHasReceivedValidMessage = true;

        // 메시지 처리
        ProcessMessage(ReceivedMessage);
        OnMessageReceived.Broadcast(ReceivedMessage);
    }
}

void UTimecodeNetworkManager::ProcessMessage(const FTimecodeNetworkMessage& Message)
{
    // 메시지 타입에 따른 처리
    switch (Message.MessageType)
    {
        case ETimecodeMessageType::TimecodeSync:
            // 타임코드 메시지 브로드캐스트
            OnTimecodeMessageReceived.Broadcast(Message);
            bHasReceivedValidMessage = true;
            break;

        case ETimecodeMessageType::Event:
            // 이벤트 메시지 처리
            UE_LOG(LogTimecodeNetwork, Log, TEXT("Received event: %s at %s"), *Message.Data, *Message.Timecode);
            break;

        default:
            UE_LOG(LogTimecodeNetwork, Warning, TEXT("Unknown message type received"));
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
    bool bFoundValidIP = false;
    FString LocalIP;

    // 로컬 IP 주소 가져오기 - 더 강력한 방식으로 구현
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (SocketSubsystem)
    {
        // 호스트 이름 얻기
        FString HostName;
        if (SocketSubsystem->GetHostName(HostName))
        {
            // 호스트의 주소 정보 얻기
            TSharedPtr<FInternetAddr> LocalAddr = SocketSubsystem->GetLocalBindAddr(*GLog);
            if (LocalAddr.IsValid())
            {
                LocalIP = LocalAddr->ToString(false); // 포트 번호 제외

                // 유효한 IP 주소인지 확인 (127.0.0.1이 아닌지)
                if (!LocalIP.IsEmpty() && !LocalIP.Equals(TEXT("127.0.0.1")) && !LocalIP.Equals(TEXT("0.0.0.0")))
                {
                    bFoundValidIP = true;
                    UE_LOG(LogTimecodeNetwork, Log, TEXT("Local IP detected: %s"), *LocalIP);
                }
            }
        }

        // 위 방법이 실패하면 다른 방법으로 시도
        if (!bFoundValidIP)
        {
            bool bCanBindAll = false;
            TSharedPtr<FInternetAddr> HostAddr = SocketSubsystem->GetLocalHostAddr(*GLog, bCanBindAll);
            if (HostAddr.IsValid())
            {
                LocalIP = HostAddr->ToString(false);
                if (!LocalIP.IsEmpty() && !LocalIP.Equals(TEXT("127.0.0.1")) && !LocalIP.Equals(TEXT("0.0.0.0")))
                {
                    bFoundValidIP = true;
                    UE_LOG(LogTimecodeNetwork, Log, TEXT("Local IP detected (alternative method): %s"), *LocalIP);
                }
            }
        }

        // 네트워크 인터페이스를 순회하여 모든 IP 주소 확인 (더 확실한 방법)
        if (!bFoundValidIP)
        {
            TArray<TSharedPtr<FInternetAddr>> LocalAddresses;
            SocketSubsystem->GetLocalAdapterAddresses(LocalAddresses);

            for (const TSharedPtr<FInternetAddr>& Addr : LocalAddresses)
            {
                if (Addr.IsValid())
                {
                    FString CandidateIP = Addr->ToString(false);
                    // 루프백이나 링크 로컬 주소가 아닌 것을 선택
                    if (!CandidateIP.StartsWith(TEXT("127.")) && !CandidateIP.StartsWith(TEXT("169.254.")))
                    {
                        LocalIP = CandidateIP;
                        bFoundValidIP = true;
                        UE_LOG(LogTimecodeNetwork, Log, TEXT("Local IP detected (from adapters): %s"), *LocalIP);
                        break;
                    }
                }
            }
        }
    }

    if (bFoundValidIP)
    {
        // 테스트 환경을 위한 단순한 규칙:
        // 포트 번호가 10000이면 마스터, 그 외에는 슬레이브
        bool bIsMaster = (PortNumber == 10000);

        UE_LOG(LogTimecodeNetwork, Log, TEXT("Auto-detected role based on port number: %s (Port: %d)"),
            bIsMaster ? TEXT("MASTER") : TEXT("SLAVE"), PortNumber);

        return bIsMaster;
    }

    // IP를 얻지 못한 경우 슬레이브로 설정
    UE_LOG(LogTimecodeNetwork, Warning, TEXT("Could not determine local IP address, defaulting to SLAVE"));
    return false;
}

FString UTimecodeNetworkManager::GetCurrentTimecode() const
{
    // 현재 타임코드 반환 (실제 구현에서는 적절한 타임코드 값을 반환해야 함)
    return TEXT("00:00:00:00");
}

bool UTimecodeNetworkManager::IsMaster() const
{
    return bIsMasterMode;
}