#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Toolkits/IToolkit.h"

class FToolBarBuilder;
class FMenuBuilder;
class SDockTab;

class FTimecodeSyncEditorModule : public IModuleInterface
{
public:
    // IModuleInterface implementation
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    // Editor UI tab creation function
    TSharedRef<SDockTab> SpawnTimecodeSyncTab(const FSpawnTabArgs& SpawnTabArgs);

private:
    // Plugin menu and toolbar extension
    void RegisterMenus();

    // Editor command binding
    void RegisterCommands();

    // Open timecode editor tab
    void OpenTimecodeSyncTab();

    // Tab manager registration handler
    TSharedPtr<FTabManager::FLayout> EditorLayout;

    // Tab spawn delegate
    FOnSpawnTab TimecodeSyncTabSpawnerDelegate;

    // Tab ID
    static const FName TimecodeSyncTabId;
};