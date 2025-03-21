#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "TimecodeNetworkTypes.h"
#include "TimecodeSettings.h"
#include "Widgets/Input/SSpinBox.h"
#include "TimecodeUtils.h"
#include "TimecodeNetworkManager.h"
#include "TimecodeSyncEditorDelegateHandler.h"

class UTimecodeNetworkManager;
class UTimecodeSyncEditorDelegateHandler;

class TIMECODESYNCEDITOR_API STimecodeSyncEditorUI : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(STimecodeSyncEditorUI)
    {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    ~STimecodeSyncEditorUI();

private:
    TSharedRef<SWidget> CreateContentArea();
    TSharedRef<SWidget> CreateRoleSettingsSection();
    TSharedRef<SWidget> CreateNetworkSettingsSection();
    TSharedRef<SWidget> CreateTimecodeSettingsSection();
    TSharedRef<SWidget> CreateMonitoringSection();

    void UpdateUI();
    UTimecodeSettings* GetTimecodeSettings() const;
    void SaveSettings();
    void OnRoleModeChanged(ETimecodeRoleMode NewMode);
    void OnManualMasterChanged(bool bNewValue);
    void OnMasterIPCommitted(const FText& NewText, ETextCommit::Type CommitType);
    FText GetRoleModeText() const;
    FText GetManualRoleText() const;
    FText GetMasterIPText() const;
    EVisibility GetManualRoleSettingsVisibility() const;
    EVisibility GetManualSlaveSettingsVisibility() const;

    // Timecode control functions
    void StartTimecode();
    void StopTimecode();
    void ResetTimecode();

    // Member variables
    FTSTicker::FDelegateHandle TickDelegateHandle;
    TArray<TSharedPtr<FText>> RoleModeOptions;
    TArray<TSharedPtr<FString>> FrameRateOptions;
    
    // State variables
    FString CurrentTimecode;
    ENetworkConnectionState ConnectionState;
    bool bIsMaster;
    bool bIsRunning;
    FText StatusMessage;

    // Network manager and delegate handler
    TSharedPtr<UTimecodeNetworkManager> TimecodeManager;
    TSharedPtr<UTimecodeSyncEditorDelegateHandler> DelegateHandler;

    // Event handlers
    void OnTimecodeMessageReceived(const FTimecodeNetworkMessage& Message);
    void OnNetworkStateChanged(ENetworkConnectionState NewState);

    // Timecode management functions
    void InitializeTimecodeManager();
    void ShutdownTimecodeManager();
}; 