// TimecodeSyncTestActor.cpp
#include "TimecodeSyncTestActor.h"
#include "Tests/TimecodeSyncNetworkTest.h"
#include "Tests/TimecodeSyncLogicTest.h"
#include "Kismet/GameplayStatics.h"
#include "TimecodeComponent.h"

ATimecodeSyncTestActor::ATimecodeSyncTestActor()
{
    PrimaryActorTick.bCanEverTick = false;

    // Set default values
    TestType = 0;
    UDPPort = 12345;
    SyncDuration = 5.0f;
}

void ATimecodeSyncTestActor::BeginPlay()
{
    Super::BeginPlay();

    // Run test automatically
    if (TestType > 0)
    {
        RunSelectedTest();
    }

    if (TestType > 0)
    {
        RunSelectedTest();
    }

    // 타임코드 모드 테스트 추가
    UE_LOG(LogTemp, Display, TEXT("========== 타임코드 모드 테스트 시작 =========="));
    UTimecodeComponent* TC = FindComponentByClass<UTimecodeComponent>();
    if (TC)
    {
        // 기존 상태 로깅
        TC->LogDebugInfo();

        // 각 모드 테스트
        UE_LOG(LogTemp, Display, TEXT("========== PLL_Only 모드 테스트 =========="));
        TC->SetTimecodeMode(ETimecodeMode::PLL_Only);
        TC->LogDebugInfo();

        UE_LOG(LogTemp, Display, TEXT("========== SMPTE_Only 모드 테스트 =========="));
        TC->SetTimecodeMode(ETimecodeMode::SMPTE_Only);
        TC->LogDebugInfo();

        UE_LOG(LogTemp, Display, TEXT("========== Integrated 모드 테스트 =========="));
        TC->SetTimecodeMode(ETimecodeMode::Integrated);
        TC->LogDebugInfo();

        UE_LOG(LogTemp, Display, TEXT("========== Raw 모드 테스트 =========="));
        TC->SetTimecodeMode(ETimecodeMode::Raw);
        TC->LogDebugInfo();

        // 디스플레이에 결과 표시
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, TEXT("타임코드 모드 테스트 완료 - 로그 확인"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("타임코드 컴포넌트를 찾을 수 없습니다."));
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("타임코드 컴포넌트를 찾을 수 없습니다."));
    }

    // 타임코드 모드 테스트 추가
    UE_LOG(LogTemp, Display, TEXT("========== 타임코드 모드 테스트 시작 =========="));
    UTimecodeComponent* TC = FindComponentByClass<UTimecodeComponent>();
    if (TC)
    {
        // 기존 상태 로깅
        TC->LogDebugInfo();

        // 각 모드 테스트
        UE_LOG(LogTemp, Display, TEXT("========== PLL_Only 모드 테스트 =========="));
        TC->SetTimecodeMode(ETimecodeMode::PLL_Only);
        TC->LogDebugInfo();

        UE_LOG(LogTemp, Display, TEXT("========== SMPTE_Only 모드 테스트 =========="));
        TC->SetTimecodeMode(ETimecodeMode::SMPTE_Only);
        TC->LogDebugInfo();

        UE_LOG(LogTemp, Display, TEXT("========== Integrated 모드 테스트 =========="));
        TC->SetTimecodeMode(ETimecodeMode::Integrated);
        TC->LogDebugInfo();

        UE_LOG(LogTemp, Display, TEXT("========== Raw 모드 테스트 =========="));
        TC->SetTimecodeMode(ETimecodeMode::Raw);
        TC->LogDebugInfo();

        // 디스플레이에 결과 표시
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, TEXT("타임코드 모드 테스트 완료 - 로그 확인"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("타임코드 컴포넌트를 찾을 수 없습니다."));
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("타임코드 컴포넌트를 찾을 수 없습니다."));
    }
}

void ATimecodeSyncTestActor::RunSelectedTest()
{
    switch (TestType)
    {
    case 1: // UDP connection test
    {
        UTimecodeSyncNetworkTest* TestInstance = NewObject<UTimecodeSyncNetworkTest>();
        if (TestInstance)
        {
            bool Result = TestInstance->TestUDPConnection(UDPPort);

            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.0f,
                    Result ? FColor::Green : FColor::Red,
                    FString::Printf(TEXT("UDP Connection Test: %s"),
                        Result ? TEXT("PASSED") : TEXT("FAILED")));
            }
        }
        break;
    }
    case 2: // Master/Slave synchronization test
    {
        UTimecodeSyncLogicTest* TestInstance = NewObject<UTimecodeSyncLogicTest>();
        if (TestInstance)
        {
            bool Result = TestInstance->TestMasterSlaveSync(SyncDuration);

            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.0f,
                    Result ? FColor::Green : FColor::Red,
                    FString::Printf(TEXT("Master/Slave Sync Test: %s"),
                        Result ? TEXT("PASSED") : TEXT("FAILED")));
            }
        }
        break;
    }
    case 3: // Multiple timecode test
    {
        UTimecodeSyncLogicTest* TestInstance = NewObject<UTimecodeSyncLogicTest>();
        if (TestInstance)
        {
            bool Result = TestInstance->TestMultipleFrameRates();

            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.0f,
                    Result ? FColor::Green : FColor::Red,
                    FString::Printf(TEXT("Multiple Timecodes Test: %s"),
                        Result ? TEXT("PASSED") : TEXT("FAILED")));
            }
        }
        break;
    }
    case 4: // System time synchronization test
    {
        UTimecodeSyncLogicTest* TestInstance = NewObject<UTimecodeSyncLogicTest>();
        if (TestInstance)
        {
            bool Result = TestInstance->TestSystemTimeSync();

            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.0f,
                    Result ? FColor::Green : FColor::Red,
                    FString::Printf(TEXT("System Time Sync Test: %s"),
                        Result ? TEXT("PASSED") : TEXT("FAILED")));
            }
        }
        break;
    }
    case 5: // 프레임 레이트 변환 테스트
    {
        UTimecodeSyncLogicTest* TestInstance = NewObject<UTimecodeSyncLogicTest>();
        if (TestInstance)
        {
            bool Result = TestInstance->TestFrameRateConversion();

            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.0f,
                    Result ? FColor::Green : FColor::Red,
                    FString::Printf(TEXT("Frame Rate Conversion Test: %s"),
                        Result ? TEXT("PASSED") : TEXT("FAILED")));
            }
        }
        break;
    }
    case 6: // PLL 동기화 테스트
    {
        UTimecodeSyncLogicTest* TestInstance = NewObject<UTimecodeSyncLogicTest>();
        if (TestInstance)
        {
            bool Result = TestInstance->TestPLLSynchronization(SyncDuration);

            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.0f,
                    Result ? FColor::Green : FColor::Red,
                    FString::Printf(TEXT("PLL Synchronization Test: %s"),
                        Result ? TEXT("PASSED") : TEXT("FAILED")));
            }
        }
        break;
    }

    default:
        UE_LOG(LogTemp, Warning, TEXT("Invalid test type selected: %d"), TestType);
        break;
    }
}

void ATimecodeSyncTestActor::RunAllTests()
{
    // UDP connection test
    UTimecodeSyncNetworkTest* NetworkTest = NewObject<UTimecodeSyncNetworkTest>();
    if (NetworkTest)
    {
        bool UDPResult = NetworkTest->TestUDPConnection(UDPPort);

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.0f,
                UDPResult ? FColor::Green : FColor::Red,
                FString::Printf(TEXT("UDP Connection Test: %s"),
                    UDPResult ? TEXT("PASSED") : TEXT("FAILED")));
        }
    }

    // Wait for a moment
    FPlatformProcess::Sleep(1.0f);

    // Timecode synchronization logic test
    UTimecodeSyncLogicTest* LogicTest = NewObject<UTimecodeSyncLogicTest>();
    if (LogicTest)
    {
        // Master/Slave synchronization test
        bool SyncResult = LogicTest->TestMasterSlaveSync(SyncDuration);

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.0f,
                SyncResult ? FColor::Green : FColor::Red,
                FString::Printf(TEXT("Master/Slave Sync Test: %s"),
                    SyncResult ? TEXT("PASSED") : TEXT("FAILED")));
        }

        // Wait for a moment
        FPlatformProcess::Sleep(1.0f);

        // Multiple timecode test
        bool TimecodeResult = LogicTest->TestMultipleFrameRates();

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.0f,
                TimecodeResult ? FColor::Green : FColor::Red,
                FString::Printf(TEXT("Multiple Timecodes Test: %s"),
                    TimecodeResult ? TEXT("PASSED") : TEXT("FAILED")));
        }

        // Wait for a moment
        FPlatformProcess::Sleep(1.0f);

        // System time synchronization test
        bool SystemTimeResult = LogicTest->TestSystemTimeSync();

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.0f,
                SystemTimeResult ? FColor::Green : FColor::Red,
                FString::Printf(TEXT("System Time Sync Test: %s"),
                    SystemTimeResult ? TEXT("PASSED") : TEXT("FAILED")));
        }
    }
}

void ATimecodeSyncTestActor::RunIntegratedTest()
{
    // 테스트 결과 저장 배열
    TArray<FString> TestResults;
    int32 PassedTests = 0;
    int32 TotalTests = 0;

    UE_LOG(LogTemp, Warning, TEXT("=== Starting Integrated TimecodeSync Tests ==="));

    // 1. 타임코드 유틸리티 테스트 (AutomationTest로 이미 구현되어 있음)
    UE_LOG(LogTemp, Warning, TEXT("1. TimecodeUtils tests can be run from the Session Frontend -> Automation tab"));
    TestResults.Add(TEXT("TimecodeUtils: See Automation tab"));

    // 2. UDP 연결 테스트
    UTimecodeSyncNetworkTest* NetworkTest = NewObject<UTimecodeSyncNetworkTest>();
    if (NetworkTest)
    {
        TotalTests++;
        bool UDPResult = NetworkTest->TestUDPConnection(UDPPort);
        if (UDPResult) PassedTests++;

        TestResults.Add(FString::Printf(TEXT("UDP Connection: %s"),
            UDPResult ? TEXT("PASSED") : TEXT("FAILED")));

        // 메시지 직렬화 테스트
        TotalTests++;
        bool SerializationResult = NetworkTest->TestMessageSerialization();
        if (SerializationResult) PassedTests++;

        TestResults.Add(FString::Printf(TEXT("Message Serialization: %s"),
            SerializationResult ? TEXT("PASSED") : TEXT("FAILED")));

        // 패킷 손실 테스트
        TotalTests++;
        bool PacketLossResult = NetworkTest->TestPacketLoss(20.0f);
        if (PacketLossResult) PassedTests++;

        TestResults.Add(FString::Printf(TEXT("Packet Loss Handling: %s"),
            PacketLossResult ? TEXT("PASSED") : TEXT("FAILED")));
    }

    // 3. 마스터/슬레이브 동기화 테스트
    UTimecodeSyncLogicTest* LogicTest = NewObject<UTimecodeSyncLogicTest>();
    if (LogicTest)
    {
        // 간단한 동기화 테스트
        TotalTests++;
        bool SyncResult = LogicTest->TestMasterSlaveSync(3.0f);
        if (SyncResult) PassedTests++;

        TestResults.Add(FString::Printf(TEXT("Master/Slave Sync: %s"),
            SyncResult ? TEXT("PASSED") : TEXT("FAILED")));

        // 다양한 프레임 레이트 테스트
        TotalTests++;
        bool FrameRateResult = LogicTest->TestMultipleFrameRates();
        if (FrameRateResult) PassedTests++;

        TestResults.Add(FString::Printf(TEXT("Multiple Frame Rates: %s"),
            FrameRateResult ? TEXT("PASSED") : TEXT("FAILED")));

        // 시스템 시간 동기화 테스트
        TotalTests++;
        bool SystemTimeResult = LogicTest->TestSystemTimeSync();
        if (SystemTimeResult) PassedTests++;

        TestResults.Add(FString::Printf(TEXT("System Time Sync: %s"),
            SystemTimeResult ? TEXT("PASSED") : TEXT("FAILED")));
    }

    // 자동 역할 감지 테스트
    TotalTests++;
    bool AutoRoleResult = LogicTest->TestAutoRoleDetection();
    if (AutoRoleResult) PassedTests++;

    TestResults.Add(FString::Printf(TEXT("Auto Role Detection: %s"),
        AutoRoleResult ? TEXT("PASSED") : TEXT("FAILED")));

    // 4. 결과 표시
    UE_LOG(LogTemp, Warning, TEXT("=== TimecodeSync Test Results ==="));
    UE_LOG(LogTemp, Warning, TEXT("Passed Tests: %d/%d (%.1f%%)"),
        PassedTests, TotalTests, TotalTests > 0 ? (float)PassedTests / TotalTests * 100.0f : 0.0f);

    for (const FString& Result : TestResults)
    {
        UE_LOG(LogTemp, Warning, TEXT("- %s"), *Result);
    }

    // 화면에 결과 표시
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow,
            FString::Printf(TEXT("=== Test Results: %d/%d Passed ==="), PassedTests, TotalTests));

        int32 MsgIdx = 0;
        for (const FString& Result : TestResults)
        {
            FColor MsgColor = Result.Contains(TEXT("PASSED")) ? FColor::Green :
                (Result.Contains(TEXT("FAILED")) ? FColor::Red : FColor::White);

            GEngine->AddOnScreenDebugMessage(-1, 15.0f, MsgColor, Result);
            MsgIdx++;
        }
    }

    // 프레임 레이트 변환 테스트 (새로 추가)
    TotalTests++;
    bool FrameRateConversionResult = LogicTest->TestFrameRateConversion();
    if (FrameRateConversionResult) PassedTests++;

    TestResults.Add(FString::Printf(TEXT("Frame Rate Conversion: %s"),
        FrameRateConversionResult ? TEXT("PASSED") : TEXT("FAILED")));

    // PLL 동기화 테스트 추가
    TotalTests++;
    bool PLLSyncResult = LogicTest->TestPLLSynchronization(2.0f);
    if (PLLSyncResult) PassedTests++;

    TestResults.Add(FString::Printf(TEXT("PLL Synchronization: %s"),
        PLLSyncResult ? TEXT("PASSED") : TEXT("FAILED")));
}

