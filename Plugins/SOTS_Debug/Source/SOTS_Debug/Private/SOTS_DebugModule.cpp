#include "Modules/ModuleManager.h"

class FSOTS_DebugModule : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FSOTS_DebugModule, SOTS_Debug);
