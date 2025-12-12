#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSOTS_EdUtilModule : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FSOTS_EdUtilModule, SOTS_EdUtil)
