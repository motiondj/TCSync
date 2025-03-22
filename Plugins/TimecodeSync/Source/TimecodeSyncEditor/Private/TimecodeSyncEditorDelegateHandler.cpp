#include "TimecodeSyncEditorDelegateHandler.h"

UTimecodeSyncEditorDelegateHandler::UTimecodeSyncEditorDelegateHandler()
{
    // 초기화
}

void UTimecodeSyncEditorDelegateHandler::HandleTimecodeMessage(const FTimecodeNetworkMessage& Message)
{
    // 콜백 호출
    if (TimecodeMessageCallback)
    {
        TimecodeMessageCallback(Message);
    }

    // 델리게이트 호출
    if (OnTimecodeMessageReceived.IsBound())
    {
        OnTimecodeMessageReceived.Execute(Message);
    }
}

void UTimecodeSyncEditorDelegateHandler::HandleNetworkStateChanged(ENetworkConnectionState NewState)
{
    // 콜백 호출
    if (NetworkStateCallback)
    {
        NetworkStateCallback(NewState);
    }

    // 델리게이트 호출
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