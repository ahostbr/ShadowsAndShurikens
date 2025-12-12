#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "SOTS_ProfileTypes.h"
#include "SOTSFXTypes.h"
#include "SOTS_FXManagerSubsystem.generated.h"

class USOTS_FXCueDefinition;
class UNiagaraComponent;
class UAudioComponent;
class USOTS_FXDefinitionLibrary;

/**
 * Internal pool per cue definition.
 * Not exposed to BP – purely runtime plumbing.
 */
USTRUCT()
struct FSOTS_FXCuePool
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<TObjectPtr<UNiagaraComponent>> NiagaraComponents;

    UPROPERTY()
    TArray<TObjectPtr<UAudioComponent>> AudioComponents;
};

UCLASS()
class SOTS_FX_PLUGIN_API USOTS_FXManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Global accessor – no world/context required on the BP side. */
    static USOTS_FXManagerSubsystem* Get();

    /** Register a cue at runtime (e.g., from GameInstance BP). */
    UFUNCTION(BlueprintCallable, Category = "SOTS|FX")
    void RegisterCue(USOTS_FXCueDefinition* CueDefinition);

    // Internal helpers – Blueprint calls go through the static library.
    FSOTS_FXHandle PlayCueByTag(FGameplayTag CueTag, const FSOTS_FXContext& Context);
    FSOTS_FXHandle PlayCueDefinition(USOTS_FXCueDefinition* CueDefinition, const FSOTS_FXContext& Context);

    /** Internal debug helper to count active FX components. */
    void GetActiveFXCountsInternal(int32& OutActiveNiagara, int32& OutActiveAudio) const;

    /** Internal helper to enumerate currently registered cues. */
    void GetRegisteredCuesInternal(TArray<FGameplayTag>& OutTags, TArray<USOTS_FXCueDefinition*>& OutDefinitions) const;

    /** Returns a human-readable summary of the current global FX toggles. */
    UFUNCTION(BlueprintPure, Category="SOTS|FX|Debug")
    FString GetFXSettingsSummary() const;

    /** Global FX toggles persisted via FSOTS_FXProfileData. */
    UFUNCTION(BlueprintCallable, Category="SOTS|FX|Toggles")
    void SetBloodEnabled(bool bEnabled);

    UFUNCTION(BlueprintPure, Category="SOTS|FX|Toggles")
    bool IsBloodEnabled() const { return bBloodEnabled; }

    UFUNCTION(BlueprintCallable, Category="SOTS|FX|Toggles")
    void SetHighIntensityFXEnabled(bool bEnabled);

    UFUNCTION(BlueprintPure, Category="SOTS|FX|Toggles")
    bool IsHighIntensityFXEnabled() const { return bHighIntensityFX; }

    UFUNCTION(BlueprintCallable, Category="SOTS|FX|Toggles")
    void SetCameraMotionFXEnabled(bool bEnabled);

    UFUNCTION(BlueprintPure, Category="SOTS|FX|Toggles")
    bool IsCameraMotionFXEnabled() const { return bCameraMotionFXEnabled; }

    /** Profile integration */
    void BuildProfileData(FSOTS_FXProfileData& OutData) const;
    void ApplyProfileData(const FSOTS_FXProfileData& InData);

    /** Stubbed FX cue request entry point for other subsystems. */
    UFUNCTION(BlueprintCallable, Category="SOTS|FX|Cues")
    void RequestFXCue(FGameplayTag FXCueTag, AActor* Instigator, AActor* Target);

    // -------------------------
    // Tag-driven FX job router
    // -------------------------

    /** Libraries searched to resolve FX tags into definitions. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|FX", meta=(AllowPrivateAccess="true"))
    TArray<TObjectPtr<USOTS_FXDefinitionLibrary>> Libraries;

    /** Broadcast when an FX job is requested and resolved. */
    UPROPERTY(BlueprintAssignable, Category="SOTS|FX")
    FSOTS_OnFXTriggered OnFXTriggered;

    /** Triggers a one-shot FX job in world space. */
    UFUNCTION(BlueprintCallable, Category="SOTS|FX", meta=(WorldContext="WorldContextObject"))
    void TriggerFXByTag(UObject* WorldContextObject,
                        FGameplayTag FXTag,
                        AActor* Instigator,
                        AActor* Target,
                        FVector Location,
                        FRotator Rotation);

    /** Triggers an FX job attached to a component/socket. */
    UFUNCTION(BlueprintCallable, Category="SOTS|FX", meta=(WorldContext="WorldContextObject"))
    void TriggerAttachedFXByTag(UObject* WorldContextObject,
                                FGameplayTag FXTag,
                                AActor* Instigator,
                                AActor* Target,
                                USceneComponent* AttachComponent,
                                FName AttachSocketName);

protected:

    /** Tag → Cue definition map. */
    UPROPERTY()
    TMap<FGameplayTag, TObjectPtr<USOTS_FXCueDefinition>> CueMap;

    /** Cue definition → component pools. */
    UPROPERTY()
    TMap<TObjectPtr<USOTS_FXCueDefinition>, FSOTS_FXCuePool> CuePools;

private:

    static TWeakObjectPtr<USOTS_FXManagerSubsystem> SingletonInstance;

    FSOTS_FXHandle SpawnCue_Internal(UWorld* World, USOTS_FXCueDefinition* CueDefinition, const FSOTS_FXContext& Context);

    // Pool helpers
    UNiagaraComponent* AcquireNiagaraComponent(UWorld* World, USOTS_FXCueDefinition* CueDefinition);
    UAudioComponent* AcquireAudioComponent(UWorld* World, USOTS_FXCueDefinition* CueDefinition);

    void ApplyContextToNiagara(UNiagaraComponent* NiagaraComp, USOTS_FXCueDefinition* CueDefinition, const FSOTS_FXContext& Context);
    void ApplyContextToAudio(UAudioComponent* AudioComp, USOTS_FXCueDefinition* CueDefinition, const FSOTS_FXContext& Context);

    // New helper path for gameplay-tag-driven FX jobs.
    const FSOTS_FXDefinition* FindDefinition(FGameplayTag FXTag) const;
    void BroadcastResolvedFX(FGameplayTag FXTag,
                             AActor* Instigator,
                             AActor* Target,
                             const FSOTS_FXDefinition* Definition,
                             const FVector& Location,
                             const FRotator& Rotation,
                             ESOTS_FXSpawnSpace Space,
                             USceneComponent* AttachComponent,
                             FName AttachSocketName);

    bool bBloodEnabled = true;
    bool bHighIntensityFX = true;
    bool bCameraMotionFXEnabled = true;
};
