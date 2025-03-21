// TimecodeUtilsTest.cpp
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "TimecodeUtils.h"

// 타임코드 변환 테스트
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimecodeConversionTest, "TimecodeSync.Utils.Conversion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTimecodeConversionTest::RunTest(const FString& Parameters)
{
    // 1. SecondsToTimecode 테스트
    {
        // 테스트 케이스: 0초
        FString Timecode0 = UTimecodeUtils::SecondsToTimecode(0.0f, 30.0f, false);
        TestEqual("0 seconds should be 00:00:00:00", Timecode0, TEXT("00:00:00:00"));

        // 테스트 케이스: 1초
        FString Timecode1 = UTimecodeUtils::SecondsToTimecode(1.0f, 30.0f, false);
        TestEqual("1 second should be 00:00:01:00", Timecode1, TEXT("00:00:01:00"));

        // 테스트 케이스: 1.5초 (30fps에서 프레임은 15가 됨)
        FString Timecode1_5 = UTimecodeUtils::SecondsToTimecode(1.5f, 30.0f, false);
        TestEqual("1.5 seconds should be 00:00:01:15", Timecode1_5, TEXT("00:00:01:15"));

        // 테스트 케이스: 59.9초 (마지막 프레임)
        FString Timecode59_9 = UTimecodeUtils::SecondsToTimecode(59.9f, 30.0f, false);
        TestEqual("59.9 seconds should be 00:00:59:27", Timecode59_9, TEXT("00:00:59:27"));

        // 테스트 케이스: 60초 (1분)
        FString Timecode60 = UTimecodeUtils::SecondsToTimecode(60.0f, 30.0f, false);
        TestEqual("60 seconds should be 00:01:00:00", Timecode60, TEXT("00:01:00:00"));

        // 테스트 케이스: 3600초 (1시간)
        FString Timecode3600 = UTimecodeUtils::SecondsToTimecode(3600.0f, 30.0f, false);
        TestEqual("3600 seconds should be 01:00:00:00", Timecode3600, TEXT("01:00:00:00"));

        // 테스트 케이스: 3661.5초 (1시간 1분 1.5초)
        FString Timecode3661_5 = UTimecodeUtils::SecondsToTimecode(3661.5f, 30.0f, false);
        TestEqual("3661.5 seconds should be 01:01:01:15", Timecode3661_5, TEXT("01:01:01:15"));
    }

    // 2. TimecodeToSeconds 테스트
    {
        // 테스트 케이스: 00:00:00:00
        float Seconds0 = UTimecodeUtils::TimecodeToSeconds(TEXT("00:00:00:00"), 30.0f, false);
        TestEqual("00:00:00:00 should be 0 seconds", Seconds0, 0.0f);

        // 테스트 케이스: 00:00:01:00
        float Seconds1 = UTimecodeUtils::TimecodeToSeconds(TEXT("00:00:01:00"), 30.0f, false);
        TestEqual("00:00:01:00 should be 1 second", Seconds1, 1.0f);

        // 테스트 케이스: 00:00:01:15
        float Seconds1_5 = UTimecodeUtils::TimecodeToSeconds(TEXT("00:00:01:15"), 30.0f, false);
        TestEqual("00:00:01:15 should be 1.5 seconds", Seconds1_5, 1.5f);

        // 테스트 케이스: 00:01:00:00
        float Seconds60 = UTimecodeUtils::TimecodeToSeconds(TEXT("00:01:00:00"), 30.0f, false);
        TestEqual("00:01:00:00 should be 60 seconds", Seconds60, 60.0f);

        // 테스트 케이스: 01:00:00:00
        float Seconds3600 = UTimecodeUtils::TimecodeToSeconds(TEXT("01:00:00:00"), 30.0f, false);
        TestEqual("01:00:00:00 should be 3600 seconds", Seconds3600, 3600.0f);

        // 테스트 케이스: 01:01:01:15
        float Seconds3661_5 = UTimecodeUtils::TimecodeToSeconds(TEXT("01:01:01:15"), 30.0f, false);
        TestEqual("01:01:01:15 should be 3661.5 seconds", Seconds3661_5, 3661.5f);
    }

    // 3. 양방향 변환 테스트 (round-trip)
    {
        // 다양한 시간값에 대해 왕복 변환 테스트
        float TestValues[] = { 0.0f, 1.0f, 1.5f, 59.9f, 60.0f, 3600.0f, 3661.5f, 7200.5f };

        for (float OriginalSeconds : TestValues)
        {
            // 초 -> 타임코드 -> 초 변환
            FString Timecode = UTimecodeUtils::SecondsToTimecode(OriginalSeconds, 30.0f, false);
            float ConvertedSeconds = UTimecodeUtils::TimecodeToSeconds(Timecode, 30.0f, false);

            // 소수점 때문에 정확히 같지 않을 수 있으므로 오차 범위 내에서 비교
            TestTrue(FString::Printf(TEXT("Round-trip conversion of %f seconds"), OriginalSeconds),
                FMath::IsNearlyEqual(OriginalSeconds, ConvertedSeconds, 0.034f)); // 30fps에서 한 프레임은 약 0.033초
        }

        // 다양한 프레임 레이트에서 테스트
        float TestFrameRates[] = { 24.0f, 25.0f, 30.0f, 60.0f };
        float TestTime = 3661.5f; // 1시간 1분 1.5초

        for (float FrameRate : TestFrameRates)
        {
            // 초 -> 타임코드 -> 초 변환
            FString Timecode = UTimecodeUtils::SecondsToTimecode(TestTime, FrameRate, false);
            float ConvertedSeconds = UTimecodeUtils::TimecodeToSeconds(Timecode, FrameRate, false);

            // 프레임 레이트에 따른 적절한 오차 범위 설정
            float FrameDuration = 1.0f / FrameRate;
            TestTrue(FString::Printf(TEXT("Round-trip conversion at %f fps"), FrameRate),
                FMath::IsNearlyEqual(TestTime, ConvertedSeconds, FrameDuration));
        }
    }

    return true;
}

// 드롭 프레임 타임코드 테스트
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDropFrameTimecodeTest, "TimecodeSync.Utils.DropFrame", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDropFrameTimecodeTest::RunTest(const FString& Parameters)
{
    // 29.97fps 드롭 프레임 테스트 케이스

    // 테스트 케이스: 정확히 1분 (프레임 드롭으로 인해 "00:01:00;02"가 됨)
    FString Timecode60sec = UTimecodeUtils::SecondsToTimecode(60.0f, 29.97f, true);
    TestEqual("60 seconds at 29.97fps drop frame should be 00:01:00;02", Timecode60sec, TEXT("00:01:00;02"));

    // 테스트 케이스: 10분 (10분은 예외이므로 프레임 드롭 없음)
    FString Timecode10min = UTimecodeUtils::SecondsToTimecode(600.0f, 29.97f, true);
    TestEqual("10 minutes at 29.97fps drop frame should be 00:10:00;00", Timecode10min, TEXT("00:10:00;00"));

    // 테스트 케이스: 11분 (프레임 드롭으로 인해 "00:11:00;02"가 됨)
    FString Timecode11min = UTimecodeUtils::SecondsToTimecode(660.0f, 29.97f, true);
    TestEqual("11 minutes at 29.97fps drop frame should be 00:11:00;02", Timecode11min, TEXT("00:11:00;02"));

    // 왕복 변환 테스트
    float TimeValues[] = { 59.94f, 60.0f, 600.0f, 660.0f, 3600.0f, 3660.0f };

    for (float OriginalTime : TimeValues)
    {
        FString Timecode = UTimecodeUtils::SecondsToTimecode(OriginalTime, 29.97f, true);
        float ConvertedTime = UTimecodeUtils::TimecodeToSeconds(Timecode, 29.97f, true);

        TestTrue(FString::Printf(TEXT("Drop frame round-trip conversion of %f seconds"), OriginalTime),
            FMath::IsNearlyEqual(OriginalTime, ConvertedTime, 0.034f));
    }

    // 59.94fps 테스트 케이스도 추가
    FString Timecode60sec_59_94 = UTimecodeUtils::SecondsToTimecode(60.0f, 59.94f, true);
    TestEqual("60 seconds at 59.94fps drop frame should be 00:01:00;04", Timecode60sec_59_94, TEXT("00:01:00;04"));

    return true;
}