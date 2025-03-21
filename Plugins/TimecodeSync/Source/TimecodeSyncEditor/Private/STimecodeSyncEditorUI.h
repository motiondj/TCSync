#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "TimecodeNetworkTypes.h"
#include "TimecodeSettings.h"

class TIMECODESYNCEDITOR_API STimecodeSyncEditorUI : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(STimecodeSyncEditorUI)
    {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

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
    void OnManualMasterChanged(bool bIsMaster);
    void OnMasterIPCommitted(const FText& NewText, ETextCommit::Type CommitType);
    FText GetRoleModeText() const;
    FText GetManualRoleText() const;
    FText GetMasterIPText() const;
    EVisibility GetManualRoleSettingsVisibility() const;
    EVisibility GetManualSlaveSettingsVisibility() const;

    FTSTicker::FDelegateHandle TickDelegateHandle;
    TArray<TSharedPtr<FText>> RoleModeOptions;
}; 