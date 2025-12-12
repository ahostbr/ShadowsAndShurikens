#include "SOTS_ExecutionHelperActor.h"

#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "SOTS_KEM_ManagerSubsystem.h"
#include "MotionWarpingComponent.h"

ASOTS_ExecutionHelperActor::ASOTS_ExecutionHelperActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ASOTS_ExecutionHelperActor::Initialize(const FSOTS_ExecutionContext& InContext,
                                            const USOTS_SpawnExecutionData* InData,
                                            const FTransform& InSpawnTransform,
                                            const USOTS_KEM_ExecutionDefinition* InExecutionDefinition,
                                            const FSOTS_KEM_OmniTraceWarpResult& InWarpResult)
{
    if (!InExecutionDefinition || !InData)
    {
        UE_LOG(LogSOTS_KEM, Warning,
            TEXT("[KEM][Helper] Initialize aborted because ExecutionDefinition or SpawnData was null"));
        Destroy();
        return;
    }

    Context = InContext;
    SpawnData = InData;
    SpawnTransform = InSpawnTransform;
    ExecutionDefinition = InExecutionDefinition;
    WarpResult = InWarpResult;

    UE_LOG(LogSOTS_KEM, Log,
        TEXT("[KEM][Helper] Initialize: Helper=%s Instigator=%s Target=%s bHasHelperSpawn=%s NumWarpTargets=%d SpawnLoc=%s"),
        *GetName(),
        *GetNameSafe(Context.Instigator.Get()),
        *GetNameSafe(Context.Target.Get()),
        WarpResult.bHasHelperSpawn ? TEXT("true") : TEXT("false"),
        WarpResult.WarpTargets.Num(),
        *SpawnTransform.GetLocation().ToString());

    // We deliberately do NOT call SetActorTransform here; the deferred spawn
    // already uses SpawnTransform. Any additional movement should happen via
    // MoveComponentTo / motion warping driven by the stored WarpResult.
}

void ASOTS_ExecutionHelperActor::PrePlaySpawnMontages_Implementation()
{
    // Default behavior:
    // - If WarpResult has a valid runtime warp target, register it with the instigator first.
    // - Else, resolve ExecutionDefinition warp points using SpawnData's candidate names (instigator first, then target).
    // - Optionally nudge a configured target warp point into place before montages start.
    // Blueprint authors: call the parent implementation first to ensure default warp placement is applied before adjusting per-execution details.

    if (!ExecutionDefinition || !SpawnData)
    {
        return;
    }

    AActor* InstigatorActor = Context.Instigator.Get();
    AActor* Target = Context.Target.Get();

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (WarpResult.bHasHelperSpawn)
    {
        SetActorTransform(WarpResult.HelperSpawnTransform);
        SpawnTransform = WarpResult.HelperSpawnTransform;
    }

    bool bAppliedWarpResult = false;
    if (InstigatorActor && WarpResult.WarpTargets.Num() > 0)
    {
        const FName PreferredWarpPoint = SpawnData->InstigatorWarpPointNames.Num() > 0
            ? SpawnData->InstigatorWarpPointNames[0]
            : NAME_None;

        const FSOTS_KEM_WarpRuntimeTarget* ChosenTarget = nullptr;
        if (PreferredWarpPoint != NAME_None)
        {
            ChosenTarget = WarpResult.WarpTargets.FindByPredicate([PreferredWarpPoint](const FSOTS_KEM_WarpRuntimeTarget& Entry)
            {
                return Entry.TargetName == PreferredWarpPoint;
            });
        }

        if (!ChosenTarget)
        {
            ChosenTarget = &WarpResult.WarpTargets[0];
        }

        auto RegisterWarpTarget = [&](const FSOTS_KEM_WarpRuntimeTarget& RuntimeTarget) -> bool
        {
            if (!InstigatorActor)
            {
                return false;
            }

            if (UMotionWarpingComponent* MotionWarp = InstigatorActor->FindComponentByClass<UMotionWarpingComponent>())
            {
                MotionWarp->AddOrUpdateWarpTargetFromTransform(RuntimeTarget.TargetName, RuntimeTarget.TargetTransform);
                return true;
            }
            return false;
        };

        if (ChosenTarget && RegisterWarpTarget(*ChosenTarget))
        {
            bAppliedWarpResult = true;
        }

        for (const FSOTS_KEM_WarpRuntimeTarget& RuntimeTarget : WarpResult.WarpTargets)
        {
            RegisterWarpTarget(RuntimeTarget);
        }
    }

    if (bAppliedWarpResult)
    {
        return;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        return;
    }

    if (USOTS_KEMManagerSubsystem* Subsystem = GameInstance->GetSubsystem<USOTS_KEMManagerSubsystem>())
    {
        const auto TryResolveWarpPoints = [&](const TArray<FName>& Candidates, AActor* Actor, bool bTransformActor) -> bool
        {
            if (!Actor)
            {
                return false;
            }

            if (Candidates.Num() == 0)
            {
                return false;
            }

            for (const FName& CandidateName : Candidates)
            {
                if (CandidateName.IsNone())
                {
                    continue;
                }

                FTransform WarpTransform;
                if (!Subsystem->ResolveWarpPointByName(ExecutionDefinition, CandidateName, Context, WarpTransform))
                {
                    continue;
                }

                if (UMotionWarpingComponent* MotionWarp = Actor->FindComponentByClass<UMotionWarpingComponent>())
                {
                    MotionWarp->AddOrUpdateWarpTargetFromTransform(CandidateName, WarpTransform);
                }

                if (bTransformActor)
                {
                    Actor->SetActorTransform(WarpTransform);
                }

                return true;
            }

            return false;
        };

        if (TryResolveWarpPoints(SpawnData->InstigatorWarpPointNames, InstigatorActor, false))
        {
            return;
        }

        TryResolveWarpPoints(SpawnData->TargetWarpPointNames, Target, true);
    }
}

void ASOTS_ExecutionHelperActor::BeginPlay()
{
    Super::BeginPlay();

    if (!ExecutionDefinition || !SpawnData)
    {
        UE_LOG(LogSOTS_KEM, Warning,
            TEXT("[KEM][Helper] BeginPlay aborted due to missing ExecutionDefinition or SpawnData"));
        NotifyExecutionEnded(false);
        Destroy();
        return;
    }

    if (!Context.Instigator.IsValid())
    {
        UE_LOG(LogSOTS_KEM, Warning,
            TEXT("[KEM][Helper] BeginPlay aborted because Instigator is invalid"));
        NotifyExecutionEnded(false);
        Destroy();
        return;
    }

    // Give Blueprint a chance to do any spatial / warp work before we fire the montages.
    PrePlaySpawnMontages();

    PlayMontages();
}

void ASOTS_ExecutionHelperActor::NotifyExecutionEnded(bool bWasSuccessful)
{
    if (!ExecutionDefinition)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        return;
    }

    if (USOTS_KEMManagerSubsystem* Subsystem = GameInstance->GetSubsystem<USOTS_KEMManagerSubsystem>())
    {
        Subsystem->NotifyExecutionEnded(Context, ExecutionDefinition, bWasSuccessful);
    }
}

void ASOTS_ExecutionHelperActor::PlayMontages()
{
    if (!SpawnData)
    {
        // Nothing to play; treat as a failed execution and notify KEM.
        NotifyExecutionEnded(false);

        Destroy();
        return;
    }

    auto PlayOn = [](TWeakObjectPtr<AActor> ActorPtr, UAnimMontage* Montage)
    {
        if (!ActorPtr.IsValid() || !Montage)
        {
            return;
        }

        if (USkeletalMeshComponent* Mesh = ActorPtr->FindComponentByClass<USkeletalMeshComponent>())
        {
            if (UAnimInstance* Anim = Mesh->GetAnimInstance())
            {
                Anim->Montage_Play(Montage);
            }
        }
    };

    PlayOn(Context.Instigator, SpawnData->InstigatorMontage);
    PlayOn(Context.Target,     SpawnData->TargetMontage);

    const float InstLen = SpawnData->InstigatorMontage ? SpawnData->InstigatorMontage->GetPlayLength() : 0.f;
    const float TgtLen  = SpawnData->TargetMontage     ? SpawnData->TargetMontage->GetPlayLength()     : 0.f;

    const float MaxLen = FMath::Max(InstLen, TgtLen);
    if (MaxLen <= 0.f)
    {
        NotifyExecutionEnded(false);
        Destroy();
    }
    else
    {
        // Helper will auto-destroy shortly after the longest montage finishes.
        const float LifeSpan = MaxLen + 0.25f;
        SetLifeSpan(LifeSpan);
    }
}
