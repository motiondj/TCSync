/**
 * Component that provides timecode functionality when attached to an actor
 */

// Timecode role setting
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode")
bool bIsMaster;

// nDisplay usage flag
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode")
bool bUseNDisplay;

// Frame rate setting
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode", meta = (ClampMin = "1.0", ClampMax = "240.0"))
float FrameRate;

// UDP port setting
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network", meta = (ClampMin = "1024", ClampMax = "65535"))
int32 UDPPort;

// Target IP setting (for unicast transmission in master mode)
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
FString TargetIP;

// Multicast group setting
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
FString MulticastGroup;

// Auto start setting
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode")
bool bAutoStart;

// Network synchronization interval (in seconds)
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network", meta = (ClampMin = "0.001", ClampMax = "1.0"))
float SyncInterval;

// Current timecode (read-only)
UPROPERTY(BlueprintReadOnly, Category = "Timecode")
FString CurrentTimecode;

// Timecode change event
UPROPERTY(BlueprintAssignable, Category = "Timecode")
FOnTimecodeChanged OnTimecodeChanged;

// Timecode event trigger
UPROPERTY(BlueprintAssignable, Category = "Timecode")
FOnTimecodeEventTriggered OnTimecodeEventTriggered;

// Network connection state change event
UPROPERTY(BlueprintAssignable, Category = "Network")
FOnNetworkConnectionChanged OnNetworkConnectionChanged;

// Network manager
UPROPERTY()
class UTimecodeNetworkManager* NetworkManager; 