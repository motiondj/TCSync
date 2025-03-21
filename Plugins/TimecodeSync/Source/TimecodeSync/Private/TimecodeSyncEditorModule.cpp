#include "TimecodeSyncEditorModule.h"
#include "TimecodeSyncEditorStyle.h"
#include "TimecodeSyncEditorCommands.h"
#include "STimecodeSyncEditorUI.h"
#include "TimecodeSettings.h"

#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "ToolMenuEntry.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "FTimecodeSyncEditorModule"

const FName FTimecodeSyncEditorModule::TimecodeSyncTabId = TEXT("TimecodeSyncTab");

void FTimecodeSyncEditorModule::StartupModule()
{
    // 스타일 및 명령어 등록
    FTimecodeSyncEditorStyle::Initialize();
    FTimecodeSyncEditorCommands::Register();

    // 명령어 바인딩
    RegisterCommands();

    // 메뉴 확장 등록
    RegisterMenus();

    // 탭 스포너 등록
    TimecodeSyncTabSpawnerDelegate = FOnSpawnTab::CreateRaw(this, &FTimecodeSyncEditorModule::SpawnTimecodeSyncTab);

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        TimecodeSyncTabId,
        FOnSpawnTab::CreateRaw(this, &FTimecodeSyncEditorModule::SpawnTimecodeSyncTab))
        .SetDisplayName(LOCTEXT("TimecodeSyncTabTitle", "Timecode Sync"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FTimecodeSyncEditorModule::ShutdownModule()
{
    // 메뉴 등록 해제
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);

    // 스타일 정리
    FTimecodeSyncEditorStyle::Shutdown();

    // 명령어 등록 해제
    FTimecodeSyncEditorCommands::Unregister();

    // 탭 스포너 등록 해제
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TimecodeSyncTabId);
}

TSharedRef<SDockTab> FTimecodeSyncEditorModule::SpawnTimecodeSyncTab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(STimecodeSyncEditorUI)
        ];
}

void FTimecodeSyncEditorModule::RegisterMenus()
{
    // 툴 메뉴 소유자 등록
    FToolMenuOwnerScoped OwnerScoped(this);

    // 메인 메뉴 확장
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");

    // 메뉴 항목 추가
    Section.AddMenuEntryWithCommandList(
        FTimecodeSyncEditorCommands::Get().OpenTimecodeSyncUI,
        FTimecodeSyncEditorCommands::Get().CommandList);

    // 툴바 확장
    UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
    FToolMenuSection& ToolbarSection = ToolbarMenu->FindOrAddSection("PluginTools");

    // 툴바 버튼 추가
    FToolMenuEntry& Entry = ToolbarSection.AddEntry(
        FToolMenuEntry::InitToolBarButton(
            FTimecodeSyncEditorCommands::Get().OpenTimecodeSyncUI,
            LOCTEXT("TimecodeSyncToolbarButton", "Timecode"),
            LOCTEXT("TimecodeSyncToolbarTooltip", "Opens the Timecode Sync panel"),
            FSlateIcon(FTimecodeSyncEditorStyle::GetStyleSetName(), "TimecodeSyncEditor.OpenTimecodeSyncUI")
        )
    );

    Entry.SetCommandList(FTimecodeSyncEditorCommands::Get().CommandList);
}

void FTimecodeSyncEditorModule::RegisterCommands()
{
    // 명령어 리스트 생성
    FTimecodeSyncEditorCommands::Get().CommandList = MakeShareable(new FUICommandList);

    // 명령어 바인딩
    FTimecodeSyncEditorCommands::Get().CommandList->MapAction(
        FTimecodeSyncEditorCommands::Get().OpenTimecodeSyncUI,
        FExecuteAction::CreateRaw(this, &FTimecodeSyncEditorModule::OpenTimecodeSyncTab),
        FCanExecuteAction());
}

void FTimecodeSyncEditorModule::OpenTimecodeSyncTab()
{
    FGlobalTabmanager::Get()->TryInvokeTab(TimecodeSyncTabId);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTimecodeSyncEditorModule, TimecodeSyncEditor)