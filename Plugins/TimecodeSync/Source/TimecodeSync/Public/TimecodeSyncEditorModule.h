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
    // IModuleInterface 구현
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    // 에디터 UI 탭 생성 함수
    TSharedRef<SDockTab> SpawnTimecodeSyncTab(const FSpawnTabArgs& SpawnTabArgs);

private:
    // 플러그인 메뉴 및 툴바 확장
    void RegisterMenus();

    // 에디터 명령어 바인딩
    void RegisterCommands();

    // 타임코드 에디터 탭 열기
    void OpenTimecodeSyncTab();

    // 탭 매니저 등록 처리
    TSharedPtr<FTabManager::FLayout> EditorLayout;

    // 탭 스폰 델리게이트
    FOnSpawnTab TimecodeSyncTabSpawnerDelegate;

    // 탭 ID
    static const FName TimecodeSyncTabId;
};