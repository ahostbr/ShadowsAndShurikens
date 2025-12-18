#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "InputCoreTypes.h"
#include "SOTS_InputBlueprintLibrary.generated.h"

class USOTS_InputRouterComponent;
class USOTS_InputBufferComponent;

// STABLE API: used by SOTS_UI and gameplay systems
UCLASS()
class SOTS_INPUT_API USOTS_InputBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    static USOTS_InputRouterComponent* GetRouterFromActor(AActor* ContextActor);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input", meta = (WorldContext = "WorldContextObject"))
    static USOTS_InputRouterComponent* GetRouterForPlayerController(UObject* WorldContextObject, AActor* ContextActor);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input", meta = (WorldContext = "WorldContextObject"))
    static USOTS_InputRouterComponent* EnsureRouterOnPlayerController(UObject* WorldContextObject, AActor* ContextActor);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input", meta = (WorldContext = "WorldContextObject"))
    static USOTS_InputBufferComponent* EnsureBufferOnPlayerController(UObject* WorldContextObject, AActor* ContextActor);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input", meta = (WorldContext = "WorldContextObject"))
    static void ReportInputDeviceFromKey(UObject* WorldContextObject, AActor* ContextActor, FKey Key);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input", meta = (WorldContext = "WorldContextObject"))
    static void PushDragonControlLayer(UObject* WorldContextObject, AActor* ContextActor);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input", meta = (WorldContext = "WorldContextObject"))
    static void PopDragonControlLayer(UObject* WorldContextObject, AActor* ContextActor);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    static void PushLayerTag(AActor* ContextActor, FGameplayTag LayerTag);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    static void PopLayerTag(AActor* ContextActor, FGameplayTag LayerTag);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    static void OpenBuffer(AActor* ContextActor, FGameplayTag ChannelTag);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    static void CloseBuffer(AActor* ContextActor, FGameplayTag ChannelTag, bool bFlush);
};
