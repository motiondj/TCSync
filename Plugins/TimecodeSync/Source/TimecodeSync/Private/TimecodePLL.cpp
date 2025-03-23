// TimecodePLL.cpp
#include "TimecodePLL.h"

UTimecodePLL::UTimecodePLL()
    : Bandwidth(0.1f)
    , Damping(0.7f)
    , PhaseError(0.0)
    , IntegratedError(0.0)
    , FrequencyRatio(1.0)
    , LastError(0.0)
    , ProportionalGain(0.0)
    , IntegralGain(0.0)
    , DerivativeGain(0.0)
    , bIsInitialized(false)
    , bIsLocked(false)
    , StableCycleCount(0)
{
}

void UTimecodePLL::Initialize(float InBandwidth, float InDamping)
{
    // Clamp parameters to valid ranges
    Bandwidth = FMath::Clamp(InBandwidth, 0.01f, 1.0f);
    Damping = FMath::Clamp(InDamping, 0.1f, 2.0f);

    // Calculate initial loop filter coefficients
    CalculateLoopCoefficients();

    // Reset state variables
    Reset();

    bIsInitialized = true;
}

void UTimecodePLL::Reset()
{
    PhaseError = 0.0;
    IntegratedError = 0.0;
    FrequencyRatio = 1.0; // Default to no adjustment
    LastError = 0.0;
    bIsLocked = false;
    StableCycleCount = 0;
}

void UTimecodePLL::Update(double MasterTime, double LocalTime, float DeltaTime)
{
    if (!bIsInitialized)
    {
        Initialize();
    }

    // Calculate current error (master time - local time)
    double CurrentError = MasterTime - LocalTime;

    // If this is a large jump (> 1 second), reset to avoid oscillation
    if (FMath::Abs(CurrentError - LastError) > 1.0)
    {
        Reset();
        PhaseError = CurrentError;
        LastError = CurrentError;
        return;
    }

    // Calculate error derivative (rate of change)
    double ErrorDerivative = (CurrentError - LastError) / FMath::Max(DeltaTime, 0.001f);

    // Update integrated error (with anti-windup protection)
    double MaxIntegral = 1.0 / IntegralGain; // Prevent excessive buildup
    IntegratedError = FMath::Clamp(
        IntegratedError + CurrentError * DeltaTime,
        -MaxIntegral,
        MaxIntegral
    );

    // Apply PID control to calculate frequency adjustment
    // P: Reacts to current error
    // I: Compensates for accumulated error
    // D: Dampens oscillations
    FrequencyRatio = 1.0 +
        ProportionalGain * CurrentError +
        IntegralGain * IntegratedError +
        DerivativeGain * ErrorDerivative;

    // Clamp frequency ratio to reasonable bounds (0.5-2.0x speed)
    FrequencyRatio = FMath::Clamp(FrequencyRatio, 0.5, 2.0);

    // Update phase error for external reporting
    PhaseError = CurrentError;

    // Store error for next iteration
    LastError = CurrentError;

    // Update lock status
    UpdateLockStatus();
}

double UTimecodePLL::GetCorrectedTime(double LocalTime) const
{
    if (!bIsInitialized)
    {
        return LocalTime;
    }

    // Apply phase and frequency correction
    return LocalTime * FrequencyRatio + PhaseError;
}

void UTimecodePLL::SetBandwidth(float InBandwidth)
{
    Bandwidth = FMath::Clamp(InBandwidth, 0.01f, 1.0f);
    CalculateLoopCoefficients();
}

void UTimecodePLL::SetDamping(float InDamping)
{
    Damping = FMath::Clamp(InDamping, 0.1f, 2.0f);
    CalculateLoopCoefficients();
}

double UTimecodePLL::GetPhaseError() const
{
    return PhaseError;
}

double UTimecodePLL::GetFrequencyRatio() const
{
    return FrequencyRatio;
}

bool UTimecodePLL::IsLocked() const
{
    return bIsLocked;
}

void UTimecodePLL::CalculateLoopCoefficients()
{
    // Convert bandwidth from 0-1 range to angular frequency
    double OmegaN = Bandwidth * 2.0 * PI; // Natural frequency

    // Calculate PID coefficients based on PLL theory
    // Standard second-order PLL coefficient mapping
    ProportionalGain = 2.0 * Damping * OmegaN;
    IntegralGain = OmegaN * OmegaN;
    DerivativeGain = 0.0; // Traditional PLL doesn't use derivative term, but we include for flexibility

    // Optional: Add some derivative gain for better stability
    // When using very low damping factors
    if (Damping < 0.4f)
    {
        DerivativeGain = 0.1 * ProportionalGain;
    }
}

void UTimecodePLL::UpdateLockStatus()
{
    // PLL is considered locked when phase error is small for several cycles
    const double LockThreshold = 0.001; // 1ms error threshold
    const int32 StableCyclesRequired = 10; // Require 10 stable updates to consider locked

    if (FMath::Abs(PhaseError) < LockThreshold)
    {
        StableCycleCount++;
        if (StableCycleCount >= StableCyclesRequired)
        {
            bIsLocked = true;
        }
    }
    else
    {
        StableCycleCount = 0;
        bIsLocked = false;
    }
}