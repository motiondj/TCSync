// Timecode message type enum
UENUM(BlueprintType)
enum class ETimecodeMessageType : uint8
{
    Command UMETA(DisplayName = "Command"), // Event trigger message
    Event UMETA(DisplayName = "Event"), // Role assignment message
    Heartbeat UMETA(DisplayName = "Heartbeat"), // Connection state check heartbeat
    RoleAssignment UMETA(DisplayName = "Role Assignment"), // Timecode synchronization message
    TimecodeSync UMETA(DisplayName = "Timecode Sync") // Timecode synchronization message
};

// Timecode string
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
FString Timecode;

// Additional data (event name, command, etc.)
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
FString Data;

// Message creation time
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
FDateTime Timestamp;

// Sender ID
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
FString SenderID; 