#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FTimecodeSyncEditorCommands : public TCommands<FTimecodeSyncEditorCommands>
{
public:
    FTimecodeSyncEditorCommands();

    // TCommands 인터페이스
    virtual void RegisterCommands() override;

    // UI 명령어
    TSharedPtr<FUICommandInfo> OpenTimecodeSyncUI;

    // 명령어 리스트
    TSharedPtr<FUICommandList> CommandList;

    // 싱글톤 인스턴스 접근자
    static const FTimecodeSyncEditorCommands& Get();
};