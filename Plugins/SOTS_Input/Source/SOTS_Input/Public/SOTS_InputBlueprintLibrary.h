#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "SOTS_InputBlueprintLibrary.generated.h"

class USOTS_InputRouterComponent;

// STABLE API: used by SOTS_UI and gameplay systems
UCLASS()
class SOTS_INPUT_API USOTS_InputBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    static USOTS_InputRouterComponent* GetRouterFromActor(AActor* ContextActor);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    static void PushLayerTag(AActor* ContextActor, FGameplayTag LayerTag);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    static void PopLayerTag(AActor* ContextActor, FGameplayTag LayerTag);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    static void OpenBuffer(AActor* ContextActor, FGameplayTag ChannelTag);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    static void CloseBuffer(AActor* ContextActor, FGameplayTag ChannelTag, bool bFlush);
};
