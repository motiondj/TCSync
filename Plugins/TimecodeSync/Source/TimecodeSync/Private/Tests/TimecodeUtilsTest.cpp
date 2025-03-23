// TimecodeUtilsTest.cpp
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "TimecodeUtils.h"

// Timecode conversion test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimecodeConversionTest, "TimecodeSync.Utils.Conversion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTimecodeConversionTest::RunTest(const FString& Parameters)
{
    // 1. SecondsToTimecode test
    {
        // Test case: 0 seconds
        FString Timecode0 = UTimecodeUtils::SecondsToTimecode(0.0f, 30.0f, false);
        TestEqual("0 seconds should be 00:00:00:00", Timecode0, TEXT("00:00:00:00"));

        // Test case: 1 second
        FString Timecode1 = UTimecodeUtils::SecondsToTimecode(1.0f, 30.0f, false);
        TestEqual("1 second should be 00:00:01:00", Timecode1, TEXT("00:00:01:00"));

        // Test case: 1.5 seconds (15 frames at 30fps)
        FString Timecode1_5 = UTimecodeUtils::SecondsToTimecode(1.5f, 30.0f, false);
        TestEqual("1.5 seconds should be 00:00:01:15", Timecode1_5, TEXT("00:00:01:15"));

        // Test case: 59.9 seconds (last frame)
        FString Timecode59_9 = UTimecodeUtils::SecondsToTimecode(59.9f, 30.0f, false);
        TestEqual("59.9 seconds should be 00:00:59:27", Timecode59_9, TEXT("00:00:59:27"));

        // Test case: 60 seconds (1 minute)
        FString Timecode60 = UTimecodeUtils::SecondsToTimecode(60.0f, 30.0f, false);
        TestEqual("60 seconds should be 00:01:00:00", Timecode60, TEXT("00:01:00:00"));

        // Test case: 3600 seconds (1 hour)
        FString Timecode3600 = UTimecodeUtils::SecondsToTimecode(3600.0f, 30.0f, false);
        TestEqual("3600 seconds should be 01:00:00:00", Timecode3600, TEXT("01:00:00:00"));

        // Test case: 3661.5 seconds (1 hour 1 minute 1.5 seconds)
        FString Timecode3661_5 = UTimecodeUtils::SecondsToTimecode(3661.5f, 30.0f, false);
        TestEqual("3661.5 seconds should be 01:01:01:15", Timecode3661_5, TEXT("01:01:01:15"));
    }

    // 2. TimecodeToSeconds test
    {
        // Test case: 00:00:00:00
        float Seconds0 = UTimecodeUtils::TimecodeToSeconds(TEXT("00:00:00:00"), 30.0f, false);
        TestEqual("00:00:00:00 should be 0 seconds", Seconds0, 0.0f);

        // Test case: 00:00:01:00
        float Seconds1 = UTimecodeUtils::TimecodeToSeconds(TEXT("00:00:01:00"), 30.0f, false);
        TestEqual("00:00:01:00 should be 1 second", Seconds1, 1.0f);

        // Test case: 00:00:01:15
        float Seconds1_5 = UTimecodeUtils::TimecodeToSeconds(TEXT("00:00:01:15"), 30.0f, false);
        TestEqual("00:00:01:15 should be 1.5 seconds", Seconds1_5, 1.5f);

        // Test case: 00:01:00:00
        float Seconds60 = UTimecodeUtils::TimecodeToSeconds(TEXT("00:01:00:00"), 30.0f, false);
        TestEqual("00:01:00:00 should be 60 seconds", Seconds60, 60.0f);

        // Test case: 01:00:00:00
        float Seconds3600 = UTimecodeUtils::TimecodeToSeconds(TEXT("01:00:00:00"), 30.0f, false);
        TestEqual("01:00:00:00 should be 3600 seconds", Seconds3600, 3600.0f);

        // Test case: 01:01:01:15
        float Seconds3661_5 = UTimecodeUtils::TimecodeToSeconds(TEXT("01:01:01:15"), 30.0f, false);
        TestEqual("01:01:01:15 should be 3661.5 seconds", Seconds3661_5, 3661.5f);
    }

    // 3. Round-trip conversion test
    {
        // Test round-trip conversion with various time values
        float TestValues[] = { 0.0f, 1.0f, 1.5f, 59.9f, 60.0f, 3600.0f, 3661.5f, 7200.5f };

        for (float OriginalSeconds : TestValues)
        {
            // seconds -> timecode -> seconds conversion
            FString Timecode = UTimecodeUtils::SecondsToTimecode(OriginalSeconds, 30.0f, false);
            float ConvertedSeconds = UTimecodeUtils::TimecodeToSeconds(Timecode, 30.0f, false);

            // Compare within error range due to decimal points
            TestTrue(FString::Printf(TEXT("Round-trip conversion of %f seconds"), OriginalSeconds),
                FMath::IsNearlyEqual(OriginalSeconds, ConvertedSeconds, 0.034f)); // One frame is about 0.033 seconds at 30fps
        }

        // Test with various frame rates
        float TestFrameRates[] = { 24.0f, 25.0f, 30.0f, 60.0f };
        float TestTime = 3661.5f; // 1 hour 1 minute 1.5 seconds

        for (float FrameRate : TestFrameRates)
        {
            // seconds -> timecode -> seconds conversion
            FString Timecode = UTimecodeUtils::SecondsToTimecode(TestTime, FrameRate, false);
            float ConvertedSeconds = UTimecodeUtils::TimecodeToSeconds(Timecode, FrameRate, false);

            // Set appropriate error range based on frame rate
            float FrameDuration = 1.0f / FrameRate;
            TestTrue(FString::Printf(TEXT("Round-trip conversion at %f fps"), FrameRate),
                FMath::IsNearlyEqual(TestTime, ConvertedSeconds, FrameDuration));
        }
    }

    return true;
}

// Drop frame timecode test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDropFrameTimecodeTest, "TimecodeSync.Utils.DropFrame", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDropFrameTimecodeTest::RunTest(const FString& Parameters)
{
    // 29.97fps drop frame test cases

    // Test case: exactly 1 minute (becomes "00:01:00;02" due to frame drop)
    FString Timecode60sec = UTimecodeUtils::SecondsToTimecode(60.0f, 29.97f, true);
    TestEqual("60 seconds at 29.97fps drop frame should be 00:01:00;02", Timecode60sec, TEXT("00:01:00;02"));

    // Test case: 10 minutes (no frame drop as it's a multiple of 10)
    FString Timecode10min = UTimecodeUtils::SecondsToTimecode(600.0f, 29.97f, true);
    TestEqual("10 minutes at 29.97fps drop frame should be 00:10:00;00", Timecode10min, TEXT("00:10:00;00"));

    // Test case: 11 minutes (becomes "00:11:00;02" due to frame drop)
    FString Timecode11min = UTimecodeUtils::SecondsToTimecode(660.0f, 29.97f, true);
    TestEqual("11 minutes at 29.97fps drop frame should be 00:11:00;02", Timecode11min, TEXT("00:11:00;02"));

    // Round-trip conversion test
    float TimeValues[] = { 59.94f, 60.0f, 600.0f, 660.0f, 3600.0f, 3660.0f };

    for (float OriginalTime : TimeValues)
    {
        FString Timecode = UTimecodeUtils::SecondsToTimecode(OriginalTime, 29.97f, true);
        float ConvertedTime = UTimecodeUtils::TimecodeToSeconds(Timecode, 29.97f, true);

        TestTrue(FString::Printf(TEXT("Drop frame round-trip conversion of %f seconds"), OriginalTime),
            FMath::IsNearlyEqual(OriginalTime, ConvertedTime, 0.034f));
    }

    // Add 59.94fps test case
    FString Timecode60sec_59_94 = UTimecodeUtils::SecondsToTimecode(60.0f, 59.94f, true);
    TestEqual("60 seconds at 59.94fps drop frame should be 00:01:00;04", Timecode60sec_59_94, TEXT("00:01:00;04"));

    return true;
}