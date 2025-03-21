/**
 * Timecode network manager class
 */

// Timecode message received delegate
UPROPERTY(BlueprintAssignable, Category = "Network")
FOnTimecodeMessageReceived OnMessageReceived;

// Network state change delegate
UPROPERTY(BlueprintAssignable, Category = "Network")
FOnNetworkStateChanged OnNetworkStateChanged;

// Role mode change delegate
UPROPERTY(BlueprintAssignable, Category = "Network")
FOnRoleModeChanged OnRoleModeChanged; 