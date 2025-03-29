// TimecodeTester.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TimecodeComponent.h"
#include "PLLSynchronizer.h"
#include "SMPTETimecodeConverter.h"
#include "TimecodeTester.generated.h"

/**
 * Utility class for testing timecode functionality
 */
UCLASS(BlueprintType)
class TIMECODESYNC_API UTimecodeTester : public UObject
{
    GENERATED_BODY()

public:
    UTimecodeTester();

    /**
     * Test PLL module independently
     * @param Duration Test duration in seconds
     * @return True if test passed
     */
    UFUNCTION(BlueprintCallable, Category = "TimecodeTesting")
    bool TestPLLModule(float Duration = 5.0f);

    /**
     * Test SMPTE timecode conversion
     * @return True if test passed
     */
    UFUNCTION(BlueprintCallable, Category = "TimecodeTesting")
    bool TestSMPTEModule();

    /**
     * Test integrated mode
     * @param Duration Test duration in seconds
     * @return True if test passed
     */
    UFUNCTION(BlueprintCallable, Category = "TimecodeTesting")
    bool TestIntegratedMode(float Duration = 5.0f);

    /**
     * Get test result details
     * @return Detailed test results
     */
    UFUNCTION(BlueprintCallable, Category = "TimecodeTesting")
    FString GetTestDetails() const { return TestDetails; }

private:
    // Test details output
    FString TestDetails;

    // Log test result
    void LogTestResult(const FString& TestName, bool bSuccess, const FString& Details);
};