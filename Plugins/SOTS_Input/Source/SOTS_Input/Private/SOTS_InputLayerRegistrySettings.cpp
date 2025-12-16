#include "SOTS_InputLayerRegistrySettings.h"

USOTS_InputLayerRegistrySettings::USOTS_InputLayerRegistrySettings()
{
    CategoryName = TEXT("Plugins");
    SectionName = TEXT("SOTS Input Layer Registry");
    bLogRegistryLoads = true;
    bUseAsyncLoads = false;
}
