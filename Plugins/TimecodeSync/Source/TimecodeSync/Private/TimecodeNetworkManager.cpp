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

// 로그 카테고리 정의
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
    // 기본 초기화 완료
    UE_LOG(LogTimecodeNetwork, Verbose, TEXT("TimecodeNetworkManager created with ID: %s"), *InstanceID);
}

UTimecodeNetworkManager::~UTimecodeNetworkManager()
{
    Shutdown();
    UE_LOG(LogTimecodeNetwork, Verbose, TEXT("TimecodeNetworkManager destroyed"));
}

bool UTimecodeNetworkManager::Initialize(bool bIsMaster, int32 Port)
{
    // 이미 초기화된 경우 종료
    if (Socket != nullptr || Receiver != nullptr)
    {
        UE_LOG(LogTimecodeNetwork, Warning, TEXT("TimecodeNetworkManager already initialized, shutting down first"));
        Shutdown();
    }

    // 역할 결정
    if (RoleMode == ETimecodeRoleMode::Automatic)
    {
        bIsMasterMode = AutoDetectRole();
        bRoleAutomaticallyDetermined = true;
        UE_LOG(LogTimecodeNetwork, Log, TEXT("Automatic role detection: %s"), bIsMasterMode ? TEXT("MASTER") : TEXT("SLAVE"));
    }
    else // Manual 모드
    {
        bIsMasterMode = bIsManuallyMaster;
        bRoleAutomaticallyDetermined = false;
        UE_LOG(LogTimecodeNetwork, Log, TEXT("Manual role setting: %s"), bIsMasterMode ? TEXT("MASTER") : TEXT("SLAVE"));
    }

    PortNumber = Port;

    // 소켓 생성
    if (!CreateSocket())
    {
        UE_LOG(LogTimecodeNetwork, Error, TEXT("Failed to create UDP socket"));
        return false;
    }

    // UDP 수신 설정
    FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(100);
    Receiver = new FUdpSocketReceiver(Socket, ThreadWaitTime, TEXT("TimecodeReceiver"));
    Receiver->OnDataReceived().BindUObject(this, &UTimecodeNetworkManager::OnUDPReceived);
    Receiver->Start();

    // 연결 상태 설정
    SetConnectionState(ENetworkConnectionState::Connected);

    UE_LOG(LogTimecodeNetwork, Log, TEXT("Network manager initialized in %s mode"), bIsMasterMode ? TEXT("Master") : TEXT("Slave"));
    return true;
}

void UTimecodeNetworkManager::Shutdown()
{
    // 수신 중지
    if (Receiver != nullptr)
    {
        Receiver->Stop();
        delete Receiver;
        Receiver = nullptr;
        UE_LOG(LogTimecodeNetwork, Verbose, TEXT("UDP receiver stopped"));
    }

    // 소켓 정리
    if (Socket != nullptr)
    {
        // 멀티캐스트 그룹 탈퇴
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

    // 연결 상태 설정
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

    // 메시지 직렬화
    FTimecodeNetworkMessage Message;
    Message.MessageType = MessageType;
    Message.Timecode = Timecode;
    Message.Timestamp = FPlatformTime::Seconds();
    Message.SenderID = InstanceID;
    Message.Data = TEXT(""); // 기본 데이터는 빈 문자열

    // 메시지 직렬화
    TArray<uint8> MessageData = Message.Serialize();

    // 메시지 전송
    int32 BytesSent = 0;

    // 수동 모드에서 슬레이브일 때 마스터 IP가 설정되어 있으면 해당 IP로 직접 전송
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
    // 타겟 IP가 설정된 경우 유니캐스트 사용
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
    // 멀티캐스트 그룹이 설정된 경우 멀티캐스트 사용
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

    // 메시지 직렬화
    FTimecodeNetworkMessage Message;
    Message.MessageType = ETimecodeMessageType::Event;
    Message.Timecode = Timecode;
    Message.Data = EventName;
    Message.Timestamp = FPlatformTime::Seconds();
    Message.SenderID = InstanceID;

    // 메시지 직렬화
    TArray<uint8> MessageData = Message.Serialize();

    // 메시지 전송 (멀티캐스트 그룹 또는 단일 대상 IP 선택)
    int32 BytesSent = 0;

    // 전송 우선순위: 멀티캐스트 > 타겟IP > 마스터IP
    if (!MulticastGroupAddress.IsEmpty())
    {
        // 멀티캐스트 그룹 전송
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
        // 단일 대상 IP 전송
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
        // 수동 슬레이브 모드에서 마스터 IP로 전송
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

        // 역할 재결정
        bool bOldMasterMode = bIsMasterMode;

        if (RoleMode == ETimecodeRoleMode::Automatic)
        {
            // 자동 모드에서는 IP 기반 또는 기타 자동 감지 로직 사용
            bIsMasterMode = AutoDetectRole();
            bRoleAutomaticallyDetermined = true;
        }
        else
        {
            // 수동 모드에서는 사용자 설정 사용
            bIsMasterMode = bIsManuallyMaster;
            bRoleAutomaticallyDetermined = false;
        }

        // 역할이 변경되었고 이미 초기화된 경우 재초기화
        if (bOldMasterMode != bIsMasterMode && Socket != nullptr)
        {
            UE_LOG(LogTimecodeNetwork, Log, TEXT("Role changed, reinitializing network..."));
            int32 OldPort = PortNumber;
            Shutdown();
            Initialize(bIsMasterMode, OldPort);
        }

        // 이벤트 발생
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

        // 수동 모드인 경우에만 역할 변경 적용
        if (RoleMode == ETimecodeRoleMode::Manual)
        {
            bool bOldMasterMode = bIsMasterMode;
            bIsMasterMode = bIsManuallyMaster;

            // 역할이 변경되었고 이미 초기화된 경우 재초기화
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

        // 수동 슬레이브 모드에서 타겟 IP 설정
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

    // UDP 소켓 생성
    Socket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("TimecodeSocket"), true);
    if (Socket == nullptr)
    {
        UE_LOG(LogTimecodeNetwork, Error, TEXT("Failed to create UDP socket"));
        return false;
    }

    // 소켓 설정
    Socket->SetReuseAddr();
    Socket->SetRecvErr();
    Socket->SetNonBlocking();

    // 브로드캐스트 설정
    Socket->SetBroadcast();

    // 로컬 주소 설정
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

    // 데이터를 TArray<uint8>로 변환
    TArray<uint8> ReceivedData;
    ReceivedData.Append(DataPtr->GetData(), DataPtr->Num());

    // 메시지 역직렬화
    FTimecodeNetworkMessage Message;
    if (Message.Deserialize(ReceivedData))
    {
        // 수신된 메시지의 발신자가 자신이 아닌 경우에만 처리
        if (Message.SenderID != InstanceID)
        {
            // 메시지 처리
            ProcessMessage(Message);
            // 메시지 수신 알림
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
    // 메시지 타입 문자열 변환
    FString MessageTypeStr = UEnum::GetValueAsString(Message.MessageType);

    switch (Message.MessageType)
    {
    case ETimecodeMessageType::Heartbeat:
        // 하트비트 처리
        UE_LOG(LogTimecodeNetwork, Verbose, TEXT("Received heartbeat (%s) from %s"), *MessageTypeStr, *Message.SenderID);
        break;

    case ETimecodeMessageType::TimecodeSync:
        // 타임코드 동기화 처리
        UE_LOG(LogTimecodeNetwork, Verbose, TEXT("Received timecode (%s): %s from %s"), *MessageTypeStr, *Message.Timecode, *Message.SenderID);
        break;

    case ETimecodeMessageType::RoleAssignment:
        // 역할 할당 처리
        UE_LOG(LogTimecodeNetwork, Log, TEXT("Received role assignment (%s): %s from %s"), *MessageTypeStr, *Message.Data, *Message.SenderID);

        // 자동 모드에서만 외부 할당 처리
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
        // 이벤트 처리
        UE_LOG(LogTimecodeNetwork, Log, TEXT("Received event (%s): %s at %s from %s"), *MessageTypeStr, *Message.Data, *Message.Timecode, *Message.SenderID);
        break;

    case ETimecodeMessageType::Command:
        // 명령 처리
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
    // 로컬 IP 주소 가져오기
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    bool bCanBindAll = false;
    TSharedPtr<FInternetAddr> LocalAddr = SocketSubsystem->GetLocalHostAddr(*GLog, bCanBindAll);

    if (LocalAddr.IsValid() && bCanBindAll)
    {
        // IP 주소 출력
        FString LocalIP = LocalAddr->ToString(false);
        UE_LOG(LogTimecodeNetwork, Log, TEXT("Local IP: %s"), *LocalIP);

        // nDisplay 통합 코드를 여기에 추가할 수 있음

        // IP 기반 간단한 결정 로직
        // 실제 구현에서는 UDP 브로드캐스트로 다른 노드를 검색하고
        // IP를 비교하거나 기타 방법으로 마스터를 결정해야 함

        // 디스커버리 메시지 브로드캐스트하여 다른 노드 검색
        // 응답을 기다리고 IP 비교하는 로직 구현

        // 임시 구현: 첫 번째 노드가 마스터가 됨 (더 복잡한 검색 로직으로 대체 필요)
        bool bHasLowerIP = false;

        // 여기서 검색 로직 구현
        // ...

        // 더 낮은 IP를 가진 노드가 없으면 마스터가 됨
        bool bIsMaster = !bHasLowerIP;

        UE_LOG(LogTimecodeNetwork, Log, TEXT("Auto-detected role: %s"), bIsMaster ? TEXT("MASTER") : TEXT("SLAVE"));

        return bIsMaster;
    }

    // IP를 가져올 수 없는 경우 기본적으로 슬레이브로 설정
    UE_LOG(LogTimecodeNetwork, Warning, TEXT("Could not determine local IP address, defaulting to SLAVE"));
    return false;
}