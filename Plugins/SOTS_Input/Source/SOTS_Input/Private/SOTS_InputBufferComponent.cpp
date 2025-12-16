#include "SOTS_InputBufferComponent.h"

#include "SOTS_InputRouterComponent.h"

bool USOTS_InputBufferComponent::IsChannelOpen(FGameplayTag Channel) const
{
    return OpenChannelStack.Contains(Channel);
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
