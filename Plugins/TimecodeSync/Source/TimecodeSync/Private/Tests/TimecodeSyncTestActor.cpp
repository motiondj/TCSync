// TimecodeSyncTestActor.cpp
#include "TimecodeSyncTestActor.h"
#include "Tests/TimecodeSyncNetworkTest.h"
#include "Tests/TimecodeSyncLogicTest.h"
#include "Kismet/GameplayStatics.h"

ATimecodeSyncTestActor::ATimecodeSyncTestActor()
{
    PrimaryActorTick.bCanEverTick = false;

    // 기본값 설정
    TestType = 0;
    UDPPort = 12345;
    SyncDuration = 5.0f;
}

void ATimecodeSyncTestActor::BeginPlay()
{
    Super::BeginPlay();

    // 자동으로 테스트 실행
    if (TestType > 0)
    {
        RunSelectedTest();
    }
}

void ATimecodeSyncTestActor::RunSelectedTest()
{
    switch (TestType)
    {
    case 1: // UDP 연결 테스트
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
    case 2: // 마스터/슬레이브 동기화 테스트
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
    case 3: // 다양한 타임코드 테스트
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
    case 4: // 시스템 시간 동기화 테스트
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
    default:
        UE_LOG(LogTemp, Warning, TEXT("Invalid test type selected: %d"), TestType);
        break;
    }
}

void ATimecodeSyncTestActor::RunAllTests()
{
    // UDP 연결 테스트
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

    // 잠시 대기
    FPlatformProcess::Sleep(1.0f);

    // 타임코드 동기화 로직 테스트
    UTimecodeSyncLogicTest* LogicTest = NewObject<UTimecodeSyncLogicTest>();
    if (LogicTest)
    {
        // 마스터/슬레이브 동기화 테스트
        bool SyncResult = LogicTest->TestMasterSlaveSync(SyncDuration);

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.0f,
                SyncResult ? FColor::Green : FColor::Red,
                FString::Printf(TEXT("Master/Slave Sync Test: %s"),
                    SyncResult ? TEXT("PASSED") : TEXT("FAILED")));
        }

        // 잠시 대기
        FPlatformProcess::Sleep(1.0f);

        // 다양한 타임코드 테스트
        bool TimecodeResult = LogicTest->TestMultipleFrameRates();

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.0f,
                TimecodeResult ? FColor::Green : FColor::Red,
                FString::Printf(TEXT("Multiple Timecodes Test: %s"),
                    TimecodeResult ? TEXT("PASSED") : TEXT("FAILED")));
        }

        // 잠시 대기
        FPlatformProcess::Sleep(1.0f);

        // 시스템 시간 동기화 테스트
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