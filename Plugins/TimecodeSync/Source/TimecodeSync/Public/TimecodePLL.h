// TimecodePLL.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TimecodePLL.generated.h"

/**
 * Phase-Locked Loop implementation for Timecode synchronization.
 * This class handles time synchronization between master and slave devices
 * by implementing a software PLL algorithm to adjust for network delays
 * and clock drift.
 */
UCLASS()
class TIMECODESYNC_API UTimecodePLL : public UObject
{
    GENERATED_BODY()

public:
    UTimecodePLL();

    /**
     * Initialize the PLL with specific parameters
     * @param InBandwidth The PLL bandwidth (0.01-1.0) - Higher values react faster but may be less stable
     * @param InDamping The damping factor (0.1-2.0) - Controls oscillation behavior
     */
    void Initialize(float InBandwidth = 0.1f, float InDamping = 0.7f);

    /**
     * Reset the PLL state
     */
    void Reset();

    /**
     * Update the PLL based on a new master timestamp
     * @param MasterTime The reference time from the master
     * @param LocalTime The current local time
     * @param DeltaTime Time elapsed since the last update
     */
    void Update(double MasterTime, double LocalTime, float DeltaTime);

    /**
     * Get the corrected time value
     * @param LocalTime The current local time
     * @return The PLL-corrected time
     */
    double GetCorrectedTime(double LocalTime) const;

    /**
     * Set the bandwidth parameter
     * @param InBandwidth The new bandwidth value (0.01-1.0)
     */
    void SetBandwidth(float InBandwidth);

    /**
     * Set the damping factor
     * @param InDamping The new damping factor (0.1-2.0)
     */
    void SetDamping(float InDamping);

    /**
     * Get the current phase error (difference between master and slave)
     * @return The current phase error in seconds
     */
    double GetPhaseError() const;

    /**
     * Get the current frequency adjustment ratio
     * @return The frequency adjustment ratio (1.0 means no adjustment)
     */
    double GetFrequencyRatio() const;

    /**
     * Check if the PLL is locked (stable synchronization achieved)
     * @return True if the PLL is locked, false otherwise
     */
    bool IsLocked() const;

private:
    // PLL parameters
    float Bandwidth;        // PLL bandwidth (0.01-1.0)
    float Damping;          // Damping factor (0.1-2.0)

    // PLL state variables
    double PhaseError;      // Current phase error (time difference)
    double IntegratedError; // Accumulated error for integral component
    double FrequencyRatio;  // Current frequency adjustment ratio
    double LastError;       // Previous error for derivative component

    // PLL loop filter coefficients
    double ProportionalGain;
    double IntegralGain;
    double DerivativeGain;

    // PLL status
    bool bIsInitialized;    // Whether the PLL has been initialized
    bool bIsLocked;         // Whether the PLL has achieved lock
    int32 StableCycleCount; // Counter for stability detection

    // Internal methods
    void CalculateLoopCoefficients();
    void UpdateLockStatus();
};