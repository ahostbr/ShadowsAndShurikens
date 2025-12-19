#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SOTS_InputBufferTypes.h"
#include "SOTS_InputBufferComponent.generated.h"

class USOTS_InputRouterComponent;
class UAnimInstance;
class AActor;
class APawn;
class UAnimMontage;

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

    // IN-03 minimal windowed intent buffering (queue size 1, latest wins)
    UFUNCTION(BlueprintCallable, Category = "Input Buffer")
    void OpenBufferWindow(FGameplayTag ChannelTag);

    UFUNCTION(BlueprintCallable, Category = "Input Buffer")
    void CloseBufferWindow(FGameplayTag ChannelTag);

    UFUNCTION(BlueprintCallable, Category = "Input Buffer")
    void TryBufferIntent(FGameplayTag ChannelTag, FGameplayTag IntentTag, bool& bBuffered);

    UFUNCTION(BlueprintCallable, Category = "Input Buffer")
    void ConsumeBufferedIntent(FGameplayTag ChannelTag, FGameplayTag& OutIntentTag, bool& bHadBuffered);

    UFUNCTION(BlueprintCallable, Category = "Input Buffer")
    void ClearAllBufferWindowsAndInputs();

    UFUNCTION(BlueprintCallable, Category = "Input Buffer")
    void ClearChannel(FGameplayTag ChannelTag);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Buffer")
    int32 MaxBufferedEventsPerChannel = 16;

    bool GetTopOpenChannel(FGameplayTag& OutChannel) const;
    void GetOpenChannels(TArray<FGameplayTag>& OutChannels) const;

private:
    UPROPERTY()
    TArray<FGameplayTag> OpenChannelStack;

    UPROPERTY()
    TMap<FGameplayTag, FSOTS_BufferedInputEventArray> BufferedByChannel;

private:
    bool IsChannelAllowedForWindow(const FGameplayTag& ChannelTag) const;
    void BindToAnimInstance();
    void UnbindFromAnimInstance();
    UFUNCTION()
    void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);
    UFUNCTION()
    void HandleMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);
    AActor* ResolveOwnerActor() const;
    APawn* ResolveOwningPawn() const;
    UAnimInstance* ResolveAnimInstance() const;
    void CleanupWindowIfEmpty();

private:
    // Open window tracking (queue size = 1 per channel, latest wins)
    UPROPERTY()
    TMap<FGameplayTag, bool> OpenWindows;

    UPROPERTY()
    TMap<FGameplayTag, FSOTS_BufferedIntent> BufferedIntentByChannel;

    UPROPERTY()
    TWeakObjectPtr<UAnimInstance> BoundAnimInstance;
};
