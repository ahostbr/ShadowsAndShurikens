#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Internationalization/Text.h"
#include "SOTS_InventoryTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class ESOTS_InventoryOpResult : uint8
{
    Success             UMETA(DisplayName="Success"),
    ProviderMissing     UMETA(DisplayName="Provider Missing"),
    ProviderNotReady    UMETA(DisplayName="Provider Not Ready"),
    InvalidTag          UMETA(DisplayName="Invalid Tag"),
    ItemNotFound        UMETA(DisplayName="Item Not Found"),
    NotEnoughQuantity   UMETA(DisplayName="Not Enough Quantity"),
    AddRejected         UMETA(DisplayName="Add Rejected"),
    RemoveRejected      UMETA(DisplayName="Remove Rejected"),
    ConsumeRejected     UMETA(DisplayName="Consume Rejected"),
    LockedOrBlocked     UMETA(DisplayName="Locked Or Blocked"),
    InternalError       UMETA(DisplayName="Internal Error")
};

USTRUCT(BlueprintType)
struct FSOTS_InventoryOpReport
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Inventory")
    ESOTS_InventoryOpResult Result = ESOTS_InventoryOpResult::InternalError;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Inventory")
    FGameplayTag ItemTag;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Inventory")
    int32 RequestedQty = 0;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Inventory")
    int32 ActualQty = 0;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Inventory")
    FGuid RequestId;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Inventory")
    TWeakObjectPtr<AActor> OwnerActor;

    // Dev-only debug text (empty in shipping/test by policy).
    UPROPERTY(BlueprintReadOnly, Category="SOTS|Inventory")
    FString DebugReason;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnInventoryOpCompleted, const FSOTS_InventoryOpReport&, Report);

USTRUCT(BlueprintType)
struct FSOTS_InvPickupRequest
{
    GENERATED_BODY()

    FSOTS_InvPickupRequest()
        : Quantity(1)
        , bConsumeWorldActorOnSuccess(true)
        , bNotifyUIOnSuccess(true)
    {}

    UPROPERTY(BlueprintReadWrite, Category="SOTS|Inventory|Pickup")
    TWeakObjectPtr<AActor> InstigatorActor;

    UPROPERTY(BlueprintReadWrite, Category="SOTS|Inventory|Pickup")
    TWeakObjectPtr<AActor> PickupActor;

    UPROPERTY(BlueprintReadWrite, Category="SOTS|Inventory|Pickup")
    FGameplayTag ItemTag;

    UPROPERTY(BlueprintReadWrite, Category="SOTS|Inventory|Pickup")
    int32 Quantity;

    UPROPERTY(BlueprintReadWrite, Category="SOTS|Inventory|Pickup")
    FGameplayTagContainer ContextTags;

    UPROPERTY(BlueprintReadWrite, Category="SOTS|Inventory|Pickup")
    bool bConsumeWorldActorOnSuccess;

    UPROPERTY(BlueprintReadWrite, Category="SOTS|Inventory|Pickup")
    bool bNotifyUIOnSuccess;
};

USTRUCT(BlueprintType)
struct FSOTS_InvPickupResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category="SOTS|Inventory|Pickup")
    bool bSuccess = false;

    UPROPERTY(BlueprintReadWrite, Category="SOTS|Inventory|Pickup")
    bool bShouldNotifyUI = false;

    UPROPERTY(BlueprintReadWrite, Category="SOTS|Inventory|Pickup")
    bool bIsFirstTimePickup = false;

    UPROPERTY(BlueprintReadWrite, Category="SOTS|Inventory|Pickup")
    FText FailReason;

    UPROPERTY(BlueprintReadWrite, Category="SOTS|Inventory|Pickup")
    FGameplayTag FailTag;

    UPROPERTY(BlueprintReadWrite, Category="SOTS|Inventory|Pickup")
    int32 QuantityApplied = 0;

    UPROPERTY(BlueprintReadWrite, Category="SOTS|Inventory|Pickup")
    FName ItemId = NAME_None;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnPickupRequested, const FSOTS_InvPickupRequest&, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSOTS_OnPickupCompleted, const FSOTS_InvPickupRequest&, Request, const FSOTS_InvPickupResult&, Result);
