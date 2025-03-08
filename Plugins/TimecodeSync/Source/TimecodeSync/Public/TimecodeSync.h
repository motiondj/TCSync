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
    /** 설정 로드 함수 */
    void LoadSettings();
};