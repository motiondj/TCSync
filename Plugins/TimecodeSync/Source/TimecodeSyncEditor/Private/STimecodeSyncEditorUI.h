#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "TimecodeNetworkManager.h"
#include "TimecodeSyncEditorDelegateHandler.h"
#include "TimecodeSettings.h"

class STimecodeSyncEditorUI : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(STimecodeSyncEditorUI)
    {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual ~STimecodeSyncEditorUI();

private:
    // UI 생성 함수들
    TSharedRef<SWidget> CreateContentArea();
    TSharedRef<SWidget> CreateRoleSettingsSection();
    TSharedRef<SWidget> CreateNetworkSettingsSection();
    TSharedRef<SWidget> CreateTimecodeSettingsSection();
    TSharedRef<SWidget> CreateMonitoringSection();

    // UI 업데이트 함수
    void UpdateUI();

    // 콜백 함수들
    void OnTimecodeMessageReceived(const FTimecodeNetworkMessage& Message);
    void OnNetworkStateChanged(ENetworkConnectionState NewState);
    void OnRoleModeChanged(ETimecodeRoleMode NewMode);
    void OnManualMasterChanged(bool bNewValue);
    void OnMasterIPCommitted(const FText& NewText, ETextCommit::Type CommitType);

    // 헬퍼 함수들
    UTimecodeSettings* GetTimecodeSettings() const;
    void SaveSettings();
    FText GetRoleModeText() const;
    FText GetManualRoleText() const;
    EVisibility GetManualRoleSettingsVisibility() const;
    EVisibility GetManualSlaveSettingsVisibility() const;
    FText GetMasterIPText() const;
    FText GetConnectionStateText() const;
    FSlateColor GetConnectionStateColor() const;

    // 타임코드 매니저 관련 함수들
    void InitializeTimecodeManager();
    void ShutdownTimecodeManager();
    void StartTimecode();
    void StopTimecode();
    void ResetTimecode();

    // 멤버 변수들
    TArray<TSharedPtr<FString>> FrameRateOptions;
    TArray<TSharedPtr<FText>> RoleModeOptions;
    TSharedPtr<UTimecodeNetworkManager> TimecodeManager;
    TSharedPtr<UTimecodeSyncEditorDelegateHandler> DelegateHandler;
    FTSTicker::FDelegateHandle TickDelegateHandle;
    FString CurrentTimecode;
    FString CurrentRole;
    ENetworkConnectionState ConnectionState;
    bool bIsMaster;
    bool bIsRunning;
    FText StatusMessage;
}; 