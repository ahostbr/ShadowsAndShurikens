#pragma once

#include "CoreMinimal.h"
#include "Save/SOTS_CoreSaveParticipant.h"

class SOTS_CORE_API FSOTS_CoreSaveParticipantRegistry
{
public:
    static void GetRegisteredSaveParticipants(TArray<ISOTS_CoreSaveParticipant*>& OutParticipants);
    static void RegisterSaveParticipant(ISOTS_CoreSaveParticipant* Participant);
    static void UnregisterSaveParticipant(ISOTS_CoreSaveParticipant* Participant);
};
