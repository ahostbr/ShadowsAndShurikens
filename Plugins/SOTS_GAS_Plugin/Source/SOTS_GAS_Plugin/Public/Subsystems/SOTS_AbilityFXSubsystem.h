#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "SOTS_AbilityFXSubsystem.generated.h"

class AActor;

UCLASS()
class SOTS_GAS_PLUGIN_API USOTS_AbilityFXSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    static USOTS_AbilityFXSubsystem* Get(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category="SOTS|GAS|Ability|FX")
    void TriggerAbilityFX(FGameplayTag FXTag, FGameplayTag AbilityTag, AActor* SourceActor);
};
