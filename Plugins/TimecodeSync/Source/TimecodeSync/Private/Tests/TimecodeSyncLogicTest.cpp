// TimecodeSyncLogicTest.cpp (Modified version)
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

    // Reset initial timecode values
    CurrentMasterTimecode = TEXT("");
    CurrentSlaveTimecode = TEXT("");

    // Create master manager instance
    UTimecodeNetworkManager* MasterManager = NewObject<UTimecodeNetworkManager>();
    if (!MasterManager)
    {
        LogTestResult(TEXT("Master/Slave Sync"), bSuccess, TEXT("Failed to create master manager"));
        return bSuccess;
    }

    // Create slave manager instance
    UTimecodeNetworkManager* SlaveManager = NewObject<UTimecodeNetworkManager>();
    if (!SlaveManager)
    {
        LogTestResult(TEXT("Master/Slave Sync"), bSuccess, TEXT("Failed to create slave manager"));
        return bSuccess;
    }

    // Setup master
    bool bMasterInitialized = MasterManager->Initialize(true, 12345);
    if (!bMasterInitialized)
    {
        LogTestResult(TEXT("Master/Slave Sync"), bSuccess, TEXT("Failed to initialize master manager"));
        return bSuccess;
    }

    // Setup slave
    bool bSlaveInitialized = SlaveManager->Initialize(false, 12346);
    if (!bSlaveInitialized)
    {
        MasterManager->Shutdown();
        LogTestResult(TEXT("Master/Slave Sync"), bSuccess, TEXT("Failed to initialize slave manager"));
        return bSuccess;
    }

    // Set target IP for slave to master IP
    SlaveManager->SetTargetIP(TEXT("127.0.0.1"));

    // Wait and synchronize for the specified duration
    UE_LOG(LogTemp, Display, TEXT("[TimecodeSyncTest] Starting Master/Slave sync test for %.1f seconds..."), Duration);

    const int32 NumSamples = 10;
    TArray<FString> MasterTimecodes;
    TArray<FString> SlaveTimecodes;
    TArray<double> TimeDiffs;
    MasterTimecodes.Reserve(NumSamples);
    SlaveTimecodes.Reserve(NumSamples);
    TimeDiffs.Reserve(NumSamples);

    // Setup delegates to capture timecode message reception
    MasterManager->OnMessageReceived.AddDynamic(this, &UTimecodeSyncLogicTest::OnMasterMessageReceived);
    SlaveManager->OnMessageReceived.AddDynamic(this, &UTimecodeSyncLogicTest::OnSlaveMessageReceived);

    // Master sends timecode message
    FString TestTimecode = TEXT("01:30:45:12");
    MasterManager->SendTimecodeMessage(TestTimecode, ETimecodeMessageType::TimecodeSync);

    for (int32 i = 0; i < NumSamples; ++i)
    {
        // Interval between each sample
        FPlatformProcess::Sleep(Duration / NumSamples);

        // Compare timecode values in actual test
        MasterTimecodes.Add(CurrentMasterTimecode);
        SlaveTimecodes.Add(CurrentSlaveTimecode);

        // Simple test using string comparison
        bool bTimecodeMatch = (CurrentSlaveTimecode == TestTimecode && !CurrentSlaveTimecode.IsEmpty());
        TimeDiffs.Add(bTimecodeMatch ? 0.0 : 1.0);

        UE_LOG(LogTemp, Display, TEXT("[TimecodeSyncTest] Sample %d: Master = %s, Slave = %s, Match = %s"),
            i + 1,
            *CurrentMasterTimecode,
            *CurrentSlaveTimecode,
            bTimecodeMatch ? TEXT("YES") : TEXT("NO"));

        // Send next timecode (actual implementation would need more complex timecode generation logic)
        TestTimecode = FString::Printf(TEXT("01:30:%02d:12"), FMath::Min(45 + i, 59));
        MasterManager->SendTimecodeMessage(TestTimecode, ETimecodeMessageType::TimecodeSync);
    }

    // Evaluate results
    int32 MatchCount = 0;
    for (double Diff : TimeDiffs)
    {
        if (Diff < 0.1) {
            MatchCount++;
        }
    }

    // Consider success if 80% or more matches
    bSuccess = (MatchCount >= NumSamples * 0.8);

    ResultMessage = FString::Printf(TEXT("Timecode matches: %d/%d (%.1f%%)"),
        MatchCount, NumSamples, (float)MatchCount / NumSamples * 100.0f);

    // Cleanup
    MasterManager->Shutdown();
    SlaveManager->Shutdown();

    LogTestResult(TEXT("Master/Slave Sync"), bSuccess, ResultMessage);
    return bSuccess;
}

bool UTimecodeSyncLogicTest::TestMultipleFrameRates()
{
    bool bSuccess = false;
    FString ResultMessage;

    // Initialize test timecode
    TestReceivedTimecode = TEXT("");

    // List of timecode strings for frame rate testing
    TArray<FString> TestTimecodes = {
        TEXT("01:30:45:12"), // Based on 30fps
        TEXT("01:30:45:23"), // Based on 24fps
        TEXT("01:30:45:24"), // Based on 25fps
        TEXT("01:30:45:29"), // Based on 30fps drop frame
        TEXT("01:30:45:59")  // Based on 60fps
    };

    TArray<FString> ResultMessages;

    // Base port to prevent port conflict in consecutive tests
    int32 BasePort = 12350;

    for (int32 i = 0; i < TestTimecodes.Num(); ++i)
    {
        const FString& TestTimecode = TestTimecodes[i];

        UE_LOG(LogTemp, Display, TEXT("[TimecodeSyncTest] Testing timecode: %s"), *TestTimecode);

        // Initialize message reception variable
        TestReceivedTimecode = TEXT("");

        // Create master manager instance
        UTimecodeNetworkManager* MasterManager = NewObject<UTimecodeNetworkManager>();

        // Create slave manager instance
        UTimecodeNetworkManager* SlaveManager = NewObject<UTimecodeNetworkManager>();

        if (!MasterManager || !SlaveManager)
        {
            ResultMessages.Add(FString::Printf(TEXT("Timecode %s: Failed to create managers"), *TestTimecode));
            continue;
        }

        // Calculate ports (to avoid overlap)
        int32 MasterPort = BasePort + (i * 2);
        int32 SlavePort = BasePort + (i * 2) + 1;

        // Setup master
        bool bMasterInitialized = MasterManager->Initialize(true, MasterPort);
        if (!bMasterInitialized)
        {
            ResultMessages.Add(FString::Printf(TEXT("Timecode %s: Failed to initialize master"), *TestTimecode));
            continue;
        }

        // Setup slave
        bool bSlaveInitialized = SlaveManager->Initialize(false, SlavePort);
        if (!bSlaveInitialized)
        {
            MasterManager->Shutdown();
            ResultMessages.Add(FString::Printf(TEXT("Timecode %s: Failed to initialize slave"), *TestTimecode));
            continue;
        }

        // Set target IP for slave to master IP
        SlaveManager->SetTargetIP(TEXT("127.0.0.1"));
        MasterManager->SetTargetIP(TEXT("127.0.0.1"));

        // Register message reception event
        SlaveManager->OnMessageReceived.AddDynamic(this, &UTimecodeSyncLogicTest::OnTestTimecodeReceived);

        // Send timecode
        MasterManager->SendTimecodeMessage(TestTimecode, ETimecodeMessageType::TimecodeSync);

        // Wait for sufficient time
        FPlatformProcess::Sleep(0.5f);

        // Check results
        bool bTimecodeMatch = (TestReceivedTimecode == TestTimecode);
        ResultMessages.Add(FString::Printf(TEXT("Timecode %s: %s (Received: %s)"),
            *TestTimecode,
            bTimecodeMatch ? TEXT("PASSED") : TEXT("FAILED"),
            TestReceivedTimecode.IsEmpty() ? TEXT("None") : *TestReceivedTimecode));

        // Unregister delegate
        SlaveManager->OnMessageReceived.RemoveDynamic(this, &UTimecodeSyncLogicTest::OnTestTimecodeReceived);

        // Cleanup
        MasterManager->Shutdown();
        SlaveManager->Shutdown();
    }

    // Check results of all timecode tests
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

    // Initialize system time test variable
    SystemTimeReceivedTimecode = TEXT("");

    // Test system time-based timecode generation and transmission
    UTimecodeNetworkManager* SenderManager = NewObject<UTimecodeNetworkManager>();
    UTimecodeNetworkManager* ReceiverManager = NewObject<UTimecodeNetworkManager>();

    if (!SenderManager || !ReceiverManager)
    {
        LogTestResult(TEXT("System Time Sync"), bSuccess, TEXT("Failed to create managers"));
        return bSuccess;
    }

    // Setup sender
    bool bSenderInitialized = SenderManager->Initialize(true, 12360);
    if (!bSenderInitialized)
    {
        LogTestResult(TEXT("System Time Sync"), bSuccess, TEXT("Failed to initialize sender"));
        return bSuccess;
    }

    // Setup receiver
    bool bReceiverInitialized = ReceiverManager->Initialize(false, 12361);
    if (!bReceiverInitialized)
    {
        SenderManager->Shutdown();
        LogTestResult(TEXT("System Time Sync"), bSuccess, TEXT("Failed to initialize receiver"));
        return bSuccess;
    }

    // Set target IP for receiver to sender IP
    ReceiverManager->SetTargetIP(TEXT("127.0.0.1"));

    // Register message reception event
    ReceiverManager->OnMessageReceived.AddDynamic(this, &UTimecodeSyncLogicTest::OnSystemTimeReceived);

    // Send system time-based timecode
    FDateTime CurrentTime = FDateTime::Now();
    FString SystemTimecode = FString::Printf(TEXT("%02d:%02d:%02d:00"),
        CurrentTime.GetHour(),
        CurrentTime.GetMinute(),
        CurrentTime.GetSecond());

    SenderManager->SendTimecodeMessage(SystemTimecode, ETimecodeMessageType::TimecodeSync);

    // Wait for message reception
    FPlatformProcess::Sleep(0.5f);

    // Check results
    bool bTimecodeMatch = (SystemTimeReceivedTimecode == SystemTimecode);
    ResultMessage = FString::Printf(TEXT("System time: %s, Received: %s"),
        *SystemTimecode,
        SystemTimeReceivedTimecode.IsEmpty() ? TEXT("None") : *SystemTimeReceivedTimecode);

    // Unregister delegate
    ReceiverManager->OnMessageReceived.RemoveDynamic(this, &UTimecodeSyncLogicTest::OnSystemTimeReceived);

    // Cleanup
    SenderManager->Shutdown();
    ReceiverManager->Shutdown();

    bSuccess = bTimecodeMatch;
    LogTestResult(TEXT("System Time Sync"), bSuccess, ResultMessage);
    return bSuccess;
}

void UTimecodeSyncLogicTest::OnMasterMessageReceived(const FString& Timecode, ETimecodeMessageType MessageType)
{
    CurrentMasterTimecode = Timecode;
}

void UTimecodeSyncLogicTest::OnSlaveMessageReceived(const FString& Timecode, ETimecodeMessageType MessageType)
{
    CurrentSlaveTimecode = Timecode;
}

void UTimecodeSyncLogicTest::OnTestTimecodeReceived(const FString& Timecode, ETimecodeMessageType MessageType)
{
    TestReceivedTimecode = Timecode;
}

void UTimecodeSyncLogicTest::OnSystemTimeReceived(const FString& Timecode, ETimecodeMessageType MessageType)
{
    SystemTimeReceivedTimecode = Timecode;
}

void UTimecodeSyncLogicTest::LogTestResult(const FString& TestName, bool bSuccess, const FString& Message)
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