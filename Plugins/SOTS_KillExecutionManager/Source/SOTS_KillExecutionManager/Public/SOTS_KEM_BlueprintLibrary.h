
#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "ContextualAnimTypes.h"
#include "SOTS_KEM_Types.h"
#include "SOTS_KEM_BlueprintLibrary.generated.h"

class AActor;

UCLASS()
class SOTS_KILLEXECUTIONMANAGER_API USOTS_KEM_BlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /** One-click helper: from Instigator + Target + ContextTags, request an execution via the KEM subsystem. */
    UFUNCTION(BlueprintCallable, Category="KEM", meta=(WorldContext="WorldContextObject"))
    static bool KEM_RequestExecution_Simple(const UObject* WorldContextObject,
                                            AActor* Instigator,
                                            AActor* Target,
                                            const FGameplayTagContainer& ContextTags);

    /** One-click helper: log debug info for all definitions for the given Instigator/Target/ContextTags. */
    UFUNCTION(BlueprintCallable, Category="KEM|Debug", meta=(WorldContext="WorldContextObject"))
    static void KEM_RunDebug_Simple(const UObject* WorldContextObject,
                                    AActor* Instigator,
                                    AActor* Target,
                                    const FGameplayTagContainer& ContextTags);

    /** Notify KEM that the current execution has finished (success or failure). */
    UFUNCTION(BlueprintCallable, Category="KEM", meta=(WorldContext="WorldContextObject"))
    static void KEM_NotifyExecutionEnded(const UObject* WorldContextObject, bool bSuccess);


    /** Returns true if the player is safe for execution (no nearby AI has HasLOS.Player tag). */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|KEM", meta=(WorldContext="WorldContextObject"))
    static bool SOTS_IsPlayerSafeForExecution(UObject* WorldContextObject);

        /** Resolve a single authored warp point for a given execution definition and context. */
    UFUNCTION(BlueprintCallable, Category="KEM|Warp")
    static bool KEM_ResolveWarpPointByName(
        const USOTS_KEM_ExecutionDefinition* ExecutionDefinition,
        FName WarpPointName,
        const FSOTS_ExecutionContext& Context,
        FTransform& OutTransform);

/** Helper to build CAS binding contexts with KEM ContextTags injected as ExternalGameplayTags. */
    UFUNCTION(BlueprintCallable, Category="KEM|CAS", meta=(DeprecatedFunction, DeprecationMessage="Use KEM_BuildCASBindingContextsForDefinition which mirrors runtime CAS binding."))
    static void KEM_BuildCASBindingContexts(
        AActor* Instigator,
        AActor* Target,
        const FGameplayTagContainer& ContextTags,
        FContextualAnimSceneBindingContext& OutInstigatorBinding,
        FContextualAnimSceneBindingContext& OutTargetBinding);

    /** Build CAS binding contexts for a specific execution definition (mirrors runtime path). */
    UFUNCTION(BlueprintCallable, Category="KEM|CAS", meta=(WorldContext="WorldContextObject"))
    static bool KEM_BuildCASBindingContextsForDefinition(
        const UObject* WorldContextObject,
        const USOTS_KEM_ExecutionDefinition* ExecutionDefinition,
        AActor* Instigator,
        AActor* Target,
        const FGameplayTagContainer& ContextTags,
        FContextualAnimSceneBindingContext& OutInstigatorBinding,
        FContextualAnimSceneBindingContext& OutTargetBinding);

#if WITH_EDITOR
    /** Editor-only: validate coverage for loaded KEM definitions and report missing families/positions. */
    UFUNCTION(BlueprintCallable, Category="KEM|Validation", meta=(WorldContext="WorldContextObject"))
    static void KEM_ValidateRegistryCoverage(const UObject* WorldContextObject, TArray<FString>& OutWarnings);
#endif
};
