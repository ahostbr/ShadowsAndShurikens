#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SOTS_InputBufferTypes.h"
#include "SOTS_InputBufferComponent.generated.h"

class USOTS_InputRouterComponent;

// STABLE API: used by SOTS_UI and gameplay systems
UCLASS(ClassGroup=(SOTS), meta=(BlueprintSpawnableComponent))
class SOTS_INPUT_API USOTS_InputBufferComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Input Buffer")
    bool IsChannelOpen(FGameplayTag Channel) const;

    UFUNCTION(BlueprintCallable, Category = "Input Buffer")
    void OpenChannel(FGameplayTag Channel);

    UFUNCTION(BlueprintCallable, Category = "Input Buffer")
    void CloseChannel(FGameplayTag Channel, bool bFlush, USOTS_InputRouterComponent* Router);

    UFUNCTION(BlueprintCallable, Category = "Input Buffer")
    void ResetAll();

    UFUNCTION(BlueprintCallable, Category = "Input Buffer")
    void BufferEvent(const FSOTS_BufferedInputEvent& Evt);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Buffer")
    int32 MaxBufferedEventsPerChannel = 16;

    bool GetTopOpenChannel(FGameplayTag& OutChannel) const;
    void GetOpenChannels(TArray<FGameplayTag>& OutChannels) const;

private:
    UPROPERTY()
    TArray<FGameplayTag> OpenChannelStack;

    UPROPERTY()
    TMap<FGameplayTag, TArray<FSOTS_BufferedInputEvent>> BufferedByChannel;
};
