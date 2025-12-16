#include "SOTS_InputBlueprintLibrary.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "SOTS_InputRouterComponent.h"

namespace
{
    USOTS_InputRouterComponent* FindRouterOnActor(const AActor* Actor)
    {
        return Actor ? Actor->FindComponentByClass<USOTS_InputRouterComponent>() : nullptr;
    }
}

USOTS_InputRouterComponent* USOTS_InputBlueprintLibrary::GetRouterFromActor(AActor* ContextActor)
{
    static bool bLoggedMissing = false;

    if (!ContextActor)
    {
        if (!bLoggedMissing)
        {
            UE_LOG(LogTemp, Warning, TEXT("SOTS_Input: GetRouterFromActor called with null ContextActor."));
            bLoggedMissing = true;
        }
        return nullptr;
    }

    // If the actor itself has the router (e.g., PlayerController), return it.
    if (USOTS_InputRouterComponent* Router = FindRouterOnActor(ContextActor))
    {
        return Router;
    }

    // If pawn, try controller first then pawn.
    if (const APawn* Pawn = Cast<APawn>(ContextActor))
    {
        if (AController* Controller = Pawn->GetController())
        {
            if (USOTS_InputRouterComponent* Router = FindRouterOnActor(Controller))
            {
                return Router;
            }
        }

        if (USOTS_InputRouterComponent* Router = FindRouterOnActor(Pawn))
        {
            return Router;
        }
    }

    // If player controller, already checked self. Otherwise, try PC components.
    if (const APlayerController* PC = Cast<APlayerController>(ContextActor))
    {
        if (USOTS_InputRouterComponent* Router = FindRouterOnActor(PC))
        {
            return Router;
        }
    }

    // Fallback to first player controller in world.
    if (UWorld* World = ContextActor->GetWorld())
    {
        if (APlayerController* FirstPC = World->GetFirstPlayerController())
        {
            if (USOTS_InputRouterComponent* Router = FindRouterOnActor(FirstPC))
            {
                return Router;
            }
        }
    }

    if (!bLoggedMissing)
    {
        UE_LOG(LogTemp, Warning, TEXT("SOTS_Input: No USOTS_InputRouterComponent found for context actor %s."), *ContextActor->GetName());
        bLoggedMissing = true;
    }

    return nullptr;
}

void USOTS_InputBlueprintLibrary::PushLayerTag(AActor* ContextActor, FGameplayTag LayerTag)
{
    if (USOTS_InputRouterComponent* Router = GetRouterFromActor(ContextActor))
    {
        Router->PushLayerByTag(LayerTag);
    }
}

void USOTS_InputBlueprintLibrary::PopLayerTag(AActor* ContextActor, FGameplayTag LayerTag)
{
    if (USOTS_InputRouterComponent* Router = GetRouterFromActor(ContextActor))
    {
        Router->PopLayerByTag(LayerTag);
    }
}

void USOTS_InputBlueprintLibrary::OpenBuffer(AActor* ContextActor, FGameplayTag ChannelTag)
{
    if (USOTS_InputRouterComponent* Router = GetRouterFromActor(ContextActor))
    {
        Router->OpenInputBuffer(ChannelTag);
    }
}

void USOTS_InputBlueprintLibrary::CloseBuffer(AActor* ContextActor, FGameplayTag ChannelTag, bool bFlush)
{
    if (USOTS_InputRouterComponent* Router = GetRouterFromActor(ContextActor))
    {
        Router->CloseInputBuffer(ChannelTag, bFlush);
    }
}
