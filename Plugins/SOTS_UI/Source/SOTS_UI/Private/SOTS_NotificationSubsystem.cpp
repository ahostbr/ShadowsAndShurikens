#include "SOTS_NotificationSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

FSOTS_NotificationEntry USOTS_NotificationSubsystem::PushNotification(const FString& Message, float DurationSeconds, FGameplayTag CategoryTag)
{
    FSOTS_NotificationEntry Entry;
    Entry.Id = FGuid::NewGuid();
    Entry.Message = Message;
    Entry.DurationSeconds = DurationSeconds;
    Entry.CategoryTag = CategoryTag;

    ActiveNotifications.Add(Entry);
    if (ActiveNotifications.Num() > MaxNotifications)
    {
        ActiveNotifications.RemoveAt(0);
    }

    OnNotificationsChanged.Broadcast();
    return Entry;
}

void USOTS_NotificationSubsystem::RemoveNotification(FGuid Id)
{
    const int32 Removed = ActiveNotifications.RemoveAll([Id](const FSOTS_NotificationEntry& Entry)
    {
        return Entry.Id == Id;
    });

    if (Removed > 0)
    {
        OnNotificationsChanged.Broadcast();
    }
}

USOTS_NotificationSubsystem* USOTS_NotificationSubsystem::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    if (const UWorld* World = WorldContextObject->GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<USOTS_NotificationSubsystem>();
        }
    }

    return nullptr;
}
