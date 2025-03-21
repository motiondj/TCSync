/**
 * Class that manages settings for the timecode synchronization plugin
 */

// Frame rate setting
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode", meta = (ClampMin = "1.0", ClampMax = "240.0"))
float FrameRate;

// Use drop frame timecode flag
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode")
bool bUseDropFrameTimecode;

// Default UDP port
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network", meta = (ClampMin = "1024", ClampMax = "65535"))
int32 DefaultUDPPort;

// Multicast group IP
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
FString MulticastGroupAddress;

// Timecode transmission interval (in seconds)
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network", meta = (ClampMin = "0.001", ClampMax = "1.0"))
float BroadcastInterval;

// Role determination mode
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Role")
ETimecodeRoleMode RoleMode;

// Master role flag in manual mode
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Role", meta = (EditCondition = "RoleMode==ETimecodeRoleMode::Manual"))
bool bIsManualMaster;

// Use nDisplay node ID for role assignment (only in automatic mode)
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Integration", meta = (EditCondition = "bEnableNDisplayIntegration && RoleMode==ETimecodeRoleMode::Automatic"))
bool bUseNDisplayRoleAssignment;

// Auto start flag
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
bool bAutoStartTimecode;

// Enable packet loss compensation
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
bool bEnablePacketLossCompensation; 