#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SOTS_GlobalStealthTypes.h"
#include "SOTS_StealthConfigDataAsset.h"
#include "SOTS_GlobalStealthManagerSubsystem.generated.h"

class AActor;
class USOTS_PlayerStealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSOTS_StealthLevelChangedSignature, ESOTSStealthLevel, OldLevel, ESOTSStealthLevel, NewLevel, float, NewScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_PlayerDetectionStateChangedSignature, bool, bDetected);

/**
 * Global, non-ticking stealth aggregator.
 *
 * Other systems (player movement, cover, AI perception, Ultra Dynamic Sky traces)
 * report discrete samples and events into this subsystem. It computes a single,
 * authoritative stealth score and level that SOTS can use for gameplay, VFX,
 * and UI decisions.
 */
UCLASS()
class SOTS_GLOBALSTEALTHMANAGER_API USOTS_GlobalStealthManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    USOTS_GlobalStealthManagerSubsystem();

    // Easy access helper for Blueprints and C++
    UFUNCTION(BlueprintCallable, Category="Stealth", meta=(WorldContext="WorldContextObject"))
    static USOTS_GlobalStealthManagerSubsystem* Get(const UObject* WorldContextObject);

    // Last sample that was processed into the score.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stealth")
    FSOTS_StealthSample LastSample;

    // Current player-facing stealth state (light, cover, noise, AI, tiers).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stealth")
    FSOTS_PlayerStealthState CurrentState;

    // 0 = perfectly hidden, 1 = fully exposed/detected.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stealth")
    float CurrentStealthScore;

    // Discrete, layered stealth state built from the score and events.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stealth")
    ESOTSStealthLevel CurrentStealthLevel;

    // True if at least one enemy has actively "spotted" the player and triggered a fail/alert state.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stealth")
    bool bPlayerDetected;

    // Latest breakdown of how the score was composed, for UI/FX/debug.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTS|Stealth")
    FSOTS_StealthScoreBreakdown CurrentBreakdown;

    // Broadcast when the stealth level changes (e.g., for dragon visibility or radial vignette).
    UPROPERTY(BlueprintAssignable, Category="Stealth")
    FSOTS_StealthLevelChangedSignature OnStealthLevelChanged;

    // Broadcast when the binary detection state changes.
    UPROPERTY(BlueprintAssignable, Category="Stealth")
    FSOTS_PlayerDetectionStateChangedSignature OnPlayerDetectionStateChanged;

public:
    /**
     * Main entry point for the player/dragon: submit a new stealth sample.
     * This recalculates the score and, if thresholds are crossed, fires events.
     */
    UFUNCTION(BlueprintCallable, Category="Stealth")
    void ReportStealthSample(const FSOTS_StealthSample& Sample);

    /**
     * Called by AI or mission logic whenever an enemy explicitly detects or loses the player.
     * This drives the bPlayerDetected flag and the detection delegate.
     */
    UFUNCTION(BlueprintCallable, Category="Stealth")
    void ReportEnemyDetectionEvent(AActor* Source, bool bDetectedNow);

    UFUNCTION(BlueprintPure, Category="Stealth")
    float GetCurrentStealthScore() const { return CurrentStealthScore; }

    UFUNCTION(BlueprintPure, Category="Stealth")
    ESOTSStealthLevel GetCurrentStealthLevel() const { return CurrentStealthLevel; }

    UFUNCTION(BlueprintPure, Category="Stealth")
    bool IsPlayerDetected() const { return bPlayerDetected; }

    // Default config asset for this world (can be set on the CDO or via Mission Director).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Config")
    USOTS_StealthConfigDataAsset* DefaultConfigAsset;

    // Active scoring config used at runtime. Copied from the active asset so we can tweak/override if needed.
    UPROPERTY(Transient)
    FSOTS_StealthScoringConfig ActiveConfig;

    // High-level state accessors used by other systems (KEM, FX, dragon).
    UFUNCTION(BlueprintCallable, Category="Stealth")
    const FSOTS_PlayerStealthState& GetStealthState() const { return CurrentState; }

    UFUNCTION(BlueprintCallable, Category="Stealth")
    ESOTS_StealthTier GetStealthTier() const { return CurrentState.StealthTier; }

    // Returns the latest score breakdown snapshot (copied by value).
    UFUNCTION(BlueprintCallable, Category="SOTS|Stealth")
    FSOTS_StealthScoreBreakdown GetCurrentStealthBreakdown() const { return CurrentBreakdown; }

    /** Returns 0-1 final stealth/visibility score for an actor (0=fully hidden, 1=fully exposed). */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Stealth")
    float GetStealthScoreFor(const AActor* Actor) const;

    // Called when AI bridge updates suspicion (0..1).
    void SetAISuspicion(float In01);

    UFUNCTION(BlueprintCallable, Category="SOTS|Stealth|AI")
    void ReportAISuspicion(AActor* GuardActor, float SuspicionNormalized);

    // Event-driven update from the player component (no Tick required).
    void UpdateFromPlayer(const FSOTS_PlayerStealthState& PlayerState);

    // C++ delegate for systems that want the full state (FX, dragon, etc.).
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnStealthStateChanged, const FSOTS_PlayerStealthState&);
    FOnStealthStateChanged OnStealthStateChanged;

private:
    ESOTSStealthLevel EvaluateStealthLevelFromScore(float Score) const;
    void SetStealthLevel(ESOTSStealthLevel NewLevel);
    void SetPlayerDetected(bool bDetectedNow);

    void RecomputeGlobalScore();
    void UpdateGameplayTags();

    USOTS_PlayerStealthComponent* FindPlayerStealthComponent() const;
    void SyncPlayerStealthComponent();

    // Active modifiers applied on top of raw state.
    UPROPERTY()
    TArray<FSOTS_StealthModifier> ActiveModifiers;

    // Per-guard AI suspicion (normalized [0..1]) used to build the
    // aggregated AISuspicion term in the stealth score.
    UPROPERTY()
    TMap<TWeakObjectPtr<AActor>, float> GuardSuspicion;

    // Stack of config assets used for scoped overrides (e.g., per-mission).
    UPROPERTY()
    TArray<TObjectPtr<USOTS_StealthConfigDataAsset>> StealthConfigStack;

    // Currently active config asset driving ActiveConfig (may be null to mean "no override").
    UPROPERTY()
    TObjectPtr<USOTS_StealthConfigDataAsset> ActiveStealthConfig = nullptr;

public:
    UFUNCTION(BlueprintCallable, Category="Stealth|Modifiers")
    void AddStealthModifier(const FSOTS_StealthModifier& Modifier);

    UFUNCTION(BlueprintCallable, Category="Stealth|Modifiers")
    void RemoveStealthModifierBySource(FName SourceId);

    // Config API for Mission Director / difficulty systems.
    UFUNCTION(BlueprintCallable, Category="Stealth|Config")
    void SetStealthConfig(const FSOTS_StealthScoringConfig& InConfig);

    UFUNCTION(BlueprintCallable, Category="Stealth|Config")
    void SetStealthConfigAsset(USOTS_StealthConfigDataAsset* InAsset);

    // Scoped config override API (e.g., used by MissionDirector).
    UFUNCTION(BlueprintCallable, Category="SOTS|Stealth")
    void PushStealthConfig(USOTS_StealthConfigDataAsset* NewConfig);

    UFUNCTION(BlueprintCallable, Category="SOTS|Stealth")
    void PopStealthConfig();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
};
