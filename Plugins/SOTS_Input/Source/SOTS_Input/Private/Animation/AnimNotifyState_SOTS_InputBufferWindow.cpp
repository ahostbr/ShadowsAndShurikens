#include "Animation/AnimNotifyState_SOTS_InputBufferWindow.h"

#include "SOTS_InputRouterComponent.h"

UAnimNotifyState_SOTS_InputBufferWindow::UAnimNotifyState_SOTS_InputBufferWindow()
{
    BufferChannel = FGameplayTag::RequestGameplayTag(TEXT("SAS.Input.Buffer.Default"), false);
}

void UAnimNotifyState_SOTS_InputBufferWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    if (USOTS_InputRouterComponent* Router = ResolveRouter(MeshComp))
    {
        Router->OpenInputBuffer(BufferChannel);
    }
}

void UAnimNotifyState_SOTS_InputBufferWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    if (USOTS_InputRouterComponent* Router = ResolveRouter(MeshComp))
    {
        Router->CloseInputBuffer(BufferChannel, bFlushOnEnd);
    }
}

USOTS_InputRouterComponent* UAnimNotifyState_SOTS_InputBufferWindow::ResolveRouter(USkeletalMeshComponent* MeshComp) const
{
    if (!MeshComp)
    {
        return nullptr;
    }

    if (AActor* Owner = MeshComp->GetOwner())
    {
        return Owner->FindComponentByClass<USOTS_InputRouterComponent>();
    }

    return nullptr;
}
