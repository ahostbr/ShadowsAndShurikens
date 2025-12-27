#include "SOTS_InputLayerRegistrySettings.h"

USOTS_InputLayerRegistrySettings::USOTS_InputLayerRegistrySettings()
{
    CategoryName = TEXT("SOTS");
    SectionName = TEXT("SOTS Input Layer Registry");
    bLogRegistryLoads = true;
    bUseAsyncLoads = false;
}
