#include "SOTS_UIAbilityLibrary.h"

#include "SOTS_HUDSubsystem.h"
#include "SOTS_NotificationSubsystem.h"

namespace SOTSUIAbilityInternal
{
	static FString BuildAbilityMessage(FGameplayTag AbilityTag, bool bSuccess, const FText& Detail)
	{
		const FString Name = AbilityTag.IsValid() ? AbilityTag.ToString() : TEXT("Ability");
		const FString Status = bSuccess ? TEXT("activated") : TEXT("failed");
		FString Message = FString::Printf(TEXT("%s %s"), *Name, *Status);
		if (!Detail.IsEmpty())
		{
			Message += FString::Printf(TEXT(" (%s)"), *Detail.ToString());
		}
		return Message;
	}
}

void USOTS_UIAbilityLibrary::NotifyAbilityEvent(const UObject* WorldContextObject,
                                                FGameplayTag AbilityTag,
                                                bool bSuccess,
                                                const FText& DetailText)
{
	const FString Message = SOTSUIAbilityInternal::BuildAbilityMessage(AbilityTag, bSuccess, DetailText);

	if (USOTS_NotificationSubsystem* Notification = USOTS_NotificationSubsystem::Get(WorldContextObject))
	{
		Notification->PushNotification(Message, 4.f, AbilityTag);
	}

	if (USOTS_HUDSubsystem* HUD = USOTS_HUDSubsystem::Get(WorldContextObject))
	{
		HUD->SetObjectiveText(Message);
	}
}
