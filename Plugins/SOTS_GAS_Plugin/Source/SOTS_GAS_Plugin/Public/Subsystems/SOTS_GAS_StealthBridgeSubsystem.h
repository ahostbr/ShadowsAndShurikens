// Copyright (c) 2025 USP45Master. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SOTS_GlobalStealthManagerSubsystem.h"
#include "SOTS_GlobalStealthTypes.h"
#include "SOTS_GAS_StealthBridgeSubsystem.generated.h"

/**
 * Lightweight GAS-facing bridge to the global stealth manager.
 * Provides cached access to the current stealth score and tier so
 * abilities can query stealth without knowing about GSM details.
 */
UCLASS()
class SOTS_GAS_PLUGIN_API USOTS_GAS_StealthBridgeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    USOTS_GAS_StealthBridgeSubsystem();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Latest global stealth score [0..1]. */
    UFUNCTION(BlueprintCallable, Category="SOTS|GAS|Stealth")
    float GetCurrentStealthScore() const { return CachedStealthScore; }

    /** Latest global stealth tier as an int (castable to ESOTS_StealthTier). */
    UFUNCTION(BlueprintCallable, Category="SOTS|GAS|Stealth")
    int32 GetCurrentStealthTier() const { return CachedStealthTier; }

private:
    UPROPERTY()
    USOTS_GlobalStealthManagerSubsystem* GlobalStealthSubsystem;

    // Cached values to avoid constant subsystem queries.
    float CachedStealthScore;
    int32 CachedStealthTier;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTS|GAS|Stealth", meta=(AllowPrivateAccess="true"))
    FSOTS_StealthScoreBreakdown CachedBreakdown;

    UFUNCTION()
    void HandleStealthLevelChanged(ESOTSStealthLevel OldLevel,
                                   ESOTSStealthLevel NewLevel,
                                   float NewScore);

public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|Stealth")
    FSOTS_StealthScoreBreakdown GetCurrentStealthBreakdown() const { return CachedBreakdown; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|Stealth")
    bool IsStealthTierAtLeast(int32 MinTier) const { return CachedBreakdown.StealthTier >= MinTier; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|Stealth")
    float GetCurrentStealthModifierMultiplier() const { return CachedBreakdown.ModifierMultiplier; }
};
