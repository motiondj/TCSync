// TimecodeSyncLogicTest.cpp (수정된 버전)
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

    // 초기 타임코드 값 리셋
    CurrentMasterTimecode = TEXT("");
    CurrentSlaveTimecode = TEXT("");

    // 마스터 매니저 인스턴스 생성
    UTimecodeNetworkManager* MasterManager = NewObject<UTimecodeNetworkManager>();
    if (!MasterManager)
    {
        LogTestResult(TEXT("Master/Slave Sync"), bSuccess, TEXT("Failed to create master manager"));
        return bSuccess;
    }

    // 슬레이브 매니저 인스턴스 생성
    UTimecodeNetworkManager* SlaveManager = NewObject<UTimecodeNetworkManager>();
    if (!SlaveManager)
    {
        LogTestResult(TEXT("Master/Slave Sync"), bSuccess, TEXT("Failed to create slave manager"));
        return bSuccess;
    }

    // 마스터 설정
    bool bMasterInitialized = MasterManager->Initialize(true, 12345);
    if (!bMasterInitialized)
    {
        LogTestResult(TEXT("Master/Slave Sync"), bSuccess, TEXT("Failed to initialize master manager"));
        return bSuccess;
    }

    // 슬레이브 설정
    bool bSlaveInitialized = SlaveManager->Initialize(false, 12346);
    if (!bSlaveInitialized)
    {
        MasterManager->Shutdown();
        LogTestResult(TEXT("Master/Slave Sync"), bSuccess, TEXT("Failed to initialize slave manager"));
        return bSuccess;
    }

    // 슬레이브가 마스터 IP로 타겟 설정
    SlaveManager->SetTargetIP(TEXT("127.0.0.1"));

    // 지정된 시간 동안 대기하며 동기화 진행
    UE_LOG(LogTemp, Display, TEXT("[TimecodeSyncTest] Starting Master/Slave sync test for %.1f seconds..."), Duration);

    const int32 NumSamples = 10;
    TArray<FString> MasterTimecodes;
    TArray<FString> SlaveTimecodes;
    TArray<double> TimeDiffs;
    MasterTimecodes.Reserve(NumSamples);
    SlaveTimecodes.Reserve(NumSamples);
    TimeDiffs.Reserve(NumSamples);

    // 타임코드 메시지 수신 캡처를 위한 델리게이트 설정
    MasterManager->OnMessageReceived.AddDynamic(this, &UTimecodeSyncLogicTest::OnMasterMessageReceived);
    SlaveManager->OnMessageReceived.AddDynamic(this, &UTimecodeSyncLogicTest::OnSlaveMessageReceived);

    // 마스터가 타임코드 메시지 전송
    FString TestTimecode = TEXT("01:30:45:12");
    MasterManager->SendTimecodeMessage(TestTimecode, ETimecodeMessageType::TimecodeSync);

    for (int32 i = 0; i < NumSamples; ++i)
    {
        // 각 샘플 간 간격
        FPlatformProcess::Sleep(Duration / NumSamples);

        // 실제 테스트에서는 타임코드 값 비교
        MasterTimecodes.Add(CurrentMasterTimecode);
        SlaveTimecodes.Add(CurrentSlaveTimecode);

        // 간단한 테스트를 위해 문자열 비교로 확인
        bool bTimecodeMatch = (CurrentSlaveTimecode == TestTimecode && !CurrentSlaveTimecode.IsEmpty());
        TimeDiffs.Add(bTimecodeMatch ? 0.0 : 1.0);

        UE_LOG(LogTemp, Display, TEXT("[TimecodeSyncTest] Sample %d: Master = %s, Slave = %s, Match = %s"),
            i + 1,
            *CurrentMasterTimecode,
            *CurrentSlaveTimecode,
            bTimecodeMatch ? TEXT("YES") : TEXT("NO"));

        // 다음 타임코드 전송 (실제로는 더 복잡한 타임코드 생성 로직 필요)
        TestTimecode = FString::Printf(TEXT("01:30:%02d:12"), FMath::Min(45 + i, 59));
        MasterManager->SendTimecodeMessage(TestTimecode, ETimecodeMessageType::TimecodeSync);
    }

    // 결과 평가
    int32 MatchCount = 0;
    for (double Diff : TimeDiffs)
    {
        if (Diff < 0.1) {
            MatchCount++;
        }
    }

    // 80% 이상 일치하면 성공으로 간주
    bSuccess = (MatchCount >= NumSamples * 0.8);

    ResultMessage = FString::Printf(TEXT("Timecode matches: %d/%d (%.1f%%)"),
        MatchCount, NumSamples, (float)MatchCount / NumSamples * 100.0f);

    // 정리
    MasterManager->Shutdown();
    SlaveManager->Shutdown();

    LogTestResult(TEXT("Master/Slave Sync"), bSuccess, ResultMessage);
    return bSuccess;
}

bool UTimecodeSyncLogicTest::TestMultipleFrameRates()
{
    bool bSuccess = false;
    FString ResultMessage;

    // 테스트 타임코드 초기화
    TestReceivedTimecode = TEXT("");

    // 프레임 레이트 테스트를 위한 타임코드 문자열 목록
    TArray<FString> TestTimecodes = {
        TEXT("01:30:45:12"), // 30fps 기준
        TEXT("01:30:45:23"), // 24fps 기준
        TEXT("01:30:45:24"), // 25fps 기준
        TEXT("01:30:45:29"), // 30fps 드롭 프레임 기준
        TEXT("01:30:45:59")  // 60fps 기준
    };

    TArray<FString> ResultMessages;

    // 단일 포트에서 연속 테스트 문제를 방지하기 위한 기본 포트
    int32 BasePort = 12350;

    for (int32 i = 0; i < TestTimecodes.Num(); ++i)
    {
        const FString& TestTimecode = TestTimecodes[i];

        UE_LOG(LogTemp, Display, TEXT("[TimecodeSyncTest] Testing timecode: %s"), *TestTimecode);

        // 메시지 수신 변수 초기화
        TestReceivedTimecode = TEXT("");

        // 마스터 매니저 인스턴스 생성
        UTimecodeNetworkManager* MasterManager = NewObject<UTimecodeNetworkManager>();

        // 슬레이브 매니저 인스턴스 생성
        UTimecodeNetworkManager* SlaveManager = NewObject<UTimecodeNetworkManager>();

        if (!MasterManager || !SlaveManager)
        {
            ResultMessages.Add(FString::Printf(TEXT("Timecode %s: Failed to create managers"), *TestTimecode));
            continue;
        }

        // 포트 계산 (겹치지 않도록)
        int32 MasterPort = BasePort + (i * 2);
        int32 SlavePort = BasePort + (i * 2) + 1;

        // 마스터 설정
        bool bMasterInitialized = MasterManager->Initialize(true, MasterPort);
        if (!bMasterInitialized)
        {
            ResultMessages.Add(FString::Printf(TEXT("Timecode %s: Failed to initialize master"), *TestTimecode));
            continue;
        }

        // 슬레이브 설정
        bool bSlaveInitialized = SlaveManager->Initialize(false, SlavePort);
        if (!bSlaveInitialized)
        {
            MasterManager->Shutdown();
            ResultMessages.Add(FString::Printf(TEXT("Timecode %s: Failed to initialize slave"), *TestTimecode));
            continue;
        }

        // 슬레이브가 마스터 IP로 타겟 설정
        SlaveManager->SetTargetIP(TEXT("127.0.0.1"));
        MasterManager->SetTargetIP(TEXT("127.0.0.1"));

        // 메시지 수신 이벤트 등록
        SlaveManager->OnMessageReceived.AddDynamic(this, &UTimecodeSyncLogicTest::OnTestTimecodeReceived);

        // 타임코드 전송
        MasterManager->SendTimecodeMessage(TestTimecode, ETimecodeMessageType::TimecodeSync);

        // 충분한 시간 대기
        FPlatformProcess::Sleep(0.5f);

        // 결과 확인
        bool bTimecodeMatch = (TestReceivedTimecode == TestTimecode);
        ResultMessages.Add(FString::Printf(TEXT("Timecode %s: %s (Received: %s)"),
            *TestTimecode,
            bTimecodeMatch ? TEXT("PASSED") : TEXT("FAILED"),
            TestReceivedTimecode.IsEmpty() ? TEXT("None") : *TestReceivedTimecode));

        // 델리게이트 해제
        SlaveManager->OnMessageReceived.RemoveDynamic(this, &UTimecodeSyncLogicTest::OnTestTimecodeReceived);

        // 정리
        MasterManager->Shutdown();
        SlaveManager->Shutdown();
    }

    // 모든 타임코드 테스트 결과 확인
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

    // 시스템 시간 테스트 변수 초기화
    SystemTimeReceivedTimecode = TEXT("");

    // 시스템 시간 기반 타임코드 생성 및 전송 테스트
    UTimecodeNetworkManager* SenderManager = NewObject<UTimecodeNetworkManager>();
    UTimecodeNetworkManager* ReceiverManager = NewObject<UTimecodeNetworkManager>();

    if (!SenderManager || !ReceiverManager)
    {
        LogTestResult(TEXT("System Time Sync"), bSuccess, TEXT("Failed to create managers"));
        return bSuccess;
    }

    // 초기화
    bool bSenderInitialized = SenderManager->Initialize(true, 12360);
    bool bReceiverInitialized = ReceiverManager->Initialize(false, 12361);

    if (!bSenderInitialized || !bReceiverInitialized)
    {
        if (SenderManager) SenderManager->Shutdown();
        if (ReceiverManager) ReceiverManager->Shutdown();
        LogTestResult(TEXT("System Time Sync"), bSuccess, TEXT("Failed to initialize managers"));
        return bSuccess;
    }

    // 타겟 IP 설정
    SenderManager->SetTargetIP(TEXT("127.0.0.1"));
    ReceiverManager->SetTargetIP(TEXT("127.0.0.1"));

    // 현재 시스템 시간 기반 타임코드 생성
    FDateTime Now = FDateTime::Now();
    FString SystemTimecode = FString::Printf(TEXT("%02d:%02d:%02d:00"),
        Now.GetHour(), Now.GetMinute(), Now.GetSecond());

    // 타임코드 메시지 수신 이벤트 등록
    ReceiverManager->OnMessageReceived.AddDynamic(this, &UTimecodeSyncLogicTest::OnSystemTimeMessageReceived);

    // 시스템 시간 기반 타임코드 전송
    SenderManager->SendTimecodeMessage(SystemTimecode, ETimecodeMessageType::TimecodeSync);

    // 충분한 시간 대기
    FPlatformProcess::Sleep(0.5f);

    // 결과 확인
    bool bTimecodeReceived = !SystemTimeReceivedTimecode.IsEmpty();
    bSuccess = bTimecodeReceived;

    ResultMessage = FString::Printf(TEXT("System time: %s, Received time: %s"),
        *SystemTimecode, SystemTimeReceivedTimecode.IsEmpty() ? TEXT("None") : *SystemTimeReceivedTimecode);

    // 델리게이트 해제
    ReceiverManager->OnMessageReceived.RemoveDynamic(this, &UTimecodeSyncLogicTest::OnSystemTimeMessageReceived);

    // 정리
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

    // 화면에도 메시지 표시
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, bSuccess ? FColor::Green : FColor::Red,
            FString::Printf(TEXT("[TimecodeSyncTest] %s: %s"), *TestName, *ResultStr));
    }
}