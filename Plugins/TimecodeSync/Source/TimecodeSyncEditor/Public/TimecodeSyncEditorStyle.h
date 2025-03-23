#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

class FTimecodeSyncEditorStyle
{
public:
    static void Initialize();
    static void Shutdown();

    static FName GetStyleSetName();

private:
    static TSharedPtr<FSlateStyleSet> StyleSet;

    static TSharedRef<FSlateStyleSet> Create();
};