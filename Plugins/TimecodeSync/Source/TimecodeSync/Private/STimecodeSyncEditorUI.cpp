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
    // ���� UI ����
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

    // UI �ʱ�ȭ
    UpdateUI();

    // �ֱ��� ������Ʈ�� ���� ƽ ���
    TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateLambda(
            [this](float DeltaTime) -> bool
            {
                UpdateUI();
                return true; // ��� ƽ
            }
        ),
        0.5f // 0.5�ʸ��� ������Ʈ
    );
}

TSharedRef<SWidget> STimecodeSyncEditorUI::CreateContentArea()
{
    return SNew(SVerticalBox)

        // ����
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 0, 0, 10)
        [
            SNew(STextBlock)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
                .Text(LOCTEXT("TimecodeSyncTitle", "Timecode Sync Settings"))
        ]

        // ���� ���� ����
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 5)
        [
            CreateRoleSettingsSection()
        ]

    // ��Ʈ��ũ ���� ����
    + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 5)
        [
            CreateNetworkSettingsSection()
        ]

    // Ÿ���ڵ� ���� ����
    + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 5)
        [
            CreateTimecodeSettingsSection()
        ]

    // ����͸� ����
    + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 5)
        [
            CreateMonitoringSection()
        ];
}

TSharedRef<SWidget> STimecodeSyncEditorUI::CreateRoleSettingsSection()
{
    // ���� ��� ������ ���� �޺��ڽ� �ɼ�
    TArray<TSharedPtr<FText>> RoleModeOptions;
    RoleModeOptions.Add(MakeShareable(new FText(LOCTEXT("AutomaticMode", "Automatic Detection"))));
    RoleModeOptions.Add(MakeShareable(new FText(LOCTEXT("ManualMode", "Manual Setting"))));

    return SNew(SBorder)
        .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            SNew(SVerticalBox)

                // ���� ����
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 5)
                [
                    SNew(STextBlock)
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                        .Text(LOCTEXT("RoleSettingsTitle", "Role Settings"))
                ]

                // ���� ��� ����
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SHorizontalBox)

                        // ��
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0, 0, 10, 0)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT("RoleMode", "Role Mode:"))
                                .MinDesiredWidth(120)
                        ]

                        // �޺��ڽ�
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

                // ���� ���� ���� (���Ǻ� ǥ��)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                .Expose()
                [
                    SNew(SBox)
                        .Visibility(this, &STimecodeSyncEditorUI::GetManualRoleSettingsVisibility)
                        [
                            SNew(SHorizontalBox)

                                // ��
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(0, 0, 10, 0)
                                [
                                    SNew(STextBlock)
                                        .Text(LOCTEXT("ManualRole", "Role:"))
                                        .MinDesiredWidth(120)
                                ]

                                // üũ�ڽ� (Master)
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

                                // Master �ؽ�Ʈ
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

                // ���� �����̺� ��� IP ���� (���Ǻ� ǥ��)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SBox)
                        .Visibility(this, &STimecodeSyncEditorUI::GetManualSlaveSettingsVisibility)
                        [
                            SNew(SHorizontalBox)

                                // ��
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(0, 0, 10, 0)
                                [
                                    SNew(STextBlock)
                                        .Text(LOCTEXT("MasterIP", "Master IP:"))
                                        .MinDesiredWidth(120)
                                ]

                                // IP �Է� �ʵ�
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

                // ���� ���� ǥ��
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
    // ������ ��Ʈ��ũ ���� ���� ����
    return SNew(SBorder)
        .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            SNew(SVerticalBox)

                // ���� ����
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 5)
                [
                    SNew(STextBlock)
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                        .Text(LOCTEXT("NetworkSettingsTitle", "Network Settings"))
                ]

                // ���⿡ ��Ʈ��ũ ���� UI �߰�
                // (UDP ��Ʈ, ��Ƽĳ��Ʈ �׷� ��)
        ];
}

TSharedRef<SWidget> STimecodeSyncEditorUI::CreateTimecodeSettingsSection()
{
    // ������ Ÿ���ڵ� ���� ���� ����
    return SNew(SBorder)
        .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            SNew(SVerticalBox)

                // ���� ����
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 5)
                [
                    SNew(STextBlock)
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                        .Text(LOCTEXT("TimecodeSettingsTitle", "Timecode Settings"))
                ]

                // ���⿡ Ÿ���ڵ� ���� UI �߰�
                // (������ ����Ʈ, ��� ������ ��)
        ];
}

TSharedRef<SWidget> STimecodeSyncEditorUI::CreateMonitoringSection()
{
    // ������ ����͸� ���� ����
    return SNew(SBorder)
        .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            SNew(SVerticalBox)

                // ���� ����
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 5)
                [
                    SNew(STextBlock)
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                        .Text(LOCTEXT("MonitoringTitle", "Status Monitoring"))
                ]

                // ���⿡ ����͸� UI �߰�
                // (���� ����, ���� ����, ���� Ÿ���ڵ� ��)
        ];
}

void STimecodeSyncEditorUI::UpdateUI()
{
    // UI ������Ʈ
}

void STimecodeSyncEditorUI::OnRoleModeChanged(ETimecodeRoleMode NewMode)
{
    UTimecodeSettings* Settings = GetTimecodeSettings();
    if (Settings)
    {
        Settings->RoleMode = NewMode;
        SaveSettings();

        // UI ������Ʈ
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

        // UI ������Ʈ
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

            // UI ������Ʈ
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