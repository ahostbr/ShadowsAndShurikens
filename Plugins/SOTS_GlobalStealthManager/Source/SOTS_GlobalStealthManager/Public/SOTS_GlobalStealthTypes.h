#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/EngineTypes.h"
#include "SOTS_GlobalStealthTypes.generated.h"

UENUM(BlueprintType)
enum class ESOTSStealthLevel : uint8
{
    Undetected     UMETA(DisplayName="Undetected"),
    LowRisk        UMETA(DisplayName="Low Risk"),
    MediumRisk     UMETA(DisplayName="Medium Risk"),
    HighRisk       UMETA(DisplayName="High Risk"),
    FullyDetected  UMETA(DisplayName="Fully Detected")
};

// High-level, player-facing stealth tier used for UI / KEM / FX.
UENUM(BlueprintType)
enum class ESOTS_StealthTier : uint8
{
    Hidden        UMETA(DisplayName="Hidden"),
    Cautious      UMETA(DisplayName="Cautious"),
    Danger        UMETA(DisplayName="Danger"),
    Compromised   UMETA(DisplayName="Compromised")
};

USTRUCT(BlueprintType)
struct FSOTS_PlayerStealthState
{
    GENERATED_BODY()

    // From LightProbe (0 = dark, 1 = bright).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth")
    float LightLevel01 = 0.f;

    // 1 - LightLevel01.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth")
    float ShadowLevel01 = 0.f;

    // 0 = fully covered, 1 = fully exposed.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth")
    float CoverExposure01 = 0.f;

    // From movement / parkour / footsteps.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth")
    float MovementNoise01 = 0.f;

    // Blended local result from player-side inputs.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth")
    float LocalVisibility01 = 0.f;

    // Aggregated from AI side.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth")
    float AISuspicion01 = 0.f;

    // Final global score for the game.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth")
    float GlobalStealthScore01 = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth")
    ESOTS_StealthTier StealthTier = ESOTS_StealthTier::Hidden;
};

// Tunable scoring configuration for the global stealth manager.
USTRUCT(BlueprintType)
struct FSOTS_StealthScoringConfig
{
    GENERATED_BODY()

    // --- Per-sample scoring (ReportStealthSample) ---

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Sample Weights")
    float LightWeight = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Sample Weights")
    float LOSWeight = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Sample Weights")
    float DistanceWeight = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Sample Weights")
    float NoiseWeight = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Sample Weights", meta=(ClampMin="1.0"))
    float DistanceClamp = 3000.0f;

    // --- Global blend (RecomputeGlobalScore) ---

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Global Blend")
    float LocalVisibilityWeight = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Global Blend")
    float AISuspicionWeight = 0.5f;

    // --- Stealth level thresholds (multi-level enum) ---

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Levels", meta=(ClampMin="0.0", ClampMax="1.0"))
    float UndetectedMax = 0.10f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Levels", meta=(ClampMin="0.0", ClampMax="1.0"))
    float LowRiskMax = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Levels", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MediumRiskMax = 0.65f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Levels", meta=(ClampMin="0.0", ClampMax="1.0"))
    float HighRiskMax = 0.99f;

    // --- Stealth tier thresholds (Hidden/Cautious/Danger/Compromised) ---

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Tiers", meta=(ClampMin="0.0", ClampMax="1.0"))
    float CautiousMin = 0.20f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Tiers", meta=(ClampMin="0.0", ClampMax="1.0"))
    float DangerMin = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Tiers", meta=(ClampMin="0.0", ClampMax="1.0"))
    float CompromisedMin = 0.80f;

    // --- Shadow candidate cache (used by AI perception) ---

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Shadow")
    bool bEnableShadowCandidateCache = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Shadow", meta=(ClampMin="0.01"))
    float ShadowCandidateUpdateInterval = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Shadow", meta=(ClampMin="0.0"))
    float ShadowCastDistance = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Shadow", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ShadowMinIlluminationForCandidate = 0.65f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Shadow")
    TEnumAsByte<ECollisionChannel> ShadowTraceChannel = ECC_Visibility;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Shadow")
    bool bShadowTraceComplex = false;

    // Debug visualization for the cached shadow candidate (off by default).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Shadow")
    bool bDebugShadowCandidateCache = false;
};

// Simple, additive stealth modifier applied on top of the raw state.
USTRUCT(BlueprintType)
struct FSOTS_StealthModifier
{
    GENERATED_BODY()

    // Unique source ID (e.g., ability or effect name).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth")
    FName SourceId = NAME_None;

    // Multipliers.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth")
    float LightMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth")
    float VisibilityMultiplier = 1.0f;

    // Additive offsets (applied before clamp).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth")
    float LightOffset01 = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth")
    float GlobalScoreOffset01 = 0.0f;
};

// Lightweight breakdown of how the current stealth score was composed.
USTRUCT(BlueprintType)
struct FSOTS_StealthScoreBreakdown
{
    GENERATED_BODY()

    // Raw light value [0,1] before any tier mapping.
    UPROPERTY(BlueprintReadOnly, Category="SOTS|Stealth")
    float Light01 = 0.0f;

    // Raw shadow value [0,1] (usually 1 - Light01).
    UPROPERTY(BlueprintReadOnly, Category="SOTS|Stealth")
    float Shadow01 = 0.0f;

    // Player-local visibility [0,1] after blending light / cover / motion.
    UPROPERTY(BlueprintReadOnly, Category="SOTS|Stealth")
    float Visibility01 = 0.0f;

    // Aggregated AI suspicion term [0,1].
    UPROPERTY(BlueprintReadOnly, Category="SOTS|Stealth")
    float AISuspicion01 = 0.0f;

    // Final combined score [0,1] after weighting and modifiers.
    UPROPERTY(BlueprintReadOnly, Category="SOTS|Stealth")
    float CombinedScore01 = 0.0f;

    // Current stealth tier as an integer (cast from ESOTS_StealthTier).
    UPROPERTY(BlueprintReadOnly, Category="SOTS|Stealth")
    int32 StealthTier = 0;

    // Effective multiplier applied by modifiers (EffectiveGlobal / RawScore).
    UPROPERTY(BlueprintReadOnly, Category="SOTS|Stealth")
    float ModifierMultiplier = 1.0f;
};

USTRUCT(BlueprintType)
struct FSOTS_ShadowCandidate
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Stealth|Shadow")
    bool bValid = false;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Stealth|Shadow")
    FVector ShadowPointWS = FVector::ZeroVector;

    // Direction light travels (e.g., sun direction).
    UPROPERTY(BlueprintReadOnly, Category="SOTS|Stealth|Shadow")
    FVector DominantLightDirWS = FVector::ForwardVector;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Stealth|Shadow")
    float Illumination01 = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Stealth|Shadow")
    float Strength01 = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Stealth|Shadow")
    double LastUpdateTimeSeconds = 0.0;
};

/**
 * A single stealth sample reported by the player, dragon, or other systems.
 * This is intentionally simple and numeric so SOTS can tune scoring later.
 */
USTRUCT(BlueprintType)
struct FSOTS_StealthSample
{
    GENERATED_BODY()

    // Absolute time of the sample (Game Time Seconds from UWorld)
    UPROPERTY(BlueprintReadWrite, Category="Stealth")
    float TimeSeconds = 0.0f;

    // 0 = total darkness, 1 = fully lit. You can feed in your own line trace / UDS data.
    UPROPERTY(BlueprintReadWrite, Category="Stealth", meta=(ClampMin="0.0", ClampMax="1.0"))
    float LightExposure = 0.0f;

    // Distance in cm to the nearest relevant enemy (0 if overlapping).
    UPROPERTY(BlueprintReadWrite, Category="Stealth", meta=(ClampMin="0.0"))
    float DistanceToNearestEnemy = 100000.0f;

    // True if at least one enemy currently has a clear line of sight to the player.
    UPROPERTY(BlueprintReadWrite, Category="Stealth")
    bool bInEnemyLineOfSight = false;

    // Aggregate noise value around the player. 0 = silent, 1 = extremely loud.
    UPROPERTY(BlueprintReadWrite, Category="Stealth", meta=(ClampMin="0.0", ClampMax="1.0"))
    float NoiseLevel = 0.0f;

    // True if currently in a valid cover state (from your cover component).
    UPROPERTY(BlueprintReadWrite, Category="Stealth")
    bool bInCover = false;

    // Optional context tags (e.g., Player.States.Crouched, Environment.Foggy, Dragon.Ability.Active)
    UPROPERTY(BlueprintReadWrite, Category="Stealth")
    FGameplayTagContainer ContextTags;
};
