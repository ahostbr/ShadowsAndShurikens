#include "SOTS_KEM_BlueprintLibrary.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "SOTS_KEM_ManagerSubsystem.h"
#include "ContextualAnimTypes.h"
#include "SOTS_KillExecutionManagerModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "SOTS_TagAccessHelpers.h"



namespace
{
    static USOTS_KEMManagerSubsystem* GetKEMSubsystem(const UObject* WorldContextObject)
    {
        if (!WorldContextObject)
        {
            return nullptr;
        }

        UWorld* World = WorldContextObject->GetWorld();
        if (!World)
        {
            return nullptr;
        }

        UGameInstance* GameInstance = World->GetGameInstance();
        if (!GameInstance)
        {
            return nullptr;
        }

        return GameInstance->GetSubsystem<USOTS_KEMManagerSubsystem>();
    }
}

bool USOTS_KEM_BlueprintLibrary::KEM_RequestExecution_Simple(
    const UObject* WorldContextObject,
    AActor* Instigator,
    AActor* Target,
    const FGameplayTagContainer& ContextTags)
{
    USOTS_KEMManagerSubsystem* Subsystem = GetKEMSubsystem(WorldContextObject);
    if (!Subsystem)
    {
        return false;
    }

    return Subsystem->RequestExecution(WorldContextObject, Instigator, Target, ContextTags);
}

void USOTS_KEM_BlueprintLibrary::KEM_RunDebug_Simple(
    const UObject* WorldContextObject,
    AActor* Instigator,
    AActor* Target,
    const FGameplayTagContainer& ContextTags)
{
    USOTS_KEMManagerSubsystem* Subsystem = GetKEMSubsystem(WorldContextObject);
    if (!Subsystem)
    {
        return;
    }

    Subsystem->RunKEMDebug(WorldContextObject, Instigator, Target, ContextTags);
}

void USOTS_KEM_BlueprintLibrary::KEM_NotifyExecutionEnded(
    const UObject* WorldContextObject,
    bool bSuccess)
{
    USOTS_KEMManagerSubsystem* Subsystem = GetKEMSubsystem(WorldContextObject);
    if (!Subsystem)
    {
        return;
    }

    Subsystem->NotifyExecutionEnded(bSuccess);
}

bool USOTS_KEM_BlueprintLibrary::SOTS_IsPlayerSafeForExecution(UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return true;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return true;
    }

    if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(WorldContextObject))
    {
        const FGameplayTag HasLOSTag =
            TagSubsystem->GetTagByName(TEXT("SAS.AI.Perception.HasLOS.Player"));

        if (HasLOSTag.IsValid())
        {
            // Conservative default: if any actor in the world reports this tag, we treat
            // the player as unsafe for execution. Callers can supply a more specific
            // guard list later if needed.
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                AActor* Actor = *It;
                if (!Actor)
                {
                    continue;
                }

                if (TagSubsystem->ActorHasTag(Actor, HasLOSTag))
                {
                    return false;
                }
            }
        }
    }

    return true;
}

bool USOTS_KEM_BlueprintLibrary::KEM_ResolveWarpPointByName(
    const USOTS_KEM_ExecutionDefinition* ExecutionDefinition,
    FName WarpPointName,
    const FSOTS_ExecutionContext& Context,
    FTransform& OutTransform)
{
    if (!ExecutionDefinition)
    {
        return false;
    }

    USOTS_KEMManagerSubsystem* Subsystem = nullptr;

    if (Context.Instigator.IsValid())
    {
        if (UWorld* World = Context.Instigator->GetWorld())
        {
            if (UGameInstance* GameInstance = World->GetGameInstance())
            {
                Subsystem = GameInstance->GetSubsystem<USOTS_KEMManagerSubsystem>();
            }
        }
    }

    if (!Subsystem && Context.Target.IsValid())
    {
        if (UWorld* World = Context.Target->GetWorld())
        {
            if (UGameInstance* GameInstance = World->GetGameInstance())
            {
                Subsystem = GameInstance->GetSubsystem<USOTS_KEMManagerSubsystem>();
            }
        }
    }

    if (!Subsystem)
    {
        return false;
    }

    return Subsystem->ResolveWarpPointByName(ExecutionDefinition, WarpPointName, Context, OutTransform);
}

void USOTS_KEM_BlueprintLibrary::KEM_BuildCASBindingContexts(
    AActor* Instigator,
    AActor* Target,
    const FGameplayTagContainer& ContextTags,
    FContextualAnimSceneBindingContext& OutInstigatorBinding,
    FContextualAnimSceneBindingContext& OutTargetBinding)
{
    // Minimal stub for now: zero the contexts so blueprints have valid structs.
    // We can wire in full CAS binding later when we lock in the final CAS pipeline.
    OutInstigatorBinding = FContextualAnimSceneBindingContext();
    OutTargetBinding = FContextualAnimSceneBindingContext();
}
