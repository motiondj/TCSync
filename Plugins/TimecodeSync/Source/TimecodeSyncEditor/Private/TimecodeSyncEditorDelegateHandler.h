#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TimecodeNetworkTypes.h"
#include "TimecodeSyncEditorDelegateHandler.generated.h"

// 함수 포인터 타입 정의
DECLARE_DELEGATE_OneParam(FOnTimecodeMessageReceivedDelegate, const FTimecodeNetworkMessage&);
DECLARE_DELEGATE_OneParam(FOnNetworkStateChangedDelegate, ENetworkConnectionState);

UCLASS()
class TIMECODESYNCEDITOR_API UTimecodeSyncEditorDelegateHandler : public UObject
{
    GENERATED_BODY()

public:
    UTimecodeSyncEditorDelegateHandler();

    // 타임코드 메시지 수신 핸들러
    UFUNCTION()
    void HandleTimecodeMessage(const FTimecodeNetworkMessage& Message);

    // 네트워크 상태 변경 핸들러
    UFUNCTION()
    void HandleNetworkStateChanged(ENetworkConnectionState NewState);

    // 함수 포인터 타입 델리게이트
    FOnTimecodeMessageReceivedDelegate OnTimecodeMessageReceived;
    FOnNetworkStateChangedDelegate OnNetworkStateChanged;

    // 콜백 설정 함수
    void SetTimecodeMessageCallback(TFunction<void(const FTimecodeNetworkMessage&)> Callback);
    void SetNetworkStateCallback(TFunction<void(ENetworkConnectionState)> Callback);

private:
    // 콜백 함수 저장
    TFunction<void(const FTimecodeNetworkMessage&)> TimecodeMessageCallback;
    TFunction<void(ENetworkConnectionState)> NetworkStateCallback;
};