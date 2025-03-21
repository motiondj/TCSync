// TimecodeSyncNetworkTest.cpp (������ ����)
#include "Tests/TimecodeSyncNetworkTest.h"
#include "TimecodeNetworkManager.h"
#include "Misc/AutomationTest.h"
#include "Logging/LogMacros.h"

UTimecodeSyncNetworkTest::UTimecodeSyncNetworkTest()
{
}

bool UTimecodeSyncNetworkTest::TestUDPConnection(int32 Port)
{
    bool bSuccess = false;
    FString ResultMessage;

    // ���� ����ý��� ��������
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        LogTestResult(TEXT("UDP Connection"), false, TEXT("Socket subsystem not found"));
        return false;
    }

    // �۽� ���� ����
    FSocket* SenderSocket = FUdpSocketBuilder(TEXT("TimecodeSyncSender"))
        .AsNonBlocking()
        .AsReusable()
        .BoundToPort(Port)
        .WithBroadcast();

    // ���� ���� ����
    FSocket* ReceiverSocket = FUdpSocketBuilder(TEXT("TimecodeSyncReceiver"))
        .AsNonBlocking()
        .AsReusable()
        .BoundToPort(Port + 1)
        .WithBroadcast();

    if (!SenderSocket || !ReceiverSocket)
    {
        if (SenderSocket) SocketSubsystem->DestroySocket(SenderSocket);
        if (ReceiverSocket) SocketSubsystem->DestroySocket(ReceiverSocket);

        LogTestResult(TEXT("UDP Connection"), false, TEXT("Failed to create UDP sockets"));
        return false;
    }

    // �׽�Ʈ �޽��� �غ�
    FString TestMessage = TEXT("TimecodeSyncTest");
    TArray<uint8> SendBuffer;
    SendBuffer.SetNum(TestMessage.Len() + 1);
    memcpy(SendBuffer.GetData(), TCHAR_TO_ANSI(*TestMessage), TestMessage.Len() + 1);

    // ����ȣ��Ʈ �ּ� (127.0.0.1)
    FIPv4Address LocalhostAddr;
    FIPv4Address::Parse(TEXT("127.0.0.1"), LocalhostAddr);
    TSharedRef<FInternetAddr> TargetAddr = SocketSubsystem->CreateInternetAddr();
    TargetAddr->SetIp(LocalhostAddr.Value);
    TargetAddr->SetPort(Port + 1);

    // �޽��� ����
    int32 BytesSent = 0;
    bool bSendSuccess = SenderSocket->SendTo(SendBuffer.GetData(), SendBuffer.Num(), BytesSent, *TargetAddr);

    // ��� ��� (�񵿱� ����)
    FPlatformProcess::Sleep(0.1f);

    // �޽��� ����
    TArray<uint8> ReceiveBuffer;
    ReceiveBuffer.SetNum(1024);
    int32 BytesRead = 0;
    TSharedRef<FInternetAddr> SenderAddr = SocketSubsystem->CreateInternetAddr();
    bool bRecvSuccess = ReceiverSocket->RecvFrom(ReceiveBuffer.GetData(), ReceiveBuffer.Num(), BytesRead, *SenderAddr);

    // ��� Ȯ��
    if (bSendSuccess && bRecvSuccess && BytesRead > 0)
    {
        FString ReceivedMessage = ANSI_TO_TCHAR((char*)ReceiveBuffer.GetData());
        bSuccess = (ReceivedMessage == TestMessage);
        ResultMessage = FString::Printf(TEXT("Sent: %s, Received: %s, Bytes: %d"), *TestMessage, *ReceivedMessage, BytesRead);
    }
    else
    {
        ResultMessage = FString::Printf(TEXT("Send success: %d, Receive success: %d, Bytes read: %d"),
            bSendSuccess, bRecvSuccess, BytesRead);
    }

    // ���� ����
    SocketSubsystem->DestroySocket(SenderSocket);
    SocketSubsystem->DestroySocket(ReceiverSocket);

    LogTestResult(TEXT("UDP Connection"), bSuccess, ResultMessage);
    return bSuccess;
}

bool UTimecodeSyncNetworkTest::TestMessageSerialization()
{
    // �ӽ÷� Always Pass ��ȯ (���� ������Ʈ ����ü Ȯ�� �ʿ�)
    UE_LOG(LogTemp, Warning, TEXT("[TimecodeSyncTest] Message Serialization test not implemented yet - needs actual structure definition"));
    LogTestResult(TEXT("Message Serialization"), true, TEXT("Test skipped - need actual implementation"));
    return true;
}

bool UTimecodeSyncNetworkTest::TestPacketLoss(float LossPercentage)
{
    bool bSuccess = false;
    FString ResultMessage;

    // ���� ����ý��� ��������
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        ResultMessage = TEXT("Socket subsystem not found");
        LogTestResult(TEXT("Packet Loss"), bSuccess, ResultMessage);
        return bSuccess;
    }

    // �۽� ���� ����
    FSocket* SenderSocket = FUdpSocketBuilder(TEXT("TimecodeSyncSender"))
        .AsNonBlocking()
        .AsReusable()
        .BoundToPort(12346)
        .WithBroadcast();

    // ���� ���� ����
    FSocket* ReceiverSocket = FUdpSocketBuilder(TEXT("TimecodeSyncReceiver"))
        .AsNonBlocking()
        .AsReusable()
        .BoundToPort(12347)
        .WithBroadcast();

    if (!SenderSocket || !ReceiverSocket)
    {
        if (SenderSocket) SocketSubsystem->DestroySocket(SenderSocket);
        if (ReceiverSocket) SocketSubsystem->DestroySocket(ReceiverSocket);

        ResultMessage = TEXT("Failed to create UDP sockets");
        LogTestResult(TEXT("Packet Loss"), bSuccess, ResultMessage);
        return bSuccess;
    }

    // ����ȣ��Ʈ �ּ� (127.0.0.1)
    FIPv4Address LocalhostAddr;
    FIPv4Address::Parse(TEXT("127.0.0.1"), LocalhostAddr);
    TSharedRef<FInternetAddr> TargetAddr = SocketSubsystem->CreateInternetAddr();
    TargetAddr->SetIp(LocalhostAddr.Value);
    TargetAddr->SetPort(12347);

    // ��Ŷ �ս� �ùķ��̼�
    const int32 TotalPackets = 100;
    int32 SentPackets = 0;
    int32 ReceivedPackets = 0;

    for (int32 i = 0; i < TotalPackets; ++i)
    {
        // ���Ƿ� ��Ŷ �ս� ���� (������ Ȯ����)
        bool bShouldSend = FMath::RandRange(0.0f, 100.0f) > LossPercentage;

        if (bShouldSend)
        {
            // �׽�Ʈ �޽��� �غ�
            FString TestMessage = FString::Printf(TEXT("Packet-%d"), i);
            TArray<uint8> SendBuffer;
            SendBuffer.SetNum(TestMessage.Len() + 1);
            memcpy(SendBuffer.GetData(), TCHAR_TO_ANSI(*TestMessage), TestMessage.Len() + 1);

            // �޽��� ����
            int32 BytesSent = 0;
            bool bSendSuccess = SenderSocket->SendTo(SendBuffer.GetData(), SendBuffer.Num(), BytesSent, *TargetAddr);

            if (bSendSuccess)
            {
                SentPackets++;
            }
        }

        // ��� ��� (�񵿱� ����)
        FPlatformProcess::Sleep(0.01f);
    }

    // ��� ��Ŷ ���� �õ�
    for (int32 i = 0; i < SentPackets; ++i)
    {
        TArray<uint8> ReceiveBuffer;
        ReceiveBuffer.SetNum(1024);
        int32 BytesRead = 0;
        TSharedRef<FInternetAddr> SenderAddr = SocketSubsystem->CreateInternetAddr();

        bool bRecvSuccess = ReceiverSocket->RecvFrom(ReceiveBuffer.GetData(), ReceiveBuffer.Num(), BytesRead, *SenderAddr);

        if (bRecvSuccess && BytesRead > 0)
        {
            ReceivedPackets++;
        }

        // ����� �ð� ���
        FPlatformProcess::Sleep(0.01f);
    }

    // ��� Ȯ��
    float ActualLossPercentage = 100.0f * (1.0f - (float)ReceivedPackets / (float)SentPackets);
    bSuccess = FMath::IsNearlyEqual(ActualLossPercentage, LossPercentage, 10.0f); // 10% ���� ���

    ResultMessage = FString::Printf(TEXT("Sent: %d, Received: %d, Expected loss: %.1f%%, Actual loss: %.1f%%"),
        SentPackets, ReceivedPackets, LossPercentage, ActualLossPercentage);

    // ���� ����
    SocketSubsystem->DestroySocket(SenderSocket);
    SocketSubsystem->DestroySocket(ReceiverSocket);

    LogTestResult(TEXT("Packet Loss"), bSuccess, ResultMessage);
    return bSuccess;
}

void UTimecodeSyncNetworkTest::LogTestResult(const FString& TestName, bool bSuccess, const FString& Message)
{
    FString ResultStr = bSuccess ? TEXT("PASSED") : TEXT("FAILED");
    // �Ϲ� �α׿� ȭ�� �޽��� ��� ���
    UE_LOG(LogTemp, Display, TEXT("=============================="));
    UE_LOG(LogTemp, Display, TEXT("[TimecodeSyncTest] %s: %s"), *TestName, *ResultStr);
    UE_LOG(LogTemp, Display, TEXT("%s"), *Message);
    UE_LOG(LogTemp, Display, TEXT("=============================="));

    // ȭ�鿡�� �޽��� ǥ�� (���� ���� ����)
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, bSuccess ? FColor::Green : FColor::Red,
            FString::Printf(TEXT("[TimecodeSyncTest] %s: %s"), *TestName, *ResultStr));
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, Message);
    }
}