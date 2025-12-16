#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/EngineTypes.h"
#include "SOTS_GlobalStealthTypes.generated.h"

class AActor;

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

UENUM(BlueprintType)
enum class ESOTS_StealthInputType : uint8
{
    Unknown      UMETA(DisplayName="Unknown"),
    Visibility   UMETA(DisplayName="Visibility"),
    Light        UMETA(DisplayName="Light"),
    Perception   UMETA(DisplayName="Perception"),
    Noise        UMETA(DisplayName="Noise"),
    Weather      UMETA(DisplayName="Weather"),
    Custom       UMETA(DisplayName="Custom")
};

UENUM(BlueprintType)
enum class ESOTS_AIAwarenessState : uint8
{
    Calm            UMETA(DisplayName="Calm"),
    Investigating   UMETA(DisplayName="Investigating"),
    Searching       UMETA(DisplayName="Searching"),
    Alerted         UMETA(DisplayName="Alerted"),
    Hostile         UMETA(DisplayName="Hostile")
};

UENUM()
enum class ESOTS_StealthIngestResult : uint8
{
    Accepted,
    Dropped_Invalid,
    Dropped_TooSoon,
    Dropped_OutOfWindow,
    Dropped_NoTarget,
    Dropped_DisabledType,
    Dropped_Internal
};

// Normalized, single-channel ingestion packet used by all GSM inputs.
USTRUCT()
struct FSOTS_StealthInputSample
{
    GENERATED_BODY()

    UPROPERTY()
    ESOTS_StealthInputType Type = ESOTS_StealthInputType::Unknown;

    // Caller-provided normalized strength (0..1). Clamped in ingestion.
    UPROPERTY()
    float StrengthNormalized = 0.0f;

    // Optional confidence for sources that can provide it (0..1).
    UPROPERTY()
    float Confidence = 1.0f;

    // Optional semantic tag for the source (e.g., Player.LightProbe).
    UPROPERTY()
    FGameplayTag SourceTag;

    // Actor that produced the input (e.g., guard, sensor).
    UPROPERTY()
    TWeakObjectPtr<AActor> InstigatorActor;

    // Actor the input is about (often the player).
    UPROPERTY()
    TWeakObjectPtr<AActor> TargetActor;

    // Optional world location for spatialized inputs.
    UPROPERTY()
    FVector WorldLocation = FVector::ZeroVector;

    // Absolute game time seconds when the sample was captured.
    UPROPERTY()
    double TimestampSeconds = 0.0;
};

USTRUCT()
struct FSOTS_StealthIngestReport
{
    GENERATED_BODY()

    UPROPERTY()
    ESOTS_StealthIngestResult Result = ESOTS_StealthIngestResult::Dropped_Internal;

    UPROPERTY()
    ESOTS_StealthInputType Type = ESOTS_StealthInputType::Unknown;

    UPROPERTY()
    float StrengthIn = 0.0f;

    UPROPERTY()
    float StrengthClamped = 0.0f;

    UPROPERTY()
    FString DebugReason;
};

USTRUCT(BlueprintType)
struct FSOTS_GSM_Handle
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stealth|Handle")
    FGuid Id;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stealth|Handle")
    FGameplayTag KindTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stealth|Handle")
    FGameplayTag OwnerTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stealth|Handle")
    double CreatedAtSeconds = 0.0;

    bool IsValid() const { return Id.IsValid(); }
};

UENUM(BlueprintType)
enum class ESOTS_GSM_RemoveResult : uint8
{
    Removed,
    NotFound,
    WrongKind,
    OwnerMismatch,
    InvalidHandle,
    InternalError
};

UENUM(BlueprintType)
enum class ESOTS_GSM_ResetReason : uint8
{
    Unknown,
    Initialize,
    ProfileLoaded,
    MissionStart,
    LevelTransition,
    Manual
};

USTRUCT(BlueprintType)
struct FSOTS_GSM_AwarenessThresholds
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|AI", meta=(ClampMin="0.0", ClampMax="1.0"))
    float InvestigatingMin01 = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|AI", meta=(ClampMin="0.0", ClampMax="1.0"))
    float SearchingMin01 = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|AI", meta=(ClampMin="0.0", ClampMax="1.0"))
    float AlertedMin01 = 0.80f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|AI", meta=(ClampMin="0.0", ClampMax="1.0"))
    float HostileMin01 = 0.95f;
};

USTRUCT(BlueprintType)
struct FSOTS_GSM_EvidenceConfig
{
    GENERATED_BODY()

    // Rolling window parameters.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|AI")
    int32 MaxEventsPerAI = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|AI")
    float WindowSeconds = 6.0f;

    // Evidence stacking.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|AI")
    float EvidencePointsPerEvent = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|AI")
    float MaxEvidencePoints = 6.0f;

    // Evidence influence on suspicion.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|AI")
    float MaxEvidenceSuspicionBonus = 0.25f;

    // Optional minimum strength gate for counting evidence.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|AI")
    float MinSuspicionToCountAsEvidence = 0.10f;

    // Optional reason multipliers (Sight/Hearing/Damage/etc.).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|AI")
    TMap<FGameplayTag, float> EvidenceReasonMultipliers;

    // Optional multiplier when the same instigator repeats.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|AI")
    float SameInstigatorMultiplier = 1.25f;

    // Numeric blend between prior and incoming suspicion.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|AI")
    float PriorSuspicionBlendAlpha = 0.35f;

    // Last reason wins switch (default true).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|AI")
    bool bLastReasonWins = true;
};

USTRUCT(BlueprintType)
struct FSOTS_GSM_GlobalAlertnessConfig
{
    GENERATED_BODY()

    // Baseline / bounds.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|Global")
    float Baseline = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|Global")
    float MinValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|Global")
    float MaxValue = 1.0f;

    // Decay toward baseline (units per second).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|Global")
    float DecayPerSecond = 0.05f;

    // Contribution from incoming reports.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|Global")
    float SuspicionWeight = 0.10f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|Global")
    float Weight_Unaware = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|Global")
    float Weight_Suspicious = 0.02f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|Global")
    float Weight_Investigating = 0.04f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|Global")
    float Weight_Alert = 0.07f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|Global")
    float Weight_Combat = 0.10f;

    // Optional smoothing/clamp.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|Global")
    float MaxIncreasePerSecond = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|Global")
    float UpdateMinDelta = 0.001f;

    // Optional evidence influence when available.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GSM|Global")
    float EvidenceBonusWeight = 0.05f;
};

USTRUCT(BlueprintType)
struct FSOTS_StealthScoreChange
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Stealth|Score")
    float OldScore = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Stealth|Score")
    float NewScore = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Stealth|Score")
    FGameplayTag ReasonTag;

    UPROPERTY(BlueprintReadOnly, Category="Stealth|Score")
    double TimestampSeconds = 0.0;
};

USTRUCT(BlueprintType)
struct FSOTS_StealthIngestTuning
{
    GENERATED_BODY()

    // Throttling and age guards (OFF by default to preserve existing behavior).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Ingest")
    bool bEnableIngestThrottle = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Ingest")
    bool bDropStaleSamples = false;

    // Max allowed age before drop when bDropStaleSamples is true.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Ingest", meta=(EditCondition="bDropStaleSamples", ClampMin="0.01"))
    float MaxSampleAgeSeconds = 1.0f;

    // Per-type throttles (seconds between accepted samples). 0 disables.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Ingest", meta=(EditCondition="bEnableIngestThrottle", ClampMin="0.0"))
    float MinSecondsBetweenVisibility = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Ingest", meta=(EditCondition="bEnableIngestThrottle", ClampMin="0.0"))
    float MinSecondsBetweenLight = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Ingest", meta=(EditCondition="bEnableIngestThrottle", ClampMin="0.0"))
    float MinSecondsBetweenPerception = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Ingest", meta=(EditCondition="bEnableIngestThrottle", ClampMin="0.0"))
    float MinSecondsBetweenNoise = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Ingest", meta=(EditCondition="bEnableIngestThrottle", ClampMin="0.0"))
    float MinSecondsBetweenWeather = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Ingest", meta=(EditCondition="bEnableIngestThrottle", ClampMin="0.0"))
    float MinSecondsBetweenCustom = 0.0f;

    // Dev-only diagnostics (off by default).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Ingest|Debug")
    bool bDebugLogStealthIngest = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Ingest|Debug")
    bool bDebugLogStealthDrops = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Ingest|Debug", meta=(ClampMin="0.0"))
    float DebugLogThrottleSeconds = 1.0f;
};

USTRUCT(BlueprintType)
struct FSOTS_GSM_TuningSummary
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Stealth|Summary")
    float LightWeight = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Stealth|Summary")
    float LOSWeight = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Stealth|Summary")
    float DistanceWeight = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Stealth|Summary")
    float NoiseWeight = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Stealth|Summary")
    float LocalVisibilityWeight = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Stealth|Summary")
    float AISuspicionWeight = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Stealth|Summary")
    float CautiousMin = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Stealth|Summary")
    float DangerMin = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Stealth|Summary")
    float CompromisedMin = 0.0f;
};

USTRUCT(BlueprintType)
struct FSOTS_StealthTierTuning
{
    GENERATED_BODY()

    // Thresholds for tier boundaries (Hidden->Cautious->Danger->Compromised).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Tier", meta=(ClampMin="0.0", ClampMax="1.0"))
    float CautiousMin = 0.20f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Tier", meta=(ClampMin="0.0", ClampMax="1.0"))
    float DangerMin = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Tier", meta=(ClampMin="0.0", ClampMax="1.0"))
    float CompromisedMin = 0.80f;

    // Hysteresis to prevent flicker near thresholds.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Tier")
    bool bEnableHysteresis = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Tier", meta=(ClampMin="0.0", ClampMax="0.5"))
    float HysteresisPadding = 0.0f;

    // Optional smoothing of the score used for tiering.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Tier")
    bool bEnableScoreSmoothing = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Tier", meta=(ClampMin="0.0"))
    float ScoreSmoothingHalfLifeSeconds = 0.0f;

    // Optional minimum dwell time between tier changes.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Tier", meta=(ClampMin="0.0"))
    float MinSecondsBetweenTierChanges = 0.0f;

    // Optional calm decay to drift score toward 0 when not refreshed.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Tier")
    bool bEnableCalmDecay = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Tier", meta=(ClampMin="0.0"))
    float CalmDecayPerSecond = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Tier", meta=(ClampMin="0.0"))
    float CalmDecayStartDelaySeconds = 0.0f;
};

USTRUCT(BlueprintType)
struct FSOTS_StealthTierTransition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Stealth|Tier")
    ESOTS_StealthTier OldTier = ESOTS_StealthTier::Hidden;

    UPROPERTY(BlueprintReadOnly, Category="Stealth|Tier")
    ESOTS_StealthTier NewTier = ESOTS_StealthTier::Hidden;

    UPROPERTY(BlueprintReadOnly, Category="Stealth|Tier")
    float OldScore = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Stealth|Tier")
    float NewScore = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Stealth|Tier")
    FGameplayTag ReasonTag;

    UPROPERTY(BlueprintReadOnly, Category="Stealth|Tier")
    double TimestampSeconds = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Stealth|Tier")
    TArray<FGameplayTag> ActiveModifierOwners;
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

    // Awareness state thresholds for AI suspicion mapping (ReportAISuspicionEx).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|AI")
    FSOTS_GSM_AwarenessThresholds AwarenessThresholds;

    // Evidence stacking and history tuning for AI suspicion ingestion.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|AI")
    FSOTS_GSM_EvidenceConfig EvidenceConfig;

    // Global alertness metric tuning.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Global")
    FSOTS_GSM_GlobalAlertnessConfig GlobalAlertnessConfig;

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

    // Input ingestion tuning (throttle, age guards, dev diagnostics).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Ingest")
    FSOTS_StealthIngestTuning IngestTuning;

    // Tier stability/smoothing tuning (defaults preserve previous behavior).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Tier")
    FSOTS_StealthTierTuning TierTuning;

    // Dev-only diagnostics for stack resolution.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Debug")
    bool bDebugDumpEffectiveTuning = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Debug")
    bool bDebugLogStackOps = false;

    // Optional repair path if tags drift away from the current tier (dev-only).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Debug")
    bool bRepairStealthTagsIfDriftDetected = false;

    // Optional reset logging (dev-only).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Debug")
    bool bDebugLogStealthResets = false;
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
struct FSOTS_GSM_AISuspicionEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    float Suspicion01 = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    FGameplayTag ReasonTag;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    bool bHasLocation = false;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    TWeakObjectPtr<AActor> InstigatorActor;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    double TimestampSeconds = 0.0;
};

USTRUCT(BlueprintType)
struct FSOTS_GSM_AIRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    TWeakObjectPtr<AActor> SubjectAI;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    float Suspicion01 = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    ESOTS_AIAwarenessState AwarenessState = ESOTS_AIAwarenessState::Calm;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    FGameplayTag AwarenessTierTag;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    FGameplayTag ReasonTag;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    FGameplayTag FocusTag;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    TArray<FSOTS_GSM_AISuspicionEvent> RecentEvents;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    bool bHasLocation = false;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    TWeakObjectPtr<AActor> InstigatorActor;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    double TimestampSeconds = 0.0;
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
