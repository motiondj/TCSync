// TimecodeModeTest.cpp
// 경로: Plugins/TimecodeSync/Source/TimecodeSync/Private/Tests/TimecodeModeTest.cpp

#include "Tests/TimecodeModeTest.h"

UTimecodeModeTest::UTimecodeModeTest()
{
    TestDetails = TEXT("No tests run yet");
}

bool UTimecodeModeTest::TestPLLModule(float Duration)
{
    TestDetails.Empty();
    LogTestResult(TEXT("PLL Module"), true, TEXT("Starting PLL module test..."));

    // Create PLL instance
    UPLLSynchronizer* PLL = NewObject<UPLLSynchronizer>(this);
    if (!PLL)
    {
        LogTestResult(TEXT("PLL Module"), false, TEXT("Failed to create PLL instance"));
        return false;
    }

    // Initialize PLL
    PLL->Initialize();

    // Simulate time synchronization
    float LocalTime = 0.0f;
    float MasterTime = 0.0f;
    float TotalError = 0.0f;
    int32 Steps = FMath::CeilToInt(Duration / 0.1f); // 0.1 second steps

    for (int32 i = 0; i < Steps; ++i)
    {
        // Simulate local and master clocks with slight drift
        LocalTime += 0.1f;
        MasterTime += 0.1f * (1.0f + 0.001f * FMath::Sin(i * 0.1f)); // Slight oscillation in master

        // Process through PLL
        float SyncedTime = PLL->ProcessTime(LocalTime, MasterTime, 0.1f);

        // Calculate error
        float Error = FMath::Abs(SyncedTime - MasterTime);
        TotalError += Error;

        // Log every few steps
        if (i % 10 == 0 || i == Steps - 1)
        {
            LogTestResult(TEXT("PLL Module"), true,
                FString::Printf(TEXT("Step %d: Local=%.3f, Master=%.3f, Synced=%.3f, Error=%.6f"),
                    i, LocalTime, MasterTime, SyncedTime, Error));
        }
    }

    // Calculate average error
    float AvgError = TotalError / Steps;
    bool bSuccess = AvgError < 0.01f; // Error threshold

    // Final results
    LogTestResult(TEXT("PLL Module"), bSuccess,
        FString::Printf(TEXT("Test completed. Average error: %.6f seconds%s"),
            AvgError, bSuccess ? TEXT("") : TEXT(" (Exceeds threshold)")));

    return bSuccess;
}

bool UTimecodeModeTest::TestSMPTEModule()
{
    TestDetails.Empty();
    LogTestResult(TEXT("SMPTE Module"), true, TEXT("Starting SMPTE timecode conversion test..."));

    // Create SMPTE converter
    USMPTETimecodeConverter* Converter = NewObject<USMPTETimecodeConverter>(this);
    if (!Converter)
    {
        LogTestResult(TEXT("SMPTE Module"), false, TEXT("Failed to create SMPTE converter instance"));
        return false;
    }

    // Test cases
    struct FTestCase
    {
        float Seconds;
        float FrameRate;
        bool bDropFrame;
        FString ExpectedTimecode;
    };

    TArray<FTestCase> TestCases = {
        { 0.0f, 30.0f, false, TEXT("00:00:00:00") },  // Zero
        { 1.5f, 30.0f, false, TEXT("00:00:01:15") },  // 1.5 seconds at 30fps
        { 60.0f, 30.0f, false, TEXT("00:01:00:00") },  // 1 minute
        { 60.0f, 29.97f, true, TEXT("00:01:00;02") },  // 1 minute drop frame
        { 600.0f, 29.97f, true, TEXT("00:10:00;00") },  // 10 minutes drop frame (no drop)
        { 3600.0f, 29.97f, true, TEXT("01:00:00;00") }   // 1 hour drop frame
    };

    int32 PassedTests = 0;
    for (const FTestCase& TestCase : TestCases)
    {
        // Convert seconds to timecode
        FString Timecode = Converter->SecondsToTimecode(TestCase.Seconds, TestCase.FrameRate, TestCase.bDropFrame);

        // Convert back to seconds
        float SecondsBack = Converter->TimecodeToSeconds(Timecode, TestCase.FrameRate, TestCase.bDropFrame);

        // Results
        bool bTimecodeMatch = (Timecode == TestCase.ExpectedTimecode);
        bool bSecondsMatch = FMath::IsNearlyEqual(SecondsBack, TestCase.Seconds, 0.034f); // Allow 1 frame of error
        bool bTestPass = bTimecodeMatch && bSecondsMatch;

        if (bTestPass)
        {
            PassedTests++;
        }

        LogTestResult(TEXT("SMPTE Module"), bTestPass,
            FString::Printf(TEXT("%.1fs at %.2ffps %s: Got '%s' (Expected '%s'), Roundtrip=%.3fs (%s)"),
                TestCase.Seconds,
                TestCase.FrameRate,
                TestCase.bDropFrame ? TEXT("DF") : TEXT("NDF"),
                *Timecode,
                *TestCase.ExpectedTimecode,
                SecondsBack,
                bSecondsMatch ? TEXT("OK") : TEXT("ERROR")));
    }

    // Final results
    bool bOverallSuccess = (PassedTests == TestCases.Num());
    LogTestResult(TEXT("SMPTE Module"), bOverallSuccess,
        FString::Printf(TEXT("Test completed. %d/%d tests passed."),
            PassedTests, TestCases.Num()));

    return bOverallSuccess;
}

bool UTimecodeModeTest::TestIntegratedMode(float Duration)
{
    TestDetails.Empty();
    LogTestResult(TEXT("Integrated Mode"), true, TEXT("Starting integrated mode test..."));

    // Create component instances
    UPLLSynchronizer* PLL = NewObject<UPLLSynchronizer>(this);
    USMPTETimecodeConverter* SMPTE = NewObject<USMPTETimecodeConverter>(this);

    if (!PLL || !SMPTE)
    {
        LogTestResult(TEXT("Integrated Mode"), false, TEXT("Failed to create required components"));
        return false;
    }

    // Initialize
    PLL->Initialize();

    // Simulation variables
    float LocalTime = 0.0f;
    float MasterTime = 0.0f;
    int32 Steps = FMath::CeilToInt(Duration / 0.1f); // 0.1 second steps
    float FrameRate = 29.97f;
    bool bDropFrame = true;

    // Track success
    bool bSuccess = true;

    for (int32 i = 0; i < Steps; ++i)
    {
        // Simulate time progression with slight drift
        LocalTime += 0.1f;
        MasterTime += 0.1f * (1.0f + 0.0005f * FMath::Sin(i * 0.2f));

        // Step 1: Apply PLL
        float SyncedTime = PLL->ProcessTime(LocalTime, MasterTime, 0.1f);

        // Step 2: Convert to SMPTE timecode
        FString Timecode = SMPTE->SecondsToTimecode(SyncedTime, FrameRate, bDropFrame);

        // Step 3: Convert back to seconds to verify
        float RoundtripTime = SMPTE->TimecodeToSeconds(Timecode, FrameRate, bDropFrame);

        // Check for consistency
        float RoundtripError = FMath::Abs(SyncedTime - RoundtripTime);

        // Log results periodically
        if (i % 10 == 0 || i == Steps - 1)
        {
            bool bStepSuccess = RoundtripError < 0.034f; // 1 frame threshold
            if (!bStepSuccess)
            {
                bSuccess = false;
            }

            LogTestResult(TEXT("Integrated Mode"), bStepSuccess,
                FString::Printf(TEXT("Step %d: Local=%.3f, Master=%.3f, Synced=%.3f, Timecode=%s, Roundtrip Error=%.6f"),
                    i, LocalTime, MasterTime, SyncedTime, *Timecode, RoundtripError));
        }
    }

    // Final results
    LogTestResult(TEXT("Integrated Mode"), bSuccess,
        FString::Printf(TEXT("Test completed. %s"),
            bSuccess ? TEXT("All steps within error threshold") : TEXT("Some steps exceeded error threshold")));

    return bSuccess;
}

void UTimecodeModeTest::LogTestResult(const FString& TestName, bool bSuccess, const FString& Details)
{
    FString StatusText = bSuccess ? TEXT("[PASS]") : TEXT("[FAIL]");
    FString LogEntry = FString::Printf(TEXT("%s %s: %s"), *StatusText, *TestName, *Details);

    // Add to test details
    TestDetails += LogEntry + TEXT("\n");

    // Also log to output
    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("%s"), *LogEntry);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s"), *LogEntry);
    }
}