// PLLSynchronizer.cpp

#include "PLLSynchronizer.h"

// Define log category
DEFINE_LOG_CATEGORY_STATIC(LogPLLSynchronizer, Log, All);

UPLLSynchronizer::UPLLSynchronizer()
{
    // Initialize PLL parameters
    Bandwidth = 0.1f;
    DampingFactor = 1.0f;
    FrequencyAdjustmentLimit = 0.2f;

    // Initialize PLL state variables
    CurrentError = 0.0f;
    IntegratedError = 0.0f;
    LastError = 0.0f;
    FrequencyAdjustment = 1.0f;
    PhaseAdjustment = 0.0f;
    PhaseOffset = 0.0;
    bIsLocked = false;

    // Default PLL gains
    Alpha = 0.2f;
    Beta = 0.1f;
}

void UPLLSynchronizer::Initialize()
{
    // Calculate PLL gains
    CalculateGains();

    // Reset state
    Reset();

    UE_LOG(LogPLLSynchronizer, Log, TEXT("PLL Synchronizer initialized: Bandwidth=%.3f, DampingFactor=%.3f, Alpha=%.3f, Beta=%.3f"),
        Bandwidth, DampingFactor, Alpha, Beta);
}

float UPLLSynchronizer::ProcessTime(float LocalTime, float MasterTime, float DeltaTime)
{
    // Calculate phase error (time difference)
    CurrentError = MasterTime - LocalTime;

    // Update error integration
    IntegratedError += CurrentError * DeltaTime;

    // Calculate frequency and phase adjustments using PLL algorithm
    FrequencyAdjustment = 1.0f + Alpha * CurrentError + Beta * IntegratedError;

    // Limit frequency adjustment to prevent instability
    FrequencyAdjustment = FMath::Clamp(FrequencyAdjustment,
        1.0f - FrequencyAdjustmentLimit,
        1.0f + FrequencyAdjustmentLimit);

    // Calculate phase adjustment (50% immediate correction)
    PhaseAdjustment = CurrentError * 0.5f;

    // Track cumulative phase offset for status reporting
    PhaseOffset += PhaseAdjustment;

    // Calculate adjusted time
    float AdjustedTime = LocalTime + PhaseAdjustment;

    // Log PLL status for debugging
    UE_LOG(LogPLLSynchronizer, Verbose, TEXT("PLL: Error=%.6f, Freq.Adj=%.6f, Integrated=%.6f, Phase.Adj=%.6f, Adjusted=%.6f"),
        CurrentError, FrequencyAdjustment, IntegratedError, PhaseAdjustment, AdjustedTime);

    // Update lock status (lock if error < 2ms)
    bIsLocked = FMath::Abs(CurrentError) < 0.002f;

    // Store error for next iteration
    LastError = CurrentError;

    return AdjustedTime;
}

void UPLLSynchronizer::Update(float DeltaTime)
{
    // Apply frequency adjustment to time progression
    // This is called even when no master updates are received
    // to maintain smooth time adjustment
    if (FMath::Abs(FrequencyAdjustment - 1.0f) > SMALL_NUMBER)
    {
        // Gradually move frequency adjustment toward 1.0 when no recent updates
        FrequencyAdjustment = FMath::FInterpTo(FrequencyAdjustment, 1.0f, DeltaTime, 0.1f);
    }
}

void UPLLSynchronizer::Reset()
{
    // Reset PLL state variables
    CurrentError = 0.0f;
    IntegratedError = 0.0f;
    LastError = 0.0f;
    FrequencyAdjustment = 1.0f;
    PhaseAdjustment = 0.0f;
    PhaseOffset = 0.0;
    bIsLocked = false;

    UE_LOG(LogPLLSynchronizer, Log, TEXT("PLL Synchronizer reset"));
}

void UPLLSynchronizer::GetStatus(double& OutPhase, double& OutFrequency, double& OutOffset) const
{
    OutPhase = CurrentError;
    OutFrequency = FrequencyAdjustment;
    OutOffset = PhaseOffset;
}

void UPLLSynchronizer::SetParameters(float InBandwidth, float InDamping)
{
    // Clamp parameters to valid ranges
    Bandwidth = FMath::Clamp(InBandwidth, 0.01f, 1.0f);
    DampingFactor = FMath::Clamp(InDamping, 0.1f, 2.0f);

    // Recalculate gains
    CalculateGains();

    UE_LOG(LogPLLSynchronizer, Log, TEXT("PLL parameters updated: Bandwidth=%.3f, DampingFactor=%.3f, Alpha=%.3f, Beta=%.3f"),
        Bandwidth, DampingFactor, Alpha, Beta);
}

void UPLLSynchronizer::CalculateGains()
{
    // Configure PLL parameters based on bandwidth and damping factor
    // These equations are derived from standard PLL design formulas
    Alpha = 2.0f * Bandwidth * DampingFactor;
    Beta = Bandwidth * Bandwidth;
}