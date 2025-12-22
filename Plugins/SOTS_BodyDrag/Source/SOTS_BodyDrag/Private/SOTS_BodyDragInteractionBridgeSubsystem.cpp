#include "SOTS_BodyDragInteractionBridgeSubsystem.h"
#include "SOTS_InteractionSubsystem.h"
#include "SOTS_InteractionTypes.h"
#include "SOTS_BodyDragPlayerComponent.h"
#include "SOTS_BodyDragLog.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

namespace
{
    const FGameplayTag TAG_InteractionVerb_DragStart = FGameplayTag::RequestGameplayTag(TEXT("Interaction.Verb.DragStart"), false);
    const FGameplayTag TAG_InteractionVerb_DragStop = FGameplayTag::RequestGameplayTag(TEXT("Interaction.Verb.DragStop"), false);

    USOTS_BodyDragPlayerComponent* ResolveBodyDragPlayerComponent(const FSOTS_InteractionActionRequest& Request)
    {
        AActor* Instigator = Request.InstigatorActor.Get();
        if (APlayerController* PC = Cast<APlayerController>(Instigator))
        {
            if (APawn* Pawn = PC->GetPawn())
            {
                Instigator = Pawn;
            }
        }

        if (!Instigator)
        {
            return nullptr;
        }

        if (APawn* Pawn = Cast<APawn>(Instigator))
        {
            if (USOTS_BodyDragPlayerComponent* Comp = Pawn->FindComponentByClass<USOTS_BodyDragPlayerComponent>())
            {
                return Comp;
            }
        }

        return Instigator->FindComponentByClass<USOTS_BodyDragPlayerComponent>();
    }
}

void USOTS_BodyDragInteractionBridgeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    TryBindInteractionSubsystem();
    if (!bBoundToInteraction)
    {
        ScheduleRetryBind();
    }
}

void USOTS_BodyDragInteractionBridgeSubsystem::Deinitialize()
{
    if (bBoundToInteraction && CachedInteractionSubsystem.IsValid())
    {
        CachedInteractionSubsystem->OnInteractionActionRequested.RemoveDynamic(this, &USOTS_BodyDragInteractionBridgeSubsystem::HandleInteractionActionRequested);
    }

    CancelRetryBind();

    bBoundToInteraction = false;
    CachedInteractionSubsystem.Reset();

    Super::Deinitialize();
}

bool USOTS_BodyDragInteractionBridgeSubsystem::TryBindInteractionSubsystem()
{
    if (bBoundToInteraction)
    {
        return true;
    }

    if (!CachedInteractionSubsystem.IsValid())
    {
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            CachedInteractionSubsystem = GameInstance->GetSubsystem<USOTS_InteractionSubsystem>();
        }
    }

    if (USOTS_InteractionSubsystem* Interaction = CachedInteractionSubsystem.Get())
    {
        Interaction->OnInteractionActionRequested.RemoveDynamic(this, &USOTS_BodyDragInteractionBridgeSubsystem::HandleInteractionActionRequested);
        Interaction->OnInteractionActionRequested.AddDynamic(this, &USOTS_BodyDragInteractionBridgeSubsystem::HandleInteractionActionRequested);
        bBoundToInteraction = true;
        bLoggedBindingFailure = false;
        return true;
    }

    return false;
}

void USOTS_BodyDragInteractionBridgeSubsystem::ScheduleRetryBind()
{
    if (bBoundToInteraction)
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        if (!World->GetTimerManager().IsTimerActive(BindingRetryHandle))
        {
            World->GetTimerManager().SetTimer(BindingRetryHandle, this, &USOTS_BodyDragInteractionBridgeSubsystem::OnRetryBindTick, 2.0f, true);
        }
    }
}

void USOTS_BodyDragInteractionBridgeSubsystem::CancelRetryBind()
{
    if (UWorld* World = GetWorld())
    {
        if (World->GetTimerManager().IsTimerActive(BindingRetryHandle))
        {
            World->GetTimerManager().ClearTimer(BindingRetryHandle);
        }
    }
}

void USOTS_BodyDragInteractionBridgeSubsystem::OnRetryBindTick()
{
    if (TryBindInteractionSubsystem())
    {
        CancelRetryBind();
        return;
    }

    if (!bLoggedBindingFailure)
    {
        UE_LOG(LogSOTSBodyDrag, Verbose, TEXT("BodyDrag bridge waiting for Interaction subsystem to bind."));
        bLoggedBindingFailure = true;
    }
}

void USOTS_BodyDragInteractionBridgeSubsystem::HandleInteractionActionRequested(const FSOTS_InteractionActionRequest& Request)
{
    if (!Request.VerbTag.IsValid())
    {
        return;
    }

    const bool bIsDragStart = Request.VerbTag.MatchesTagExact(TAG_InteractionVerb_DragStart);
    const bool bIsDragStop = Request.VerbTag.MatchesTagExact(TAG_InteractionVerb_DragStop);
    if (!bIsDragStart && !bIsDragStop)
    {
        return;
    }

    USOTS_BodyDragPlayerComponent* PlayerComp = ResolveBodyDragPlayerComponent(Request);
    if (!PlayerComp)
    {
        UE_LOG(LogSOTSBodyDrag, Verbose, TEXT("Interaction drag verb ignored: no BodyDrag component on instigator %s."), *GetNameSafe(Request.InstigatorActor.Get()));
        return;
    }

    if (bIsDragStart)
    {
        if (!Request.TargetActor.IsValid())
        {
            UE_LOG(LogSOTSBodyDrag, Verbose, TEXT("Interaction drag start ignored: no target actor."));
            return;
        }

        PlayerComp->TryBeginDrag(Request.TargetActor.Get());
        return;
    }

    PlayerComp->TryDropBody();
}
