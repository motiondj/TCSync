#include "TimecodeSyncEditorCommands.h"
#include "TimecodeSyncEditorStyle.h"

#define LOCTEXT_NAMESPACE "FTimecodeSyncEditorCommands"

FTimecodeSyncEditorCommands::FTimecodeSyncEditorCommands()
    : TCommands<FTimecodeSyncEditorCommands>(
        TEXT("TimecodeSyncEditor"),
        NSLOCTEXT("Contexts", "TimecodeSyncEditor", "Timecode Sync Editor Plugin"),
        NAME_None,
        FTimecodeSyncEditorStyle::GetStyleSetName())
{
}

void FTimecodeSyncEditorCommands::RegisterCommands()
{
    UI_COMMAND(
        OpenTimecodeSyncUI,
        "Timecode Sync",
        "Open the Timecode Sync panel",
        EUserInterfaceActionType::Button,
        FInputChord());
}

const FTimecodeSyncEditorCommands& FTimecodeSyncEditorCommands::Get()
{
    return TCommands<FTimecodeSyncEditorCommands>::Get();
}

#undef LOCTEXT_NAMESPACE