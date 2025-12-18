#include "SOTS_InputBlueprintLibrary.h"

#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "SOTS_InputBufferComponent.h"
#include "SOTS_InputRouterComponent.h"

namespace
{
    USOTS_InputRouterComponent* FindRouterOnActor(const AActor* Actor)
    {
        return Actor ? Actor->FindComponentByClass<USOTS_InputRouterComponent>() : nullptr;
    }

    APlayerController* ResolvePlayerController(UObject* WorldContextObject, AActor* ContextActor)
    {
        if (APlayerController* AsPC = Cast<APlayerController>(ContextActor))
        {
            return AsPC;
        }

        if (APawn* Pawn = Cast<APawn>(ContextActor))
        {
            if (AController* Controller = Pawn->GetController())
            {
                if (APlayerController* PC = Cast<APlayerController>(Controller))
                {
                    return PC;
                }
            }
        }

        if (ContextActor)
        {
            if (AController* InstigatorController = ContextActor->GetInstigatorController())
            {
                if (APlayerController* PC = Cast<APlayerController>(InstigatorController))
                {
                    return PC;
                }
            }
        }

        if (WorldContextObject)
        {
            if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr)
            {
                if (APlayerController* FirstPC = World->GetFirstPlayerController())
                {
                    return FirstPC;
                }
            }
        }

        if (ContextActor)
        {
            if (UWorld* World = ContextActor->GetWorld())
            {
                return World->GetFirstPlayerController();
            }
        }

        return nullptr;
    }

    USOTS_InputRouterComponent* ResolveRouter(UObject* WorldContextObject, AActor* ContextActor)
    {
        if (USOTS_InputRouterComponent* Router = FindRouterOnActor(ContextActor))
        {
            return Router;
        }

        if (APawn* Pawn = Cast<APawn>(ContextActor))
        {
            if (AController* Controller = Pawn->GetController())
            {
                if (USOTS_InputRouterComponent* Router = FindRouterOnActor(Controller))
                {
                    return Router;
                }
            }
        }

        if (ContextActor)
        {
            if (USOTS_InputRouterComponent* Router = FindRouterOnActor(ContextActor->GetInstigatorController()))
            {
                return Router;
            }
        }

        if (APlayerController* PC = ResolvePlayerController(WorldContextObject, ContextActor))
        {
            if (USOTS_InputRouterComponent* Router = FindRouterOnActor(PC))
            {
                return Router;
            }
        }

        return nullptr;
    }

    template <typename TComponent>
    TComponent* EnsureComponentOnPlayerController(APlayerController* PC)
    {
        if (!PC)
        {
            return nullptr;
        }

        if (TComponent* Existing = PC->FindComponentByClass<TComponent>())
        {
            return Existing;
        }

        TComponent* NewComp = NewObject<TComponent>(PC, TComponent::StaticClass(), NAME_None, RF_Transient);
        if (!NewComp)
        {
            return nullptr;
        }

        PC->AddInstanceComponent(NewComp);
        NewComp->OnComponentCreated();
        NewComp->RegisterComponent();
        return NewComp;
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

    if (USOTS_InputRouterComponent* Router = ResolveRouter(nullptr, ContextActor))
    {
        return Router;
    }

    if (!bLoggedMissing)
    {
        UE_LOG(LogTemp, Warning, TEXT("SOTS_Input: No USOTS_InputRouterComponent found for context actor %s."), *ContextActor->GetName());
        bLoggedMissing = true;
    }

    return nullptr;
}

USOTS_InputRouterComponent* USOTS_InputBlueprintLibrary::GetRouterForPlayerController(UObject* WorldContextObject, AActor* ContextActor)
{
    return ResolveRouter(WorldContextObject, ContextActor);
}

USOTS_InputRouterComponent* USOTS_InputBlueprintLibrary::EnsureRouterOnPlayerController(UObject* WorldContextObject, AActor* ContextActor)
{
    APlayerController* PC = ResolvePlayerController(WorldContextObject, ContextActor);
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("SOTS_Input: EnsureRouterOnPlayerController could not resolve a PlayerController."));
        return nullptr;
    }

    return EnsureComponentOnPlayerController<USOTS_InputRouterComponent>(PC);
}

USOTS_InputBufferComponent* USOTS_InputBlueprintLibrary::EnsureBufferOnPlayerController(UObject* WorldContextObject, AActor* ContextActor)
{
    APlayerController* PC = ResolvePlayerController(WorldContextObject, ContextActor);
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("SOTS_Input: EnsureBufferOnPlayerController could not resolve a PlayerController."));
        return nullptr;
    }

    return EnsureComponentOnPlayerController<USOTS_InputBufferComponent>(PC);
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
