// TimecodeSyncNetworkTest.cpp (Modified version)
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

    // Get socket subsystem
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        LogTestResult(TEXT("UDP Connection"), false, TEXT("Socket subsystem not found"));
        return false;
    }

    // Create sender socket
    FSocket* SenderSocket = FUdpSocketBuilder(TEXT("TimecodeSyncSender"))
        .AsNonBlocking()
        .AsReusable()
        .BoundToPort(Port)
        .WithBroadcast();

    // Create receiver socket
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

    // Prepare test message
    FString TestMessage = TEXT("TimecodeSyncTest");
    TArray<uint8> SendBuffer;
    SendBuffer.SetNum(TestMessage.Len() + 1);
    memcpy(SendBuffer.GetData(), TCHAR_TO_ANSI(*TestMessage), TestMessage.Len() + 1);

    // Localhost address (127.0.0.1)
    FIPv4Address LocalhostAddr;
    FIPv4Address::Parse(TEXT("127.0.0.1"), LocalhostAddr);
    TSharedRef<FInternetAddr> TargetAddr = SocketSubsystem->CreateInternetAddr();
    TargetAddr->SetIp(LocalhostAddr.Value);
    TargetAddr->SetPort(Port + 1);

    // Send message
    int32 BytesSent = 0;
    bool bSendSuccess = SenderSocket->SendTo(SendBuffer.GetData(), SendBuffer.Num(), BytesSent, *TargetAddr);

    // Wait for a moment (asynchronous send)
    FPlatformProcess::Sleep(0.1f);

    // Receive message
    TArray<uint8> ReceiveBuffer;
    ReceiveBuffer.SetNum(1024);
    int32 BytesRead = 0;
    TSharedRef<FInternetAddr> SenderAddr = SocketSubsystem->CreateInternetAddr();
    bool bRecvSuccess = ReceiverSocket->RecvFrom(ReceiveBuffer.GetData(), ReceiveBuffer.Num(), BytesRead, *SenderAddr);

    // Check results
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

    // Clean up sockets
    SocketSubsystem->DestroySocket(SenderSocket);
    SocketSubsystem->DestroySocket(ReceiverSocket);

    LogTestResult(TEXT("UDP Connection"), bSuccess, ResultMessage);
    return bSuccess;
}

bool UTimecodeSyncNetworkTest::TestPacketLoss(float LossPercentage)
{
    bool bSuccess = false;
    FString ResultMessage;

    // Get socket subsystem
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        ResultMessage = TEXT("Socket subsystem not found");
        LogTestResult(TEXT("Packet Loss"), bSuccess, ResultMessage);
        return bSuccess;
    }

    // Create sender socket
    FSocket* SenderSocket = FUdpSocketBuilder(TEXT("TimecodeSyncSender"))
        .AsNonBlocking()
        .AsReusable()
        .BoundToPort(12346)
        .WithBroadcast();

    // Create receiver socket
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

    // Localhost address (127.0.0.1)
    FIPv4Address LocalhostAddr;
    FIPv4Address::Parse(TEXT("127.0.0.1"), LocalhostAddr);
    TSharedRef<FInternetAddr> TargetAddr = SocketSubsystem->CreateInternetAddr();
    TargetAddr->SetIp(LocalhostAddr.Value);
    TargetAddr->SetPort(12347);

    // Simulate packet loss
    const int32 TotalPackets = 100;
    int32 SentPackets = 0;
    int32 ReceivedPackets = 0;

    for (int32 i = 0; i < TotalPackets; ++i)
    {
        // Randomly determine packet loss (based on specified probability)
        bool bShouldSend = FMath::RandRange(0.0f, 100.0f) > LossPercentage;

        if (bShouldSend)
        {
            // Prepare test message
            FString TestMessage = FString::Printf(TEXT("Packet-%d"), i);
            TArray<uint8> SendBuffer;
            SendBuffer.SetNum(TestMessage.Len() + 1);
            memcpy(SendBuffer.GetData(), TCHAR_TO_ANSI(*TestMessage), TestMessage.Len() + 1);

            // Send message
            int32 BytesSent = 0;
            bool bSendSuccess = SenderSocket->SendTo(SendBuffer.GetData(), SendBuffer.Num(), BytesSent, *TargetAddr);

            if (bSendSuccess)
            {
                SentPackets++;
            }
        }

        // Wait for a moment (asynchronous send)
        FPlatformProcess::Sleep(0.01f);
    }

    // Try to receive all packets
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

        // Wait for sufficient time
        FPlatformProcess::Sleep(0.01f);
    }

    // Check results
    float ActualLossPercentage = 100.0f * (1.0f - (float)ReceivedPackets / (float)SentPackets);
    bSuccess = FMath::IsNearlyEqual(ActualLossPercentage, LossPercentage, 10.0f); // Allow 10% error margin

    ResultMessage = FString::Printf(TEXT("Sent: %d, Received: %d, Expected loss: %.1f%%, Actual loss: %.1f%%"),
        SentPackets, ReceivedPackets, LossPercentage, ActualLossPercentage);

    // Clean up sockets
    SocketSubsystem->DestroySocket(SenderSocket);
    SocketSubsystem->DestroySocket(ReceiverSocket);

    LogTestResult(TEXT("Packet Loss"), bSuccess, ResultMessage);
    return bSuccess;
}

void UTimecodeSyncNetworkTest::LogTestResult(const FString& TestName, bool bSuccess, const FString& Message)
{
    FString ResultStr = bSuccess ? TEXT("PASSED") : TEXT("FAILED");
    // Output to both regular log and screen message
    UE_LOG(LogTemp, Display, TEXT("=============================="));
    UE_LOG(LogTemp, Display, TEXT("[TimecodeSyncTest] %s: %s"), *TestName, *ResultStr);
    UE_LOG(LogTemp, Display, TEXT("%s"), *Message);
    UE_LOG(LogTemp, Display, TEXT("=============================="));

    // Display message on screen (useful during development)
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, bSuccess ? FColor::Green : FColor::Red,
            FString::Printf(TEXT("[TimecodeSyncTest] %s: %s"), *TestName, *ResultStr));
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, Message);
    }
}

bool UTimecodeSyncNetworkTest::TestMessageSerialization()
{
    bool bSuccess = true;
    FString ResultMessage;

    // Test case 1: Basic timecode message
    {
        FTimecodeNetworkMessage OriginalMessage;
        OriginalMessage.MessageType = ETimecodeMessageType::TimecodeSync;
        OriginalMessage.Timecode = TEXT("01:23:45:15");
        OriginalMessage.Data = TEXT("");
        OriginalMessage.Timestamp = 1234567890.123;
        OriginalMessage.SenderID = TEXT("TestSender1");

        // Serialize
        TArray<uint8> SerializedData = OriginalMessage.Serialize();

        // Check data size
        if (SerializedData.Num() <= 0)
        {
            bSuccess = false;
            ResultMessage += TEXT("Test Case 1: Serialization failed - Empty data\n");
        }
        else
        {
            // Deserialize
            FTimecodeNetworkMessage DeserializedMessage;
            bool bDeserializeSuccess = DeserializedMessage.Deserialize(SerializedData);

            // Check deserialization result
            if (!bDeserializeSuccess)
            {
                bSuccess = false;
                ResultMessage += TEXT("Test Case 1: Deserialization failed\n");
            }
            else
            {
                // Compare messages
                bool bMessagesEqual =
                    (DeserializedMessage.MessageType == OriginalMessage.MessageType) &&
                    (DeserializedMessage.Timecode == OriginalMessage.Timecode) &&
                    (DeserializedMessage.SenderID == OriginalMessage.SenderID);

                if (!bMessagesEqual)
                {
                    bSuccess = false;
                    ResultMessage += FString::Printf(TEXT("Test Case 1: Message mismatch - Original: %s, Deserialized: %s\n"),
                        *OriginalMessage.Timecode, *DeserializedMessage.Timecode);
                }
                else
                {
                    ResultMessage += TEXT("Test Case 1: Successfully serialized and deserialized basic message\n");
                }
            }
        }
    }

    // Test case 2: Event message
    {
        FTimecodeNetworkMessage OriginalMessage;
        OriginalMessage.MessageType = ETimecodeMessageType::Event;
        OriginalMessage.Timecode = TEXT("02:34:56:12");
        OriginalMessage.Data = TEXT("TestEvent");
        OriginalMessage.Timestamp = 9876543210.987;
        OriginalMessage.SenderID = TEXT("TestSender2");

        // Serialize
        TArray<uint8> SerializedData = OriginalMessage.Serialize();

        // Check data size
        if (SerializedData.Num() <= 0)
        {
            bSuccess = false;
            ResultMessage += TEXT("Test Case 2: Serialization failed - Empty data\n");
        }
        else
        {
            // Deserialize
            FTimecodeNetworkMessage DeserializedMessage;
            bool bDeserializeSuccess = DeserializedMessage.Deserialize(SerializedData);

            // Check deserialization result
            if (!bDeserializeSuccess)
            {
                bSuccess = false;
                ResultMessage += TEXT("Test Case 2: Deserialization failed\n");
            }
            else
            {
                // Compare messages
                bool bMessagesEqual =
                    (DeserializedMessage.MessageType == OriginalMessage.MessageType) &&
                    (DeserializedMessage.Timecode == OriginalMessage.Timecode) &&
                    (DeserializedMessage.Data == OriginalMessage.Data) &&
                    (DeserializedMessage.SenderID == OriginalMessage.SenderID);

                if (!bMessagesEqual)
                {
                    bSuccess = false;
                    ResultMessage += FString::Printf(TEXT("Test Case 2: Message mismatch - Original event: %s, Deserialized event: %s\n"),
                        *OriginalMessage.Data, *DeserializedMessage.Data);
                }
                else
                {
                    ResultMessage += TEXT("Test Case 2: Successfully serialized and deserialized event message\n");
                }
            }
        }
    }

    // Log overall result
    LogTestResult(TEXT("Message Serialization"), bSuccess, ResultMessage);
    return bSuccess;
}