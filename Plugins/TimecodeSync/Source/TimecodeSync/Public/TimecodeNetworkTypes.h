// TimecodeNetworkTypes.h
#pragma once

#include "CoreMinimal.h"
#include "TimecodeNetworkTypes.generated.h"

// Role determination mode enum
UENUM(BlueprintType)
enum class ETimecodeRoleMode : uint8
{
    Automatic UMETA(DisplayName = "Automatic Detection"),
    Manual    UMETA(DisplayName = "Manual Setting")
};

// Network connection state enum - 추가
UENUM(BlueprintType)
enum class ENetworkConnectionState : uint8
{
    Disconnected UMETA(DisplayName = "Disconnected"),
    Connecting   UMETA(DisplayName = "Connecting"),
    Connected    UMETA(DisplayName = "Connected"),
    Error        UMETA(DisplayName = "Error")
};

// Delegate for role mode change event
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRoleModeChangedDelegate, ETimecodeRoleMode, NewMode);