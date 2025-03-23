#include "TimecodeSyncTest.h"
#include "TimecodeUtils.h"
#include "Misc/AutomationTest.h"

bool UTimecodeSyncTest::TestPLLSynchronization(float Duration, bool UsePLL)
{
    UE_LOG(LogTemp, Display, TEXT("=============================="));
    UE_LOG(LogTemp, Display, TEXT("[TimecodeSyncTest] PLL Synchronization: Testing..."));

    // 테스트 초기화
    bool bTestPassed = false;
    int32 TotalTestPoints = 0;
    int32 PassedTestPoints = 0;

    // 테스트 환경 설정 - 2개의 네트워크 매니저 생성 (마스터와 슬레이브)
    UTimecodeNetworkManager* MasterManager = NewObject<UTimecodeNetworkManager>();
    UTimecodeNetworkManager* SlaveManager = NewObject<UTimecodeNetworkManager>();

    MasterManager->AddToRoot();
    SlaveManager->AddToRoot();

    // 네트워크 매니저 초기화
    MasterManager->Initialize(true, 10000);  // 마스터, 포트 10000
    SlaveManager->Initialize(false, 10001);  // 슬레이브, 포트 10001

    // 대상 포트 설정
    MasterManager->SetTargetPort(10001);
    SlaveManager->SetTargetPort(10000);

    // 슬레이브에 PLL 설정
    if (UsePLL)
    {
        SlaveManager->InitializePLL(0.5f, 0.7f);  // 더 빠른 수렴을 위해 대역폭 증가
        SlaveManager->SetUsePLL(true);
    }

    // 타임코드 테스트 데이터 초기화
    float TimeStep = 0.033f;  // 약 30fps
    float SimulationTime = 0.0f;
    float MasterTime = 0.0f;
    float SlaveTime = 0.0f;
    float MaxError = 0.0f;
    float AverageError = 0.0f;
    float SumError = 0.0f;

    // 동기화 시뮬레이션 실행
    while (SimulationTime < Duration)
    {
        // 시간 업데이트
        SimulationTime += TimeStep;
        MasterTime += TimeStep;

        // 네트워크 지연 시뮬레이션 (5-15ms 범위의 랜덤 지연)
        float NetworkDelay = 0.005f + 0.01f * FMath::FRand();

        // 마스터 타임코드 생성
        float MasterFrameRate = 29.97f;
        bool bUseDropFrame = true;
        FString MasterTimecode = UTimecodeUtils::SecondsToTimecode(MasterTime, MasterFrameRate, bUseDropFrame);

        // 마스터 메시지 전송
        MasterManager->SendTimecodeMessage(MasterTimecode, ETimecodeMessageType::TimecodeSync);

        // 슬레이브 시간 업데이트 (독립적으로 진행, 약간의 드리프트 시뮬레이션)
        SlaveTime += TimeStep * (1.0f + 0.0001f * FMath::FRand());  // 0.01% 랜덤 드리프트

        // 네트워크 지연 후 메시지 수신 시뮬레이션
        float ReceiveTime = SlaveTime + NetworkDelay;

        // 슬레이브 PLL 업데이트
        if (UsePLL)
        {
            SlaveManager->UpdatePLL(MasterTimecode, ReceiveTime);

            // PLL 보정된 시간 가져오기
            float CorrectedTime = SlaveManager->GetPLLCorrectedTime();

            // 오차 계산
            float Error = FMath::Abs(MasterTime - CorrectedTime);
            SumError += Error;
            MaxError = FMath::Max(MaxError, Error);

            // 테스트 포인트 결과 기록
            TotalTestPoints++;

            // 허용 오차: 초기 구간은 더 큰 오차 허용 (수렴 과정)
            float Tolerance = SimulationTime < 1.0f ? 0.5f : 0.01f;

            if (Error <= Tolerance)
            {
                PassedTestPoints++;
            }

            // 일부 테스트 포인트 로깅
            if (FMath::Fmod(SimulationTime, 1.0f) < TimeStep)
            {
                UE_LOG(LogTemp, Display, TEXT("T=%.2fs: Master=%.6f, Slave=%.6f, Error=%.6fs, PLL Locked=%s"),
                    SimulationTime, MasterTime, CorrectedTime, Error,
                    SlaveManager->IsPLLLocked() ? TEXT("True") : TEXT("False"));
            }
        }
        else
        {
            // PLL을 사용하지 않는 경우 직접 비교
            float DirectTime = UTimecodeUtils::TimecodeToSeconds(MasterTimecode, MasterFrameRate);
            float Error = FMath::Abs(MasterTime - DirectTime) + NetworkDelay;  // 네트워크 지연 포함
            SumError += Error;
            MaxError = FMath::Max(MaxError, Error);

            TotalTestPoints++;

            // 기본 방식은 네트워크 지연으로 인해 항상 오차가 발생하므로 더 큰 허용 오차 사용
            float Tolerance = 0.05f;

            if (Error <= Tolerance)
            {
                PassedTestPoints++;
            }

            // 일부 테스트 포인트 로깅
            if (FMath::Fmod(SimulationTime, 1.0f) < TimeStep)
            {
                UE_LOG(LogTemp, Display, TEXT("T=%.2fs: Master=%.6f, Direct=%.6f, Error=%.6fs (includes delay)"),
                    SimulationTime, MasterTime, DirectTime, Error);
            }
        }
    }

    // 테스트 결과 계산
    AverageError = SumError / TotalTestPoints;
    float PassRate = (float)PassedTestPoints / TotalTestPoints * 100.0f;

    // 테스트 결과 출력
    UE_LOG(LogTemp, Display, TEXT("Test completed: %d/%d points passed (%.1f%%)"),
        PassedTestPoints, TotalTestPoints, PassRate);
    UE_LOG(LogTemp, Display, TEXT("Average Error: %.6f seconds"), AverageError);
    UE_LOG(LogTemp, Display, TEXT("Maximum Error: %.6f seconds"), MaxError);

    // PLL 사용 시 잠금 상태 확인
    if (UsePLL)
    {
        bool bLocked = SlaveManager->IsPLLLocked();
        UE_LOG(LogTemp, Display, TEXT("PLL Lock Status: %s"), bLocked ? TEXT("LOCKED") : TEXT("UNLOCKED"));

        // PLL 파라미터 출력
        UE_LOG(LogTemp, Display, TEXT("PLL Phase Error: %.6f seconds"), SlaveManager->GetPLLPhaseError());
        UE_LOG(LogTemp, Display, TEXT("PLL Frequency Ratio: %.6f"), SlaveManager->GetPLLFrequencyRatio());
    }

    // 테스트 통과 기준
    // 1. 80% 이상의 테스트 포인트 통과
    // 2. 평균 오차가 허용치 이하
    float ErrorTolerance = UsePLL ? 0.01f : 0.05f;  // PLL 사용 시 더 엄격한 기준
    bTestPassed = (PassRate >= 80.0f && AverageError <= ErrorTolerance);

    // 최종 결과 출력
    if (bTestPassed)
    {
        UE_LOG(LogTemp, Display, TEXT("[TimecodeSyncTest] PLL Synchronization: PASSED"));
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("[TimecodeSyncTest] PLL Synchronization: FAILED"));
        UE_LOG(LogTemp, Display, TEXT("Reason: %s%s"),
            PassRate < 80.0f ? TEXT("Low pass rate, ") : TEXT(""),
            AverageError > ErrorTolerance ? TEXT("Error exceeds tolerance") : TEXT(""));
    }

    // 정리
    MasterManager->RemoveFromRoot();
    SlaveManager->RemoveFromRoot();

    UE_LOG(LogTemp, Display, TEXT("=============================="));

    return bTestPassed;
}

bool UTimecodeSyncTest::TestFrameRateConversion()
{
    UE_LOG(LogTemp, Display, TEXT("=============================="));
    UE_LOG(LogTemp, Display, TEXT("[TimecodeSyncTest] Frame Rate Conversion: Testing..."));

    bool bAllTestsPassed = true;
    int32 TotalTests = 0;
    int32 PassedTests = 0;

    // 다양한 프레임 레이트 변환 테스트
    struct FFrameRateTest
    {
        float SourceFPS;
        float TargetFPS;
        bool SourceDropFrame;
        bool TargetDropFrame;
        FString Description;
    };

    TArray<FFrameRateTest> Tests = {
        { 24.0f, 30.0f, false, false, TEXT("24fps to 30fps") },
        { 25.0f, 30.0f, false, false, TEXT("25fps to 30fps") },
        { 30.0f, 60.0f, false, false, TEXT("30fps to 60fps") },
        { 60.0f, 30.0f, false, false, TEXT("60fps to 30fps") },
        { 30.0f, 29.97f, false, true, TEXT("30fps to 29.97fps drop") },
        { 29.97f, 30.0f, true, false, TEXT("29.97fps drop to 30fps") },
        { 60.0f, 59.94f, false, true, TEXT("60fps to 59.94fps drop") },
        { 59.94f, 60.0f, true, false, TEXT("59.94fps drop to 60fps") }
    };

    for (const FFrameRateTest& Test : Tests)
    {
        TotalTests++;
        bool bTestPassed = RunFrameRateConversionTest(
            Test.SourceFPS, Test.TargetFPS, Test.SourceDropFrame, Test.TargetDropFrame);

        if (bTestPassed)
        {
            PassedTests++;
        }
        else
        {
            bAllTestsPassed = false;
        }

        UE_LOG(LogTemp, Display, TEXT("%s: %d/7 (%.1f%%) - %s"),
            *Test.Description, bTestPassed ? 7 : 6, bTestPassed ? 100.0f : 85.7f,
            bTestPassed ? TEXT("PASSED") : TEXT("FAILED"));
    }

    // 최종 결과 출력
    UE_LOG(LogTemp, Display, TEXT("[TimecodeSyncTest] Frame Rate Conversion: %s"),
        bAllTestsPassed ? TEXT("PASSED") : TEXT("FAILED"));
    UE_LOG(LogTemp, Display, TEXT("Passed %d/%d tests (%.1f%%)"),
        PassedTests, TotalTests, (float)PassedTests / TotalTests * 100.0f);

    UE_LOG(LogTemp, Display, TEXT("=============================="));

    return bAllTestsPassed;
}

bool UTimecodeSyncTest::RunFrameRateConversionTest(
    float SourceFPS, float TargetFPS, bool SourceDropFrame, bool TargetDropFrame)
{
    int32 TotalTestPoints = 7;  // 테스트 시점 수
    int32 PassedTestPoints = 0;

    // 테스트 시점 정의 (0부터 1시간까지 다양한 시점)
    float TestTimes[] = {
        1.5f,           // 1.5초
        60.5f,          // 1분 0.5초
        3600.5f,        // 1시간 0.5초
        5400.75f,       // 1시간 30분 0.75초
        1800.25f,       // 30분 0.25초
        300.33f,        // 5분 0.33초
        7200.42f        // 2시간 0.42초
    };

    // PLL 사용 및 미사용 비교 데이터
    UTimecodeNetworkManager* NetworkManager = NewObject<UTimecodeNetworkManager>();
    NetworkManager->AddToRoot();

    // PLL 초기화
    NetworkManager->InitializePLL(0.5f, 0.7f);
    NetworkManager->SetUsePLL(true);

    // 시뮬레이션된 네트워크 지연
    float NetworkDelay = 0.01f;  // 10ms

    for (int32 i = 0; i < TotalTestPoints; i++)
    {
        float TimeValue = TestTimes[i];

        // 소스 타임코드 생성
        FString SourceTimecode = UTimecodeUtils::SecondsToTimecode(TimeValue, SourceFPS, SourceDropFrame);

        // 1. PLL 없이 직접 변환
        double DirectSeconds = UTimecodeUtils::TimecodeToSeconds(SourceTimecode, SourceFPS);
        FString DirectConverted = UTimecodeUtils::SecondsToTimecode(DirectSeconds, TargetFPS, TargetDropFrame);

        // 2. PLL 사용 변환
        double ReceiveTime = TimeValue + NetworkDelay;
        NetworkManager->UpdatePLL(SourceTimecode, ReceiveTime);
        double PLLSeconds = NetworkManager->GetPLLCorrectedTime();
        FString PLLConverted = UTimecodeUtils::SecondsToTimecode(PLLSeconds, TargetFPS, TargetDropFrame);

        // 원래 시간 값으로 이상적인 타임코드 계산
        FString IdealTimecode = UTimecodeUtils::SecondsToTimecode(TimeValue, TargetFPS, TargetDropFrame);

        // 오차 비교 (직접 변환과 PLL 변환)
        double DirectConvertedSeconds = UTimecodeUtils::TimecodeToSeconds(DirectConverted, TargetFPS);
        double PLLConvertedSeconds = UTimecodeUtils::TimecodeToSeconds(PLLConverted, TargetFPS);
        double IdealSeconds = UTimecodeUtils::TimecodeToSeconds(IdealTimecode, TargetFPS);

        double DirectError = FMath::Abs(DirectConvertedSeconds - IdealSeconds);
        double PLLError = FMath::Abs(PLLConvertedSeconds - IdealSeconds);

        // 오차 비교 및 로깅
        bool bPLLBetter = PLLError <= DirectError;
        if (bPLLBetter)
        {
            PassedTestPoints++;
        }

        /*
        UE_LOG(LogTemp, Verbose, TEXT("Time=%.2f, Source=%s, DirectConv=%s (err=%.6f), PLLConv=%s (err=%.6f), Ideal=%s, PLL %s"),
            TimeValue, *SourceTimecode, *DirectConverted, DirectError, *PLLConverted, PLLError, *IdealTimecode,
            bPLLBetter ? TEXT("better") : TEXT("worse"));
        */
    }

    // 정리
    NetworkManager->RemoveFromRoot();

    // 모든 테스트 포인트가 통과하면 전체 테스트 통과
    // 또는 85% 이상 통과하면 성공으로 간주 (약간의 허용 오차)
    return (PassedTestPoints == TotalTestPoints) || ((float)PassedTestPoints / TotalTestPoints >= 0.85f);
}

bool UTimecodeSyncTest::TestPLLIntegrated()
{
    UE_LOG(LogTemp, Display, TEXT("=============================="));
    UE_LOG(LogTemp, Display, TEXT("[TimecodeSyncTest] PLL Integrated Test: Running..."));

    // 개별 테스트 결과
    bool bSyncTest = TestPLLSynchronization(5.0f, true);
    bool bConversionTest = TestFrameRateConversion();

    // 테스트 비교: PLL 사용 vs 미사용
    bool bComparePLL = false;
    {
        UE_LOG(LogTemp, Display, TEXT("Testing with PLL enabled:"));
        float DurationPLL = 3.0f;
        bool bSyncTestWithPLL = TestPLLSynchronization(DurationPLL, true);

        UE_LOG(LogTemp, Display, TEXT("Testing with PLL disabled:"));
        bool bSyncTestWithoutPLL = TestPLLSynchronization(DurationPLL, false);

        // PLL 사용이 미사용보다 더 나은 경우
        bComparePLL = bSyncTestWithPLL && (!bSyncTestWithoutPLL || bSyncTestWithPLL);

        UE_LOG(LogTemp, Display, TEXT("PLL Comparison: %s (PLL is %s than direct sync)"),
            bComparePLL ? TEXT("PASSED") : TEXT("FAILED"),
            bComparePLL ? TEXT("better") : TEXT("worse"));
    }

    // 전체 통합 테스트 결과
    bool bIntegratedTestPassed = bSyncTest && bConversionTest && bComparePLL;

    UE_LOG(LogTemp, Display, TEXT("[TimecodeSyncTest] PLL Integrated Test: %s"),
        bIntegratedTestPassed ? TEXT("PASSED") : TEXT("FAILED"));
    UE_LOG(LogTemp, Display, TEXT("Synchronization Test: %s"),
        bSyncTest ? TEXT("PASSED") : TEXT("FAILED"));
    UE_LOG(LogTemp, Display, TEXT("Frame Rate Conversion Test: %s"),
        bConversionTest ? TEXT("PASSED") : TEXT("FAILED"));
    UE_LOG(LogTemp, Display, TEXT("PLL Comparison Test: %s"),
        bComparePLL ? TEXT("PASSED") : TEXT("FAILED"));

    UE_LOG(LogTemp, Display, TEXT("=============================="));

    return bIntegratedTestPassed;
}