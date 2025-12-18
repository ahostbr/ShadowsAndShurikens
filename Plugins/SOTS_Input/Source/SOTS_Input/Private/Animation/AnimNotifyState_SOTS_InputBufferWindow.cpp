#include "Animation/AnimNotifyState_SOTS_InputBufferWindow.h"

#include "SOTS_InputBlueprintLibrary.h"
#include "SOTS_InputBufferComponent.h"

UAnimNotifyState_SOTS_InputBufferWindow::UAnimNotifyState_SOTS_InputBufferWindow()
{
    BufferChannel = FGameplayTag::RequestGameplayTag(TEXT("Input.Buffer.Channel.Execution"), false);
}

void UAnimNotifyState_SOTS_InputBufferWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    if (USOTS_InputBufferComponent* Buffer = ResolveBuffer(MeshComp))
    {
        Buffer->OpenBufferWindow(BufferChannel);
    }
}

void UAnimNotifyState_SOTS_InputBufferWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    if (USOTS_InputBufferComponent* Buffer = ResolveBuffer(MeshComp))
    {
        Buffer->CloseBufferWindow(BufferChannel);
    }
}

USOTS_InputBufferComponent* UAnimNotifyState_SOTS_InputBufferWindow::ResolveBuffer(USkeletalMeshComponent* MeshComp) const
{
    if (!MeshComp)
    {
        return nullptr;
    }

    if (AActor* Owner = MeshComp->GetOwner())
    {
        if (USOTS_InputBufferComponent* Existing = Owner->FindComponentByClass<USOTS_InputBufferComponent>())
        {
            return Existing;
        }

        return USOTS_InputBlueprintLibrary::EnsureBufferOnPlayerController(MeshComp->GetWorld(), Owner);
    }

    return nullptr;
}
