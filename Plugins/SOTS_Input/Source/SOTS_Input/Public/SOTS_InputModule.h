#pragma once

#include "Modules/ModuleManager.h"

class FSOTS_InputModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
