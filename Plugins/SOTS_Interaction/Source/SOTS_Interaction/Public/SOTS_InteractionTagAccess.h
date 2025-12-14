#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SOTS_InteractionTagAccess.generated.h"

/**
 * Minimal tag access seam so SOTS_Interaction can query "player state tags" without
 * hard-coding where they come from. Backed by SOTS_TagManager when available.
 */
USTRUCT(BlueprintType)
struct FSOTS_TagAccessResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    FGameplayTagContainer Tags;
};
