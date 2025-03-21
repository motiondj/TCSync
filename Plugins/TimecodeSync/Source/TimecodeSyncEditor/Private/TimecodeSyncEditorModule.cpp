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
#include "EditorStyle.h"

#define LOCTEXT_NAMESPACE "FTimecodeSyncEditorModule"

const FName FTimecodeSyncEditorModule::TimecodeSyncTabId = TEXT("TimecodeSyncTab");

void FTimecodeSyncEditorModule::StartupModule()
{
    // 플러그인 로드 확인을 위한 로그 추가
    UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditor module attempting to start up"));

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

    UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditor module started up successfully"));
}

void FTimecodeSyncEditorModule::ShutdownModule()
{
    UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditor module shutting down"));

    // Unregister menus
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);

    // Clean up styles
    FTimecodeSyncEditorStyle::Shutdown();

    // Unregister commands
    FTimecodeSyncEditorCommands::Unregister();

    // Unregister tab spawner
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TimecodeSyncTabId);

    UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditor module shut down successfully"));
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
    UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditor: Starting menu registration"));

    // Register tool menu owner
    FToolMenuOwnerScoped OwnerScoped(this);

    // Extend main menu
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    if (Menu)
    {
        UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditor: Successfully extended Window menu"));
        FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");

        // Add menu items
        Section.AddMenuEntryWithCommandList(
            FTimecodeSyncEditorCommands::Get().OpenTimecodeSyncUI,
            FTimecodeSyncEditorCommands::Get().CommandList);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("TimecodeSyncEditor: Failed to extend Window menu"));
    }

    // Extend toolbar
    UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
    if (ToolbarMenu)
    {
        UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditor: Successfully extended toolbar"));
        FToolMenuSection& ToolbarSection = ToolbarMenu->FindOrAddSection("PluginTools");

        // Add toolbar button
        FToolMenuEntry& Entry = ToolbarSection.AddEntry(
            FToolMenuEntry::InitToolBarButton(
                FTimecodeSyncEditorCommands::Get().OpenTimecodeSyncUI,
                LOCTEXT("TimecodeSyncToolbarButton", "Timecode"),
                LOCTEXT("TimecodeSyncToolbarTooltip", "Opens the Timecode Sync panel"),
                FSlateIcon(FTimecodeSyncEditorStyle::GetStyleSetName(), "TimecodeSyncEditor.OpenTimecodeSyncUI.Small")
            )
        );

        Entry.SetCommandList(FTimecodeSyncEditorCommands::Get().CommandList);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("TimecodeSyncEditor: Failed to extend toolbar"));
    }

    UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditor: Completed menu registration"));
}

void FTimecodeSyncEditorModule::RegisterCommands()
{
    // Create command list
    auto& Commands = FTimecodeSyncEditorCommands::Get();
    Commands.CommandList = MakeShareable(new FUICommandList());

    // Bind commands
    Commands.CommandList->MapAction(
        Commands.OpenTimecodeSyncUI,
        FExecuteAction::CreateRaw(this, &FTimecodeSyncEditorModule::OpenTimecodeSyncTab),
        FCanExecuteAction());
}

void FTimecodeSyncEditorModule::OpenTimecodeSyncTab()
{
    FGlobalTabmanager::Get()->TryInvokeTab(TimecodeSyncTabId);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTimecodeSyncEditorModule, TimecodeSyncEditor)