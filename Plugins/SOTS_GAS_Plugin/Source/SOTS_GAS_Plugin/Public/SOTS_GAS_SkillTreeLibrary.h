#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "SOTS_GAS_SkillTreeLibrary.generated.h"

class USOTS_SkillTreeSubsystem;

UCLASS()
class SOTS_GAS_PLUGIN_API USOTS_GAS_SkillTreeLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|SkillTree", meta=(WorldContext="WorldContextObject"))
    static bool HasSkillTagOnTree(const UObject* WorldContextObject,
                                  FName TreeId,
                                  FGameplayTag SkillTag);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|SkillTree", meta=(WorldContext="WorldContextObject"))
    static FGameplayTagContainer GetAllSkillTagsForTree(const UObject* WorldContextObject,
                                                        FName TreeId);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|SkillTree", meta=(WorldContext="WorldContextObject"))
    static FGameplayTagContainer GetAllSkillTags(const UObject* WorldContextObject);

private:
    static USOTS_SkillTreeSubsystem* GetSkillTreeSubsystem(const UObject* WorldContextObject);
};

