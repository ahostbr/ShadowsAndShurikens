#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "SOTS_ProfileSnapshotProvider.h"
#include "SOTS_ProfileTypes.h"
#include "SOTSFXTypes.h"
#include "UObject/SoftObjectPtr.h"
#include "SOTS_FXManagerSubsystem.generated.h"

class USOTS_FXCueDefinition;
class UNiagaraComponent;
class UAudioComponent;
class USOTS_FXDefinitionLibrary;

/** Configurable registration for a hard-referenced FX library. */
USTRUCT(BlueprintType)
struct FSOTS_FXLibraryRegistration
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|FX")
    TObjectPtr<USOTS_FXDefinitionLibrary> Library = nullptr;

    /** Higher priority wins before registration order. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|FX")
    int32 Priority = 0;
};

/** Optional soft reference registration for deferred loading. */
USTRUCT(BlueprintType)
struct FSOTS_FXSoftLibraryRegistration
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|FX")
    TSoftObjectPtr<USOTS_FXDefinitionLibrary> Library;

    /** Higher priority wins before registration order. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|FX")
    int32 Priority = 0;
};

/** Internal bookkeeping for registered libraries. */
USTRUCT()
struct FSOTS_FXRegisteredLibrary
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<USOTS_FXDefinitionLibrary> Library = nullptr;

    UPROPERTY()
    int32 Priority = 0;

    UPROPERTY()
    int32 RegistrationOrder = INDEX_NONE;

    bool IsValid() const { return Library != nullptr; }
};

/** Map entry storing resolved definition metadata. */
USTRUCT()
struct FSOTS_FXResolvedDefinitionEntry
{
    GENERATED_BODY()

    UPROPERTY()
    FSOTS_FXDefinition Definition;

    UPROPERTY()
    TObjectPtr<USOTS_FXDefinitionLibrary> SourceLibrary = nullptr;

    UPROPERTY()
    int32 Priority = 0;

    UPROPERTY()
    int32 RegistrationOrder = INDEX_NONE;
};

// Internal execution payload used by the unified spawn pipeline.
struct FSOTS_FXExecutionParams
{
    UObject* WorldContextObject = nullptr;
    FGameplayTag RequestedTag;
    FGameplayTag ResolvedTag;
    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;
    float Scale = 1.0f;
    ESOTS_FXSpawnSpace SpawnSpace = ESOTS_FXSpawnSpace::World;
    USceneComponent* AttachComponent = nullptr;
    FName AttachSocketName = NAME_None;
    bool bAttach = false;
    bool bHasSurfaceNormal = false;
    FVector SurfaceNormal = FVector::UpVector;
    AActor* Instigator = nullptr;
    AActor* Target = nullptr;
    bool bAllowCameraShake = true;
};

/** Pooled Niagara component entry with simple usage bookkeeping. */
USTRUCT()
struct FSOTS_FXPooledNiagaraEntry
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UNiagaraComponent> Component = nullptr;

    UPROPERTY()
    bool bInUse = false;

    UPROPERTY()
    float LastUseTime = 0.0f;
};

/** Pooled audio component entry with simple usage bookkeeping. */
USTRUCT()
struct FSOTS_FXPooledAudioEntry
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UAudioComponent> Component = nullptr;

    UPROPERTY()
    bool bInUse = false;

    UPROPERTY()
    float LastUseTime = 0.0f;
};

/**
 * Internal pool per cue definition.
 * Not exposed to BP – purely runtime plumbing.
 */
USTRUCT()
struct FSOTS_FXCuePool
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<struct FSOTS_FXPooledNiagaraEntry> NiagaraComponents;

    UPROPERTY()
    TArray<struct FSOTS_FXPooledAudioEntry> AudioComponents;
};

UCLASS(Config=Game)
class SOTS_FX_PLUGIN_API USOTS_FXManagerSubsystem : public UGameInstanceSubsystem, public ISOTS_ProfileSnapshotProvider
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
    virtual void BuildProfileSnapshot(FSOTS_ProfileSnapshot& InOutSnapshot) override;
    virtual void ApplyProfileSnapshot(const FSOTS_ProfileSnapshot& Snapshot) override;

    /** Blueprint entry point to apply profile-driven FX settings. */
    UFUNCTION(BlueprintCallable, Category="SOTS|FX|Profile")
    void ApplyFXProfileSettings(const FSOTS_FXProfileData& InData);

    /** FX cue request entry point for other subsystems. */
    UFUNCTION(BlueprintCallable, Category="SOTS|FX|Cues")
    FSOTS_FXRequestReport RequestFXCue(FGameplayTag FXCueTag, AActor* Instigator, AActor* Target);

    /** FX cue request with explicit result report. */
    UFUNCTION(BlueprintCallable, Category="SOTS|FX|Cues")
    FSOTS_FXRequestReport RequestFXCueWithReport(FGameplayTag FXCueTag, AActor* Instigator, AActor* Target);

    // -------------------------
    // Library registration API
    // -------------------------

    /** Register a library with explicit priority (higher wins, then registration order). */
    UFUNCTION(BlueprintCallable, Category="SOTS|FX|Libraries")
    bool RegisterLibrary(USOTS_FXDefinitionLibrary* Library, int32 Priority = 0);

    /** Register multiple libraries at once (shared priority). */
    UFUNCTION(BlueprintCallable, Category="SOTS|FX|Libraries")
    bool RegisterLibraries(const TArray<USOTS_FXDefinitionLibrary*>& InLibraries, int32 Priority = 0);

    /** Unregister a previously registered library. */
    UFUNCTION(BlueprintCallable, Category="SOTS|FX|Libraries")
    bool UnregisterLibrary(USOTS_FXDefinitionLibrary* Library);

    /** Register a soft-referenced library (loaded once on demand). */
    UFUNCTION(BlueprintCallable, Category="SOTS|FX|Libraries")
    bool RegisterLibrarySoft(TSoftObjectPtr<USOTS_FXDefinitionLibrary> Library, int32 Priority = 0);

    // -------------------------
    // Tag-driven FX job router
    // -------------------------

    /** Legacy libraries searched to resolve FX tags into definitions (priority defaults to 0, keeps array order). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|FX", meta=(AllowPrivateAccess="true"))
    TArray<TObjectPtr<USOTS_FXDefinitionLibrary>> Libraries;

    /** Explicit library registrations that carry per-library priority. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|FX", meta=(AllowPrivateAccess="true"))
    TArray<FSOTS_FXLibraryRegistration> LibraryRegistrations;

    /** Optional soft-reference libraries queued for load during init. */
    UPROPERTY(EditAnywhere, Config, Category="SOTS|FX|Libraries", meta=(AllowPrivateAccess="true"))
    TArray<FSOTS_FXSoftLibraryRegistration> SoftLibraryRegistrations;

    /** Dev-only: validate FX registry on init. Default OFF. */
    UPROPERTY(EditAnywhere, Config, Category="SOTS|FX|Validation")
    bool bValidateFXRegistryOnInit = false;

    /** Dev-only: log library registration decisions. Default OFF. */
    UPROPERTY(EditAnywhere, Config, Category="SOTS|FX|Validation")
    bool bLogLibraryRegistration = false;

    /** Dev-only: warn on duplicate FX tags; first registered wins. Default OFF. */
    UPROPERTY(EditAnywhere, Config, Category="SOTS|FX|Validation")
    bool bWarnOnDuplicateFXTags = false;

    /** Dev-only: warn when FX assets are missing/unloadable. Default OFF. */
    UPROPERTY(EditAnywhere, Config, Category="SOTS|FX|Validation")
    bool bWarnOnMissingFXAssets = false;

    /** Dev-only: warn once when triggers fire before registry is ready. Default OFF. */
    UPROPERTY(EditAnywhere, Config, Category="SOTS|FX|Validation")
    bool bWarnOnTriggerBeforeRegistryReady = false;

    /** Dev-only: log failures for tag-driven FX requests. Default OFF. */
    UPROPERTY(EditAnywhere, Config, Category="SOTS|FX|Validation")
    bool bLogFXRequestFailures = false;

    /** Dev-only: verbose logging for cue resolution path. Default OFF. */
    UPROPERTY(EditAnywhere, Config, Category="SOTS|FX|Debug")
    bool bDebugLogCueResolution = false;

    /** Max pooled Niagara components per cue (legacy pooled path). */
    UPROPERTY(EditAnywhere, Config, Category="SOTS|FX|Pooling")
    int32 MaxNiagaraPoolSizePerCue = 8;

    /** Max pooled audio components per cue (legacy pooled path). */
    UPROPERTY(EditAnywhere, Config, Category="SOTS|FX|Pooling")
    int32 MaxAudioPoolSizePerCue = 8;

    /** Hard cap on concurrently active Niagara components per cue; oldest can be reclaimed. */
    UPROPERTY(EditAnywhere, Config, Category="SOTS|FX|Pooling")
    int32 MaxActiveNiagaraPerCue = 12;

    /** Hard cap on concurrently active audio components per cue; oldest can be reclaimed. */
    UPROPERTY(EditAnywhere, Config, Category="SOTS|FX|Pooling")
    int32 MaxActiveAudioPerCue = 12;

    /** If true, recycle the oldest pooled component when the cap is reached; otherwise drop the request. */
    UPROPERTY(EditAnywhere, Config, Category="SOTS|FX|Pooling")
    bool bRecycleWhenExhausted = true;

    /** Dev-only: log pool reuse and reclamation decisions. */
    UPROPERTY(EditAnywhere, Config, Category="SOTS|FX|Debug")
    bool bLogPoolActions = false;

    /** Broadcast when an FX job is requested and resolved. */
    UPROPERTY(BlueprintAssignable, Category="SOTS|FX")
    FSOTS_OnFXTriggered OnFXTriggered;

    /** Debug helper: snapshot of pooled component usage. */
    UFUNCTION(BlueprintPure, Category="SOTS|FX|Debug")
    FSOTS_FXPoolStats GetPoolStats() const;

    /** Tag-driven FX job with explicit result payload. */
    UFUNCTION(BlueprintCallable, Category="SOTS|FX", meta=(WorldContext="WorldContextObject"))
    FSOTS_FXRequestResult TriggerFXByTagWithReport(UObject* WorldContextObject, const FSOTS_FXRequest& Request);

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

    FSOTS_FXRequestReport ProcessFXRequest(const FSOTS_FXRequest& Request, FSOTS_FXRequestResult* OutLegacyResult);
    FSOTS_FXRequestResult ConvertReportToLegacy(const FSOTS_FXRequestReport& Report, const FSOTS_FXRequest& Request, const FVector& ResolvedLocation, const FRotator& ResolvedRotation, float ResolvedScale, const FSOTS_FXDefinition* Definition) const;
    void MaybeLogFXFailure(const FSOTS_FXRequestReport& Report, const TCHAR* Reason) const;

    bool RegisterLibraryInternal(USOTS_FXDefinitionLibrary* Library, int32 Priority, bool bRebuildRegistry);
    bool RegisterSoftLibraryInternal(const FSOTS_FXSoftLibraryRegistration& Entry, bool bRebuildRegistry);
    void RebuildLibraryOrderAndRegistry();
    void RebuildSortedLibraries();
    void SeedLibrariesFromConfig();
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    void LogLibraryOrderDebug() const;
#endif
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    FString BuildLibraryOrderDebugString() const;
#endif

    void BuildRegistryFromLibraries();
    void ValidateLibraryDefinitions() const;
    void ValidateCueDefinition(const FSOTS_FXDefinition& Def, TSet<FGameplayTag>& SeenTags) const;
    bool EnsureRegistryReady() const;

    // Pool helpers
    UNiagaraComponent* AcquireNiagaraComponent(UWorld* World, USOTS_FXCueDefinition* CueDefinition);
    UAudioComponent* AcquireAudioComponent(UWorld* World, USOTS_FXCueDefinition* CueDefinition);
    void CleanupInvalidPoolEntries(FSOTS_FXCuePool& Pool) const;
    UNiagaraComponent* ReclaimNiagaraComponent(FSOTS_FXCuePool& Pool, const TCHAR* CueName, float NowSeconds);
    UAudioComponent* ReclaimAudioComponent(FSOTS_FXCuePool& Pool, const TCHAR* CueName, float NowSeconds);
    void MarkNiagaraEntryFree(UNiagaraComponent* Component);
    void MarkAudioEntryFree(UAudioComponent* Component);
    void LogPoolEvent(const FString& Message) const;

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
                             FName AttachSocketName,
                             float RequestedScale);

    bool EvaluateFXPolicy(const FSOTS_FXDefinition& Definition, const FSOTS_FXExecutionParams& Params, ESOTS_FXRequestResult& OutResult, FString& OutFailReason, bool& bOutAllowCameraShake) const;
    ESOTS_FXRequestResult TryResolveCue(FGameplayTag FXTag, const FSOTS_FXDefinition*& OutDefinition, FGameplayTag& OutResolvedTag, FString& OutFailReason) const;
    FSOTS_FXRequestReport ExecuteCue(const FSOTS_FXExecutionParams& Params, const FSOTS_FXDefinition* Definition, const FSOTS_FXRequest& OriginalRequest, FSOTS_FXRequestResult* OutLegacyResult);
    FSOTS_FXExecutionParams BuildExecutionParams(const FSOTS_FXRequest& Request, const FSOTS_FXDefinition* Definition, const FGameplayTag& ResolvedTag) const;
    void ApplySurfaceAlignment(const FSOTS_FXDefinition* Definition, const FSOTS_FXExecutionParams& Params, FVector& InOutLocation, FRotator& InOutRotation) const;
    void ApplyOffsets(const FSOTS_FXDefinition* Definition, bool bAttached, FVector& InOutLocation, FRotator& InOutRotation) const;
    UNiagaraComponent* SpawnNiagara(const FSOTS_FXDefinition* Definition, const FSOTS_FXExecutionParams& Params, const FVector& SpawnLocation, const FRotator& SpawnRotation, float SpawnScale, UWorld* World) const;
    void ApplyNiagaraParameters(UNiagaraComponent* NiagaraComp, const FSOTS_FXDefinition* Definition) const;
    UAudioComponent* SpawnAudio(const FSOTS_FXDefinition* Definition, const FSOTS_FXExecutionParams& Params, const FVector& SpawnLocation, const FRotator& SpawnRotation, UWorld* World) const;
    void TriggerCameraShake(const FSOTS_FXDefinition* Definition, UWorld* World) const;

    UFUNCTION()
    void HandleNiagaraFinished(UNiagaraComponent* FinishedComponent);

    UFUNCTION()
    void HandleAudioFinished();
    void HandleAudioFinishedNative(UAudioComponent* FinishedComponent);

    bool bBloodEnabled = true;
    bool bHighIntensityFX = true;
    bool bCameraMotionFXEnabled = true;
    bool bRegistryReady = false;

    int32 NextRegistrationOrder = 0;

    UPROPERTY()
    TArray<FSOTS_FXRegisteredLibrary> RegisteredLibraries;

    UPROPERTY()
    TArray<FSOTS_FXRegisteredLibrary> SortedLibraries;

    // Cache of resolved library definitions for deterministic lookup.
    UPROPERTY()
    TMap<FGameplayTag, FSOTS_FXResolvedDefinitionEntry> RegisteredLibraryDefinitions;
};
