#include "BEP_NodeJsonTab.h"

#include "Widgets/Docking/SDockTab.h"
#include "Widgets/SBEP_NodeJsonPanel.h"
#include "Framework/Docking/TabManager.h"

const FName FBEP_NodeJsonTab::TabId(TEXT("BEP.NodeJson"));

void FBEP_NodeJsonTab::Register()
{
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        TabId,
        FOnSpawnTab::CreateStatic(&FBEP_NodeJsonTab::Spawn))
        .SetDisplayName(FText::FromString(TEXT("BEP Node JSON")))
        .SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FBEP_NodeJsonTab::Unregister()
{
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabId);
}

TSharedRef<SDockTab> FBEP_NodeJsonTab::Spawn(const FSpawnTabArgs& Args)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SBEP_NodeJsonPanel)
        ];
}
