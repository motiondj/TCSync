#include "TimecodeSettings.h"

UTimecodeSettings::UTimecodeSettings()
{
    // Set default values
    FrameRate = 30.0f;
    bUseDropFrameTimecode = false;
    DefaultUDPPort = 10000;
    MulticastGroupAddress = "239.0.0.1";
    BroadcastInterval = 0.033f; // Approximately 30Hz

    // Default role settings
    RoleMode = ETimecodeRoleMode::Automatic;
    bIsManualMaster = false;
    MasterIPAddress = "";

    // Default nDisplay settings
    bEnableNDisplayIntegration = false;
    bUseNDisplayRoleAssignment = true;

    // Default advanced settings
    bAutoStartTimecode = true;
    bEnablePacketLossCompensation = true;
    bEnableNetworkLatencyCompensation = true;
    ConnectionCheckInterval = 1.0f;
}

FName UTimecodeSettings::GetCategoryName() const
{
    return FName("Plugins");
}

FText UTimecodeSettings::GetSectionText() const
{
    return FText::FromString("Timecode Sync");
}

FText UTimecodeSettings::GetSectionDescription() const
{
    return FText::FromString("Configure settings for the Timecode Sync plugin.");
}

#if WITH_EDITOR
void UTimecodeSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // Validate and process logic when properties change
    const FName PropertyName = (PropertyChangedEvent.Property != nullptr)
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    // Validate frame rate
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UTimecodeSettings, FrameRate))
    {
        // Warn if frame rate is too low or high
        if (FrameRate < 1.0f || FrameRate > 240.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("FrameRate should be between 1.0 and 240.0"));
            FrameRate = FMath::Clamp(FrameRate, 1.0f, 240.0f);
        }

        // Drop frame timecode is typically used with 29.97fps or 59.94fps
        if (bUseDropFrameTimecode)
        {
            if (!FMath::IsNearlyEqual(FrameRate, 29.97f) && !FMath::IsNearlyEqual(FrameRate, 59.94f))
            {
                UE_LOG(LogTemp, Warning, TEXT("Drop frame timecode is typically used with 29.97fps or 59.94fps"));
            }
        }
    }

    // When drop frame timecode setting changes
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTimecodeSettings, bUseDropFrameTimecode))
    {
        if (bUseDropFrameTimecode)
        {
            // Drop frame is typically used with 29.97fps or 59.94fps
            if (!FMath::IsNearlyEqual(FrameRate, 29.97f) && !FMath::IsNearlyEqual(FrameRate, 59.94f))
            {
                UE_LOG(LogTemp, Warning, TEXT("Drop frame timecode is typically used with 29.97fps or 59.94fps"));
            }
        }
    }

    // When role mode changes
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTimecodeSettings, RoleMode))
    {
        if (RoleMode == ETimecodeRoleMode::Manual)
        {
            UE_LOG(LogTemp, Log, TEXT("Switched to manual role mode"));

            // Warn if master IP is empty in manual slave mode
            if (!bIsManualMaster && MasterIPAddress.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("Manual slave mode requires a Master IP address"));
            }
        }
        else // Automatic
        {
            UE_LOG(LogTemp, Log, TEXT("Switched to automatic role mode"));
        }
    }

    // When manual master setting changes
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTimecodeSettings, bIsManualMaster))
    {
        if (RoleMode == ETimecodeRoleMode::Manual)
        {
            UE_LOG(LogTemp, Log, TEXT("Manual master setting changed to: %s"),
                bIsManualMaster ? TEXT("MASTER") : TEXT("SLAVE"));

            // Warn if master IP is empty when switching to slave
            if (!bIsManualMaster && MasterIPAddress.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("Manual slave mode requires a Master IP address"));
            }
        }
    }

    // When master IP address changes
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTimecodeSettings, MasterIPAddress))
    {
        if (RoleMode == ETimecodeRoleMode::Manual && !bIsManualMaster)
        {
            if (MasterIPAddress.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("Master IP address cannot be empty in manual slave mode"));
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("Master IP address set to: %s"), *MasterIPAddress);

                // Simple IP format validation (more strict validation may be needed)
                TArray<FString> IPParts;
                MasterIPAddress.ParseIntoArray(IPParts, TEXT("."), true);

                if (IPParts.Num() != 4)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Invalid IP address format: %s"), *MasterIPAddress);
                }
            }
        }
    }
}
#endif