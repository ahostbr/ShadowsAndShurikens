// Ninja Bear Studio Inc., all rights reserved.
#include "Components/NinjaInputBufferComponent.h"

#include "InputAction.h"
#include "NinjaInputFunctionLibrary.h"
#include "NinjaInputHandler.h"
#include "NinjaInputLogs.h"
#include "GameFramework/Actor.h"

UNinjaInputBufferComponent::UNinjaInputBufferComponent()
{
    InputBufferMode = EInputBufferMode::LastCommand;
    bUsingInputBuffer = false;
}

bool UNinjaInputBufferComponent::IsInputBufferEnabled_Implementation() const
{
    return InputBufferMode != EInputBufferMode::Disabled;
}

bool UNinjaInputBufferComponent::IsInputBufferOpen_Implementation(const FGameplayTag ChannelTag) const
{
	return Channels.HasTagExact(ChannelTag);
}

void UNinjaInputBufferComponent::BufferInputCommands_Implementation(TArray<FBufferedInputCommand>& InputCommandsForAction)
{
	
	const TArray<FBufferedInputCommand>& ValidCommands = InputCommandsForAction.FilterByPredicate([](const FBufferedInputCommand& Command)
		{ return Command.IsValid(); });
	
    if (ValidCommands.IsEmpty())
    {
    	// No commands to buffer, all entries were invalid.
    	return;
    }

	const FGameplayTag BufferChannelTag = InputCommandsForAction[0].GetHandler()->GetBufferChannelTag();
	if (!BufferChannelTag.IsValid() || !CanAddToBuffer(BufferChannelTag))
	{
		// Invalid channel tag or buffer request.
		return;
	}

	const FStoredBufferedInputCommands& StoredCommands = FStoredBufferedInputCommands(ValidCommands);
	if (StoredCommands.IsValid())
	{
		BufferedCommands.Add(BufferChannelTag, StoredCommands);

		UE_LOG(LogNinjaInputBufferComponent, Verbose, TEXT("[%s] Action %s added %d Handlers to the Input Buffer."),
			*GetNameSafe(GetOwner()), *GetNameSafe(StoredCommands.InputAction), StoredCommands.Num());	
	}
	else
	{
		UE_LOG(LogNinjaInputBufferComponent, Warning, TEXT("[%s] Action %s generated an invalid saved command!"),
			*GetNameSafe(GetOwner()), *GetNameSafe(StoredCommands.InputAction));	
	}
}

void UNinjaInputBufferComponent::OpenInputBuffer_Implementation(const FGameplayTag ChannelTag)
{
    if (!Execute_IsInputBufferOpen(this, ChannelTag))
    {
    	Channels.AddTagFast(ChannelTag);

    	UE_LOG(LogNinjaInputBufferComponent, Verbose, TEXT("[%s] Buffer channel %s is open."),
			*GetNameSafe(GetOwner()), *ChannelTag.ToString());
    }
}

void UNinjaInputBufferComponent::CloseInputBuffer_Implementation(const FGameplayTag ChannelTag, const bool bCancelled)
{
	if (Execute_IsInputBufferOpen(this, ChannelTag))
	{
		if (bCancelled)
		{
			const FStoredBufferedInputCommands* Commands = BufferedCommands.Find(ChannelTag);
			const int32 CommandCount = Commands ? Commands->Num() : 0;
			
			UE_LOG(LogNinjaInputBufferComponent, Verbose, TEXT("[%s] Buffer channel %s was cancelled, %d commands will be discarded."),
        		*GetNameSafe(GetOwner()), *ChannelTag.ToString(), CommandCount);
		}
		else
		{
			FStoredBufferedInputCommands* Commands = BufferedCommands.Find(ChannelTag);
			if (Commands && Commands->IsValid())
			{
				for (const FBufferedInputCommand& Command : Commands->InputCommandsForAction)
				{
					if (Command.IsValid())
					{
						const UInputAction* InputAction = Command.GetSourceAction();
						const UNinjaInputHandler* Handler = Command.GetHandler();
            
						UE_LOG(LogNinjaInputBufferComponent, Verbose, TEXT("[%s] Releasing Input Action %s and Handler %s from buffer channel %s."),
							*GetNameSafe(GetOwner()), *GetNameSafe(InputAction), *GetNameSafe(Handler), *ChannelTag.ToString());
    
						Command.Execute();
					}
				}
			}
			else
			{
				UE_LOG(LogNinjaInputBufferComponent, Verbose, TEXT("[%s] Buffer channel %s was closed without commands to execute."),
					*GetNameSafe(GetOwner()), *ChannelTag.ToString());
			}
		}
	}
	
	Channels.RemoveTag(ChannelTag);
	BufferedCommands.Remove(ChannelTag);
}

UActorComponent* UNinjaInputBufferComponent::GetInputBufferComponent()
{
    // If this buffer is enabled, then there's no need to perform the look-up.
    if (Execute_IsInputBufferEnabled(this))
    {
        return Cast<UActorComponent>(this);
    }

    // Try to find another buffer that's currently active.
    return UNinjaInputFunctionLibrary::GetInputBufferComponent(GetOwner());
}

bool UNinjaInputBufferComponent::CanAddToBuffer(const FGameplayTag ChannelTag) const
{
    return InputBufferMode == EInputBufferMode::LastCommand
        || (InputBufferMode == EInputBufferMode::FirstCommand && BufferedCommands.Contains(ChannelTag));
}

void UNinjaInputBufferComponent::ResetInputBuffer_Implementation()
{
	Channels.Reset();
	BufferedCommands.Empty();
}
