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
    bForceLegacyTraces = false;
    ThrottleBypassViewDotThreshold = 0.98f;
    ThrottleBypassLocationThreshold = 35.f;
    bLogScoreDebug = false;
    bDebugDrawInteractionTraces = false;
    bDebugLogInteractionTraceHits = false;
    bDebugLogInteractionExecutionFailures = false;
    bWarnOnUnboundInteractionIntents = false;
    bWarnOnLegacyIntentUsage = false;
    bDebugLogIntentPayloads = false;
    WarnIntentCooldownSeconds = 2.0f;
    bWarnOnUnboundUIIntent = true;

    InteractionSequenceCounter = 1;
    LastWarnUnboundIntentTime = 0.0;
    LastWarnLegacyIntentTime = 0.0;

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

    FVector ViewLoc; FRotator ViewRot;
    if (!BuildViewContext(PlayerController, ViewLoc, ViewRot))
    {
        return;
    }

    FSOTS_InteractionCandidateState& State = CandidateStates.FindOrAdd(PlayerController);

    const double Now = World->GetTimeSeconds();
    const bool bBypassThrottle = ShouldBypassThrottle(State, ViewLoc, ViewRot);
    if (!bBypassThrottle && Now < State.NextAllowedUpdateTimeSeconds)
    {
        return;
    }

    State.NextAllowedUpdateTimeSeconds = Now + FMath::Max(0.0f, UpdateIntervalSeconds);
    State.LastViewLocation = ViewLoc;
    State.LastViewRotation = ViewRot;

    UpdateCandidateInternal(PlayerController, ViewLoc, ViewRot);
}

void USOTS_InteractionSubsystem::UpdateCandidateNow(APlayerController* PlayerController)
{
    if (!PlayerController) return;

    FVector ViewLoc; FRotator ViewRot;
    if (!BuildViewContext(PlayerController, ViewLoc, ViewRot))
    {
        return;
    }

    FSOTS_InteractionCandidateState& State = CandidateStates.FindOrAdd(PlayerController);
    State.LastViewLocation = ViewLoc;
    State.LastViewRotation = ViewRot;

    UpdateCandidateInternal(PlayerController, ViewLoc, ViewRot);
}

void USOTS_InteractionSubsystem::UpdateCandidateInternal(APlayerController* PlayerController, const FVector& ViewLoc, const FRotator& ViewRot)
{
    FSOTS_InteractionContext Best;
    bool bHasBest = false;
    ESOTS_InteractionNoCandidateReason NoCandidateReason = ESOTS_InteractionNoCandidateReason::None;
    bool bUsedOmniTrace = false;
    FindBestCandidate(PlayerController, ViewLoc, ViewRot, Best, bHasBest, NoCandidateReason, bUsedOmniTrace);

    FSOTS_InteractionCandidateState& State = CandidateStates.FindOrAdd(PlayerController);
    State.LastNoCandidateReason = NoCandidateReason;
    State.bLastUsedOmniTrace = bUsedOmniTrace;

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
    State.LastNoCandidateReason = ESOTS_InteractionNoCandidateReason::None;
    State.bLastUsedOmniTrace = false;

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

FSOTS_InteractionResult USOTS_InteractionSubsystem::RequestInteraction(APlayerController* PlayerController)
{
    FSOTS_InteractionResult Result;

    if (!PlayerController)
    {
        return Result;
    }

    FSOTS_InteractionExecuteReport Report = RequestInteraction_WithResult(PlayerController);

    const FSOTS_InteractionCandidateState* State = CandidateStates.Find(PlayerController);
    Result.bUsedOmniTrace = State ? State->bLastUsedOmniTrace : false;

    // Map new report to legacy result struct for back-compat.
    Result.Context = Report.Target.IsValid() ? Result.Context : FSOTS_InteractionContext();
    Result.ExecutedOptionTag = Report.OptionTag;
    switch (Report.Result)
    {
    case ESOTS_InteractionExecuteResult::Success:
        Result.Result = ESOTS_InteractionResultCode::Success;
        break;
    case ESOTS_InteractionExecuteResult::NoCandidate:
        Result.Result = ESOTS_InteractionResultCode::NoCandidate;
        Result.FailReasonTag = UIFailReason_NoCandidate;
        break;
    case ESOTS_InteractionExecuteResult::OptionNotFound:
    case ESOTS_InteractionExecuteResult::OptionNotAvailable:
        Result.Result = ESOTS_InteractionResultCode::OptionNotFound;
        break;
    case ESOTS_InteractionExecuteResult::CandidateNoLineOfSight:
        Result.Result = ESOTS_InteractionResultCode::BlockedByLOS;
        break;
    case ESOTS_InteractionExecuteResult::MissingInteractableInterface:
        Result.Result = ESOTS_InteractionResultCode::MissingInterface;
        break;
    case ESOTS_InteractionExecuteResult::MissingInteractableComponent:
        Result.Result = ESOTS_InteractionResultCode::MissingComponent;
        break;
    default:
        Result.Result = ESOTS_InteractionResultCode::CandidateInvalid;
        break;
    }

    // Preserve legacy UI behavior already handled by RequestInteraction_WithResult.
    return Result;
}

FSOTS_InteractionResult USOTS_InteractionSubsystem::ExecuteInteractionOption(APlayerController* PlayerController, FGameplayTag OptionTag)
{
    FSOTS_InteractionResult Result;
    if (!PlayerController) return Result;

    FSOTS_InteractionExecuteReport Report = ExecuteInteractionOption_WithResult(PlayerController, OptionTag);

    const FSOTS_InteractionCandidateState* State = CandidateStates.Find(PlayerController);
    Result.bUsedOmniTrace = State ? State->bLastUsedOmniTrace : false;

    Result.ExecutedOptionTag = Report.OptionTag;
    switch (Report.Result)
    {
    case ESOTS_InteractionExecuteResult::Success:
        Result.Result = ESOTS_InteractionResultCode::Success;
        break;
    case ESOTS_InteractionExecuteResult::NoCandidate:
        Result.Result = ESOTS_InteractionResultCode::NoCandidate;
        Result.FailReasonTag = UIFailReason_NoCandidate;
        break;
    case ESOTS_InteractionExecuteResult::CandidateNoLineOfSight:
        Result.Result = ESOTS_InteractionResultCode::BlockedByLOS;
        break;
    case ESOTS_InteractionExecuteResult::MissingInteractableInterface:
        Result.Result = ESOTS_InteractionResultCode::MissingInterface;
        break;
    case ESOTS_InteractionExecuteResult::MissingInteractableComponent:
        Result.Result = ESOTS_InteractionResultCode::MissingComponent;
        break;
    case ESOTS_InteractionExecuteResult::OptionNotFound:
    case ESOTS_InteractionExecuteResult::OptionNotAvailable:
        Result.Result = ESOTS_InteractionResultCode::OptionNotFound;
        break;
    default:
        Result.Result = ESOTS_InteractionResultCode::CandidateInvalid;
        break;
    }
    return Result;
}

FSOTS_InteractionExecuteReport USOTS_InteractionSubsystem::BuildExecuteReport(int32 SequenceId, ESOTS_InteractionExecuteResult Result, const FSOTS_InteractionContext& Context, const FGameplayTag& OptionTag, const FString& DebugReason) const
{
    FSOTS_InteractionExecuteReport Report;
    Report.SequenceId = SequenceId;
    Report.Result = Result;
    Report.OptionTag = OptionTag;
    Report.DebugReason = DebugReason;
    Report.Target = Context.TargetActor;
    Report.Distance = Context.Distance;

    if (Context.PlayerPawn.IsValid())
    {
        Report.Instigator = Context.PlayerPawn.Get();
    }
    else if (Context.PlayerController.IsValid())
    {
        Report.Instigator = Context.PlayerController.Get();
    }
    return Report;
}

FSOTS_InteractionExecuteReport USOTS_InteractionSubsystem::ExecuteInteractionInternal(const FSOTS_InteractionContext& Context, const FGameplayTag& OptionTag)
{
    const int32 SeqId = InteractionSequenceCounter++;
    FSOTS_InteractionExecuteReport Report = BuildExecuteReport(SeqId, ESOTS_InteractionExecuteResult::InternalError, Context, OptionTag, TEXT(""));

    if (!Context.TargetActor.IsValid())
    {
        Report.Result = ESOTS_InteractionExecuteResult::CandidateInvalid;
        Report.DebugReason = TEXT("Target invalid");
        return Report;
    }

    USOTS_InteractableComponent* Interactable = Context.TargetActor->FindComponentByClass<USOTS_InteractableComponent>();
    if (!Interactable)
    {
        Report.Result = ESOTS_InteractionExecuteResult::MissingInteractableComponent;
        Report.DebugReason = TEXT("Missing interactable component");
        return Report;
    }

    FGameplayTag FailReason;
    if (!PassesTagGates(Context.PlayerController.Get(), Interactable, FailReason))
    {
        Report.Result = ESOTS_InteractionExecuteResult::CandidateBlocked;
        Report.DebugReason = FailReason.IsValid() ? FailReason.ToString() : TEXT("Tag gate failed");
        return Report;
    }

    if (Interactable->MaxDistance > 0.f && Context.Distance > Interactable->MaxDistance)
    {
        Report.Result = ESOTS_InteractionExecuteResult::CandidateOutOfRange;
        Report.DebugReason = TEXT("Out of range");
        return Report;
    }

    if (Interactable->bRequiresLineOfSight)
    {
        FVector ViewLoc = Context.HitResult.TraceStart;
        FRotator ViewRot = FRotator::ZeroRotator;
        if (ViewLoc.IsNearlyZero() && Context.PlayerController.IsValid())
        {
            BuildViewContext(Context.PlayerController.Get(), ViewLoc, ViewRot);
        }

        bool bUsedOmniTrace = false;
        if (!PassesLOS(Context.PlayerController.Get(), ViewLoc, Context.TargetActor.Get(), Context.HitResult, bUsedOmniTrace))
        {
            Report.Result = ESOTS_InteractionExecuteResult::CandidateNoLineOfSight;
            Report.DebugReason = TEXT("LOS blocked");
            return Report;
        }
    }

    TArray<FSOTS_InteractionOption> Options;
    GatherOptions(Context, Options);
    if (Options.Num() > 0)
    {
        const FSOTS_InteractionOption* FoundOpt = Options.FindByPredicate([&OptionTag](const FSOTS_InteractionOption& Opt)
        {
            return Opt.OptionTag == OptionTag || (!Opt.OptionTag.IsValid() && !OptionTag.IsValid());
        });

        if (!FoundOpt)
        {
            Report.Result = ESOTS_InteractionExecuteResult::OptionNotFound;
            Report.DebugReason = TEXT("Option not found");
            return Report;
        }

        if (FoundOpt->BlockedReasonTag.IsValid())
        {
            Report.Result = ESOTS_InteractionExecuteResult::OptionNotAvailable;
            Report.DebugReason = FoundOpt->BlockedReasonTag.ToString();
            return Report;
        }
    }

    UObject* Implementer = ResolveInteractableImplementer(Context);
    if (!Implementer)
    {
        Report.Result = ESOTS_InteractionExecuteResult::MissingInteractableInterface;
        Report.DebugReason = TEXT("No interactable implementer");
        return Report;
    }

    // Respect CanInteract contract if present.
    if (!ISOTS_InteractableInterface::Execute_CanInteract(Implementer, Context, FailReason))
    {
        Report.Result = ESOTS_InteractionExecuteResult::ExecutionRejectedByTarget;
        Report.DebugReason = FailReason.IsValid() ? FailReason.ToString() : TEXT("CanInteract rejected");
        return Report;
    }

    ISOTS_InteractableInterface::Execute_ExecuteInteraction(Implementer, Context, OptionTag);

    Report.Result = ESOTS_InteractionExecuteResult::Success;
    return Report;
}

FSOTS_InteractionExecuteReport USOTS_InteractionSubsystem::RequestInteraction_WithResult(APlayerController* PlayerController)
{
    const int32 SeqId = InteractionSequenceCounter++;
    FSOTS_InteractionExecuteReport Report = BuildExecuteReport(SeqId, ESOTS_InteractionExecuteResult::NoCandidate, FSOTS_InteractionContext(), FGameplayTag(), TEXT(""));

    if (!PlayerController)
    {
        return Report;
    }

    FSOTS_InteractionContext Context;
    if (!GetCurrentCandidate(PlayerController, Context))
    {
        Report.DebugReason = TEXT("No candidate");
        OnInteractionExecuted.Broadcast(Report);
        if (UIIntent_InteractionFail.IsValid())
        {
            FSOTS_InteractionUIIntentPayload Payload;
            Payload.IntentTag = UIIntent_InteractionFail;
            Payload.FailReasonTag = UIFailReason_NoCandidate;
            BroadcastUIIntent(PlayerController, Payload);
        }
        return Report;
    }

    // Refresh report with candidate context.
    Report = BuildExecuteReport(SeqId, ESOTS_InteractionExecuteResult::NoCandidate, Context, FGameplayTag(), TEXT(""));

    TArray<FSOTS_InteractionOption> Options;
    GatherOptions(Context, Options);
    if (Options.Num() == 0)
    {
        FSOTS_InteractionOption DefaultOpt;
        DefaultOpt.OptionTag = Context.InteractionTypeTag;
        Options.Add(DefaultOpt);
    }

    if (Options.Num() == 1)
    {
        Report = ExecuteInteractionInternal(Context, Options[0].OptionTag);
    }
    else
    {
        Report = BuildExecuteReport(SeqId, ESOTS_InteractionExecuteResult::OptionNotAvailable, Context, FGameplayTag(), TEXT("Multiple options presented"));
        if (UIIntent_InteractionOptions.IsValid())
        {
            FSOTS_InteractionUIIntentPayload Payload;
            Payload.IntentTag = UIIntent_InteractionOptions;
            Payload.Context = Context;
            Payload.Options = Options;
            BroadcastUIIntent(PlayerController, Payload);
        }
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bDebugLogInteractionExecutionFailures && Report.Result != ESOTS_InteractionExecuteResult::Success)
    {
        const FString InstigatorName = Report.Instigator.IsValid() ? Report.Instigator->GetName() : TEXT("None");
        const FString TargetName = Report.Target.IsValid() ? Report.Target->GetName() : TEXT("None");
        UE_LOG(LogSOTSInteraction, Verbose, TEXT("[Exec] Result=%d Instigator=%s Target=%s Option=%s Reason=%s"),
            static_cast<int32>(Report.Result), *InstigatorName, *TargetName, *Report.OptionTag.ToString(), *Report.DebugReason);
    }
#endif

    OnInteractionExecuted.Broadcast(Report);
    return Report;
}

FSOTS_InteractionExecuteReport USOTS_InteractionSubsystem::ExecuteInteractionOption_WithResult(APlayerController* PlayerController, FGameplayTag OptionTag)
{
    const int32 SeqId = InteractionSequenceCounter++;
    FSOTS_InteractionExecuteReport Report = BuildExecuteReport(SeqId, ESOTS_InteractionExecuteResult::NoCandidate, FSOTS_InteractionContext(), OptionTag, TEXT(""));

    if (!PlayerController)
    {
        Report.DebugReason = TEXT("No player controller");
        OnInteractionExecuted.Broadcast(Report);
        return Report;
    }

    FSOTS_InteractionContext Context;
    if (!GetCurrentCandidate(PlayerController, Context))
    {
        Report.DebugReason = TEXT("No candidate");
        OnInteractionExecuted.Broadcast(Report);
        return Report;
    }

    Report = BuildExecuteReport(SeqId, ESOTS_InteractionExecuteResult::NoCandidate, Context, OptionTag, TEXT(""));

    // Ensure the requested option exists and is available before executing.
    TArray<FSOTS_InteractionOption> Options;
    GatherOptions(Context, Options);
    if (Options.Num() > 0)
    {
        const FSOTS_InteractionOption* FoundOpt = Options.FindByPredicate([&OptionTag](const FSOTS_InteractionOption& Opt)
        {
            return Opt.OptionTag == OptionTag || (!Opt.OptionTag.IsValid() && !OptionTag.IsValid());
        });

        if (!FoundOpt)
        {
            Report.Result = ESOTS_InteractionExecuteResult::OptionNotFound;
            Report.DebugReason = TEXT("Option not found");
            OnInteractionExecuted.Broadcast(Report);
            return Report;
        }

        if (FoundOpt->BlockedReasonTag.IsValid())
        {
            Report.Result = ESOTS_InteractionExecuteResult::OptionNotAvailable;
            Report.DebugReason = FoundOpt->BlockedReasonTag.ToString();
            OnInteractionExecuted.Broadcast(Report);
            return Report;
        }
    }

    Report = ExecuteInteractionInternal(Context, OptionTag);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bDebugLogInteractionExecutionFailures && Report.Result != ESOTS_InteractionExecuteResult::Success)
    {
        const FString InstigatorName = Report.Instigator.IsValid() ? Report.Instigator->GetName() : TEXT("None");
        const FString TargetName = Report.Target.IsValid() ? Report.Target->GetName() : TEXT("None");
        UE_LOG(LogSOTSInteraction, Verbose, TEXT("[Exec] Result=%d Instigator=%s Target=%s Option=%s Reason=%s"),
            static_cast<int32>(Report.Result), *InstigatorName, *TargetName, *Report.OptionTag.ToString(), *Report.DebugReason);
    }
#endif

    OnInteractionExecuted.Broadcast(Report);
    return Report;
}

void USOTS_InteractionSubsystem::FindBestCandidate(APlayerController* PC, const FVector& ViewLoc, const FRotator& ViewRot, FSOTS_InteractionContext& OutBest, bool& bOutHasBest, ESOTS_InteractionNoCandidateReason& OutNoCandidateReason, bool& bOutUsedOmniTrace) const
{
    bOutHasBest = false;
    OutNoCandidateReason = ESOTS_InteractionNoCandidateReason::None;
    bOutUsedOmniTrace = false;

    UWorld* World = GetWorld();
    if (!World || !PC)
    {
        OutNoCandidateReason = ESOTS_InteractionNoCandidateReason::Unknown;
        return;
    }

    APawn* Pawn = PC->GetPawn();
    if (!Pawn)
    {
        OutNoCandidateReason = ESOTS_InteractionNoCandidateReason::Unknown;
        return;
    }

    const FVector ViewDir = ViewRot.Vector();
    const FVector Start = ViewLoc;
    const FVector End = Start + (ViewDir * SearchDistance);

    TArray<FHitResult> Hits;
    TArray<const AActor*> Ignore;
    Ignore.Add(Pawn);

    bool bSweepUsedOmniTrace = false;
    const bool bHit = SOTSInteractionTrace::SphereSweepMulti(
        World,
        Hits,
        Start,
        End,
        SearchRadius,
        ECC_Visibility,
        Ignore,
        bForceLegacyTraces,
        bSweepUsedOmniTrace,
    #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        bDebugDrawInteractionTraces,
        bDebugLogInteractionTraceHits
    #else
        false,
        false
    #endif
    );
    bOutUsedOmniTrace = bOutUsedOmniTrace || bSweepUsedOmniTrace;

    if (!bHit)
    {
        OutNoCandidateReason = ESOTS_InteractionNoCandidateReason::NoHits;
        return;
    }

    float BestScore = -FLT_MAX;

    for (const FHitResult& Hit : Hits)
    {
        AActor* HitActor = Hit.GetActor();
        if (!HitActor)
        {
            OutNoCandidateReason = ESOTS_InteractionNoCandidateReason::NotInteractable;
            continue;
        }

        USOTS_InteractableComponent* Interactable = HitActor->FindComponentByClass<USOTS_InteractableComponent>();
        if (!Interactable)
        {
            OutNoCandidateReason = ESOTS_InteractionNoCandidateReason::NotInteractable;
            continue;
        }

        FGameplayTag FailReason;
        if (!PassesTagGates(PC, Interactable, FailReason))
        {
            OutNoCandidateReason = ESOTS_InteractionNoCandidateReason::BlockedByTags;
            continue;
        }

        FSOTS_InteractionContext Ctx;
        if (!MakeContextForActor(PC, Pawn, HitActor, ViewLoc, ViewRot, Hit, Ctx))
        {
            OutNoCandidateReason = ESOTS_InteractionNoCandidateReason::Unknown;
            continue;
        }

        // Gate by component max distance
        if (Interactable->MaxDistance > 0.f && Ctx.Distance > Interactable->MaxDistance)
        {
            OutNoCandidateReason = ESOTS_InteractionNoCandidateReason::OutOfRange;
            continue;
        }

        // Gate by LOS if required
        bool bLosUsedOmni = false;
        if (Interactable->bRequiresLineOfSight && !PassesLOS(PC, ViewLoc, HitActor, Hit, bLosUsedOmni))
        {
            OutNoCandidateReason = ESOTS_InteractionNoCandidateReason::BlockedByLOS;
            bOutUsedOmniTrace = bOutUsedOmniTrace || bLosUsedOmni;
            continue;
        }
        bOutUsedOmniTrace = bOutUsedOmniTrace || bLosUsedOmni;

        const float Score = ScoreCandidate(ViewLoc, ViewDir, Ctx);
        if (bLogScoreDebug)
        {
            LogCandidateScoreDebug(Ctx, Score);
        }
        if (Score > BestScore)
        {
            BestScore = Score;
            OutBest = Ctx;
            bOutHasBest = true;
            OutNoCandidateReason = ESOTS_InteractionNoCandidateReason::None;
        }
    }

    if (!bOutHasBest && OutNoCandidateReason == ESOTS_InteractionNoCandidateReason::None)
    {
        OutNoCandidateReason = ESOTS_InteractionNoCandidateReason::Unknown;
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

bool USOTS_InteractionSubsystem::PassesLOS(APlayerController* PC, const FVector& ViewLoc, AActor* Target, const FHitResult& CandidateHit, bool& bOutUsedOmniTrace) const
{
    UWorld* World = GetWorld();
    if (!World || !PC || !Target) return false;

    bOutUsedOmniTrace = false;

    FVector TargetLoc = CandidateHit.ImpactPoint;
    if (TargetLoc.IsNearlyZero())
    {
        TargetLoc = Target->GetActorLocation();
    }

    FHitResult LOSHit;
    TArray<const AActor*> Ignore;
    Ignore.Add(PC->GetPawn());
    Ignore.Add(Target);

    bool bLineUsedOmniTrace = false;
    const bool bBlocked = SOTSInteractionTrace::LineTraceBlocked(
        World,
        LOSHit,
        ViewLoc,
        TargetLoc,
        ECC_Visibility,
        Ignore,
        bForceLegacyTraces,
        bLineUsedOmniTrace,
    #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        bDebugDrawInteractionTraces,
        bDebugLogInteractionTraceHits
    #else
        false,
        false
    #endif
    );
    bOutUsedOmniTrace = bLineUsedOmniTrace;
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

bool USOTS_InteractionSubsystem::ShouldBypassThrottle(const FSOTS_InteractionCandidateState& State, const FVector& ViewLoc, const FRotator& ViewRot) const
{
    if (State.LastViewLocation.IsNearlyZero() && State.LastViewRotation.IsNearlyZero())
    {
        return false;
    }

    const float ViewDot = FVector::DotProduct(State.LastViewRotation.Vector(), ViewRot.Vector());
    const bool bViewChanged = ViewDot < ThrottleBypassViewDotThreshold;
    const bool bMovedFar = FVector::DistSquared(State.LastViewLocation, ViewLoc) > FMath::Square(ThrottleBypassLocationThreshold);

    return bViewChanged || bMovedFar;
}

void USOTS_InteractionSubsystem::LogCandidateScoreDebug(const FSOTS_InteractionContext& Context, float Score) const
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    const AActor* Target = Context.TargetActor.Get();
    const FString TargetName = Target ? Target->GetName() : TEXT("None");
    const FString TagString = Context.InteractionTypeTag.IsValid() ? Context.InteractionTypeTag.ToString() : TEXT("<None>");
    UE_LOG(LogSOTSInteraction, Verbose, TEXT("[Interaction] Candidate %s Score=%.3f Dist=%.1f Tag=%s"), *TargetName, Score, Context.Distance, *TagString);
#else
    (void)Context;
    (void)Score;
#endif
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
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    const double NowSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : FPlatformTime::Seconds();
    const FString InstigatorName = Payload.Context.PlayerController.IsValid() ? Payload.Context.PlayerController->GetName() : TEXT("None");
    const FString TargetName = Payload.Context.TargetActor.IsValid() ? Payload.Context.TargetActor->GetName() : TEXT("None");
    const float Distance = Payload.Context.Distance;

    if (bWarnOnUnboundInteractionIntents && !OnUIIntentPayload.IsBound() && !OnUIIntent.IsBound())
    {
        if (NowSeconds - LastWarnUnboundIntentTime > WarnIntentCooldownSeconds)
        {
            UE_LOG(LogSOTSInteraction, Warning, TEXT("[Intent] Dropped (no listeners) Intent=%s Instigator=%s Target=%s Dist=%.1f"), *Payload.IntentTag.ToString(), *InstigatorName, *TargetName, Distance);
            LastWarnUnboundIntentTime = NowSeconds;
        }
    }

    if (bDebugLogIntentPayloads)
    {
        const int32 OptionCount = Payload.Options.Num();
        UE_LOG(LogSOTSInteraction, Verbose, TEXT("[Intent] Broadcast Intent=%s Instigator=%s Target=%s Dist=%.1f Options=%d ShowPrompt=%s"), *Payload.IntentTag.ToString(), *InstigatorName, *TargetName, Distance, OptionCount, Payload.bShowPrompt ? TEXT("true") : TEXT("false"));
    }
#endif

    OnUIIntentPayload.Broadcast(PlayerController, Payload);

    // Deprecated legacy broadcast for existing bindings.
    if (OnUIIntent.IsBound())
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bWarnOnLegacyIntentUsage)
        {
            const double NowSecondsLegacy = GetWorld() ? GetWorld()->GetTimeSeconds() : FPlatformTime::Seconds();
            if (NowSecondsLegacy - LastWarnLegacyIntentTime > WarnIntentCooldownSeconds)
            {
                UE_LOG(LogSOTSInteraction, Warning, TEXT("[Intent] Legacy delegate used for %s"), *Payload.IntentTag.ToString());
                LastWarnLegacyIntentTime = NowSecondsLegacy;
            }
        }
#endif
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
