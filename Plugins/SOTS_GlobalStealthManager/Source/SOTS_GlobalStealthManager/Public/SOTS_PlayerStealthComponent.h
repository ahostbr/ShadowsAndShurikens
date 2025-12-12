#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SOTS_GlobalStealthTypes.h"
#include "SOTS_PlayerStealthComponent.generated.h"

class USOTS_GlobalStealthManagerSubsystem;
class USOTS_GameplayTagManagerSubsystem;

/**
 * Lightweight, non-ticking component that owns the local player stealth state.
 * Other systems (LightProbe, Cover, Movement, AI bridge) feed normalized inputs
 * and this blends them into LocalVisibility01, then forwards the state to the
 * global stealth manager subsystem.
 */
UCLASS(ClassGroup=(SOTS), meta=(BlueprintSpawnableComponent))
class SOTS_GLOBALSTEALTHMANAGER_API USOTS_PlayerStealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USOTS_PlayerStealthComponent();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stealth")
    FSOTS_PlayerStealthState State;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Stealth")
    ESOTS_StealthTier CurrentTier = ESOTS_StealthTier::Hidden;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Stealth")
    bool bIsInCover = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Stealth")
    bool bIsInShadow = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Stealth")
    bool bIsDetected = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTS|Stealth")
    float LastDetectionScore = 0.0f;

    // Weights for blending into LocalVisibility01.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Weights", meta=(ClampMin="0.0"))
    float Weight_Light;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Weights", meta=(ClampMin="0.0"))
    float Weight_Cover;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth|Weights", meta=(ClampMin="0.0"))
    float Weight_Motion;

    // Inputs from other systems (all clamped to [0,1]).
    UFUNCTION(BlueprintCallable, Category="Stealth")
    void SetLightLevel(float In01);

    UFUNCTION(BlueprintCallable, Category="Stealth")
    void SetCoverExposure(float In01);

    UFUNCTION(BlueprintCallable, Category="Stealth")
    void SetMovementNoise(float In01);

    UFUNCTION(BlueprintCallable, Category="Stealth")
    void SetAISuspicion(float In01);

    // Main recompute function (event-driven, no Tick).
    UFUNCTION(BlueprintCallable, Category="Stealth")
    void RecomputeLocalVisibility();

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLocalVisibilityUpdated, float, NewValue);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStealthTierChanged, ESOTS_StealthTier, NewTier);

    UPROPERTY(BlueprintAssignable, Category="SOTS|Stealth")
    FOnStealthTierChanged OnStealthTierChanged;

    UFUNCTION(BlueprintCallable, Category="SOTS|Stealth")
    void SetStealthTier(ESOTS_StealthTier NewTier);

    UFUNCTION(BlueprintCallable, Category="SOTS|Stealth")
    void SetIsInCover(bool bNewInCover);

    UFUNCTION(BlueprintCallable, Category="SOTS|Stealth")
    void SetIsInShadow(bool bNewInShadow);

    UFUNCTION(BlueprintCallable, Category="SOTS|Stealth")
    void SetDetected(bool bNewDetected);

    void OnDetectionScoreUpdated(float NewScore);

    UPROPERTY(BlueprintAssignable, Category="Stealth")
    FOnLocalVisibilityUpdated OnLocalVisibilityUpdated;

    UFUNCTION(BlueprintPure, Category="Stealth")
    const FSOTS_PlayerStealthState& GetState() const { return State; }

private:
    virtual void BeginPlay() override;

    void PushStateToGlobalManager();

    USOTS_GameplayTagManagerSubsystem* GetTagManager() const;
    void UpdateTagPresence(const FGameplayTag& Tag, bool bShouldHave);
    static FGameplayTag MakeTierTag(ESOTS_StealthTier Tier);

    FGameplayTag CurrentTierTag;
};

