// TimecodeSyncLogicTest.cpp (������ ����)
#include "Tests/TimecodeSyncLogicTest.h"
#include "TimecodeNetworkManager.h"
#include "TimecodeNetworkTypes.h"
#include "Logging/LogMacros.h"

UTimecodeSyncLogicTest::UTimecodeSyncLogicTest()
    : CurrentMasterTimecode(TEXT(""))
    , CurrentSlaveTimecode(TEXT(""))
    , TestReceivedTimecode(TEXT(""))
    , SystemTimeReceivedTimecode(TEXT(""))
{
}

bool UTimecodeSyncLogicTest::TestMasterSlaveSync(float Duration)
{
    bool bSuccess = false;
    FString ResultMessage;

    // �ʱ� Ÿ���ڵ� �� ����
    CurrentMasterTimecode = TEXT("");
    CurrentSlaveTimecode = TEXT("");

    // ������ �Ŵ��� �ν��Ͻ� ����
    UTimecodeNetworkManager* MasterManager = NewObject<UTimecodeNetworkManager>();
    if (!MasterManager)
    {
        LogTestResult(TEXT("Master/Slave Sync"), bSuccess, TEXT("Failed to create master manager"));
        return bSuccess;
    }

    // �����̺� �Ŵ��� �ν��Ͻ� ����
    UTimecodeNetworkManager* SlaveManager = NewObject<UTimecodeNetworkManager>();
    if (!SlaveManager)
    {
        LogTestResult(TEXT("Master/Slave Sync"), bSuccess, TEXT("Failed to create slave manager"));
        return bSuccess;
    }

    // ������ ����
    bool bMasterInitialized = MasterManager->Initialize(true, 12345);
    if (!bMasterInitialized)
    {
        LogTestResult(TEXT("Master/Slave Sync"), bSuccess, TEXT("Failed to initialize master manager"));
        return bSuccess;
    }

    // �����̺� ����
    bool bSlaveInitialized = SlaveManager->Initialize(false, 12346);
    if (!bSlaveInitialized)
    {
        MasterManager->Shutdown();
        LogTestResult(TEXT("Master/Slave Sync"), bSuccess, TEXT("Failed to initialize slave manager"));
        return bSuccess;
    }

    // �����̺갡 ������ IP�� Ÿ�� ����
    SlaveManager->SetTargetIP(TEXT("127.0.0.1"));

    // ������ �ð� ���� ����ϸ� ����ȭ ����
    UE_LOG(LogTemp, Display, TEXT("[TimecodeSyncTest] Starting Master/Slave sync test for %.1f seconds..."), Duration);

    const int32 NumSamples = 10;
    TArray<FString> MasterTimecodes;
    TArray<FString> SlaveTimecodes;
    TArray<double> TimeDiffs;
    MasterTimecodes.Reserve(NumSamples);
    SlaveTimecodes.Reserve(NumSamples);
    TimeDiffs.Reserve(NumSamples);

    // Ÿ���ڵ� �޽��� ���� ĸó�� ���� ��������Ʈ ����
    MasterManager->OnMessageReceived.AddDynamic(this, &UTimecodeSyncLogicTest::OnMasterMessageReceived);
    SlaveManager->OnMessageReceived.AddDynamic(this, &UTimecodeSyncLogicTest::OnSlaveMessageReceived);

    // �����Ͱ� Ÿ���ڵ� �޽��� ����
    FString TestTimecode = TEXT("01:30:45:12");
    MasterManager->SendTimecodeMessage(TestTimecode, ETimecodeMessageType::TimecodeSync);

    for (int32 i = 0; i < NumSamples; ++i)
    {
        // �� ���� �� ����
        FPlatformProcess::Sleep(Duration / NumSamples);

        // ���� �׽�Ʈ������ Ÿ���ڵ� �� ��
        MasterTimecodes.Add(CurrentMasterTimecode);
        SlaveTimecodes.Add(CurrentSlaveTimecode);

        // ������ �׽�Ʈ�� ���� ���ڿ� �񱳷� Ȯ��
        bool bTimecodeMatch = (CurrentSlaveTimecode == TestTimecode && !CurrentSlaveTimecode.IsEmpty());
        TimeDiffs.Add(bTimecodeMatch ? 0.0 : 1.0);

        UE_LOG(LogTemp, Display, TEXT("[TimecodeSyncTest] Sample %d: Master = %s, Slave = %s, Match = %s"),
            i + 1,
            *CurrentMasterTimecode,
            *CurrentSlaveTimecode,
            bTimecodeMatch ? TEXT("YES") : TEXT("NO"));

        // ���� Ÿ���ڵ� ���� (�����δ� �� ������ Ÿ���ڵ� ���� ���� �ʿ�)
        TestTimecode = FString::Printf(TEXT("01:30:%02d:12"), FMath::Min(45 + i, 59));
        MasterManager->SendTimecodeMessage(TestTimecode, ETimecodeMessageType::TimecodeSync);
    }

    // ��� ��
    int32 MatchCount = 0;
    for (double Diff : TimeDiffs)
    {
        if (Diff < 0.1) {
            MatchCount++;
        }
    }

    // 80% �̻� ��ġ�ϸ� �������� ����
    bSuccess = (MatchCount >= NumSamples * 0.8);

    ResultMessage = FString::Printf(TEXT("Timecode matches: %d/%d (%.1f%%)"),
        MatchCount, NumSamples, (float)MatchCount / NumSamples * 100.0f);

    // ����
    MasterManager->Shutdown();
    SlaveManager->Shutdown();

    LogTestResult(TEXT("Master/Slave Sync"), bSuccess, ResultMessage);
    return bSuccess;
}

bool UTimecodeSyncLogicTest::TestMultipleFrameRates()
{
    bool bSuccess = false;
    FString ResultMessage;

    // �׽�Ʈ Ÿ���ڵ� �ʱ�ȭ
    TestReceivedTimecode = TEXT("");

    // ������ ����Ʈ �׽�Ʈ�� ���� Ÿ���ڵ� ���ڿ� ���
    TArray<FString> TestTimecodes = {
        TEXT("01:30:45:12"), // 30fps ����
        TEXT("01:30:45:23"), // 24fps ����
        TEXT("01:30:45:24"), // 25fps ����
        TEXT("01:30:45:29"), // 30fps ��� ������ ����
        TEXT("01:30:45:59")  // 60fps ����
    };

    TArray<FString> ResultMessages;

    // ���� ��Ʈ���� ���� �׽�Ʈ ������ �����ϱ� ���� �⺻ ��Ʈ
    int32 BasePort = 12350;

    for (int32 i = 0; i < TestTimecodes.Num(); ++i)
    {
        const FString& TestTimecode = TestTimecodes[i];

        UE_LOG(LogTemp, Display, TEXT("[TimecodeSyncTest] Testing timecode: %s"), *TestTimecode);

        // �޽��� ���� ���� �ʱ�ȭ
        TestReceivedTimecode = TEXT("");

        // ������ �Ŵ��� �ν��Ͻ� ����
        UTimecodeNetworkManager* MasterManager = NewObject<UTimecodeNetworkManager>();

        // �����̺� �Ŵ��� �ν��Ͻ� ����
        UTimecodeNetworkManager* SlaveManager = NewObject<UTimecodeNetworkManager>();

        if (!MasterManager || !SlaveManager)
        {
            ResultMessages.Add(FString::Printf(TEXT("Timecode %s: Failed to create managers"), *TestTimecode));
            continue;
        }

        // ��Ʈ ��� (��ġ�� �ʵ���)
        int32 MasterPort = BasePort + (i * 2);
        int32 SlavePort = BasePort + (i * 2) + 1;

        // ������ ����
        bool bMasterInitialized = MasterManager->Initialize(true, MasterPort);
        if (!bMasterInitialized)
        {
            ResultMessages.Add(FString::Printf(TEXT("Timecode %s: Failed to initialize master"), *TestTimecode));
            continue;
        }

        // �����̺� ����
        bool bSlaveInitialized = SlaveManager->Initialize(false, SlavePort);
        if (!bSlaveInitialized)
        {
            MasterManager->Shutdown();
            ResultMessages.Add(FString::Printf(TEXT("Timecode %s: Failed to initialize slave"), *TestTimecode));
            continue;
        }

        // �����̺갡 ������ IP�� Ÿ�� ����
        SlaveManager->SetTargetIP(TEXT("127.0.0.1"));
        MasterManager->SetTargetIP(TEXT("127.0.0.1"));

        // �޽��� ���� �̺�Ʈ ���
        SlaveManager->OnMessageReceived.AddDynamic(this, &UTimecodeSyncLogicTest::OnTestTimecodeReceived);

        // Ÿ���ڵ� ����
        MasterManager->SendTimecodeMessage(TestTimecode, ETimecodeMessageType::TimecodeSync);

        // ����� �ð� ���
        FPlatformProcess::Sleep(0.5f);

        // ��� Ȯ��
        bool bTimecodeMatch = (TestReceivedTimecode == TestTimecode);
        ResultMessages.Add(FString::Printf(TEXT("Timecode %s: %s (Received: %s)"),
            *TestTimecode,
            bTimecodeMatch ? TEXT("PASSED") : TEXT("FAILED"),
            TestReceivedTimecode.IsEmpty() ? TEXT("None") : *TestReceivedTimecode));

        // ��������Ʈ ����
        SlaveManager->OnMessageReceived.RemoveDynamic(this, &UTimecodeSyncLogicTest::OnTestTimecodeReceived);

        // ����
        MasterManager->Shutdown();
        SlaveManager->Shutdown();
    }

    // ��� Ÿ���ڵ� �׽�Ʈ ��� Ȯ��
    bool bAllTimecodeTestsOK = true;
    FString CombinedResults;

    for (const FString& Result : ResultMessages)
    {
        CombinedResults += Result + TEXT("\n");
        if (Result.Contains(TEXT("FAILED")))
        {
            bAllTimecodeTestsOK = false;
        }
    }

    bSuccess = bAllTimecodeTestsOK;
    ResultMessage = CombinedResults;

    LogTestResult(TEXT("Multiple Timecodes"), bSuccess, ResultMessage);
    return bSuccess;
}

bool UTimecodeSyncLogicTest::TestSystemTimeSync()
{
    bool bSuccess = false;
    FString ResultMessage;

    // �ý��� �ð� �׽�Ʈ ���� �ʱ�ȭ
    SystemTimeReceivedTimecode = TEXT("");

    // �ý��� �ð� ��� Ÿ���ڵ� ���� �� ���� �׽�Ʈ
    UTimecodeNetworkManager* SenderManager = NewObject<UTimecodeNetworkManager>();
    UTimecodeNetworkManager* ReceiverManager = NewObject<UTimecodeNetworkManager>();

    if (!SenderManager || !ReceiverManager)
    {
        LogTestResult(TEXT("System Time Sync"), bSuccess, TEXT("Failed to create managers"));
        return bSuccess;
    }

    // �ʱ�ȭ
    bool bSenderInitialized = SenderManager->Initialize(true, 12360);
    bool bReceiverInitialized = ReceiverManager->Initialize(false, 12361);

    if (!bSenderInitialized || !bReceiverInitialized)
    {
        if (SenderManager) SenderManager->Shutdown();
        if (ReceiverManager) ReceiverManager->Shutdown();
        LogTestResult(TEXT("System Time Sync"), bSuccess, TEXT("Failed to initialize managers"));
        return bSuccess;
    }

    // Ÿ�� IP ����
    SenderManager->SetTargetIP(TEXT("127.0.0.1"));
    ReceiverManager->SetTargetIP(TEXT("127.0.0.1"));

    // ���� �ý��� �ð� ��� Ÿ���ڵ� ����
    FDateTime Now = FDateTime::Now();
    FString SystemTimecode = FString::Printf(TEXT("%02d:%02d:%02d:00"),
        Now.GetHour(), Now.GetMinute(), Now.GetSecond());

    // Ÿ���ڵ� �޽��� ���� �̺�Ʈ ���
    ReceiverManager->OnMessageReceived.AddDynamic(this, &UTimecodeSyncLogicTest::OnSystemTimeMessageReceived);

    // �ý��� �ð� ��� Ÿ���ڵ� ����
    SenderManager->SendTimecodeMessage(SystemTimecode, ETimecodeMessageType::TimecodeSync);

    // ����� �ð� ���
    FPlatformProcess::Sleep(0.5f);

    // ��� Ȯ��
    bool bTimecodeReceived = !SystemTimeReceivedTimecode.IsEmpty();
    bSuccess = bTimecodeReceived;

    ResultMessage = FString::Printf(TEXT("System time: %s, Received time: %s"),
        *SystemTimecode, SystemTimeReceivedTimecode.IsEmpty() ? TEXT("None") : *SystemTimeReceivedTimecode);

    // ��������Ʈ ����
    ReceiverManager->OnMessageReceived.RemoveDynamic(this, &UTimecodeSyncLogicTest::OnSystemTimeMessageReceived);

    // ����
    SenderManager->Shutdown();
    ReceiverManager->Shutdown();

    LogTestResult(TEXT("System Time Sync"), bSuccess, ResultMessage);
    return bSuccess;
}

void UTimecodeSyncLogicTest::OnMasterMessageReceived(const FTimecodeNetworkMessage& Message)
{
    if (Message.MessageType == ETimecodeMessageType::TimecodeSync) {
        CurrentMasterTimecode = Message.Timecode;
    }
}

void UTimecodeSyncLogicTest::OnSlaveMessageReceived(const FTimecodeNetworkMessage& Message)
{
    if (Message.MessageType == ETimecodeMessageType::TimecodeSync) {
        CurrentSlaveTimecode = Message.Timecode;
    }
}

void UTimecodeSyncLogicTest::OnTestTimecodeReceived(const FTimecodeNetworkMessage& Message)
{
    if (Message.MessageType == ETimecodeMessageType::TimecodeSync) {
        TestReceivedTimecode = Message.Timecode;
    }
}

void UTimecodeSyncLogicTest::OnSystemTimeMessageReceived(const FTimecodeNetworkMessage& Message)
{
    if (Message.MessageType == ETimecodeMessageType::TimecodeSync) {
        SystemTimeReceivedTimecode = Message.Timecode;
    }
}

void UTimecodeSyncLogicTest::LogTestResult(const FString& TestName, bool bSuccess, const FString& Message)
{
    FString ResultStr = bSuccess ? TEXT("PASSED") : TEXT("FAILED");
    UE_LOG(LogTemp, Display, TEXT("=============================="));
    UE_LOG(LogTemp, Display, TEXT("[TimecodeSyncTest] %s: %s"), *TestName, *ResultStr);
    UE_LOG(LogTemp, Display, TEXT("%s"), *Message);
    UE_LOG(LogTemp, Display, TEXT("=============================="));

    // ȭ�鿡�� �޽��� ǥ��
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, bSuccess ? FColor::Green : FColor::Red,
            FString::Printf(TEXT("[TimecodeSyncTest] %s: %s"), *TestName, *ResultStr));
    }
}