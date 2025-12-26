#include "Save/SOTS_CoreSaveParticipantRegistry.h"

#include "Features/IModularFeatures.h"
#include "SOTS_Core.h"
#include "Settings/SOTS_CoreSettings.h"

void FSOTS_CoreSaveParticipantRegistry::GetRegisteredSaveParticipants(
    TArray<ISOTS_CoreSaveParticipant*>& OutParticipants)
{
    OutParticipants.Reset();

    const USOTS_CoreSettings* Settings = USOTS_CoreSettings::Get();
    if (!Settings || !Settings->bEnableSaveParticipantQueries)
    {
        if (Settings && Settings->bEnableSaveContractLogs)
        {
            UE_LOG(LogSOTS_Core, Verbose, TEXT("Save participant queries are disabled."));
        }
        return;
    }

    TArray<IModularFeature*> Features = IModularFeatures::Get().GetModularFeatureImplementations<IModularFeature>(
        SOTS_CoreSaveParticipantFeatureName);

    OutParticipants.Reserve(Features.Num());
    for (IModularFeature* Feature : Features)
    {
        ISOTS_CoreSaveParticipant* Participant = static_cast<ISOTS_CoreSaveParticipant*>(Feature);
        if (!ensureMsgf(Participant, TEXT("Null save participant encountered during registry query.")))
        {
            continue;
        }

        OutParticipants.Add(Participant);
    }

    if (Settings->bEnableSaveContractLogs)
    {
        UE_LOG(LogSOTS_Core, Verbose, TEXT("Save participant query returned %d entries."), OutParticipants.Num());
    }
}

void FSOTS_CoreSaveParticipantRegistry::RegisterSaveParticipant(ISOTS_CoreSaveParticipant* Participant)
{
    if (!Participant)
    {
        return;
    }

    IModularFeatures::Get().RegisterModularFeature(SOTS_CoreSaveParticipantFeatureName, Participant);
}

void FSOTS_CoreSaveParticipantRegistry::UnregisterSaveParticipant(ISOTS_CoreSaveParticipant* Participant)
{
    if (!Participant)
    {
        return;
    }

    IModularFeatures::Get().UnregisterModularFeature(SOTS_CoreSaveParticipantFeatureName, Participant);
}
