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
    // IModuleInterface ����
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    // ������ UI �� ���� �Լ�
    TSharedRef<SDockTab> SpawnTimecodeSyncTab(const FSpawnTabArgs& SpawnTabArgs);

private:
    // �÷����� �޴� �� ���� Ȯ��
    void RegisterMenus();

    // ������ ��ɾ� ���ε�
    void RegisterCommands();

    // Ÿ���ڵ� ������ �� ����
    void OpenTimecodeSyncTab();

    // �� �Ŵ��� ��� ó��
    TSharedPtr<FTabManager::FLayout> EditorLayout;

    // �� ���� ��������Ʈ
    FOnSpawnTab TimecodeSyncTabSpawnerDelegate;

    // �� ID
    static const FName TimecodeSyncTabId;
};