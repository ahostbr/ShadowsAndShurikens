#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Data/SOTS_AbilityTypes.h"
#include "SOTS_AbilityDataAssets.generated.h"

USTRUCT(BlueprintType)
struct SOTS_GAS_PLUGIN_API F_SOTS_AbilitySetEntry
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTag AbilityTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    F_SOTS_AbilityGrantOptions GrantOptions;
};

UCLASS(BlueprintType)
class SOTS_GAS_PLUGIN_API USOTS_AbilityDefinitionDA : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
    F_SOTS_AbilityDefinition Ability;
};

// Simple container for registering a set of ability definition data assets in
// one place (for example, from a GameInstance or project-level config asset).
UCLASS(BlueprintType)
class SOTS_GAS_PLUGIN_API USOTS_AbilityDefinitionLibrary : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
    TArray<TObjectPtr<USOTS_AbilityDefinitionDA>> Definitions;
};

UCLASS(BlueprintType)
class SOTS_GAS_PLUGIN_API USOTS_AbilitySetDA : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability Set")
    FGameplayTag SetTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability Set")
    TArray<F_SOTS_AbilitySetEntry> Abilities;
};

UCLASS(BlueprintType)
class SOTS_GAS_PLUGIN_API USOTS_AbilityMetadataDA : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Metadata")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Metadata")
    FText ShortDescription;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Metadata")
    FText LongDescription;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Metadata")
    UTexture2D* Icon = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Metadata")
    FGameplayTag CategoryTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Metadata")
    int32 SortOrder = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Metadata")
    FText UnlockHint;
};

UCLASS(BlueprintType)
class SOTS_GAS_PLUGIN_API USOTS_AbilityCueDA : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cue")
    FGameplayTag CueTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cue")
    TSoftObjectPtr<UNiagaraSystem> StartVFX;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cue")
    TSoftObjectPtr<UNiagaraSystem> LoopVFX;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cue")
    TSoftObjectPtr<UNiagaraSystem> EndVFX;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cue")
    TSoftObjectPtr<USoundBase> StartSFX;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cue")
    TSoftObjectPtr<USoundBase> LoopSFX;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cue")
    TSoftObjectPtr<USoundBase> EndSFX;
};
