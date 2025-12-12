#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "SOTS_AIPerceptionTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class ESOTS_PerceptionState : uint8
{
    Unaware         UMETA(DisplayName="Unaware"),
    SoftSuspicious  UMETA(DisplayName="Soft Suspicious"),
    HardSuspicious  UMETA(DisplayName="Hard Suspicious"),
    Alerted         UMETA(DisplayName="Alerted")
};

USTRUCT(BlueprintType)
struct FSOTS_PerceivedTargetState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Perception")
    TWeakObjectPtr<AActor> Target;

    // Internal scoring for LOS; 0 = none, >0 = has LOS / strength.
    UPROPERTY(BlueprintReadOnly, Category="Perception")
    float SightScore = 0.f;

    // Internal scoring for hearing; decays over time.
    UPROPERTY(BlueprintReadOnly, Category="Perception")
    float HearingScore = 0.f;

    // Final awareness [0..1] after integrating sight/hearing/stealth.
    UPROPERTY(BlueprintReadOnly, Category="Perception")
    float Awareness = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="Perception")
    ESOTS_PerceptionState State = ESOTS_PerceptionState::Unaware;

    UPROPERTY(BlueprintReadOnly, Category="Perception")
    FVector LastKnownLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="Perception")
    float TimeSinceLastSeen = 0.f;
};

USTRUCT(BlueprintType)
struct FSOTS_AIGuardPerceptionConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI")
    float SightSuspicionPerSecond = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI")
    float HearingSuspicionPerEvent = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI")
    float MaxSuspicion = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI")
    float SuspicionDecayPerSecond = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Tags")
    FGameplayTag Tag_OnFullyAlerted;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Tags")
    FGameplayTag Tag_OnLostSight;

    // Optional FX tags fired on key perception transitions. These are
    // looked up via the central FX manager and are fully data-driven.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Perception|FX")
    FGameplayTag FXTag_OnSoftSuspicious;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Perception|FX")
    FGameplayTag FXTag_OnHardSuspicious;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Perception|FX")
    FGameplayTag FXTag_OnAlerted;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Perception|FX")
    FGameplayTag FXTag_OnLostSight;
};

/**
 * Blackboard key configuration so each AI archetype can decide which
 * blackboard entries should mirror perception/suspicion state.
 */
USTRUCT(BlueprintType)
struct FSOTS_AIPerceptionBlackboardConfig
{
    GENERATED_BODY()

    // Float: current normalized suspicion [0,1].
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Blackboard")
    FBlackboardKeySelector SuspicionKey;

    // Int/Enum: high-level perception state as ESOTS_PerceptionState index.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Blackboard")
    FBlackboardKeySelector StateKey;

    // Bool: whether this AI currently has line of sight to the primary target.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Blackboard")
    FBlackboardKeySelector HasLOSKey;
};
