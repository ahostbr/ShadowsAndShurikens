#include "SOTS_InteractionSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "SOTS_InteractableInterface.h"
#include "SOTS_InteractableComponent.h"
#include "SOTS_InteractionTrace.h"
#include "SOTS_InteractionTagAccess_Impl.h"
#include "SOTS_InteractionLog.h"
#include "SOTS_TagAccessHelpers.h"
#include "SOTS_GameplayTagManagerSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Components/ActorComponent.h"

USOTS_InteractionSubsystem::USOTS_InteractionSubsystem()
{
    UpdateIntervalSeconds = 0.10f; // 10 Hz default
    SearchRadius = 30.f;
    SearchDistance = 400.f;
    DistanceScoreWeight = 1.0f;
    FacingScoreWeight = 0.75f;
    bEnableDebug = false;

    UIIntent_InteractionPrompt = FGameplayTag();
    UIIntent_InteractionOptions = FGameplayTag();
    UIIntent_InteractionFail = FGameplayTag();
    UIFailReason_NoCandidate = FGameplayTag();
    UIFailReason_BlockedByTags = FGameplayTag();
    UIFailReason_InterfaceDenied = FGameplayTag();
}

bool USOTS_InteractionSubsystem::BuildViewContext(APlayerController* PC, FVector& OutViewLoc, FRotator& OutViewRot) const
{
    if (!PC) return false;
    PC->GetPlayerViewPoint(OutViewLoc, OutViewRot);
    return true;
}

void USOTS_InteractionSubsystem::UpdateCandidateThrottled(APlayerController* PlayerController)
{
    if (!PlayerController) return;

    UWorld* World = GetWorld();
    if (!World) return;

    FSOTS_InteractionCandidateState& State = CandidateStates.FindOrAdd(PlayerController);

    const double Now = World->GetTimeSeconds();
    if (Now < State.NextAllowedUpdateTimeSeconds)
    {
        return;
    }

    State.NextAllowedUpdateTimeSeconds = Now + FMath::Max(0.0f, UpdateIntervalSeconds);
    UpdateCandidateNow(PlayerController);
}

void USOTS_InteractionSubsystem::UpdateCandidateNow(APlayerController* PlayerController)
{
    if (!PlayerController) return;

    FVector ViewLoc; FRotator ViewRot;
    if (!BuildViewContext(PlayerController, ViewLoc, ViewRot))
    {
        return;
    }

    FSOTS_InteractionContext Best;
    bool bHasBest = false;
    FindBestCandidate(PlayerController, ViewLoc, ViewRot, Best, bHasBest);

    FSOTS_InteractionCandidateState& State = CandidateStates.FindOrAdd(PlayerController);

    const bool bChanged =
        (State.bHasCandidate != bHasBest) ||
        (bHasBest && !AreSameTarget(State.Current, Best));

    if (bChanged)
    {
        State.bHasCandidate = bHasBest;
        State.Current = bHasBest ? Best : FSOTS_InteractionContext();

        OnCandidateChanged.Broadcast(PlayerController, State.Current);

        if (UIIntent_InteractionPrompt.IsValid())
        {
            FSOTS_InteractionUIIntentPayload Payload;
            Payload.IntentTag = UIIntent_InteractionPrompt;
            Payload.Context = State.Current;
            Payload.bShowPrompt = State.bHasCandidate;
            BroadcastUIIntent(PlayerController, Payload);
        }
    }
}

void USOTS_InteractionSubsystem::ClearCandidate(APlayerController* PlayerController)
{
    if (!PlayerController) return;

    FSOTS_InteractionCandidateState& State = CandidateStates.FindOrAdd(PlayerController);
    const bool bWas = State.bHasCandidate;

    State.bHasCandidate = false;
    State.Current = FSOTS_InteractionContext();

    if (bWas)
    {
        OnCandidateChanged.Broadcast(PlayerController, State.Current);

        if (UIIntent_InteractionPrompt.IsValid())
        {
            FSOTS_InteractionUIIntentPayload Payload;
            Payload.IntentTag = UIIntent_InteractionPrompt;
            Payload.Context = State.Current;
            Payload.bShowPrompt = false;
            BroadcastUIIntent(PlayerController, Payload);
        }
    }
}

bool USOTS_InteractionSubsystem::GetCurrentCandidate(APlayerController* PlayerController, FSOTS_InteractionContext& OutContext) const
{
    if (!PlayerController) return false;

    const FSOTS_InteractionCandidateState* Found = CandidateStates.Find(PlayerController);
    if (!Found || !Found->bHasCandidate)
    {
        return false;
    }

    OutContext = Found->Current;
    return true;
}

void USOTS_InteractionSubsystem::RequestInteraction(APlayerController* PlayerController)
{
    if (!PlayerController) return;

    FSOTS_InteractionContext Context;
    if (!GetCurrentCandidate(PlayerController, Context))
    {
        if (UIIntent_InteractionFail.IsValid())
        {
            FSOTS_InteractionUIIntentPayload Payload;
            Payload.IntentTag = UIIntent_InteractionFail;
            Payload.FailReasonTag = UIFailReason_NoCandidate;
            BroadcastUIIntent(PlayerController, Payload);
        }
        return;
    }

    FGameplayTag FailReason;
    if (!ValidateCanInteract(Context, FailReason))
    {
        if (UIIntent_InteractionFail.IsValid())
        {
            FSOTS_InteractionUIIntentPayload Payload;
            Payload.IntentTag = UIIntent_InteractionFail;
            Payload.Context = Context;
            Payload.FailReasonTag = FailReason.IsValid() ? FailReason : UIFailReason_InterfaceDenied;
            if (!Payload.FailReasonTag.IsValid())
            {
                Payload.FailReasonTag = UIFailReason_BlockedByTags;
            }
            BroadcastUIIntent(PlayerController, Payload);
        }
        return;
    }

    TArray<FSOTS_InteractionOption> Options;
    GatherOptions(Context, Options);

    if (Options.Num() == 0)
    {
        FSOTS_InteractionOption DefaultOpt;
        DefaultOpt.OptionTag = Context.InteractionTypeTag;
        DefaultOpt.DisplayText = FText::GetEmpty();
        Options.Add(DefaultOpt);
    }

    if (Options.Num() == 1)
    {
        ExecuteOptionInternal(Context, Options[0].OptionTag);
        return;
    }

    if (UIIntent_InteractionOptions.IsValid())
    {
        FSOTS_InteractionUIIntentPayload Payload;
        Payload.IntentTag = UIIntent_InteractionOptions;
        Payload.Context = Context;
        Payload.Options = Options;
        BroadcastUIIntent(PlayerController, Payload);
    }
}

bool USOTS_InteractionSubsystem::ExecuteInteractionOption(APlayerController* PlayerController, FGameplayTag OptionTag)
{
    if (!PlayerController) return false;

    FSOTS_InteractionContext Context;
    if (!GetCurrentCandidate(PlayerController, Context))
    {
        return false;
    }

    FGameplayTag FailReason;
    if (!ValidateCanInteract(Context, FailReason))
    {
        return false;
    }

    return ExecuteOptionInternal(Context, OptionTag);
}

void USOTS_InteractionSubsystem::FindBestCandidate(APlayerController* PC, const FVector& ViewLoc, const FRotator& ViewRot, FSOTS_InteractionContext& OutBest, bool& bOutHasBest) const
{
    bOutHasBest = false;

    UWorld* World = GetWorld();
    if (!World || !PC) return;

    APawn* Pawn = PC->GetPawn();
    if (!Pawn) return;

    const FVector ViewDir = ViewRot.Vector();
    const FVector Start = ViewLoc;
    const FVector End = Start + (ViewDir * SearchDistance);

    TArray<FHitResult> Hits;
    TArray<const AActor*> Ignore;
    Ignore.Add(Pawn);

    const bool bHit = SOTSInteractionTrace::SphereSweepMulti(
        World,
        Hits,
        Start,
        End,
        SearchRadius,
        ECC_Visibility,
        Ignore,
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        bEnableDebug
#else
        false
#endif
    );

    if (!bHit)
    {
        return;
    }

    float BestScore = -FLT_MAX;

    for (const FHitResult& Hit : Hits)
    {
        AActor* HitActor = Hit.GetActor();
        if (!HitActor) continue;

        USOTS_InteractableComponent* Interactable = HitActor->FindComponentByClass<USOTS_InteractableComponent>();
        if (!Interactable) continue;

        FGameplayTag FailReason;
        if (!PassesTagGates(PC, Interactable, FailReason))
        {
            continue;
        }

        FSOTS_InteractionContext Ctx;
        if (!MakeContextForActor(PC, Pawn, HitActor, ViewLoc, ViewRot, Hit, Ctx))
        {
            continue;
        }

        // Gate by component max distance
        if (Interactable->MaxDistance > 0.f && Ctx.Distance > Interactable->MaxDistance)
        {
            continue;
        }

        // Gate by LOS if required
        if (Interactable->bRequiresLineOfSight && !PassesLOS(PC, ViewLoc, HitActor, Hit))
        {
            continue;
        }

        const float Score = ScoreCandidate(ViewLoc, ViewDir, Ctx);
        if (Score > BestScore)
        {
            BestScore = Score;
            OutBest = Ctx;
            bOutHasBest = true;
        }
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    // Optional debug draw (kept minimal)
    if (bEnableDebug)
    {
        DrawDebugLine(World, Start, End, FColor::Green, false, 0.05f, 0, 0.5f);
        if (bOutHasBest && OutBest.TargetActor.IsValid())
        {
            DrawDebugSphere(World, OutBest.TargetActor->GetActorLocation(), 12.f, 12, FColor::Yellow, false, 0.05f);
        }
    }
#endif
}

bool USOTS_InteractionSubsystem::MakeContextForActor(APlayerController* PC, APawn* Pawn, AActor* Target, const FVector& ViewLoc, const FRotator& ViewRot, const FHitResult& Hit, FSOTS_InteractionContext& OutContext) const
{
    if (!PC || !Pawn || !Target) return false;

    USOTS_InteractableComponent* Interactable = Target->FindComponentByClass<USOTS_InteractableComponent>();
    if (!Interactable) return false;

    (void)ViewLoc;
    (void)ViewRot;

    OutContext.PlayerController = PC;
    OutContext.PlayerPawn = Pawn;
    OutContext.TargetActor = Target;
    OutContext.InteractionTypeTag = Interactable->InteractionTypeTag;
    OutContext.Distance = FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation());
    OutContext.HitResult = Hit;

    return true;
}

bool USOTS_InteractionSubsystem::PassesLOS(APlayerController* PC, const FVector& ViewLoc, AActor* Target, const FHitResult& CandidateHit) const
{
    UWorld* World = GetWorld();
    if (!World || !PC || !Target) return false;

    FVector TargetLoc = CandidateHit.ImpactPoint;
    if (TargetLoc.IsNearlyZero())
    {
        TargetLoc = Target->GetActorLocation();
    }

    FHitResult LOSHit;
    TArray<const AActor*> Ignore;
    Ignore.Add(PC->GetPawn());
    Ignore.Add(Target);

    const bool bBlocked = SOTSInteractionTrace::LineTraceBlocked(
        World,
        LOSHit,
        ViewLoc,
        TargetLoc,
        ECC_Visibility,
        Ignore,
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        bEnableDebug
#else
        false
#endif
    );
    return !bBlocked;
}

float USOTS_InteractionSubsystem::ScoreCandidate(const FVector& ViewLoc, const FVector& ViewDir, const FSOTS_InteractionContext& Ctx) const
{
    const AActor* Target = Ctx.TargetActor.Get();
    if (!Target) return -FLT_MAX;

    // Distance score: nearer = better (normalized against search distance)
    const float DistNorm = FMath::Clamp(Ctx.Distance / FMath::Max(1.f, SearchDistance), 0.f, 1.f);
    const float DistanceScore = (1.f - DistNorm) * DistanceScoreWeight;

    // Facing score: dot of view dir vs direction to target
    const FVector ToTarget = (Target->GetActorLocation() - ViewLoc).GetSafeNormal();
    const float Dot = FVector::DotProduct(ViewDir.GetSafeNormal(), ToTarget);
    const float FacingScore = FMath::Clamp(Dot, 0.f, 1.f) * FacingScoreWeight;

    return DistanceScore + FacingScore;
}

bool USOTS_InteractionSubsystem::AreSameTarget(const FSOTS_InteractionContext& A, const FSOTS_InteractionContext& B)
{
    return A.TargetActor == B.TargetActor && A.InteractionTypeTag == B.InteractionTypeTag;
}

bool USOTS_InteractionSubsystem::PassesTagGates(APlayerController* PC, const USOTS_InteractableComponent* Interactable, FGameplayTag& OutFailReason) const
{
    OutFailReason = FGameplayTag();

    if (!PC || !Interactable)
    {
        return true;
    }

    if (Interactable->RequiredPlayerTags.IsEmpty() && Interactable->BlockedPlayerTags.IsEmpty())
    {
        return true;
    }

    AActor* OwnerActor = PC->GetPawn();
    if (!OwnerActor)
    {
        OwnerActor = PC;
    }

    if (!OwnerActor)
    {
        return true;
    }

    FGameplayTagContainer PlayerTags;
    const bool bHasProvider = SOTSInteractionTagAccess::TryGetPlayerTags(PC, PlayerTags);
    (void)PlayerTags;

    USOTS_GameplayTagManagerSubsystem* Manager = SOTS_GetTagSubsystem(PC);

    // Fail open if provider is unavailable.
    if (!bHasProvider || !Manager)
    {
        return true;
    }

    if (!Interactable->BlockedPlayerTags.IsEmpty() && Manager->ActorHasAnyTags(OwnerActor, Interactable->BlockedPlayerTags))
    {
        for (const FGameplayTag& Tag : Interactable->BlockedPlayerTags)
        {
            if (Manager->ActorHasTag(OwnerActor, Tag))
            {
                OutFailReason = Tag;
                break;
            }
        }
        if (!OutFailReason.IsValid() && Interactable->BlockedPlayerTags.Num() > 0)
        {
            OutFailReason = Interactable->BlockedPlayerTags.First();
        }
        return false;
    }

    if (!Interactable->RequiredPlayerTags.IsEmpty() && !Manager->ActorHasAllTags(OwnerActor, Interactable->RequiredPlayerTags))
    {
        for (const FGameplayTag& Tag : Interactable->RequiredPlayerTags)
        {
            if (!Manager->ActorHasTag(OwnerActor, Tag))
            {
                OutFailReason = Tag;
                break;
            }
        }
        if (!OutFailReason.IsValid() && Interactable->RequiredPlayerTags.Num() > 0)
        {
            OutFailReason = Interactable->RequiredPlayerTags.First();
        }
        return false;
    }

    return true;
}

bool USOTS_InteractionSubsystem::ValidateCanInteract(const FSOTS_InteractionContext& Context, FGameplayTag& OutFailReason) const
{
    OutFailReason = FGameplayTag();

    const AActor* Target = Context.TargetActor.Get();
    if (!Target) return false;

    if (const USOTS_InteractableComponent* IC = Target->FindComponentByClass<USOTS_InteractableComponent>())
    {
        if (!PassesTagGates(Context.PlayerController.Get(), IC, OutFailReason))
        {
            return false;
        }
    }

    if (UObject* Implementer = ResolveInteractableImplementer(Context))
    {
        return ISOTS_InteractableInterface::Execute_CanInteract(Implementer, Context, OutFailReason);
    }

    return true;
}

void USOTS_InteractionSubsystem::GatherOptions(const FSOTS_InteractionContext& Context, TArray<FSOTS_InteractionOption>& OutOptions) const
{
    OutOptions.Reset();

    const AActor* Target = Context.TargetActor.Get();
    if (!Target) return;

    if (UObject* Implementer = ResolveInteractableImplementer(Context))
    {
        ISOTS_InteractableInterface::Execute_GetInteractionOptions(Implementer, Context, OutOptions);
    }
}

bool USOTS_InteractionSubsystem::ExecuteOptionInternal(const FSOTS_InteractionContext& Context, FGameplayTag OptionTag)
{
    AActor* Target = Context.TargetActor.Get();
    if (!Target) return false;

    if (UObject* Implementer = ResolveInteractableImplementer(Context))
    {
        ISOTS_InteractableInterface::Execute_ExecuteInteraction(Implementer, Context, OptionTag);
        return true;
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    UE_LOG(LogSOTSInteraction, Verbose, TEXT("ExecuteOptionInternal: Target has no ISOTS_InteractableInterface; nothing executed."));
#endif
    return false;
}

void USOTS_InteractionSubsystem::BroadcastUIIntent(APlayerController* PlayerController, const FSOTS_InteractionUIIntentPayload& Payload)
{
    OnUIIntentPayload.Broadcast(PlayerController, Payload);

    // Deprecated legacy broadcast for existing bindings.
    if (OnUIIntent.IsBound())
    {
        OnUIIntent.Broadcast(PlayerController, Payload.IntentTag, Payload.Context, Payload.Options);
    }
}

UObject* USOTS_InteractionSubsystem::ResolveInteractableImplementer(const FSOTS_InteractionContext& Context) const
{
    AActor* Target = Context.TargetActor.Get();
    if (!Target)
    {
        return nullptr;
    }

    const UClass* InterfaceClass = USOTS_InteractableInterface::StaticClass();

    // Actor wins if it implements the interface.
    if (Target->GetClass()->ImplementsInterface(InterfaceClass))
    {
        return Target;
    }

    // Otherwise scan components and return first implementer.
    TArray<UActorComponent*> Components;
    Target->GetComponents(Components);
    for (UActorComponent* Comp : Components)
    {
        if (Comp && Comp->GetClass()->ImplementsInterface(InterfaceClass))
        {
            return Comp;
        }
    }

    return nullptr;
}
