#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TimecodeNetworkTypes.h"
#include "TimecodeSyncEditorDelegateHandler.generated.h"

// Timecode message delegate
DECLARE_DELEGATE_OneParam(FOnTimecodeMessageReceivedDelegate, const FTimecodeNetworkMessage&);

// Network state delegate
DECLARE_DELEGATE_OneParam(FOnNetworkStateChangedDelegate, ENetworkConnectionState);

UCLASS()
class TIMECODESYNCEDITOR_API UTimecodeSyncEditorDelegateHandler : public UObject
{
    GENERATED_BODY()

public:
    UTimecodeSyncEditorDelegateHandler();

    // Timecode message delegate
    FOnTimecodeMessageReceivedDelegate OnTimecodeMessageReceived;

    // Network state delegate
    FOnNetworkStateChangedDelegate OnNetworkStateChanged;

    // Handle timecode message
    UFUNCTION()
    void HandleTimecodeMessage(const FTimecodeNetworkMessage& Message);

    // Handle network state change
    UFUNCTION()
    void HandleNetworkStateChanged(ENetworkConnectionState NewState);

    // Set callbacks
    void SetTimecodeMessageCallback(TFunction<void(const FTimecodeNetworkMessage&)> Callback);
    void SetNetworkStateCallback(TFunction<void(ENetworkConnectionState)> Callback);

private:
    // Callback functions
    TFunction<void(const FTimecodeNetworkMessage&)> TimecodeMessageCallback;
    TFunction<void(ENetworkConnectionState)> NetworkStateCallback;
}; 