#pragma once

#include "CoreMinimal.h"
#include "SOTS_LooseTagHandle.generated.h"

/**
 * Blueprint-friendly handle for scoped loose gameplay tags. Represents a single scoped add operation.
 */
USTRUCT(BlueprintType)
struct SOTS_TAGMANAGER_API FSOTS_LooseTagHandle
{
    GENERATED_BODY()

public:
    /** Unique identifier for this scoped tag reference. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Tags")
    FGuid Id;

    FSOTS_LooseTagHandle()
        : Id()
    {
    }

    explicit FSOTS_LooseTagHandle(const FGuid& InId)
        : Id(InId)
    {
    }

    /** Returns true if this handle refers to a real scoped tag record. */
    FORCEINLINE bool IsValid() const
    {
        return Id.IsValid();
    }

    /** Helper to construct a handle from a known guid. */
    static FORCEINLINE FSOTS_LooseTagHandle Make(const FGuid& InId)
    {
        return FSOTS_LooseTagHandle(InId);
    }
};
