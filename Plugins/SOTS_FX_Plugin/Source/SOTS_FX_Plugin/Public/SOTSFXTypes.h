#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SOTSFXTypes.generated.h"

class UNiagaraSystem;
class USoundBase;
class UAudioComponent;
class UNiagaraComponent;
class USceneComponent;
class AActor;
class UObject;

// Lightweight request outcome for FX triggers.
UENUM(BlueprintType)
enum class ESOTS_FXRequestResult : uint8
{
    Success           UMETA(DisplayName="Success"),
    NotFound          UMETA(DisplayName="Not Found"),
    DisabledByPolicy  UMETA(DisplayName="Disabled By Policy"),
    InvalidWorld      UMETA(DisplayName="Invalid World"),
    InvalidParams     UMETA(DisplayName="Invalid Params"),
    FailedToSpawn     UMETA(DisplayName="Failed To Spawn")
};

// Minimal report payload returned by FX requests.
USTRUCT(BlueprintType)
struct FSOTS_FXRequestReport
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    ESOTS_FXRequestResult Result = ESOTS_FXRequestResult::InvalidParams;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    FGameplayTag RequestedCueTag;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    FGameplayTag ResolvedCueTag;

    // Optional spawned object (Niagara/Audio). May be null when broadcast-only.
    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    TObjectPtr<UObject> SpawnedObject = nullptr;

    // Dev-only detail. Remains empty in shipping/test.
    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    FString DebugMessage;
};

// Where an FX job expects to be applied.
UENUM(BlueprintType)
enum class ESOTS_FXSpawnSpace : uint8
{
    World              UMETA(DisplayName="World"),
    AttachToActor      UMETA(DisplayName="Attach To Actor"),
    AttachToComponent  UMETA(DisplayName="Attach To Component")
};

// Outcome for the unified FX request pipeline.
UENUM(BlueprintType)
enum class ESOTS_FXRequestStatus : uint8
{
    Success             UMETA(DisplayName="Success"),
    InvalidTag          UMETA(DisplayName="Invalid Tag"),
    RegistryNotReady    UMETA(DisplayName="Registry Not Ready"),
    DefinitionNotFound  UMETA(DisplayName="Definition Not Found"),
    MissingContext      UMETA(DisplayName="Missing Context"),
    MissingAttachment   UMETA(DisplayName="Missing Attachment Target")
};

/**
 * Unified FX request payload used by the tag-driven router.
 */
USTRUCT(BlueprintType)
struct FSOTS_FXRequest
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="SOTS|FX")
    FGameplayTag FXTag;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="SOTS|FX")
    AActor* Instigator = nullptr;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="SOTS|FX")
    AActor* Target = nullptr;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="SOTS|FX")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="SOTS|FX")
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="SOTS|FX")
    ESOTS_FXSpawnSpace SpawnSpace = ESOTS_FXSpawnSpace::World;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="SOTS|FX")
    USceneComponent* AttachComponent = nullptr;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="SOTS|FX")
    FName AttachSocketName = NAME_None;

    // Optional per-request scale override (applied on top of the definition default).
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="SOTS|FX")
    float Scale = 1.0f;
};

/**
 * Authored FX definition used by lightweight libraries to map
 * GameplayTags to Niagara / Sound assets.
 */
USTRUCT(BlueprintType)
struct FSOTS_FXDefinition
{
    GENERATED_BODY()

    // Tag that identifies this FX (e.g. FX.Stealth.Tier.1, FX.Mission.Start).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|FX")
    FGameplayTag FXTag;

    // Optional Niagara and sound assets (soft refs so we do not force-load).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|FX")
    TSoftObjectPtr<UNiagaraSystem> NiagaraSystem;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|FX")
    TSoftObjectPtr<USoundBase> Sound;

    // Default spawn space and scale.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|FX")
    ESOTS_FXSpawnSpace DefaultSpace = ESOTS_FXSpawnSpace::World;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|FX")
    float DefaultScale = 1.0f;
};

/**
 * Context used when requesting an FX cue for the older pooled
 * FX manager path. This stays for backward compatibility.
 */
USTRUCT(BlueprintType)
struct FSOTS_FXContext
{
    GENERATED_BODY()

    FSOTS_FXContext()
        : Location(FVector::ZeroVector)
        , Rotation(FRotator::ZeroRotator)
        , Scale(1.0f)
        , AttachComponent(nullptr)
        , AttachSocketName(NAME_None)
        , bAttach(false)
        , bFollowRotation(false)
        , Instigator(nullptr)
        , Target(nullptr)
    {}

    // Where to spawn if not attaching
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FX")
    FVector Location;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FX")
    FRotator Rotation;

    // Uniform scale for spawned FX
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FX")
    float Scale;

    // Optional attachment
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FX")
    USceneComponent* AttachComponent;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FX")
    FName AttachSocketName;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FX")
    bool bAttach;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FX")
    bool bFollowRotation;

    // Gameplay context – plain actors, nothing GAS-specific
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FX")
    AActor* Instigator;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FX")
    AActor* Target;

    // Extra semantic info (surface, damage type, etc) for future expansion
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FX")
    FGameplayTagContainer ExtraTags;
};

/**
 * Handle to spawned FX so you can stop / fade / debug later.
 */
USTRUCT(BlueprintType)
struct FSOTS_FXHandle
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "FX")
    TObjectPtr<UNiagaraComponent> NiagaraComponent = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "FX")
    TObjectPtr<UAudioComponent> AudioComponent = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "FX")
    FGameplayTag CueTag;
};

/**
 * Debug info about how many FX components are currently active.
 */
USTRUCT(BlueprintType)
struct FSOTS_FXActiveCounts
{
    GENERATED_BODY()

    // Number of Niagara components currently active
    UPROPERTY(BlueprintReadOnly, Category = "FX|Debug")
    int32 ActiveNiagara = 0;

    // Number of Audio components currently playing
    UPROPERTY(BlueprintReadOnly, Category = "FX|Debug")
    int32 ActiveAudio = 0;
};

/**
 * Resolved runtime FX job payload broadcast by the global FX manager.
 * Blueprint listeners can spawn Niagara / play audio in response.
 */
USTRUCT(BlueprintType)
struct FSOTS_FXResolvedRequest
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    FGameplayTag FXTag;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    ESOTS_FXSpawnSpace SpawnSpace = ESOTS_FXSpawnSpace::World;

    // Who triggered it & who it targets (optional)
    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    TWeakObjectPtr<AActor> Instigator;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    TWeakObjectPtr<AActor> Target;

    // Optional attach target.
    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    TWeakObjectPtr<USceneComponent> AttachComponent;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    FName AttachSocketName = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    float Scale = 1.0f;

    // Resolved assets (can be null).
    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    TSoftObjectPtr<UNiagaraSystem> NiagaraSystem;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    TSoftObjectPtr<USoundBase> Sound;
};

/**
 * Result payload for the unified FX request pipeline.
 */
USTRUCT(BlueprintType)
struct FSOTS_FXRequestResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    ESOTS_FXRequestStatus Status = ESOTS_FXRequestStatus::InvalidTag;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    FGameplayTag FXTag;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    ESOTS_FXSpawnSpace SpawnSpace = ESOTS_FXSpawnSpace::World;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    TWeakObjectPtr<AActor> Instigator;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    TWeakObjectPtr<AActor> Target;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    TWeakObjectPtr<USceneComponent> AttachComponent;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    FName AttachSocketName = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    float ResolvedScale = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    TSoftObjectPtr<UNiagaraSystem> NiagaraSystem;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    TSoftObjectPtr<USoundBase> Sound;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|FX")
    bool bSucceeded = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnFXTriggered, const FSOTS_FXResolvedRequest&, FXRequest);
