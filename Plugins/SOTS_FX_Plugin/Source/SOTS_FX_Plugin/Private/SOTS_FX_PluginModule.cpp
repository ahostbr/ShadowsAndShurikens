#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSOTS_FX_PluginModule : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FSOTS_FX_PluginModule, SOTS_FX_Plugin);
