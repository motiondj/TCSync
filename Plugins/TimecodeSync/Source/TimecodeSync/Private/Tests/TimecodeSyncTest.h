#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TimecodeNetworkManager.h"
#include "TimecodePLL.h"
#include "TimecodeSyncTest.generated.h"

/**
 * Class for testing timecode synchronization features including PLL
 */
UCLASS()
class TIMECODESYNC_API UTimecodeSyncTest : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Test the PLL synchronization accuracy
     * @param Duration Test duration in seconds
     * @param UsePLL Whether to use PLL for the test
     * @return True if test passed
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|Testing")
    static bool TestPLLSynchronization(float Duration = 5.0f, bool UsePLL = true);

    /**
     * Test frame rate conversion with PLL
     * @return True if all tests passed
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|Testing")
    static bool TestFrameRateConversion();

    /**
     * Test all PLL features in an integrated test
     * @return True if all tests passed
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|Testing")
    static bool TestPLLIntegrated();

private:
    /**
     * Run a specific frame rate conversion test
     * @param SourceFPS Source frame rate
     * @param TargetFPS Target frame rate
     * @param SourceDropFrame Whether source uses drop frame
     * @param TargetDropFrame Whether target uses drop frame
     * @return True if test passed
     */
    static bool RunFrameRateConversionTest(
        float SourceFPS,
        float TargetFPS,
        bool SourceDropFrame,
        bool TargetDropFrame);
};