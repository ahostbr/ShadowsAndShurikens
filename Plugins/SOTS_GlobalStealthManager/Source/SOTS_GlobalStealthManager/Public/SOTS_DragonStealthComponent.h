#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SOTS_GlobalStealthTypes.h"
#include "SOTS_DragonStealthComponent.generated.h"

/**
 * Lightweight listener component for the dragon companion.
 * It mirrors the latest global stealth state and exposes simple
 * Blueprint getters for VFX / animation graphs to query.
 */
UCLASS(ClassGroup=(SOTS), meta=(BlueprintSpawnableComponent))
class SOTS_GLOBALSTEALTHMANAGER_API USOTS_DragonStealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USOTS_DragonStealthComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintPure, Category="Stealth")
    float GetLightLevel01() const { return CachedState.LightLevel01; }

    UFUNCTION(BlueprintPure, Category="Stealth")
    float GetShadowLevel01() const { return CachedState.ShadowLevel01; }

    UFUNCTION(BlueprintPure, Category="Stealth")
    float GetGlobalStealthScore01() const { return CachedState.GlobalStealthScore01; }

    UFUNCTION(BlueprintPure, Category="Stealth")
    ESOTS_StealthTier GetStealthTier() const { return CachedState.StealthTier; }

private:
    void HandleStealthStateChanged(const FSOTS_PlayerStealthState& InState);

    FSOTS_PlayerStealthState CachedState;
};

