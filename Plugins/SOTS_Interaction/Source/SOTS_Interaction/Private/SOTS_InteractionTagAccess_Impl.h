#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class APlayerController;

namespace SOTSInteractionTagAccess
{
    /** Best-effort: fetch player tags via SOTS_TagManager (if available). Returns false if unavailable. */
    bool TryGetPlayerTags(APlayerController* PC, FGameplayTagContainer& OutTags);
}
