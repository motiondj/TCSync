// TimecodeSyncTestActor.cpp
#include "TimecodeSyncTestActor.h"
#include "Tests/TimecodeSyncNetworkTest.h"
#include "Tests/TimecodeSyncLogicTest.h"
#include "Kismet/GameplayStatics.h"

ATimecodeSyncTestActor::ATimecodeSyncTestActor()
{
    PrimaryActorTick.bCanEverTick = false;

    // �⺻�� ����
    TestType = 0;
    UDPPort = 12345;
    SyncDuration = 5.0f;
}

void ATimecodeSyncTestActor::BeginPlay()
{
    Super::BeginPlay();

    // �ڵ����� �׽�Ʈ ����
    if (TestType > 0)
    {
        RunSelectedTest();
    }
}

void ATimecodeSyncTestActor::RunSelectedTest()
{
    switch (TestType)
    {
    case 1: // UDP ���� �׽�Ʈ
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
    case 2: // ������/�����̺� ����ȭ �׽�Ʈ
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
    case 3: // �پ��� Ÿ���ڵ� �׽�Ʈ
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
    case 4: // �ý��� �ð� ����ȭ �׽�Ʈ
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
    // UDP ���� �׽�Ʈ
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

    // ��� ���
    FPlatformProcess::Sleep(1.0f);

    // Ÿ���ڵ� ����ȭ ���� �׽�Ʈ
    UTimecodeSyncLogicTest* LogicTest = NewObject<UTimecodeSyncLogicTest>();
    if (LogicTest)
    {
        // ������/�����̺� ����ȭ �׽�Ʈ
        bool SyncResult = LogicTest->TestMasterSlaveSync(SyncDuration);

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.0f,
                SyncResult ? FColor::Green : FColor::Red,
                FString::Printf(TEXT("Master/Slave Sync Test: %s"),
                    SyncResult ? TEXT("PASSED") : TEXT("FAILED")));
        }

        // ��� ���
        FPlatformProcess::Sleep(1.0f);

        // �پ��� Ÿ���ڵ� �׽�Ʈ
        bool TimecodeResult = LogicTest->TestMultipleFrameRates();

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.0f,
                TimecodeResult ? FColor::Green : FColor::Red,
                FString::Printf(TEXT("Multiple Timecodes Test: %s"),
                    TimecodeResult ? TEXT("PASSED") : TEXT("FAILED")));
        }

        // ��� ���
        FPlatformProcess::Sleep(1.0f);

        // �ý��� �ð� ����ȭ �׽�Ʈ
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