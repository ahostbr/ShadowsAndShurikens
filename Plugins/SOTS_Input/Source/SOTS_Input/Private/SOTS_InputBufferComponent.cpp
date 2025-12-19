#include "SOTS_InputBufferComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "SOTS_InputRouterComponent.h"

bool USOTS_InputBufferComponent::IsChannelOpen(FGameplayTag Channel) const
{
    return OpenChannelStack.Contains(Channel);
}

bool USOTS_InputBufferComponent::IsChannelAllowedForWindow(const FGameplayTag& ChannelTag) const
{
    static const FGameplayTag ExecutionTag = FGameplayTag::RequestGameplayTag(TEXT("Input.Buffer.Channel.Execution"), false);
    static const FGameplayTag VanishTag = FGameplayTag::RequestGameplayTag(TEXT("Input.Buffer.Channel.Vanish"), false);

    return ChannelTag.IsValid() && (ChannelTag.MatchesTagExact(ExecutionTag) || ChannelTag.MatchesTagExact(VanishTag));
}

void USOTS_InputBufferComponent::OpenChannel(FGameplayTag Channel)
{
    if (!Channel.IsValid())
    {
        return;
    }

    OpenChannelStack.Remove(Channel);
    OpenChannelStack.Add(Channel); // Treat end as top of stack.

    FSOTS_BufferedInputEventArray& ArrayRef = BufferedByChannel.FindOrAdd(Channel);
    if (MaxBufferedEventsPerChannel > 0)
    {
        ArrayRef.Events.Reserve(MaxBufferedEventsPerChannel);
    }
}

void USOTS_InputBufferComponent::CloseChannel(FGameplayTag Channel, bool bFlush, USOTS_InputRouterComponent* Router)
{
    OpenChannelStack.Remove(Channel);

    if (!bFlush)
    {
        BufferedByChannel.Remove(Channel);
        return;
    }

    if (!Channel.IsValid())
    {
        return;
    }

    if (FSOTS_BufferedInputEventArray* ArrayRef = BufferedByChannel.Find(Channel))
    {
        if (Router)
        {
            for (const FSOTS_BufferedInputEvent& Evt : ArrayRef->Events)
            {
                Router->DispatchBufferedEvent(Evt);
            }
        }
    }

    BufferedByChannel.Remove(Channel);
}

void USOTS_InputBufferComponent::ResetAll()
{
    OpenChannelStack.Reset();
    BufferedByChannel.Reset();
    ClearAllBufferWindowsAndInputs();
}

void USOTS_InputBufferComponent::BufferEvent(const FSOTS_BufferedInputEvent& Evt)
{
    const FGameplayTag Channel = Evt.Channel;
    if (!Channel.IsValid())
    {
        return;
    }

    FSOTS_BufferedInputEventArray& ArrayRef = BufferedByChannel.FindOrAdd(Channel);
    TArray<FSOTS_BufferedInputEvent>& Events = ArrayRef.Events;

    if (MaxBufferedEventsPerChannel > 0 && Events.Num() >= MaxBufferedEventsPerChannel)
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        UE_LOG(LogTemp, Warning, TEXT("SOTS_InputBuffer: dropping oldest buffered event for channel %s (max=%d)"), *Channel.ToString(), MaxBufferedEventsPerChannel);
#endif
        Events.RemoveAt(0);
    }

    Events.Add(Evt);
}

void USOTS_InputBufferComponent::OpenBufferWindow(FGameplayTag ChannelTag)
{
    if (!IsChannelAllowedForWindow(ChannelTag))
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        UE_LOG(LogTemp, Verbose, TEXT("SOTS_InputBuffer: ignoring window open for disallowed channel %s"), *ChannelTag.ToString());
#endif
        return;
    }

    const bool bWasEmpty = OpenWindows.Num() == 0;
    OpenWindows.Add(ChannelTag, true);

    if (bWasEmpty)
    {
        BindToAnimInstance();
    }
}

void USOTS_InputBufferComponent::CloseBufferWindow(FGameplayTag ChannelTag)
{
    if (!IsChannelAllowedForWindow(ChannelTag))
    {
        return;
    }

    OpenWindows.Remove(ChannelTag);
    CleanupWindowIfEmpty();
}

void USOTS_InputBufferComponent::TryBufferIntent(FGameplayTag ChannelTag, FGameplayTag IntentTag, bool& bBuffered)
{
    bBuffered = false;

    if (!IsChannelAllowedForWindow(ChannelTag) || !OpenWindows.Contains(ChannelTag))
    {
        return;
    }

    FSOTS_BufferedIntent& Slot = BufferedIntentByChannel.FindOrAdd(ChannelTag);
    Slot.IntentTag = IntentTag;
    Slot.TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    bBuffered = true;
}

void USOTS_InputBufferComponent::ConsumeBufferedIntent(FGameplayTag ChannelTag, FGameplayTag& OutIntentTag, bool& bHadBuffered)
{
    bHadBuffered = false;
    OutIntentTag = FGameplayTag();

    if (FSOTS_BufferedIntent* Slot = BufferedIntentByChannel.Find(ChannelTag))
    {
        OutIntentTag = Slot->IntentTag;
        BufferedIntentByChannel.Remove(ChannelTag);
        bHadBuffered = OutIntentTag.IsValid();
    }
}

void USOTS_InputBufferComponent::ClearAllBufferWindowsAndInputs()

{
    OpenWindows.Reset();
    BufferedIntentByChannel.Reset();
    CleanupWindowIfEmpty();
}

void USOTS_InputBufferComponent::ClearChannel(FGameplayTag ChannelTag)
{
    OpenWindows.Remove(ChannelTag);
    BufferedIntentByChannel.Remove(ChannelTag);
    CleanupWindowIfEmpty();
}

bool USOTS_InputBufferComponent::GetTopOpenChannel(FGameplayTag& OutChannel) const
{
    if (OpenChannelStack.Num() == 0)
    {
        return false;
    }

    OutChannel = OpenChannelStack.Last();
    return true;
}

void USOTS_InputBufferComponent::GetOpenChannels(TArray<FGameplayTag>& OutChannels) const
{
    OutChannels = OpenChannelStack;
}

AActor* USOTS_InputBufferComponent::ResolveOwnerActor() const
{
    return GetOwner();
}

APawn* USOTS_InputBufferComponent::ResolveOwningPawn() const
{
    if (AActor* Owner = ResolveOwnerActor())
    {
        if (APawn* AsPawn = Cast<APawn>(Owner))
        {
            return AsPawn;
        }

        if (const APlayerController* PC = Cast<APlayerController>(Owner))
        {
            return PC->GetPawn();
        }
    }

    return nullptr;
}

UAnimInstance* USOTS_InputBufferComponent::ResolveAnimInstance() const
{
    if (const APawn* Pawn = ResolveOwningPawn())
    {
        if (USkeletalMeshComponent* Mesh = Pawn->FindComponentByClass<USkeletalMeshComponent>())
        {
            return Mesh->GetAnimInstance();
        }
    }

    return nullptr;
}

void USOTS_InputBufferComponent::BindToAnimInstance()
{
    UAnimInstance* AnimInstance = ResolveAnimInstance();
    if (!AnimInstance)
    {
        return;
    }

    if (BoundAnimInstance.IsValid() && BoundAnimInstance.Get() == AnimInstance)
    {
        return;
    }

    UnbindFromAnimInstance();

    BoundAnimInstance = AnimInstance;
    AnimInstance->OnMontageEnded.AddDynamic(this, &USOTS_InputBufferComponent::HandleMontageEnded);
    AnimInstance->OnMontageBlendingOut.AddDynamic(this, &USOTS_InputBufferComponent::HandleMontageBlendingOut);
}

void USOTS_InputBufferComponent::UnbindFromAnimInstance()
{
    if (UAnimInstance* AnimInstance = BoundAnimInstance.Get())
    {
        AnimInstance->OnMontageEnded.RemoveDynamic(this, &USOTS_InputBufferComponent::HandleMontageEnded);
        AnimInstance->OnMontageBlendingOut.RemoveDynamic(this, &USOTS_InputBufferComponent::HandleMontageBlendingOut);
    }

    BoundAnimInstance.Reset();
}

void USOTS_InputBufferComponent::HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    ClearAllBufferWindowsAndInputs();
}

void USOTS_InputBufferComponent::HandleMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
    ClearAllBufferWindowsAndInputs();
}

void USOTS_InputBufferComponent::CleanupWindowIfEmpty()
{
    if (OpenWindows.Num() == 0)
    {
        UnbindFromAnimInstance();
    }
}
