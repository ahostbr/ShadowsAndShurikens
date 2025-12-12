#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SOTS_FXCueDefinition.generated.h"

class UNiagaraSystem;
class USoundBase;
class UCameraShakeBase;

/**
 * A single FX cue = VFX + SFX (+ optional camera shake) bound to a GameplayTag.
 */
UCLASS(BlueprintType)
class SOTS_FX_PLUGIN_API USOTS_FXCueDefinition : public UDataAsset
{
    GENERATED_BODY()

public:

    // Tag used to request this cue (FX.Cue.Whatever)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
    FGameplayTag CueTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX|Assets")
    TObjectPtr<UNiagaraSystem> NiagaraSystem;

    // Optional legacy particle system support if you ever need it
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX|Assets")
    TObjectPtr<UParticleSystem> CascadeSystem;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX|Assets")
    TObjectPtr<USoundBase> Sound;

    // Optional camera shake
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX|Camera")
    TSubclassOf<UCameraShakeBase> CameraShakeClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX|Camera", meta = (EditCondition = "CameraShakeClass != nullptr", ClampMin = "0.0"))
    float CameraShakeScale = 1.0f;

    // Whether this cue is looping (so systems know not to auto-destroy)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX|Behavior")
    bool bLooping = false;

    // Auto-destroy spawned components when FX finishes (for non-looping)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX|Behavior")
    bool bAutoDestroy = true;
};
