#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SOTS_InteractionTypes.h"
#include "SOTS_InteractionUIIntentPayload.generated.h"

/**
 * Unified UI intent payload for SOTS_Interaction -> UI glue.
 * Allows prompt show/hide, options, and standardized fail reasons without fake option hacks.
 */
USTRUCT(BlueprintType)
struct FSOTS_InteractionUIIntentPayload
{
    GENERATED_BODY()

    /** Intent identifier (e.g., InteractionPrompt / InteractionOptions / InteractionFail). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|UI")
    FGameplayTag IntentTag;

    /** Interaction context (player + target). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|UI")
    FSOTS_InteractionContext Context;

    /** Options for options UI. Empty for prompt/fail unless desired. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|UI")
    TArray<FSOTS_InteractionOption> Options;

    /** Standard fail reason tag (valid when IntentTag == Fail). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|UI")
    FGameplayTag FailReasonTag;

    /** For prompt intent: true = show, false = hide. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|UI")
    bool bShowPrompt = false;
};
