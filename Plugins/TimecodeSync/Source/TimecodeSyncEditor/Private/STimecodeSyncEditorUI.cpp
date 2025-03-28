﻿#include "STimecodeSyncEditorUI.h"
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

    // 추가 초기화
    TimecodeManager = nullptr;
    DelegateHandler = nullptr;
    CurrentRole = TEXT("Not Connected");

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

// 소멸자 수정
STimecodeSyncEditorUI::~STimecodeSyncEditorUI()
{
    // 먼저 모든 예약된 틱 이벤트를 처리하지 않도록 설정
    if (TickDelegateHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
        TickDelegateHandle.Reset();
    }

    // 델리게이트 바인딩 해제를 명시적으로 먼저 처리
    if (TimecodeManager && DelegateHandler)
    {
        TimecodeManager->OnTimecodeMessageReceived.RemoveAll(DelegateHandler);
        TimecodeManager->OnNetworkStateChanged.RemoveAll(DelegateHandler);
    }

    // 이제 타임코드 매니저 종료
    if (TimecodeManager)
    {
        TimecodeManager->Shutdown();
        TimecodeManager->RemoveFromRoot(); // 새로 추가된 부분
        TimecodeManager = nullptr;
    }

    // 마지막으로 델리게이트 핸들러 정리
    if (DelegateHandler)
    {
        DelegateHandler->RemoveFromRoot(); // 새로 추가된 부분
        DelegateHandler = nullptr;
    }
}


void STimecodeSyncEditorUI::ShutdownTimecodeManager()
{
    // 델리게이트 제거
    if (DelegateHandler != nullptr && TimecodeManager != nullptr)
    {
        TimecodeManager->OnTimecodeMessageReceived.RemoveAll(DelegateHandler);
        TimecodeManager->OnNetworkStateChanged.RemoveAll(DelegateHandler);
    }

    // 타임코드 매니저 종료
    if (TimecodeManager != nullptr)
    {
        TimecodeManager->Shutdown();
        TimecodeManager->RemoveFromRoot();
        TimecodeManager = nullptr;
    }

    // 델리게이트 핸들러 정리
    if (DelegateHandler != nullptr)
    {
        DelegateHandler->RemoveFromRoot();
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

                // 여기에 전용 마스터 서버 옵션 추가
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SHorizontalBox)

                        // 레이블
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0, 0, 10, 0)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT("DedicatedMaster", "Dedicated Master:"))
                                .MinDesiredWidth(120)
                                .ToolTipText(LOCTEXT("DedicatedMasterTooltip", "When enabled, this device will act as a dedicated timecode master server"))
                        ]

                        // 체크박스
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        [
                            SNew(SCheckBox)
                                .IsChecked_Lambda([this]()
                                    {
                                        UTimecodeSettings* Settings = GetTimecodeSettings();
                                        return Settings && Settings->bIsDedicatedMaster ?
                                            ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                                    })
                                .OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
                                    {
                                        UTimecodeSettings* Settings = GetTimecodeSettings();
                                        if (Settings)
                                        {
                                            Settings->bIsDedicatedMaster = (NewState == ECheckBoxState::Checked);

                                            // 전용 마스터가 활성화되면 자동으로 수동 마스터 모드로 설정
                                            if (Settings->bIsDedicatedMaster)
                                            {
                                                Settings->RoleMode = ETimecodeRoleMode::Manual;
                                                Settings->bIsManualMaster = true;
                                            }

                                            SaveSettings();

                                            // 타임코드 매니저에 설정 적용 (이미 초기화된 경우)
                                            if (TimecodeManager)
                                            {
                                                TimecodeManager->SetDedicatedMaster(Settings->bIsDedicatedMaster);
                                            }
                                        }
                                    })
                        ]

                        // 설명 텍스트
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        .VAlign(VAlign_Center)
                        .Padding(5, 0, 0, 0)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT("DedicatedMasterLabel", "Enable Dedicated Master Server"))
                                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                                .ColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)))
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

                // Receive Port (수신 포트) - 명확한 레이블링
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
                                .Text(LOCTEXT("ReceivePort", "Receive Port:"))
                                .MinDesiredWidth(120)
                                .ToolTipText(LOCTEXT("ReceivePortTooltip", "Port used for receiving incoming messages (this device listens on this port)"))
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

                // Send Port (송신 포트) - 새로 추가
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
                                .Text(LOCTEXT("SendPort", "Send Port:"))
                                .MinDesiredWidth(120)
                                .ToolTipText(LOCTEXT("SendPortTooltip", "Port used for sending outgoing messages to target devices"))
                        ]
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SSpinBox<int32>)
                                .MinValue(1024)
                                .MaxValue(65535)
                                .Value_Lambda([this]() -> int32 {
                                // 기본적으로 수신 포트 + 1
                                UTimecodeSettings* Settings = GetTimecodeSettings();
                                return Settings ? (Settings->DefaultUDPPort + 1) : 10001;
                                    })
                                .OnValueChanged_Lambda([this](int32 NewValue) {
                                if (TimecodeManager) {
                                    TimecodeManager->SetTargetPort(NewValue);
                                }
                                    })
                                .ToolTipText(LOCTEXT("SendPortValueTooltip", "Typically set to Receive Port + 1"))
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

                // Sync Interval
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

                // Current Role (첫 번째 항목)
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

                // Current Timecode (두 번째 항목을 변경)
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
    if (TimecodeManager == nullptr)
    {
        // 유효하지 않은 경우 기본값 설정
        CurrentTimecode = TEXT("--:--:--:--");
        ConnectionState = ENetworkConnectionState::Disconnected;
        bIsMaster = false;
        CurrentRole = TEXT("Not Connected");
        StatusMessage = FText::FromString(TEXT("Manager not available"));
        return;
    }

    // 타임코드 매니저로부터 정보 가져오기
    // 'bHasValidCommunication' 변수가 선언되지 않았으므로 직접 호출하도록 수정
    bool bHasValidMessage = TimecodeManager->HasReceivedValidMessage();
    ENetworkConnectionState CurrentConnectionState = TimecodeManager->GetConnectionState();
    bool bCurrentIsMaster = TimecodeManager->IsMaster();
    FString CurrentTimecodeValue = TimecodeManager->GetCurrentTimecode();

    // 연결 상태 업데이트
    ConnectionState = CurrentConnectionState;

    // 역할 상태 업데이트
    bIsMaster = bCurrentIsMaster;
    CurrentRole = bIsMaster ? TEXT("MASTER") : TEXT("SLAVE");

    // 역할 모드 (자동/수동) 상태 표시 추가
    ETimecodeRoleMode CurrentRoleMode = TimecodeManager->GetRoleMode();
    FString RoleModeStr = (CurrentRoleMode == ETimecodeRoleMode::Automatic) ?
        TEXT("(Auto)") : TEXT("(Manual)");
    CurrentRole = FString::Printf(TEXT("%s %s"), *CurrentRole, *RoleModeStr);

    // 타임코드 업데이트
    if (ConnectionState == ENetworkConnectionState::Connected && !CurrentTimecodeValue.IsEmpty())
    {
        CurrentTimecode = CurrentTimecodeValue;
    }
    else
    {
        CurrentTimecode = TEXT("--:--:--:--");
    }

    // 상태 메시지 업데이트
    switch (ConnectionState)
    {
    case ENetworkConnectionState::Connected:
        StatusMessage = FText::FromString(TEXT("Connected and syncing"));
        break;
    case ENetworkConnectionState::Connecting:
        StatusMessage = FText::FromString(TEXT("Connecting..."));
        break;
    case ENetworkConnectionState::Disconnected:
        StatusMessage = FText::FromString(TEXT("Disconnected - Check network settings"));
        break;
    }

    // 통신 상태에 따른 추가 메시지
    if (!bHasValidMessage && ConnectionState == ENetworkConnectionState::Connected)
    {
        StatusMessage = FText::FromString(TEXT("Connected but no sync data received"));
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
    // 이전 인스턴스 종료
    ShutdownTimecodeManager();

    // 설정 불러오기
    UTimecodeSettings* Settings = GetTimecodeSettings();
    if (!Settings)
    {
        StatusMessage = FText::FromString(TEXT("Failed to load settings"));
        return;
    }

    // 델리게이트 핸들러 생성
    DelegateHandler = NewObject<UTimecodeSyncEditorDelegateHandler>();
    if (!DelegateHandler)
    {
        StatusMessage = FText::FromString(TEXT("Failed to create delegate handler"));
        return;
    }

    // GC 방지를 위해 Root에 추가
    DelegateHandler->AddToRoot();

    // 타임코드 매니저 생성
    TimecodeManager = NewObject<UTimecodeNetworkManager>();
    if (!TimecodeManager)
    {
        // 정리
        if (DelegateHandler)
        {
            DelegateHandler->RemoveFromRoot();
            DelegateHandler = nullptr;
        }

        StatusMessage = FText::FromString(TEXT("Failed to create timecode manager"));
        return;
    }

    // GC 방지를 위해 Root에 추가
    TimecodeManager->AddToRoot();

    // 콜백 함수 설정 (약한 참조 사용) - 변수명 변경
    TWeakPtr<STimecodeSyncEditorUI> WeakSelf = SharedThis(this);
    DelegateHandler->SetTimecodeMessageCallback([WeakSelf](const FTimecodeNetworkMessage& Message) {
        if (WeakSelf.IsValid())
        {
            WeakSelf.Pin()->OnTimecodeMessageReceived(Message);
        }
    });

    DelegateHandler->SetNetworkStateCallback([WeakSelf](ENetworkConnectionState NewState) {
        if (WeakSelf.IsValid())
        {
            WeakSelf.Pin()->OnNetworkStateChanged(NewState);
        }
    });

    // 바인딩 - 약한 참조 사용
    TimecodeManager->OnTimecodeMessageReceived.AddDynamic(DelegateHandler, &UTimecodeSyncEditorDelegateHandler::HandleTimecodeMessage);
    TimecodeManager->OnNetworkStateChanged.AddDynamic(DelegateHandler, &UTimecodeSyncEditorDelegateHandler::HandleNetworkStateChanged);

    // 설정 적용
    TimecodeManager->SetRoleMode(Settings->RoleMode);
    if (Settings->RoleMode == ETimecodeRoleMode::Manual)
    {
        TimecodeManager->SetManualMaster(Settings->bIsManualMaster);
        if (!Settings->bIsManualMaster && !Settings->MasterIPAddress.IsEmpty())
        {
            TimecodeManager->SetMasterIPAddress(Settings->MasterIPAddress);
        }
    }

    // 네트워크 초기화
    bool bIsMasterRole = (Settings->RoleMode == ETimecodeRoleMode::Manual) ?
        Settings->bIsManualMaster : false; // 자동 모드에서는 초기값 설정
    bool bSuccess = TimecodeManager->Initialize(bIsMasterRole, Settings->DefaultUDPPort);

    if (bSuccess)
    {
        // 멀티캐스트 그룹 설정
        if (!Settings->MulticastGroupAddress.IsEmpty())
        {
            TimecodeManager->JoinMulticastGroup(Settings->MulticastGroupAddress);
        }

        StatusMessage = FText::FromString(TEXT("Timecode manager initialized"));
    }
    else
    {
        StatusMessage = FText::FromString(TEXT("Failed to initialize network"));
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
    if (TimecodeManager != nullptr && ConnectionState == ENetworkConnectionState::Connected)
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
    if (TimecodeManager != nullptr && bIsRunning)
    {
        bIsRunning = false;
        StatusMessage = FText::FromString(TEXT("Timecode stopped"));

        // 타임코드 중지 메시지 전송
        TimecodeManager->SendTimecodeMessage(CurrentTimecode, ETimecodeMessageType::Command);
    }
}

void STimecodeSyncEditorUI::ResetTimecode()
{
    if (TimecodeManager != nullptr)
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