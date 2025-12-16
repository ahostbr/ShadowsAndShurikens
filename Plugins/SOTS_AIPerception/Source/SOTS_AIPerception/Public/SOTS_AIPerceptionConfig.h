#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SOTS_AIPerceptionTypes.h"
#include "SOTS_AIPerceptionConfig.generated.h"

class UCurveFloat;

/**
 * Per-archetype perception tuning for a single AI type.
 */
UCLASS(BlueprintType)
class SOTS_AIPERCEPTION_API USOTS_AIPerceptionConfig : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // --- Sight ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sight")
    float MaxSightDistance = 2500.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sight")
    float PeripheralSightDistance = 2000.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sight")
    float CoreFOVDegrees = 70.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sight")
    float PeripheralFOVDegrees = 110.f;

    // --- Detection Speeds ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Detection")
    float DetectionSpeed_Core = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Detection")
    float DetectionSpeed_Peripheral = 0.5f;

    // Suspicion decay for the overall guard state (mirrors GuardConfig->Config.DecayPerSecond).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Detection")
    float SuspicionDecayPerSecond = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Detection")
    float DetectionDecayPerSecond = 0.75f;

    // StealthScore (0–1) → detection multiplier.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Detection")
    TSoftObjectPtr<UCurveFloat> StealthScoreToDetectionMultiplier;

    // --- State thresholds (0–1) ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="States")
    float Threshold_SoftSuspicious = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="States")
    float Threshold_HardSuspicious = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="States")
    float Threshold_Alerted = 0.9f;

    // --- Hearing ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hearing")
    float HearingRadius_Quiet = 600.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hearing")
    float HearingRadius_Loud = 1600.f;

    // --- Tag mapping (applied on the AI itself) ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tags")
    FGameplayTag Tag_State_Unaware;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tags")
    FGameplayTag Tag_State_SoftSuspicious;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tags")
    FGameplayTag Tag_State_HardSuspicious;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tags")
    FGameplayTag Tag_State_Alerted;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Tags")
    FGameplayTag Tag_OnHasLOS_Player;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Tags")
    FGameplayTag Tag_OnLostLOS_Player;

    // --- Target Point LOS (multi-point, budgeted; default OFF) ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|TargetPoints")
    bool bEnableTargetPointLOS = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|TargetPoints")
    TArray<FName> TargetPointBones = { TEXT("head"), TEXT("spine_03"), TEXT("pelvis") };

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|TargetPoints", meta=(ClampMin="0"))
    int32 MaxPointTracesPerTarget = 3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|TargetPoints")
    bool bEarlyOutOnCorePointVisible = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|TargetPoints", meta=(ClampMin="0"))
    int32 MaxTargetsPerUpdate = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|TargetPoints", meta=(ClampMin="0"))
    int32 MaxTotalTracesPerUpdate = 8;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|TargetPoints")
    bool bFallbackToSinglePointWhenNoSkelMesh = true;

    // --- Target Point LOS -> Suspicion scaling ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|TargetPoints|Suspicion")
    bool bScaleSuspicionByVisibility = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|TargetPoints|Suspicion", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MinVisibilityFractionForSuspicion = 0.10f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|TargetPoints|Suspicion", meta=(ClampMin="0.0", ClampMax="2.0"))
    float VisibilitySuspicionMinMultiplier = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|TargetPoints|Suspicion", meta=(ClampMin="0.0", ClampMax="3.0"))
    float VisibilitySuspicionMaxMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|TargetPoints|Suspicion")
    bool bCorePointGivesFullMultiplier = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|TargetPoints|Suspicion", meta=(ClampMin="0.0", ClampMax="2.0"))
    float CorePointVisibleMultiplier = 1.0f;

    // --- Shadow awareness (GSM-driven; default OFF) ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|ShadowAwareness")
    bool bEnableShadowAwareness = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|ShadowAwareness", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ShadowMinIllumination = 0.65f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|ShadowAwareness", meta=(ClampMin="0.0"))
    float ShadowAwarenessMaxDistance = 1200.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|ShadowAwareness", meta=(ClampMin="0.05"))
    float ShadowCheckInterval = 0.40f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|ShadowAwareness", meta=(ClampMin="0.0"))
    float ShadowSuspicionGainPerSecond = 0.12f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|ShadowAwareness")
    int32 MaxShadowTracesPerSecondPerGuard = 2;

    // --- Telemetry (runtime logging; default OFF) ---

    // Disabled by default; when enabled, emits a throttled log line per guard summarizing trace/target usage.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|Telemetry")
    bool bEnablePerceptionTelemetry = false;

    // Minimum interval between telemetry log lines for a single guard.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|Telemetry", meta=(ClampMin="0.05"))
    float PerceptionTelemetryIntervalSeconds = 1.5f;

    // Include shadow awareness info (shadow traces + next schedule) in telemetry output.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|Telemetry")
    bool bTelemetryIncludeShadow = false;

    // Include target point visibility breakdown (visible/tested/core) in telemetry output.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|Telemetry")
    bool bTelemetryIncludeTargetPoints = false;

    // --- Debug ---

    // Draw evaluated target points (green visible / red blocked) for active target. Disabled by default.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|Debug")
    bool bDebugDrawTargetPoints = false;

    // Draw shadow awareness traces/points when shadow awareness runs. Disabled by default.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|Debug")
    bool bDebugDrawShadowAwareness = false;

    // Duration for debug draw primitives (seconds).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Perception|Debug", meta=(ClampMin="0.01"))
    float DebugDrawDuration = 0.1f;

    // Blackboard keys this AI archetype should drive.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Blackboard")
    FSOTS_AIPerceptionBlackboardConfig BlackboardConfig;
};



