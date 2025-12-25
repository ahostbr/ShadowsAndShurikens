#include "Characters/SOTS_PlayerCharacterBase.h"

#include "GameFramework/Controller.h"
#include "SOTS_Core.h"
#include "Settings/SOTS_CoreSettings.h"

namespace
{
    bool ShouldLogVerbose()
    {
        const USOTS_CoreSettings* Settings = USOTS_CoreSettings::Get();
        return Settings && Settings->bEnableVerboseCoreLogs;
    }
}

void ASOTS_PlayerCharacterBase::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    if (ShouldLogVerbose())
    {
        UE_LOG(
            LogSOTS_Core,
            Verbose,
            TEXT("[PlayerCharacter] PossessedBy: %s"),
            *GetNameSafe(NewController));
    }
}

void ASOTS_PlayerCharacterBase::UnPossessed()
{
    Super::UnPossessed();

    if (ShouldLogVerbose())
    {
        UE_LOG(LogSOTS_Core, Verbose, TEXT("[PlayerCharacter] UnPossessed"));
    }
}
