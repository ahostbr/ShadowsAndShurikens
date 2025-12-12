#include "SOTS_UIAbilityLibrary.h"

#include "SOTS_UIRouterSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_UIAbilityLibrary, Log, All);

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

	if (USOTS_UIRouterSubsystem* Router = USOTS_UIRouterSubsystem::Get(WorldContextObject))
	{
		Router->ShowNotification(Message, 4.f, AbilityTag);
		Router->SetObjectiveText(Message);
		return;
	}

	UE_LOG(LogSOTS_UIAbilityLibrary, Error, TEXT("NotifyAbilityEvent: Router missing. UI Hub Law violated if callers rely on fallback routing. Message=%s"), *Message);
}
