// PLLSynchronizer.h
// PLL algorithm for time synchronization between systems

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PLLSynchronizer.generated.h"

/**
 * Phase-Locked Loop synchronizer for network time synchronization
 * Used to synchronize time between multiple Unreal Engine instances
 */
UCLASS(BlueprintType, Blueprintable)
class TIMECODESYNC_API UPLLSynchronizer : public UObject
{
    GENERATED_BODY()

public:
    UPLLSynchronizer();

    /**
     * Initialize the PLL synchronizer
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|PLL")
    void Initialize();

    /**
     * Process time through PLL algorithm
     * @param LocalTime - Local system time (seconds)
     * @param MasterTime - Master system time (seconds)
     * @param DeltaTime - Time since last update (seconds)
     * @return Adjusted time (seconds)
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|PLL")
    float ProcessTime(float LocalTime, float MasterTime, float DeltaTime);

    /**
     * Update PLL state
     * @param DeltaTime - Time since last update (seconds)
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|PLL")
    void Update(float DeltaTime);

    /**
     * Reset PLL state
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|PLL")
    void Reset();

    /**
     * Get current PLL status
     * @param OutPhase - Output phase error (seconds)
     * @param OutFrequency - Output frequency adjustment factor
     * @param OutOffset - Output offset (seconds)
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|PLL")
    void GetStatus(double& OutPhase, double& OutFrequency, double& OutOffset) const;

    /**
     * Get current PLL error
     * @return Error between master and slave time (seconds)
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|PLL")
    float GetCurrentError() const { return CurrentError; }

    /**
     * Get current PLL frequency adjustment
     * @return Current frequency adjustment factor
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|PLL")
    float GetFrequencyAdjustment() const { return FrequencyAdjustment; }

    /**
     * Set PLL parameters
     * @param InBandwidth - PLL bandwidth (0.01-1.0)
     * @param InDamping - PLL damping factor (0.1-2.0)
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode|PLL")
    void SetParameters(float InBandwidth, float InDamping);

    // PLL Parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode|PLL|Parameters", meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float Bandwidth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode|PLL|Parameters", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float DampingFactor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode|PLL|Parameters")
    float FrequencyAdjustmentLimit;

private:
    // PLL state variables
    float CurrentError;
    float IntegratedError;
    float LastError;
    float FrequencyAdjustment;
    float PhaseAdjustment;
    double PhaseOffset;  // Track cumulative phase offset
    bool bIsLocked;

    // Internal parameters
    float Alpha;  // Proportional gain
    float Beta;   // Integral gain

    // Configure PLL gains based on bandwidth and damping factor
    void CalculateGains();
};