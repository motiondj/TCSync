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
    // ��Ÿ�� �� ��ɾ� ���
    FTimecodeSyncEditorStyle::Initialize();
    FTimecodeSyncEditorCommands::Register();

    // ��ɾ� ���ε�
    RegisterCommands();

    // �޴� Ȯ�� ���
    RegisterMenus();

    // �� ������ ���
    TimecodeSyncTabSpawnerDelegate = FOnSpawnTab::CreateRaw(this, &FTimecodeSyncEditorModule::SpawnTimecodeSyncTab);

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        TimecodeSyncTabId,
        FOnSpawnTab::CreateRaw(this, &FTimecodeSyncEditorModule::SpawnTimecodeSyncTab))
        .SetDisplayName(LOCTEXT("TimecodeSyncTabTitle", "Timecode Sync"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FTimecodeSyncEditorModule::ShutdownModule()
{
    // �޴� ��� ����
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);

    // ��Ÿ�� ����
    FTimecodeSyncEditorStyle::Shutdown();

    // ��ɾ� ��� ����
    FTimecodeSyncEditorCommands::Unregister();

    // �� ������ ��� ����
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
    // �� �޴� ������ ���
    FToolMenuOwnerScoped OwnerScoped(this);

    // ���� �޴� Ȯ��
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");

    // �޴� �׸� �߰�
    Section.AddMenuEntryWithCommandList(
        FTimecodeSyncEditorCommands::Get().OpenTimecodeSyncUI,
        FTimecodeSyncEditorCommands::Get().CommandList);

    // ���� Ȯ��
    UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
    FToolMenuSection& ToolbarSection = ToolbarMenu->FindOrAddSection("PluginTools");

    // ���� ��ư �߰�
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
    // ��ɾ� ����Ʈ ����
    FTimecodeSyncEditorCommands::Get().CommandList = MakeShareable(new FUICommandList);

    // ��ɾ� ���ε�
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