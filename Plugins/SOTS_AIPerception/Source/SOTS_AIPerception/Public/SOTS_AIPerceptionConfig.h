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

    // Blackboard keys this AI archetype should drive.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Blackboard")
    FSOTS_AIPerceptionBlackboardConfig BlackboardConfig;
};
