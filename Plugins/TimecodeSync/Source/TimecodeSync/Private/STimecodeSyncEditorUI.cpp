#include "STimecodeSyncEditorUI.h"
#include "TimecodeSettings.h"
#include "EditorStyleSet.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "STimecodeSyncEditorUI"

void STimecodeSyncEditorUI::Construct(const FArguments& InArgs)
{
    // 메인 UI 생성
    ChildSlot
        [
            SNew(SBorder)
                .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
                .Padding(4.0f)
                [
                    SNew(SScrollBox)
                        + SScrollBox::Slot()
                        [
                            CreateContentArea()
                        ]
                ]
        ];

    // UI 초기화
    UpdateUI();

    // 주기적 업데이트를 위한 틱 등록
    TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateLambda(
            [this](float DeltaTime) -> bool
            {
                UpdateUI();
                return true; // 계속 틱
            }
        ),
        0.5f // 0.5초마다 업데이트
    );
}

TSharedRef<SWidget> STimecodeSyncEditorUI::CreateContentArea()
{
    return SNew(SVerticalBox)

        // 제목
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 0, 0, 10)
        [
            SNew(STextBlock)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
                .Text(LOCTEXT("TimecodeSyncTitle", "Timecode Sync Settings"))
        ]

        // 역할 설정 섹션
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 5)
        [
            CreateRoleSettingsSection()
        ]

    // 네트워크 설정 섹션
    + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 5)
        [
            CreateNetworkSettingsSection()
        ]

    // 타임코드 설정 섹션
    + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 5)
        [
            CreateTimecodeSettingsSection()
        ]

    // 모니터링 섹션
    + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 5)
        [
            CreateMonitoringSection()
        ];
}

TSharedRef<SWidget> STimecodeSyncEditorUI::CreateRoleSettingsSection()
{
    // 역할 모드 선택을 위한 콤보박스 옵션
    TArray<TSharedPtr<FText>> RoleModeOptions;
    RoleModeOptions.Add(MakeShareable(new FText(LOCTEXT("AutomaticMode", "Automatic Detection"))));
    RoleModeOptions.Add(MakeShareable(new FText(LOCTEXT("ManualMode", "Manual Setting"))));

    return SNew(SBorder)
        .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            SNew(SVerticalBox)

                // 섹션 제목
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 5)
                [
                    SNew(STextBlock)
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                        .Text(LOCTEXT("RoleSettingsTitle", "Role Settings"))
                ]

                // 역할 모드 선택
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SHorizontalBox)

                        // 라벨
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0, 0, 10, 0)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT("RoleMode", "Role Mode:"))
                                .MinDesiredWidth(120)
                        ]

                        // 콤보박스
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SComboBox<TSharedPtr<FText>>)
                                .OptionsSource(&RoleModeOptions)
                                .OnSelectionChanged_Lambda([this](TSharedPtr<FText> NewValue, ESelectInfo::Type SelectType)
                                    {
                                        if (SelectType != ESelectInfo::Direct)
                                        {
                                            UTimecodeSettings* Settings = GetTimecodeSettings();
                                            if (Settings)
                                            {
                                                int32 Index = RoleModeOptions.IndexOfByPredicate([NewValue](const TSharedPtr<FText>& Option)
                                                    {
                                                        return Option->EqualTo(*NewValue);
                                                    });

                                                ETimecodeRoleMode NewMode = Index == 1 ?
                                                    ETimecodeRoleMode::Manual : ETimecodeRoleMode::Automatic;

                                                OnRoleModeChanged(NewMode);
                                            }
                                        }
                                    })
                                .OnGenerateWidget_Lambda([](TSharedPtr<FText> InOption)
                                    {
                                        return SNew(STextBlock).Text(*InOption);
                                    })
                                [
                                    SNew(STextBlock)
                                        .Text(this, &STimecodeSyncEditorUI::GetRoleModeText)
                                ]
                        ]
                ]

                // 수동 역할 설정 (조건부 표시)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                .Expose()
                [
                    SNew(SBox)
                        .Visibility(this, &STimecodeSyncEditorUI::GetManualRoleSettingsVisibility)
                        [
                            SNew(SHorizontalBox)

                                // 라벨
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(0, 0, 10, 0)
                                [
                                    SNew(STextBlock)
                                        .Text(LOCTEXT("ManualRole", "Role:"))
                                        .MinDesiredWidth(120)
                                ]

                                // 체크박스 (Master)
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                [
                                    SNew(SCheckBox)
                                        .IsChecked_Lambda([this]()
                                            {
                                                UTimecodeSettings* Settings = GetTimecodeSettings();
                                                return Settings && Settings->bIsManualMaster ?
                                                    ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                                            })
                                        .OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
                                            {
                                                OnManualMasterChanged(NewState == ECheckBoxState::Checked);
                                            })
                                ]

                                // Master 텍스트
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(5, 0)
                                [
                                    SNew(STextBlock)
                                        .Text(LOCTEXT("MasterRole", "Master"))
                                ]
                        ]
                ]

                // 수동 슬레이브 모드 IP 설정 (조건부 표시)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SBox)
                        .Visibility(this, &STimecodeSyncEditorUI::GetManualSlaveSettingsVisibility)
                        [
                            SNew(SHorizontalBox)

                                // 라벨
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(0, 0, 10, 0)
                                [
                                    SNew(STextBlock)
                                        .Text(LOCTEXT("MasterIP", "Master IP:"))
                                        .MinDesiredWidth(120)
                                ]

                                // IP 입력 필드
                                + SHorizontalBox::Slot()
                                .FillWidth(1.0f)
                                [
                                    SNew(SEditableTextBox)
                                        .Text_Lambda([this]()
                                            {
                                                UTimecodeSettings* Settings = GetTimecodeSettings();
                                                return FText::FromString(Settings ? Settings->MasterIPAddress : TEXT(""));
                                            })
                                        .OnTextCommitted(this, &STimecodeSyncEditorUI::OnMasterIPAddressChanged)
                                        .ToolTipText(LOCTEXT("MasterIPTooltip", "IP address of the Master node (e.g. 192.168.1.100)"))
                                ]
                        ]
                ]

                // 현재 역할 표시
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 10, 0, 0)
                [
                    SNew(STextBlock)
                        .Text_Lambda([this]()
                            {
                                UTimecodeSettings* Settings = GetTimecodeSettings();
                                if (!Settings)
                                    return FText::FromString(TEXT("Current role: Unknown"));

                                if (Settings->RoleMode == ETimecodeRoleMode::Automatic)
                                    return FText::FromString(TEXT("Current role: Auto-detected (run-time determined)"));
                                else
                                    return FText::FromString(FString::Printf(
                                        TEXT("Current role: %s"),
                                        Settings->bIsManualMaster ? TEXT("MASTER") : TEXT("SLAVE")));
                            })
                        .ColorAndOpacity(FLinearColor(0.5f, 0.5f, 1.0f))
                ]
        ];
}

TSharedRef<SWidget> STimecodeSyncEditorUI::CreateNetworkSettingsSection()
{
    // 간단한 네트워크 설정 섹션 구현
    return SNew(SBorder)
        .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            SNew(SVerticalBox)

                // 섹션 제목
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 5)
                [
                    SNew(STextBlock)
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                        .Text(LOCTEXT("NetworkSettingsTitle", "Network Settings"))
                ]

                // 여기에 네트워크 설정 UI 추가
                // (UDP 포트, 멀티캐스트 그룹 등)
        ];
}

TSharedRef<SWidget> STimecodeSyncEditorUI::CreateTimecodeSettingsSection()
{
    // 간단한 타임코드 설정 섹션 구현
    return SNew(SBorder)
        .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            SNew(SVerticalBox)

                // 섹션 제목
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 5)
                [
                    SNew(STextBlock)
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                        .Text(LOCTEXT("TimecodeSettingsTitle", "Timecode Settings"))
                ]

                // 여기에 타임코드 설정 UI 추가
                // (프레임 레이트, 드롭 프레임 등)
        ];
}

TSharedRef<SWidget> STimecodeSyncEditorUI::CreateMonitoringSection()
{
    // 간단한 모니터링 섹션 구현
    return SNew(SBorder)
        .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            SNew(SVerticalBox)

                // 섹션 제목
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 5)
                [
                    SNew(STextBlock)
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                        .Text(LOCTEXT("MonitoringTitle", "Status Monitoring"))
                ]

                // 여기에 모니터링 UI 추가
                // (현재 역할, 연결 상태, 현재 타임코드 등)
        ];
}

void STimecodeSyncEditorUI::UpdateUI()
{
    // UI 업데이트
}

void STimecodeSyncEditorUI::OnRoleModeChanged(ETimecodeRoleMode NewMode)
{
    UTimecodeSettings* Settings = GetTimecodeSettings();
    if (Settings)
    {
        Settings->RoleMode = NewMode;
        SaveSettings();

        // UI 업데이트
        UpdateUI();
    }
}

void STimecodeSyncEditorUI::OnManualMasterChanged(bool bNewValue)
{
    UTimecodeSettings* Settings = GetTimecodeSettings();
    if (Settings)
    {
        Settings->bIsManualMaster = bNewValue;
        SaveSettings();

        // UI 업데이트
        UpdateUI();
    }
}

void STimecodeSyncEditorUI::OnMasterIPAddressChanged(const FText& NewText, ETextCommit::Type CommitType)
{
    if (CommitType == ETextCommit::OnEnter || CommitType == ETextCommit::OnUserMovedFocus)
    {
        UTimecodeSettings* Settings = GetTimecodeSettings();
        if (Settings)
        {
            Settings->MasterIPAddress = NewText.ToString();
            SaveSettings();

            // UI 업데이트
            UpdateUI();
        }
    }
}

UTimecodeSettings* STimecodeSyncEditorUI::GetTimecodeSettings() const
{
    return GetMutableDefault<UTimecodeSettings>();
}

void STimecodeSyncEditorUI::SaveSettings()
{
    UTimecodeSettings* Settings = GetTimecodeSettings();
    if (Settings)
    {
        Settings->SaveConfig();
    }
}

FText STimecodeSyncEditorUI::GetRoleModeText() const
{
    UTimecodeSettings* Settings = GetTimecodeSettings();
    if (Settings)
    {
        return Settings->RoleMode == ETimecodeRoleMode::Manual ?
            LOCTEXT("ManualMode", "Manual Setting") : LOCTEXT("AutomaticMode", "Automatic Detection");
    }

    return LOCTEXT("AutomaticMode", "Automatic Detection");
}

FText STimecodeSyncEditorUI::GetManualRoleText() const
{
    UTimecodeSettings* Settings = GetTimecodeSettings();
    if (Settings)
    {
        return Settings->bIsManualMaster ?
            LOCTEXT("MasterRole", "Master") : LOCTEXT("SlaveRole", "Slave");
    }

    return LOCTEXT("MasterRole", "Master");
}

EVisibility STimecodeSyncEditorUI::GetManualRoleSettingsVisibility() const
{
    UTimecodeSettings* Settings = GetTimecodeSettings();
    if (Settings && Settings->RoleMode == ETimecodeRoleMode::Manual)
    {
        return EVisibility::Visible;
    }

    return EVisibility::Collapsed;
}

EVisibility STimecodeSyncEditorUI::GetManualSlaveSettingsVisibility() const
{
    UTimecodeSettings* Settings = GetTimecodeSettings();
    if (Settings && Settings->RoleMode == ETimecodeRoleMode::Manual && !Settings->bIsManualMaster)
    {
        return EVisibility::Visible;
    }

    return EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE