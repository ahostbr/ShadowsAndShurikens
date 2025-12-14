#include "SOTS_InteractionTagAccess_Impl.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "SOTS_GameplayTagManagerSubsystem.h"
#include "SOTS_TagAccessHelpers.h"

namespace SOTSInteractionTagAccess
{
    bool TryGetPlayerTags(APlayerController* PC, FGameplayTagContainer& OutTags)
    {
        OutTags.Reset();

        if (!PC)
        {
            return false;
        }

        // Prefer pawn; fallback to controller as the tag owner.
        AActor* OwnerActor = PC->GetPawn();
        if (!OwnerActor)
        {
            OwnerActor = PC;
        }

        if (!OwnerActor)
        {
            return false;
        }

        if (USOTS_GameplayTagManagerSubsystem* Manager = SOTS_GetTagSubsystem(PC))
        {
            // Current TagManager API exposes query helpers but not a direct export of all tags.
            // Leave OutTags empty; callers should use the manager's ActorHasAnyTags/ActorHasAllTags
            // for gate evaluation. Returning true signals the provider exists.
            return true;
        }

        return false;
    }
}
