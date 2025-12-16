#pragma once

#include "CoreMinimal.h"

class SDockTab;
class SBEP_NodeJsonPanel;

struct FBEP_NodeJsonTab
{
    static const FName TabId;

    static void Register();
    static void Unregister();
    static TSharedRef<SDockTab> Spawn(const FSpawnTabArgs& Args);
};
