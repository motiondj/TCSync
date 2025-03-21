#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "TimecodeSettings.h"
#include "TimecodeNetworkTypes.h"

/**
 * Ÿ���ڵ� ����ȭ ������ ���� ������ UI
 */
class STimecodeSyncEditorUI : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(STimecodeSyncEditorUI) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    // UI ������ ���� �Լ�
    TSharedRef<SWidget> CreateContentArea();

    // ���� ���� ���� ���� �Լ�
    TSharedRef<SWidget> CreateRoleSettingsSection();

    // ��Ʈ��ũ ���� ���� ���� �Լ�
    TSharedRef<SWidget> CreateNetworkSettingsSection();

    // Ÿ���ڵ� ���� ���� ���� �Լ�
    TSharedRef<SWidget> CreateTimecodeSettingsSection();

    // ���� ����͸� ���� ���� �Լ�
    TSharedRef<SWidget> CreateMonitoringSection();

    // UI ������Ʈ �Լ�
    void UpdateUI();

    // �ݹ� �Լ�
    void OnRoleModeChanged(ETimecodeRoleMode NewMode);
    void OnManualMasterChanged(bool bNewValue);
    void OnMasterIPAddressChanged(const FText& NewText, ETextCommit::Type CommitType);

    // ���� ���� �Լ�
    UTimecodeSettings* GetTimecodeSettings() const;
    void SaveSettings();

    // ���� ��� �ؽ�Ʈ ���
    FText GetRoleModeText() const;
    FText GetManualRoleText() const;

    // �ڵ�/���� ǥ�ÿ� ���� ���� ���ü� 
    EVisibility GetManualRoleSettingsVisibility() const;
    EVisibility GetManualSlaveSettingsVisibility() const;

    // Ÿ�̸� �ڵ�
    FDelegateHandle TickDelegateHandle;
};