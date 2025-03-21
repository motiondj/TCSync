#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "TimecodeSettings.h"
#include "TimecodeNetworkTypes.h"

/**
 * 타임코드 동기화 설정을 위한 에디터 UI
 */
class STimecodeSyncEditorUI : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(STimecodeSyncEditorUI) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    // UI 콘텐츠 생성 함수
    TSharedRef<SWidget> CreateContentArea();

    // 역할 설정 섹션 생성 함수
    TSharedRef<SWidget> CreateRoleSettingsSection();

    // 네트워크 설정 섹션 생성 함수
    TSharedRef<SWidget> CreateNetworkSettingsSection();

    // 타임코드 설정 섹션 생성 함수
    TSharedRef<SWidget> CreateTimecodeSettingsSection();

    // 상태 모니터링 섹션 생성 함수
    TSharedRef<SWidget> CreateMonitoringSection();

    // UI 업데이트 함수
    void UpdateUI();

    // 콜백 함수
    void OnRoleModeChanged(ETimecodeRoleMode NewMode);
    void OnManualMasterChanged(bool bNewValue);
    void OnMasterIPAddressChanged(const FText& NewText, ETextCommit::Type CommitType);

    // 설정 관련 함수
    UTimecodeSettings* GetTimecodeSettings() const;
    void SaveSettings();

    // 역할 모드 텍스트 얻기
    FText GetRoleModeText() const;
    FText GetManualRoleText() const;

    // 자동/수동 표시용 세부 섹션 가시성 
    EVisibility GetManualRoleSettingsVisibility() const;
    EVisibility GetManualSlaveSettingsVisibility() const;

    // 타이머 핸들
    FDelegateHandle TickDelegateHandle;
};