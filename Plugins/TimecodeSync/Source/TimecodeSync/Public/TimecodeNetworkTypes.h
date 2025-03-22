#pragma once

#include "CoreMinimal.h"
#include "TimecodeNetworkTypes.generated.h"

// Timecode message type enum
UENUM(BlueprintType)
enum class ETimecodeMessageType : uint8
{
    Heartbeat,       // Heartbeat for connection status check
    TimecodeSync,    // Timecode synchronization message
    RoleAssignment,  // Role assignment message
    Event,           // Event trigger message
    Command          // Command message
};

// Role determination mode enum
UENUM(BlueprintType)
enum class ETimecodeRoleMode : uint8
{
    Automatic UMETA(DisplayName = "Automatic Detection"),
    Manual    UMETA(DisplayName = "Manual Setting")
};

// Delegate for role mode change event
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRoleModeChangedDelegate, ETimecodeRoleMode, NewMode);

// Timecode message structure for network transmission
USTRUCT(BlueprintType)
struct FTimecodeNetworkMessage
{
    GENERATED_BODY()

    // Message type
    UPROPERTY(BlueprintReadWrite, Category = "Network")
    ETimecodeMessageType MessageType;

    // Timecode string
    UPROPERTY(BlueprintReadWrite, Category = "Network")
    FString Timecode;

    // Additional data (event name, command, etc.)
    UPROPERTY(BlueprintReadWrite, Category = "Network")
    FString Data;

    // Message creation time
    UPROPERTY(BlueprintReadWrite, Category = "Network")
    double Timestamp;

    // Sender ID
    UPROPERTY(BlueprintReadWrite, Category = "Network")
    FString SenderID;

    // Default constructor
    FTimecodeNetworkMessage()
        : MessageType(ETimecodeMessageType::Heartbeat)
        , Timecode(TEXT("00:00:00:00"))
        , Data(TEXT(""))
        , Timestamp(0.0)
        , SenderID(TEXT(""))
    {
    }

    // Serialize message to byte array
    TArray<uint8> Serialize() const;

    // Deserialize message from byte array
    bool Deserialize(const TArray<uint8>& Data);
};
