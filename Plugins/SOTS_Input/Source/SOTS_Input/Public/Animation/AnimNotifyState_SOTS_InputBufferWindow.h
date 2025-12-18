#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "AnimNotifyState_SOTS_InputBufferWindow.generated.h"

class USOTS_InputRouterComponent;

UCLASS()
class SOTS_INPUT_API UAnimNotifyState_SOTS_InputBufferWindow : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    UAnimNotifyState_SOTS_InputBufferWindow();

    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:
    class USOTS_InputBufferComponent* ResolveBuffer(USkeletalMeshComponent* MeshComp) const;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Input")
    FGameplayTag BufferChannel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Input")
    bool bFlushOnEnd = true;

    // Optional list of intents this window is expected to capture (informational only for now)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Input")
    TArray<FGameplayTag> AllowedIntentTags;
};
