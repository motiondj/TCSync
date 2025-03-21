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
    // Register styles and commands
    FTimecodeSyncEditorStyle::Initialize();
    FTimecodeSyncEditorCommands::Register();

    // Bind commands
    RegisterCommands();

    // Register menu extensions
    RegisterMenus();

    // Register tab spawner
    TimecodeSyncTabSpawnerDelegate = FOnSpawnTab::CreateRaw(this, &FTimecodeSyncEditorModule::SpawnTimecodeSyncTab);

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        TimecodeSyncTabId,
        FOnSpawnTab::CreateRaw(this, &FTimecodeSyncEditorModule::SpawnTimecodeSyncTab))
        .SetDisplayName(LOCTEXT("TimecodeSyncTabTitle", "Timecode Sync"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FTimecodeSyncEditorModule::ShutdownModule()
{
    // Unregister menus
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);

    // Clean up styles
    FTimecodeSyncEditorStyle::Shutdown();

    // Unregister commands
    FTimecodeSyncEditorCommands::Unregister();

    // Unregister tab spawner
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
    // Register tool menu owner
    FToolMenuOwnerScoped OwnerScoped(this);

    // Extend main menu
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");

    // Add menu items
    Section.AddMenuEntryWithCommandList(
        FTimecodeSyncEditorCommands::Get().OpenTimecodeSyncUI,
        FTimecodeSyncEditorCommands::Get().CommandList);

    // Extend toolbar
    UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
    FToolMenuSection& ToolbarSection = ToolbarMenu->FindOrAddSection("PluginTools");

    // Add toolbar button
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
    // Create command list
    FTimecodeSyncEditorCommands::Get().CommandList = MakeShareable(new FUICommandList);

    // Bind commands
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