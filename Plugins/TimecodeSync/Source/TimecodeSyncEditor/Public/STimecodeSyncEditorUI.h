#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "TimecodeSettings.h"
#include "TimecodeNetworkTypes.h"

/**
 * Editor UI for timecode synchronization settings
 */
class STimecodeSyncEditorUI : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(STimecodeSyncEditorUI) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    // UI content creation functions
    TSharedRef<SWidget> CreateContentArea();

    // Role settings section creation function
    TSharedRef<SWidget> CreateRoleSettingsSection();

    // Network settings section creation function
    TSharedRef<SWidget> CreateNetworkSettingsSection();

    // Timecode settings section creation function
    TSharedRef<SWidget> CreateTimecodeSettingsSection();

    // Status monitoring section creation function
    TSharedRef<SWidget> CreateMonitoringSection();

    // UI update function
    void UpdateUI();

    // Callback functions
    void OnRoleModeChanged(ETimecodeRoleMode NewMode);
    void OnManualMasterChanged(bool bNewValue);
    void OnMasterIPAddressChanged(const FText& NewText, ETextCommit::Type CommitType);

    // Settings related functions
    UTimecodeSettings* GetTimecodeSettings() const;
    void SaveSettings();

    // Get role mode text
    FText GetRoleModeText() const;
    FText GetManualRoleText() const;

    // Visibility for automatic/manual detail sections
    EVisibility GetManualRoleSettingsVisibility() const;
    EVisibility GetManualSlaveSettingsVisibility() const;

    // Timer handle
    FDelegateHandle TickDelegateHandle;
};