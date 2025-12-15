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
    float GetLightLevel01() const { return LightLevel01; }

    UFUNCTION(BlueprintPure, Category="Stealth")
    float GetShadowLevel01() const { return CachedState.ShadowLevel01; }

    UFUNCTION(BlueprintPure, Category="Stealth")
    float GetGlobalStealthScore01() const { return CachedState.GlobalStealthScore01; }

    UFUNCTION(BlueprintPure, Category="Stealth")
    ESOTS_StealthTier GetStealthTier() const { return CachedState.StealthTier; }

    UFUNCTION(BlueprintPure, Category="Stealth")
    float GetAIPerceptionLevel01() const { return AIPerceptionLevel01; }

    UFUNCTION(BlueprintPure, Category="Stealth")
    float GetOverallDetection01() const { return OverallDetection01; }

private:
    void HandleStealthStateChanged(const FSOTS_PlayerStealthState& InState);

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Dragon|Stealth", meta=(AllowPrivateAccess="true", ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
    float AIPerceptionLevel01 = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Dragon|Stealth", meta=(AllowPrivateAccess="true", ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
    float LightLevel01 = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Dragon|Stealth", meta=(AllowPrivateAccess="true", ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
    float OverallDetection01 = 0.0f;

    FSOTS_PlayerStealthState CachedState;
};
