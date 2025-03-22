#include "STimecodeSyncEditorUI.h"
#include "TimecodeSettings.h"
#include "Styling/AppStyle.h"
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
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SButton.h"
#include "TimecodeUtils.h"

#define LOCTEXT_NAMESPACE "STimecodeSyncEditorUI"

void STimecodeSyncEditorUI::Construct(const FArguments& InArgs)
{
    // 프레임 레이트 옵션 초기화
    FrameRateOptions.Add(MakeShareable(new FString("24.00")));
    FrameRateOptions.Add(MakeShareable(new FString("25.00")));
    FrameRateOptions.Add(MakeShareable(new FString("29.97")));
    FrameRateOptions.Add(MakeShareable(new FString("30.00")));
    FrameRateOptions.Add(MakeShareable(new FString("60.00")));

    // 기본값 초기화
    CurrentTimecode = TEXT("00:00:00:00");
    ConnectionState = ENetworkConnectionState::Disconnected;
    bIsMaster = false;
    bIsRunning = false;
    StatusMessage = FText::FromString(TEXT("Initializing..."));
    CurrentRole = TEXT("Initializing...");
    TimecodeManager = nullptr;
    DelegateHandler = nullptr;

    // Create main UI
    ChildSlot
        [
            SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                .Padding(4.0f)
                [
                    SNew(SScrollBox)
                        + SScrollBox::Slot()
                        [
                            CreateContentArea()
                        ]
                ]
        ];

    // 타임코드 매니저 초기화
    InitializeTimecodeManager();

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

STimecodeSyncEditorUI::~STimecodeSyncEditorUI()
{
    // 타이머 중지 먼저 수행하여 UI 업데이트 멈춤
    if (TickDelegateHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
        TickDelegateHandle.Reset();
    }

    // 타임코드 매니저 종료
    ShutdownTimecodeManager();
}

void STimecodeSyncEditorUI::ShutdownTimecodeManager()
{
    // 델리게이트 등록 해제
    if (DelegateHandler)
    {
        DelegateHandler->OnTimecodeMessageReceived.Unbind();
        DelegateHandler->OnNetworkStateChanged.Unbind();
    }

    // 타임코드 매니저의 델리게이트 등록 해제
    if (TimecodeManager && DelegateHandler)
    {
        TimecodeManager->OnTimecodeMessageReceived.RemoveAll(DelegateHandler);
        TimecodeManager->OnNetworkStateChanged.RemoveAll(DelegateHandler);
    }

    // 타임코드 매니저 종료 호출
    if (TimecodeManager)
    {
        TimecodeManager->Shutdown();
        TimecodeManager = nullptr;
    }

    // 델리게이트 핸들러 제거
    if (DelegateHandler)
    {
        DelegateHandler = nullptr;
    }
}

TSharedRef<SWidget> STimecodeSyncEditorUI::CreateContentArea()
{
    return SNew(SVerticalBox)

        // Title
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 0, 0, 10)
        [
            SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
                        .Text(LOCTEXT("TimecodeSyncTitle", "Timecode Sync Settings"))
                ]
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
    // Initialize role mode options if not already done
    if (RoleModeOptions.Num() == 0)
    {
        RoleModeOptions.Add(MakeShareable(new FText(LOCTEXT("AutomaticMode", "Automatic Detection"))));
        RoleModeOptions.Add(MakeShareable(new FText(LOCTEXT("ManualMode", "Manual Setting"))));
    }

    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            SNew(SVerticalBox)

                // Section title
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 5)
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                                .Text(LOCTEXT("RoleSettingsTitle", "Role Settings"))
                        ]
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
                                                const int32 Index = RoleModeOptions.IndexOfByPredicate([NewValue](const TSharedPtr<FText>& Option)
                                                    {
                                                        return Option->EqualTo(*NewValue);
                                                    });

                                                const ETimecodeRoleMode NewMode = (Index == 1) ?
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
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            SNew(SVerticalBox)

                // Section title
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 5)
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                                .Text(LOCTEXT("NetworkSettingsTitle", "Network Settings"))
                        ]
                ]

                // UDP Port
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0, 0, 10, 0)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT("UDPPort", "UDP Port:"))
                                .MinDesiredWidth(120)
                        ]
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SSpinBox<int32>)
                                .MinValue(1024)
                                .MaxValue(65535)
                                .Value_Lambda([this]() -> int32 {
                                UTimecodeSettings* Settings = GetTimecodeSettings();
                                return Settings ? Settings->DefaultUDPPort : 10000;
                                    })
                                .OnValueChanged_Lambda([this](int32 NewValue) {
                                UTimecodeSettings* Settings = GetTimecodeSettings();
                                if (Settings) {
                                    Settings->DefaultUDPPort = NewValue;
                                    SaveSettings();
                                }
                                    })
                        ]
                ]

                // Multicast Group
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0, 0, 10, 0)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT("MulticastGroup", "Multicast Group:"))
                                .MinDesiredWidth(120)
                        ]
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SEditableTextBox)
                                .Text_Lambda([this]() -> FText {
                                UTimecodeSettings* Settings = GetTimecodeSettings();
                                return FText::FromString(Settings ? Settings->MulticastGroupAddress : TEXT("239.0.0.1"));
                                    })
                                .OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type CommitType) {
                                if (CommitType == ETextCommit::OnEnter || CommitType == ETextCommit::OnUserMovedFocus) {
                                    UTimecodeSettings* Settings = GetTimecodeSettings();
                                    if (Settings) {
                                        Settings->MulticastGroupAddress = NewText.ToString();
                                        SaveSettings();
                                    }
                                }
                                    })
                                .HintText(LOCTEXT("MulticastGroupHint", "Enter multicast group address (e.g., 239.0.0.1)"))
                        ]
                ]

                // Broadcast Interval
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0, 0, 10, 0)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT("SyncInterval", "Sync Interval (sec):"))
                                .MinDesiredWidth(120)
                        ]
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SSpinBox<float>)
                                .MinValue(0.001f)
                                .MaxValue(1.0f)
                                .Delta(0.001f)
                                .Value_Lambda([this]() -> float {
                                UTimecodeSettings* Settings = GetTimecodeSettings();
                                return Settings ? Settings->BroadcastInterval : 0.033f;
                                    })
                                .OnValueChanged_Lambda([this](float NewValue) {
                                UTimecodeSettings* Settings = GetTimecodeSettings();
                                if (Settings) {
                                    Settings->BroadcastInterval = NewValue;
                                    SaveSettings();
                                }
                                    })
                        ]
                ]
        ];
}

TSharedRef<SWidget> STimecodeSyncEditorUI::CreateTimecodeSettingsSection()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            SNew(SVerticalBox)

                // Section title
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 5)
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                                .Text(LOCTEXT("TimecodeSettingsTitle", "Timecode Settings"))
                        ]
                ]

                // Frame Rate
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0, 0, 10, 0)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT("FrameRate", "Frame Rate:"))
                                .MinDesiredWidth(120)
                        ]
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SComboBox<TSharedPtr<FString>>)
                                .OptionsSource(&FrameRateOptions)
                                .OnGenerateWidget_Lambda([](TSharedPtr<FString> InOption) {
                                return SNew(STextBlock).Text(FText::FromString(*InOption));
                                    })
                                .OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewValue, ESelectInfo::Type SelectType) {
                                if (NewValue.IsValid() && SelectType != ESelectInfo::Direct) {
                                    UTimecodeSettings* Settings = GetTimecodeSettings();
                                    if (Settings) {
                                        Settings->FrameRate = FCString::Atof(**NewValue);
                                        SaveSettings();
                                    }
                                }
                                    })
                                .Content()
                                [
                                    SNew(STextBlock)
                                        .Text_Lambda([this]() -> FText {
                                        UTimecodeSettings* Settings = GetTimecodeSettings();
                                        return FText::FromString(FString::Printf(TEXT("%.2f"), Settings ? Settings->FrameRate : 30.0f));
                                            })
                                ]
                        ]
                ]

                // Drop Frame Timecode
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0, 0, 10, 0)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT("DropFrameTimecode", "Use Drop Frame:"))
                                .MinDesiredWidth(120)
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        [
                            SNew(SCheckBox)
                                .IsChecked_Lambda([this]() -> ECheckBoxState {
                                UTimecodeSettings* Settings = GetTimecodeSettings();
                                return Settings && Settings->bUseDropFrameTimecode ?
                                    ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                                    })
                                .OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) {
                                UTimecodeSettings* Settings = GetTimecodeSettings();
                                if (Settings) {
                                    Settings->bUseDropFrameTimecode = (NewState == ECheckBoxState::Checked);
                                    SaveSettings();
                                }
                                    })
                        ]
                ]

                // Auto Start Timecode
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0, 0, 10, 0)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT("AutoStartTimecode", "Auto Start:"))
                                .MinDesiredWidth(120)
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        [
                            SNew(SCheckBox)
                                .IsChecked_Lambda([this]() -> ECheckBoxState {
                                UTimecodeSettings* Settings = GetTimecodeSettings();
                                return Settings && Settings->bAutoStartTimecode ?
                                    ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                                    })
                                .OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) {
                                UTimecodeSettings* Settings = GetTimecodeSettings();
                                if (Settings) {
                                    Settings->bAutoStartTimecode = (NewState == ECheckBoxState::Checked);
                                    SaveSettings();
                                }
                                    })
                        ]
                ]
        ];
}

TSharedRef<SWidget> STimecodeSyncEditorUI::CreateMonitoringSection()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            SNew(SVerticalBox)

                // Section title
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 5)
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                                .Text(LOCTEXT("MonitoringTitle", "Monitoring"))
                        ]
                ]

                // Current Timecode
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0, 0, 10, 0)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT("CurrentTimecode", "Current Timecode:"))
                                .MinDesiredWidth(120)
                        ]
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(STextBlock)
                                .Text_Lambda([this]() -> FText {
                                return FText::FromString(CurrentTimecode);
                                    })
                                .ColorAndOpacity(FSlateColor(FLinearColor(0.1f, 0.8f, 0.1f)))
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
                        ]
                ]

                // Connection Status
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0, 0, 10, 0)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT("ConnectionStatus", "Connection Status:"))
                                .MinDesiredWidth(120)
                        ]
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(STextBlock)
                                .Text(this, &STimecodeSyncEditorUI::GetConnectionStateText)
                                .ColorAndOpacity(this, &STimecodeSyncEditorUI::GetConnectionStateColor)
                        ]
                ]

                // Role Status
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0, 0, 10, 0)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT("CurrentRole", "Current Role:"))
                                .MinDesiredWidth(120)
                        ]
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(STextBlock)
                                .Text_Lambda([this]() -> FText {
                                return FText::FromString(CurrentRole);
                                    })
                                .ColorAndOpacity_Lambda([this]() -> FSlateColor {
                                return bIsMaster ?
                                    FSlateColor(FLinearColor(0.8f, 0.2f, 0.2f)) : // Red for Master
                                    FSlateColor(FLinearColor(0.2f, 0.2f, 0.8f));  // Blue for Slave
                                    })
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                        ]
                ]

                // Control Buttons
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 10)
                [
                    SNew(SHorizontalBox)

                        // Start Button
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(0, 0, 5, 0)
                        [
                            SNew(SButton)
                                .Text(LOCTEXT("StartTimecode", "Start"))
                                .OnClicked_Lambda([this]() -> FReply {
                                StartTimecode();
                                return FReply::Handled();
                                    })
                        ]

                        // Stop Button
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(5, 0)
                        [
                            SNew(SButton)
                                .Text(LOCTEXT("StopTimecode", "Stop"))
                                .OnClicked_Lambda([this]() -> FReply {
                                StopTimecode();
                                return FReply::Handled();
                                    })
                        ]

                        // Reset Button
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(5, 0)
                        [
                            SNew(SButton)
                                .Text(LOCTEXT("ResetTimecode", "Reset"))
                                .OnClicked_Lambda([this]() -> FReply {
                                ResetTimecode();
                                return FReply::Handled();
                                    })
                        ]
                ]

                    // Status Text
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 5)
                    [
                        SNew(STextBlock)
                            .Text_Lambda([this]() -> FText {
                            return StatusMessage;
                                })
                            .ColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)))
                    ]
       ];
}

void STimecodeSyncEditorUI::UpdateUI()
{
    // 포인터 유효성 체크
    if (!TimecodeManager)
    {
        // 유효하지 않은 경우 기본값 설정
        CurrentTimecode = TEXT("--:--:--:--");
        ConnectionState = ENetworkConnectionState::Disconnected;
        bIsMaster = false;
        CurrentRole = TEXT("Not Connected");
        StatusMessage = FText::FromString(TEXT("Manager not available"));
        return;
    }

    // 네트워크 매니저가 연결 상태인지 확인
    ConnectionState = TimecodeManager->GetConnectionState();

    // 역할 업데이트
    bIsMaster = TimecodeManager->IsMaster();
    CurrentRole = bIsMaster ? TEXT("MASTER") : TEXT("SLAVE");

    // 타임코드 업데이트
    FString ManagerTimecode = TimecodeManager->GetCurrentTimecode();
    if (!ManagerTimecode.IsEmpty())
    {
        CurrentTimecode = ManagerTimecode;
    }

    // 상태 메시지 업데이트
    if (ConnectionState == ENetworkConnectionState::Connected)
    {
        StatusMessage = FText::FromString(TEXT("Connected"));
    }
    else if (ConnectionState == ENetworkConnectionState::Connecting)
    {
        StatusMessage = FText::FromString(TEXT("Connecting..."));
    }
    else
    {
        StatusMessage = FText::FromString(TEXT("Disconnected"));
    }
}

void STimecodeSyncEditorUI::OnRoleModeChanged(ETimecodeRoleMode NewMode)
{
    UTimecodeSettings* Settings = GetTimecodeSettings();
    if (Settings)
    {
        Settings->RoleMode = NewMode;
        SaveSettings();

        // 타임코드 매니저에 역할 모드 변경 적용
        if (TimecodeManager)
        {
            TimecodeManager->SetRoleMode(NewMode);
        }

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

        // 타임코드 매니저에 마스터 설정 변경 적용
        if (TimecodeManager)
        {
            TimecodeManager->SetManualMaster(bNewValue);
        }

        // UI 업데이트
        UpdateUI();
    }
}

void STimecodeSyncEditorUI::OnMasterIPCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
    if (CommitType == ETextCommit::OnEnter || CommitType == ETextCommit::OnUserMovedFocus)
    {
        UTimecodeSettings* Settings = GetTimecodeSettings();
        if (Settings)
        {
            Settings->MasterIPAddress = NewText.ToString();
            SaveSettings();

            // 타임코드 매니저에 마스터 IP 변경 적용
            if (TimecodeManager)
            {
                TimecodeManager->SetMasterIPAddress(Settings->MasterIPAddress);
            }
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

FText STimecodeSyncEditorUI::GetMasterIPText() const
{
    UTimecodeSettings* Settings = GetTimecodeSettings();
    if (Settings)
    {
        return FText::FromString(Settings->MasterIPAddress);
    }
    return FText::GetEmpty();
}

void STimecodeSyncEditorUI::InitializeTimecodeManager()
{
    // 이미 초기화된 경우 이전 인스턴스 종료
    ShutdownTimecodeManager();

    // 새로운 델리게이트 핸들러와 매니저 생성
    DelegateHandler = NewObject<UTimecodeSyncEditorDelegateHandler>();
    if (!DelegateHandler)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create delegate handler"));
        return;
    }

    TimecodeManager = NewObject<UTimecodeNetworkManager>();
    if (!TimecodeManager)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create timecode manager"));
        DelegateHandler = nullptr;
        return;
    }

    // 델리게이트 설정
    DelegateHandler->SetTimecodeMessageCallback([this](const FTimecodeNetworkMessage& Message) {
        this->OnTimecodeMessageReceived(Message);
        });

    DelegateHandler->SetNetworkStateCallback([this](ENetworkConnectionState NewState) {
        this->OnNetworkStateChanged(NewState);
        });

    // 델리게이트 연결
    TimecodeManager->OnTimecodeMessageReceived.AddDynamic(DelegateHandler, &UTimecodeSyncEditorDelegateHandler::HandleTimecodeMessage);
    TimecodeManager->OnNetworkStateChanged.AddDynamic(DelegateHandler, &UTimecodeSyncEditorDelegateHandler::HandleNetworkStateChanged);

    // 타임코드 매니저 초기화
    UTimecodeSettings* Settings = GetTimecodeSettings();
    if (Settings)
    {
        // 설정에서 초기 역할 가져오기
        bool bIsMasterRole = (Settings->RoleMode == ETimecodeRoleMode::Manual) ?
            Settings->bIsManualMaster : false; // 자동 모드는 초기에 slave로 시작

        // 매니저 초기화
        TimecodeManager->Initialize(bIsMasterRole, Settings->DefaultUDPPort);

        // 추가 설정 적용
        TimecodeManager->SetRoleMode(Settings->RoleMode);

        if (Settings->RoleMode == ETimecodeRoleMode::Manual)
        {
            TimecodeManager->SetManualMaster(Settings->bIsManualMaster);

            if (!Settings->bIsManualMaster && !Settings->MasterIPAddress.IsEmpty())
            {
                TimecodeManager->SetMasterIPAddress(Settings->MasterIPAddress);
            }
        }

        // 멀티캐스트 그룹 설정
        if (!Settings->MulticastGroupAddress.IsEmpty())
        {
            TimecodeManager->JoinMulticastGroup(Settings->MulticastGroupAddress);
        }
    }
}

void STimecodeSyncEditorUI::OnTimecodeMessageReceived(const FTimecodeNetworkMessage& Message)
{
    if (Message.MessageType == ETimecodeMessageType::TimecodeSync)
    {
        CurrentTimecode = Message.Timecode;
    }
}

void STimecodeSyncEditorUI::OnNetworkStateChanged(ENetworkConnectionState NewState)
{
    // 네트워크 상태 변경 처리
    ConnectionState = NewState;

    switch (NewState)
    {
    case ENetworkConnectionState::Connected:
        StatusMessage = FText::FromString(TEXT("Network connected"));
        break;
    case ENetworkConnectionState::Connecting:
        StatusMessage = FText::FromString(TEXT("Network connecting..."));
        break;
    case ENetworkConnectionState::Disconnected:
        StatusMessage = FText::FromString(TEXT("Network disconnected"));
        break;
    }
}

void STimecodeSyncEditorUI::StartTimecode()
{
    if (TimecodeManager && ConnectionState == ENetworkConnectionState::Connected)
    {
        bIsRunning = true;
        StatusMessage = FText::FromString(TEXT("Timecode started"));

        // 타임코드 시작 메시지 전송
        TimecodeManager->SendTimecodeMessage(CurrentTimecode, ETimecodeMessageType::Command);
    }
    else
    {
        StatusMessage = FText::FromString(TEXT("Cannot start: Manager not initialized or disconnected"));
    }
}

void STimecodeSyncEditorUI::StopTimecode()
{
    if (TimecodeManager && bIsRunning)
    {
        bIsRunning = false;
        StatusMessage = FText::FromString(TEXT("Timecode stopped"));

        // 타임코드 중지 메시지 전송
        TimecodeManager->SendTimecodeMessage(CurrentTimecode, ETimecodeMessageType::Command);
    }
}

void STimecodeSyncEditorUI::ResetTimecode()
{
    if (TimecodeManager)
    {
        // 타임코드 리셋
        CurrentTimecode = TEXT("00:00:00:00");
        StatusMessage = FText::FromString(TEXT("Timecode reset"));

        // 타임코드 리셋 메시지 전송
        TimecodeManager->SendTimecodeMessage(CurrentTimecode, ETimecodeMessageType::Command);
    }
}

FText STimecodeSyncEditorUI::GetConnectionStateText() const
{
    switch (ConnectionState)
    {
    case ENetworkConnectionState::Connected:
        return FText::FromString(TEXT("Connected"));
    case ENetworkConnectionState::Connecting:
        return FText::FromString(TEXT("Connecting..."));
    case ENetworkConnectionState::Disconnected:
        return FText::FromString(TEXT("Disconnected"));
    default:
        return FText::FromString(TEXT("Unknown"));
    }
}

FSlateColor STimecodeSyncEditorUI::GetConnectionStateColor() const
{
    switch (ConnectionState)
    {
    case ENetworkConnectionState::Connected:
        return FSlateColor(FLinearColor(0.2f, 0.8f, 0.2f)); // 녹색
    case ENetworkConnectionState::Connecting:
        return FSlateColor(FLinearColor(0.8f, 0.8f, 0.2f)); // 노란색
    case ENetworkConnectionState::Disconnected:
        return FSlateColor(FLinearColor(0.8f, 0.2f, 0.2f)); // 빨간색
    default:
        return FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)); // 회색
    }
}

#undef LOCTEXT_NAMESPACE