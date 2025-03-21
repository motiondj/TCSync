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
    // Create main UI
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

    // Initialize UI
    UpdateUI();

    // Register tick for periodic updates
    TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateLambda(
            [this](float DeltaTime) -> bool
            {
                UpdateUI();
                return true; // Continue tick
            }
        ),
        0.5f // Update every 0.5 seconds
    );
}

TSharedRef<SWidget> STimecodeSyncEditorUI::CreateContentArea()
{
    return SNew(SVerticalBox)

        // Title
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 0, 0, 10)
        [
            SNew(STextBlock)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
                .Text(LOCTEXT("TimecodeSyncTitle", "Timecode Sync Settings"))
        ]

        // Role settings section
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 5)
        [
            CreateRoleSettingsSection()
        ]

        // Network settings section
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 5)
        [
            CreateNetworkSettingsSection()
        ]

        // Timecode settings section
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 5)
        [
            CreateTimecodeSettingsSection()
        ]

        // Monitoring section
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 5)
        [
            CreateMonitoringSection()
        ];
}

TSharedRef<SWidget> STimecodeSyncEditorUI::CreateRoleSettingsSection()
{
    // ComboBox options for role mode selection
    TArray<TSharedPtr<FText>> RoleModeOptions;
    RoleModeOptions.Add(MakeShareable(new FText(LOCTEXT("AutomaticMode", "Automatic Detection"))));
    RoleModeOptions.Add(MakeShareable(new FText(LOCTEXT("ManualMode", "Manual Setting"))));

    return SNew(SBorder)
        .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            SNew(SVerticalBox)

                // Section title
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 5)
                [
                    SNew(STextBlock)
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                        .Text(LOCTEXT("RoleSettingsTitle", "Role Settings"))
                ]

                // Role mode selection
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SHorizontalBox)

                        // Label
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0, 0, 10, 0)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT("RoleMode", "Role Mode:"))
                                .MinDesiredWidth(120)
                        ]

                        // ComboBox
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

                // Manual role settings (conditional display)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                .Expose()
                [
                    SNew(SBox)
                        .Visibility(this, &STimecodeSyncEditorUI::GetManualRoleSettingsVisibility)
                        [
                            SNew(SHorizontalBox)

                                // Label
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(0, 0, 10, 0)
                                [
                                    SNew(STextBlock)
                                        .Text(LOCTEXT("ManualRole", "Role:"))
                                        .MinDesiredWidth(120)
                                ]

                                // CheckBox (Master)
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

                                // Master text
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

                // Manual slave mode IP settings (conditional display)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SBox)
                        .Visibility(this, &STimecodeSyncEditorUI::GetManualSlaveSettingsVisibility)
                        [
                            SNew(SHorizontalBox)

                                // Label
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(0, 0, 10, 0)
                                [
                                    SNew(STextBlock)
                                        .Text(LOCTEXT("MasterIP", "Master IP:"))
                                        .MinDesiredWidth(120)
                                ]

                                // IP input field
                                + SHorizontalBox::Slot()
                                .FillWidth(1.0f)
                                [
                                    SNew(SEditableTextBox)
                                        .Text(this, &STimecodeSyncEditorUI::GetMasterIPText)
                                        .OnTextCommitted(this, &STimecodeSyncEditorUI::OnMasterIPCommitted)
                                        .HintText(LOCTEXT("MasterIPHint", "Enter master IP address"))
                                ]
                        ]
                ]
        ];
}

TSharedRef<SWidget> STimecodeSyncEditorUI::CreateNetworkSettingsSection()
{
    // Simple network settings section implementation
    return SNew(SBorder)
        .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            SNew(SVerticalBox)

                // Section title
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 5)
                [
                    SNew(STextBlock)
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                        .Text(LOCTEXT("NetworkSettingsTitle", "Network Settings"))
                ]

                // Add network settings UI here (UDP port, multicast group, etc.)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SBox)
                ]
        ];
}

TSharedRef<SWidget> STimecodeSyncEditorUI::CreateTimecodeSettingsSection()
{
    // Simple timecode settings section implementation
    return SNew(SBorder)
        .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            SNew(SVerticalBox)

                // Section title
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 5)
                [
                    SNew(STextBlock)
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                        .Text(LOCTEXT("TimecodeSettingsTitle", "Timecode Settings"))
                ]

                // Add timecode settings UI here (frame rate, drop frame, etc.)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SBox)
                ]
        ];
}

TSharedRef<SWidget> STimecodeSyncEditorUI::CreateMonitoringSection()
{
    // Simple monitoring section implementation
    return SNew(SBorder)
        .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            SNew(SVerticalBox)

                // Section title
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 5)
                [
                    SNew(STextBlock)
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                        .Text(LOCTEXT("MonitoringSettingsTitle", "Monitoring"))
                ]

                // Add monitoring UI here (connection status, current timecode, etc.)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SBox)
                ]
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