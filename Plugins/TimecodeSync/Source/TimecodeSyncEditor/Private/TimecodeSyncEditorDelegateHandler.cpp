#include "TimecodeSyncEditorDelegateHandler.h"

UTimecodeSyncEditorDelegateHandler::UTimecodeSyncEditorDelegateHandler()
{
}

void UTimecodeSyncEditorDelegateHandler::HandleTimecodeMessage(const FTimecodeNetworkMessage& Message)
{
    if (TimecodeMessageCallback)
    {
        TimecodeMessageCallback(Message);
    }
    if (OnTimecodeMessageReceived.IsBound())
    {
        OnTimecodeMessageReceived.Execute(Message);
    }
}

void UTimecodeSyncEditorDelegateHandler::HandleNetworkStateChanged(ENetworkConnectionState NewState)
{
    if (NetworkStateCallback)
    {
        NetworkStateCallback(NewState);
    }
    if (OnNetworkStateChanged.IsBound())
    {
        OnNetworkStateChanged.Execute(NewState);
    }
}

void UTimecodeSyncEditorDelegateHandler::SetTimecodeMessageCallback(TFunction<void(const FTimecodeNetworkMessage&)> Callback)
{
    TimecodeMessageCallback = Callback;
}

void UTimecodeSyncEditorDelegateHandler::SetNetworkStateCallback(TFunction<void(ENetworkConnectionState)> Callback)
{
    NetworkStateCallback = Callback;
} 