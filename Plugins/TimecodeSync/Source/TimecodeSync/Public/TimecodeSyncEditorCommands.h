#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FTimecodeSyncEditorCommands : public TCommands<FTimecodeSyncEditorCommands>
{
public:
    FTimecodeSyncEditorCommands();

    // TCommands �������̽�
    virtual void RegisterCommands() override;

    // UI ��ɾ�
    TSharedPtr<FUICommandInfo> OpenTimecodeSyncUI;

    // ��ɾ� ����Ʈ
    TSharedPtr<FUICommandList> CommandList;

    // �̱��� �ν��Ͻ� ������
    static const FTimecodeSyncEditorCommands& Get();
};