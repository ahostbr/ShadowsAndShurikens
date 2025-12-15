#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SOTS_InventoryTypes.generated.h"

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
