#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FTimecodeSyncModule : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    /** Settings load function */
    void LoadSettings();
};