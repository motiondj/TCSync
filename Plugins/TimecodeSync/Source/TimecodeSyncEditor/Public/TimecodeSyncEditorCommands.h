#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FTimecodeSyncEditorCommands : public TCommands<FTimecodeSyncEditorCommands>
{
public:
    FTimecodeSyncEditorCommands();

    // TCommands interface
    virtual void RegisterCommands() override;

    // UI commands
    TSharedPtr<FUICommandInfo> OpenTimecodeSyncUI;

    // Command list
    TSharedPtr<FUICommandList> CommandList;

    // Singleton instance accessor
    static const FTimecodeSyncEditorCommands& Get();
};