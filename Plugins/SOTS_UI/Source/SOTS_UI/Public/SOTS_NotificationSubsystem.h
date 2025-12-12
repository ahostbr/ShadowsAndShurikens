#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SOTS_NotificationSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FSOTS_NotificationEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Notifications")
    FGuid Id;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Notifications")
    FString Message;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Notifications")
    float DurationSeconds = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Notifications")
    FGameplayTag CategoryTag;
};

UCLASS()
class SOTS_UI_API USOTS_NotificationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static USOTS_NotificationSubsystem* Get(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category="SOTS|Notifications")
    FSOTS_NotificationEntry PushNotification(const FString& Message, float DurationSeconds, FGameplayTag CategoryTag);

    UFUNCTION(BlueprintCallable, Category="SOTS|Notifications")
    void RemoveNotification(FGuid Id);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Notifications")
    const TArray<FSOTS_NotificationEntry>& GetNotifications() const { return ActiveNotifications; }

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSOTS_OnNotificationsChanged);

    UPROPERTY(BlueprintAssignable, Category="SOTS|Notifications")
    FSOTS_OnNotificationsChanged OnNotificationsChanged;

private:
    UPROPERTY()
    TArray<FSOTS_NotificationEntry> ActiveNotifications;

    static constexpr int32 MaxNotifications = 20;
};
