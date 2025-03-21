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

UTimecodeNetworkManager::UTimecodeNetworkManager()
    : Socket(nullptr)
    , Receiver(nullptr)
    , ConnectionState(ENetworkConnectionState::Disconnected)
    , PortNumber(10000)
    , TargetIPAddress(TEXT("127.0.0.1"))
    , MulticastGroupAddress(TEXT("239.0.0.1"))
    , bIsMasterMode(false)
{
    // 임의의 고유 ID 생성
    InstanceID = FGuid::NewGuid().ToString();
}

UTimecodeNetworkManager::~UTimecodeNetworkManager()
{
    Shutdown();
}

bool UTimecodeNetworkManager::Initialize(bool bIsMaster, int32 Port)
{
    // 이미 초기화된 경우 종료
    if (Socket != nullptr || Receiver != nullptr)
    {
        Shutdown();
    }

    bIsMasterMode = bIsMaster;
    PortNumber = Port;

    // 소켓 생성
    if (!CreateSocket())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create UDP socket"));
        return false;
    }

    // UDP 수신 설정
    FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(100);
    Receiver = new FUdpSocketReceiver(Socket, ThreadWaitTime, TEXT("TimecodeReceiver"));
    Receiver->OnDataReceived().BindUObject(this, &UTimecodeNetworkManager::OnUDPReceived);
    Receiver->Start();

    // 연결 상태 설정
    SetConnectionState(ENetworkConnectionState::Connected);

    UE_LOG(LogTemp, Log, TEXT("Network manager initialized in %s mode"), bIsMaster ? TEXT("Master") : TEXT("Slave"));
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
    }

    // 소켓 종료
    if (Socket != nullptr)
    {
        ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
        SocketSubsystem->DestroySocket(Socket);
        Socket = nullptr;
    }

    // 연결 상태 설정
    SetConnectionState(ENetworkConnectionState::Disconnected);

    UE_LOG(LogTemp, Log, TEXT("Network manager shutdown"));
}

bool UTimecodeNetworkManager::SendTimecodeMessage(const FString& Timecode, ETimecodeMessageType MessageType)
{
    if (Socket == nullptr || ConnectionState != ENetworkConnectionState::Connected)
    {
        return false;
    }

    // 메시지 직렬화
    FTimecodeNetworkMessage Message;
    Message.MessageType = MessageType;
    Message.Timecode = Timecode;
    Message.Timestamp = FPlatformTime::Seconds();
    Message.SenderID = InstanceID;

    // 메시지 직렬화
    TArray<uint8> MessageData = Message.Serialize();

    // 메시지 전송
    int32 BytesSent = 0;
    if (!TargetIPAddress.IsEmpty())
    {
        // 단일 대상 IP 전송
        TSharedRef<FInternetAddr> TargetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
        bool bIsValid = false;
        TargetAddr->SetIp(*TargetIPAddress, bIsValid);
        if (!bIsValid)
        {
            UE_LOG(LogTemp, Error, TEXT("Invalid target IP: %s"), *TargetIPAddress);
            return false;
        }
        TargetAddr->SetPort(PortNumber);

        Socket->SendTo(MessageData.GetData(), MessageData.Num(), BytesSent, *TargetAddr);
    }
    else if (!MulticastGroupAddress.IsEmpty())
    {
        // 멀티캐스트 그룹 전송
        TSharedRef<FInternetAddr> MulticastAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
        bool bIsValid = false;
        MulticastAddr->SetIp(*MulticastGroupAddress, bIsValid);
        if (!bIsValid)
        {
            UE_LOG(LogTemp, Error, TEXT("Invalid multicast group: %s"), *MulticastGroupAddress);
            return false;
        }
        MulticastAddr->SetPort(PortNumber);

        Socket->SendTo(MessageData.GetData(), MessageData.Num(), BytesSent, *MulticastAddr);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("No target IP or multicast group specified"));
        return false;
    }

    return BytesSent == MessageData.Num();
}

bool UTimecodeNetworkManager::SendEventMessage(const FString& EventName, const FString& Timecode)
{
    if (Socket == nullptr || ConnectionState != ENetworkConnectionState::Connected)
    {
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
    if (!MulticastGroupAddress.IsEmpty())
    {
        // 멀티캐스트 그룹 전송
        TSharedRef<FInternetAddr> MulticastAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
        bool bIsValid = false;
        MulticastAddr->SetIp(*MulticastGroupAddress, bIsValid);
        if (!bIsValid)
        {
            UE_LOG(LogTemp, Error, TEXT("Invalid multicast group: %s"), *MulticastGroupAddress);
            return false;
        }
        MulticastAddr->SetPort(PortNumber);

        Socket->SendTo(MessageData.GetData(), MessageData.Num(), BytesSent, *MulticastAddr);
    }
    else if (!TargetIPAddress.IsEmpty())
    {
        // 단일 대상 IP 전송
        TSharedRef<FInternetAddr> TargetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
        bool bIsValid = false;
        TargetAddr->SetIp(*TargetIPAddress, bIsValid);
        if (!bIsValid)
        {
            UE_LOG(LogTemp, Error, TEXT("Invalid target IP: %s"), *TargetIPAddress);
            return false;
        }
        TargetAddr->SetPort(PortNumber);

        Socket->SendTo(MessageData.GetData(), MessageData.Num(), BytesSent, *TargetAddr);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("No target IP or multicast group specified"));
        return false;
    }

    return BytesSent == MessageData.Num();
}

void UTimecodeNetworkManager::SetTargetIP(const FString& IPAddress)
{
    TargetIPAddress = IPAddress;
}

bool UTimecodeNetworkManager::JoinMulticastGroup(const FString& MulticastGroup)
{
    if (Socket == nullptr)
    {
        return false;
    }

    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    TSharedRef<FInternetAddr> GroupAddr = SocketSubsystem->CreateInternetAddr();

    bool bIsValid = false;
    GroupAddr->SetIp(*MulticastGroup, bIsValid);

    if (!bIsValid)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid multicast group address: %s"), *MulticastGroup);
        return false;
    }

    if (!Socket->JoinMulticastGroup(*GroupAddr))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to join multicast group: %s"), *MulticastGroup);
        return false;
    }

    MulticastGroupAddress = MulticastGroup;
    UE_LOG(LogTemp, Log, TEXT("Joined multicast group: %s"), *MulticastGroup);
    return true;
}

ENetworkConnectionState UTimecodeNetworkManager::GetConnectionState() const
{
    return ConnectionState;
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
        UE_LOG(LogTemp, Warning, TEXT("Failed to deserialize received message from %s"), *Endpoint.ToString());
    }
}

bool UTimecodeNetworkManager::CreateSocket()
{
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (SocketSubsystem == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("Socket subsystem not found"));
        return false;
    }

    // UDP 소켓 생성
    Socket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("TimecodeSocket"), true);
    if (Socket == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create UDP socket"));
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
        UE_LOG(LogTemp, Error, TEXT("Failed to bind socket to port %d"), PortNumber);
        SocketSubsystem->DestroySocket(Socket);
        Socket = nullptr;
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("UDP socket created and bound to port %d"), PortNumber);
    return true;
}

void UTimecodeNetworkManager::ProcessMessage(const FTimecodeNetworkMessage& Message)
{
    // ETimecodeMessageType? int32? ???? ????
    int32 MessageTypeInt = static_cast<int32>(Message.MessageType);

    switch (Message.MessageType)
    {
    case ETimecodeMessageType::Heartbeat:
        // ???? ????
        UE_LOG(LogTemp, Verbose, TEXT("Received heartbeat (type: %d) from %s"), MessageTypeInt, *Message.SenderID);
        break;

    case ETimecodeMessageType::TimecodeSync:
        // ???? ??? ????
        UE_LOG(LogTemp, Verbose, TEXT("Received timecode (type: %d): %s from %s"), MessageTypeInt, *Message.Timecode, *Message.SenderID);
        break;

    case ETimecodeMessageType::RoleAssignment:
        // ?? ?? ????
        UE_LOG(LogTemp, Log, TEXT("Received role assignment (type: %d): %s from %s"), MessageTypeInt, *Message.Data, *Message.SenderID);
        break;

    case ETimecodeMessageType::Event:
        // ??? ????
        UE_LOG(LogTemp, Log, TEXT("Received event (type: %d): %s at %s from %s"), MessageTypeInt, *Message.Data, *Message.Timecode, *Message.SenderID);
        break;

    case ETimecodeMessageType::Command:
        // ?? ????
        UE_LOG(LogTemp, Log, TEXT("Received command (type: %d): %s from %s"), MessageTypeInt, *Message.Data, *Message.SenderID);
        break;

    default:
        UE_LOG(LogTemp, Warning, TEXT("Received unknown message type (%d) from %s"), MessageTypeInt, *Message.SenderID);
        break;
    }
}

void UTimecodeNetworkManager::SetConnectionState(ENetworkConnectionState NewState)
{
    if (ConnectionState != NewState)
    {
        ConnectionState = NewState;
        OnNetworkStateChanged.Broadcast(ConnectionState);

        UE_LOG(LogTemp, Log, TEXT("Network connection state changed: %d"), static_cast<int32>(NewState));
    }
}

void UTimecodeNetworkManager::SendHeartbeat()
{
    if (Socket != nullptr && ConnectionState == ENetworkConnectionState::Connected)
    {
        SendTimecodeMessage(TEXT(""), ETimecodeMessageType::Heartbeat);
    }
}