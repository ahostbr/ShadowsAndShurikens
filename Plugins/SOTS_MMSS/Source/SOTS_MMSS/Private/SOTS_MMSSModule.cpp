#include "Modules/ModuleManager.h"

class FSOTS_MMSSModule : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FSOTS_MMSSModule, SOTS_MMSS)
