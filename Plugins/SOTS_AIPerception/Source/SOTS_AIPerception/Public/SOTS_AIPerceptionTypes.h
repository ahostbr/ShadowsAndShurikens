#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "Logging/LogMacros.h"
#include "SOTS_AIPerceptionTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSOTS_AIPerception, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogSOTS_AIPerceptionTelemetry, Log, All);

class AActor;

UENUM(BlueprintType)
enum class ESOTS_PerceptionState : uint8
{
    Unaware         UMETA(DisplayName="Unaware"),
    SoftSuspicious  UMETA(DisplayName="Soft Suspicious"),
    HardSuspicious  UMETA(DisplayName="Hard Suspicious"),
    Alerted         UMETA(DisplayName="Alerted")
};

UENUM(BlueprintType)
enum class ESOTS_AIPerceptionResetReason : uint8
{
    Unknown     UMETA(DisplayName="Unknown"),
    BeginPlay   UMETA(DisplayName="Begin Play"),
    EndPlay     UMETA(DisplayName="End Play"),
    TargetChanged UMETA(DisplayName="Target Changed"),
    MissionStart UMETA(DisplayName="Mission Start"),
    ProfileLoaded UMETA(DisplayName="Profile Loaded"),
    Manual      UMETA(DisplayName="Manual")
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

    // Visibility bookkeeping for multi-point LOS.
    UPROPERTY(BlueprintReadOnly, Category="Perception|TargetPoints")
    float LastVisibilityFraction = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Perception|TargetPoints")
    int32 LastVisiblePoints = 0;

    UPROPERTY(BlueprintReadOnly, Category="Perception|TargetPoints")
    int32 LastTestedPoints = 0;

    UPROPERTY(BlueprintReadOnly, Category="Perception|TargetPoints")
    bool bLastAnyCoreVisible = false;
};

USTRUCT(BlueprintType)
struct FSOTS_TargetPointVisibilityResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Perception|TargetPoints")
    int32 TestedPoints = 0;

    UPROPERTY(BlueprintReadOnly, Category="Perception|TargetPoints")
    int32 VisiblePoints = 0;

    UPROPERTY(BlueprintReadOnly, Category="Perception|TargetPoints")
    bool bAnyCorePointVisible = false;

    UPROPERTY(BlueprintReadOnly, Category="Perception|TargetPoints")
    float VisibilityFraction = 0.0f;
};

USTRUCT(BlueprintType)
struct FSOTS_LastPerceptionStimulus
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|AI|Perception")
    float Strength01 = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|AI|Perception")
    FGameplayTag SenseTag;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|AI|Perception")
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|AI|Perception")
    TWeakObjectPtr<AActor> SourceActor;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|AI|Perception")
    double TimestampSeconds = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|AI|Perception")
    bool bSuccessfullySensed = false;

    double GetAgeSeconds(double NowSeconds) const
    {
        return (NowSeconds > TimestampSeconds) ? (NowSeconds - TimestampSeconds) : 0.0;
    }
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

    // --- Suspicion tuning (stability / hysteresis) ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Suspicion")
    float RampUpPerSecond_Sight = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Suspicion")
    float RampUpPerSecond_Hearing = 0.75f; // 0.75 * 0.2s ~= 0.15 per noise event (matches prior default).

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Suspicion")
    float RampUpPerSecond_Shadow = 0.12f; // matches previous ShadowSuspicionGainPerSecond.

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Suspicion")
    float RampUpPerSecond_Damage = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Suspicion")
    float DecayPerSecond = 0.1f; // matches previous SuspicionDecayPerSecond.

    // Detection hysteresis thresholds (normalized 0..1).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Suspicion", meta=(ClampMin="0.0", ClampMax="1.0"))
    float SpottedThreshold01 = 0.9f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Suspicion", meta=(ClampMin="0.0", ClampMax="1.0"))
    float LostThreshold01 = 0.85f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Suspicion")
    bool bEnableHysteresis = true;

    // Optional dwell/flip gates (default OFF to preserve feel).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Suspicion", meta=(ClampMin="0.0"))
    float MinSecondsInSpottedState = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Suspicion", meta=(ClampMin="0.0"))
    float MinSecondsBetweenStateFlips = 0.0f;

    // Delay before decay begins after last stimulus (0 = immediate, matches prior behavior).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Suspicion", meta=(ClampMin="0.0"))
    float StimulusForgetDelaySeconds = 0.0f;

    // Event throttles for future SPINE_3 payloads.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Suspicion", meta=(ClampMin="0.0", ClampMax="1.0"))
    float SuspicionChangeEpsilon01 = 0.01f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Suspicion", meta=(ClampMin="0.0"))
    float SuspicionEventMinIntervalSeconds = 0.10f;

    // GSM reporting controls (normalized AISuspicion01 feed).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|GSM")
    bool bEnableGSMReporting = true;

    // Minimum time between GSM reports (seconds).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|GSM", meta=(ClampMin="0.0"))
    float GSMReportMinIntervalSeconds = 0.15f;

    // Minimum delta in suspicion01 required to trigger a report.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|GSM", meta=(ClampMin="0.0"))
    float GSMReportMinDelta01 = 0.01f;

    // Optional sense reason tags forwarded with GSM reports (all optional/empty by default).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|GSM|Tags")
    FGameplayTag ReasonTag_Sight;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|GSM|Tags")
    FGameplayTag ReasonTag_Hearing;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|GSM|Tags")
    FGameplayTag ReasonTag_Shadow;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|GSM|Tags")
    FGameplayTag ReasonTag_Damage;

    // Fallback when no specific sense tag is set.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|GSM|Tags")
    FGameplayTag ReasonTag_Generic;

    // Dev-only: emit a verbose log if GSM cannot be resolved (off by default).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|GSM|Debug")
    bool bDebugLogMissingGSM = false;

    // Dev-only: emit perception event logs (spotted/lost/suspicion changed). Off by default.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Debug")
    bool bDebugLogAIPerceptionEvents = false;
};

USTRUCT(BlueprintType)
struct FSOTS_AIPerceptionTelemetry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Perception")
    float Suspicion01 = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Perception")
    bool bDetected = false;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Perception")
    FGameplayTag ReasonSenseTag;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Perception")
    FVector LastStimulusWorldLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Perception")
    TWeakObjectPtr<AActor> LastStimulusActor;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Perception")
    double LastStimulusTimeSeconds = 0.0;
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
