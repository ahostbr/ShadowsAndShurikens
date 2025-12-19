#include "SOTS_InventoryBridgeSubsystem.h"

#include "Interfaces/SOTS_InventoryProviderInterface.h"
#include "Interfaces/SOTS_InventoryProvider.h"
#include "SOTS_InventoryFacadeLibrary.h"
#include "SOTS_ProfileSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "UObject/SoftObjectPath.h"

class UInvSPInventoryProviderComponent;

namespace
{
    UObject* ResolveUIRouter(const UObject* WorldContextObject)
    {
        if (!WorldContextObject)
        {
            return nullptr;
        }

        const UWorld* World = WorldContextObject->GetWorld();
        if (!World)
        {
            return nullptr;
        }

        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            static const FSoftClassPath RouterPath(TEXT("/Script/SOTS_UI.SOTS_UIRouterSubsystem"));
            if (UClass* RouterClass = RouterPath.ResolveClass())
            {
                return GameInstance->GetSubsystemBase(RouterClass);
            }
        }

        return nullptr;
    }

    template <typename ParamsType>
    void CallRouterWithParams(UObject* Router, const FName FuncName, ParamsType& Params)
    {
        if (!Router)
        {
            return;
        }

        if (UFunction* Func = Router->FindFunction(FuncName))
        {
            Router->ProcessEvent(Func, &Params);
        }
    }
}

void USOTS_InventoryBridgeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (UGameInstance* GI = GetGameInstance())
    {
        if (USOTS_ProfileSubsystem* ProfileSubsystem = GI->GetSubsystem<USOTS_ProfileSubsystem>())
        {
            ProfileSubsystem->RegisterProvider(this, 0);
        }
    }
}

void USOTS_InventoryBridgeSubsystem::Deinitialize()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (USOTS_ProfileSubsystem* ProfileSubsystem = GI->GetSubsystem<USOTS_ProfileSubsystem>())
        {
            ProfileSubsystem->UnregisterProvider(this);
        }
    }

    Super::Deinitialize();
}

USOTS_InventoryBridgeSubsystem* USOTS_InventoryBridgeSubsystem::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    if (UWorld* World = WorldContextObject->GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<USOTS_InventoryBridgeSubsystem>();
        }
    }

    return nullptr;
}

bool USOTS_InventoryBridgeSubsystem::TryResolveInventoryProviderNow(AActor* OwnerOverride)
{
    CachedProvider.Reset();
    bProviderResolved = false;

    const AActor* PreferredOwner = OwnerOverride;

    if (!PreferredOwner)
    {
        if (UWorld* World = GetWorld())
        {
            if (APlayerController* PC = World->GetFirstPlayerController())
            {
                PreferredOwner = PC->GetPawn();
            }
        }
    }

    if (PreferredOwner)
    {
        if (UObject* ProviderObj = ResolveInventoryProvider(PreferredOwner))
        {
            CachedProvider = ProviderObj;
            bProviderResolved = true;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            if (bDebugLogProviderResolution)
            {
                UE_LOG(LogTemp, Log, TEXT("[SOTS_INV] Provider resolved: %s"), *GetNameSafe(ProviderObj));
            }
#endif
            return true;
        }
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    const double Now = FPlatformTime::Seconds();
    if (bWarnOnProviderMissing && Now >= NextProviderWarnTime)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SOTS_INV] Inventory provider not found; operations will fail until resolved."));
        NextProviderWarnTime = Now + 5.0; // throttle
    }
#endif

    return false;
}

bool USOTS_InventoryBridgeSubsystem::EnsureProviderResolved(const AActor* Owner) const
{
    if (CachedProvider.IsValid() && bProviderResolved)
    {
        return true;
    }

    const_cast<USOTS_InventoryBridgeSubsystem*>(this)->TryResolveInventoryProviderNow(const_cast<AActor*>(Owner));
    return CachedProvider.IsValid() && bProviderResolved;
}

UObject* USOTS_InventoryBridgeSubsystem::GetResolvedProvider(const AActor* Owner) const
{
    if (EnsureProviderResolved(Owner))
    {
        return CachedProvider.Get();
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    const double Now = FPlatformTime::Seconds();
    if (bWarnOnProviderNotReady && Now >= NextProviderWarnTime)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SOTS_INV] Provider not ready for owner %s"), *GetNameSafe(Owner));
        NextProviderWarnTime = Now + 5.0;
    }
#endif

    return nullptr;
}

UObject* USOTS_InventoryBridgeSubsystem::ResolveProviderForPickup(const UObject* WorldContextObject, AActor* Instigator) const
{
    if (UObject* FromInstigator = USOTS_InventoryFacadeLibrary::ResolveInventoryProviderFromControllerFirst(Instigator))
    {
        return FromInstigator;
    }

    if (const AActor* ContextActor = Cast<AActor>(WorldContextObject))
    {
        if (ContextActor != Instigator)
        {
            if (UObject* FromContextActor = USOTS_InventoryFacadeLibrary::ResolveInventoryProviderFromControllerFirst(const_cast<AActor*>(ContextActor)))
            {
                return FromContextActor;
            }
        }
    }

    return GetResolvedProvider_ForUI(Instigator);
}

void USOTS_InventoryBridgeSubsystem::RequestPickupUINotifications(const FSOTS_InvPickupRequest& Request, const FSOTS_InvPickupResult& Result) const
{
    if (!Result.bSuccess || !Result.bShouldNotifyUI || !Request.bNotifyUIOnSuccess)
    {
        return;
    }

    if (UObject* Router = ResolveUIRouter(this))
    {
        const int32 QuantityToReport = Result.QuantityApplied > 0 ? Result.QuantityApplied : Request.Quantity;

        struct FNotifyPickupItemParams
        {
            FGameplayTag ItemTag;
            int32 Quantity = 0;
        };

        FNotifyPickupItemParams PickupParams;
        PickupParams.ItemTag = Request.ItemTag;
        PickupParams.Quantity = QuantityToReport;
        CallRouterWithParams(Router, TEXT("RequestInvSP_NotifyPickupItem"), PickupParams);

        if (Result.bIsFirstTimePickup)
        {
            struct FNotifyFirstParams
            {
                FGameplayTag ItemTag;
            };

            FNotifyFirstParams FirstParams;
            FirstParams.ItemTag = Request.ItemTag;
            CallRouterWithParams(Router, TEXT("RequestInvSP_NotifyFirstTimePickup"), FirstParams);
        }
    }
}

UObject* USOTS_InventoryBridgeSubsystem::ResolveInventoryProvider(const AActor* Owner) const
{
    if (!Owner)
    {
        return nullptr;
    }

    if (Owner->GetClass()->ImplementsInterface(USOTS_InventoryProviderInterface::StaticClass()))
    {
        return const_cast<AActor*>(Owner);
    }


    TInlineComponentArray<UActorComponent*> Components;
    Owner->GetComponents(Components);
    for (UActorComponent* Component : Components)
    {
        if (!Component)
        {
            continue;
        }

         if (Component->GetClass()->ImplementsInterface(USOTS_InventoryProviderInterface::StaticClass()))
        {
            return Component;
        }
    }

    return nullptr;
}

FSOTS_InventoryOpReport USOTS_InventoryBridgeSubsystem::MakeBaseReport(AActor* Owner, const FGameplayTag& ItemTag, int32 RequestedQty) const
{
    FSOTS_InventoryOpReport Report;
    Report.ItemTag = ItemTag;
    Report.RequestedQty = RequestedQty;
    Report.RequestId = FGuid::NewGuid();
    Report.OwnerActor = Owner;
    return Report;
}

void USOTS_InventoryBridgeSubsystem::LogOpFailureIfNeeded(const FSOTS_InventoryOpReport& Report) const
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (!bDebugLogInventoryOpFailures)
    {
        return;
    }
    if (Report.Result == ESOTS_InventoryOpResult::Success)
    {
        return;
    }
    UE_LOG(LogTemp, Warning, TEXT("[SOTS_INV] Op failed Result=%d Tag=%s Req=%d Actual=%d Reason=%s"),
        static_cast<int32>(Report.Result),
        *Report.ItemTag.ToString(),
        Report.RequestedQty,
        Report.ActualQty,
        *Report.DebugReason);
#endif
}

bool USOTS_InventoryBridgeSubsystem::RequestPickup(const FSOTS_InvPickupRequest& Request, FSOTS_InvPickupResult& OutResult)
{
    OutResult = FSOTS_InvPickupResult();
    OutResult.ItemId = Request.ItemTag.GetTagName();
    OutResult.bShouldNotifyUI = Request.bNotifyUIOnSuccess;

    if (!Request.ItemTag.IsValid() || Request.Quantity <= 0)
    {
        OutResult.FailReason = FText::FromString(TEXT("Invalid pickup request (tag or quantity)"));
        return false;
    }

    AActor* InstigatorActor = Request.InstigatorActor.Get();
    AActor* PickupActor = Request.PickupActor.Get();

    UObject* Provider = ResolveProviderForPickup(this, InstigatorActor);
    if (!Provider)
    {
        OutResult.FailReason = FText::FromString(TEXT("Inventory provider missing"));
        return false;
    }

    OnPickupRequested.Broadcast(Request);

    bool bHandled = false;

    if (Provider->GetClass()->ImplementsInterface(USOTS_InventoryProviderInterface::StaticClass()))
    {
        FText FailReason;
        const bool bCanPickup = ISOTS_InventoryProviderInterface::Execute_CanPickup(Provider, InstigatorActor, PickupActor, Request.ItemTag, Request.Quantity, FailReason);
        if (!bCanPickup)
        {
            OutResult.FailReason = FailReason;
            OnPickupCompleted.Broadcast(Request, OutResult);
            return true;
        }

        OutResult.bSuccess = ISOTS_InventoryProviderInterface::Execute_ExecutePickup(Provider, InstigatorActor, PickupActor, Request.ItemTag, Request.Quantity);
        OutResult.QuantityApplied = OutResult.bSuccess ? Request.Quantity : 0;
        if (!OutResult.bSuccess && FailReason.IsEmpty())
        {
            OutResult.FailReason = FText::FromString(TEXT("ExecutePickup rejected"));
        }
        bHandled = true;
    }
    else if (ISOTS_InventoryProvider* ProviderInterface = Cast<ISOTS_InventoryProvider>(Provider))
    {
        if (!ProviderInterface->IsInventoryReady())
        {
            OutResult.FailReason = FText::FromString(TEXT("Provider not ready"));
            OnPickupCompleted.Broadcast(Request, OutResult);
            return true;
        }

        OutResult.bSuccess = ProviderInterface->AddItemByTag(Request.ItemTag, Request.Quantity);
        OutResult.QuantityApplied = OutResult.bSuccess ? Request.Quantity : 0;
        if (!OutResult.bSuccess)
        {
            OutResult.FailReason = FText::FromString(TEXT("AddItemByTag rejected"));
        }
        bHandled = true;
    }

    if (!bHandled)
    {
        OutResult.FailReason = FText::FromString(TEXT("Resolved provider lacks pickup support"));
        OnPickupCompleted.Broadcast(Request, OutResult);
        return false;
    }

    // Consumption of the pickup actor is deferred to calling systems to avoid side effects in SPINE_2.

    OnPickupCompleted.Broadcast(Request, OutResult);

    if (OutResult.bSuccess)
    {
        RequestPickupUINotifications(Request, OutResult);
    }
    return true;
}

bool USOTS_InventoryBridgeSubsystem::RequestPickupSimple(AActor* Instigator, AActor* PickupActor, FGameplayTag ItemTag, int32 Quantity, bool& bSuccess)
{
    FSOTS_InvPickupRequest Request;
    Request.InstigatorActor = Instigator;
    Request.PickupActor = PickupActor;
    Request.ItemTag = ItemTag;
    Request.Quantity = Quantity;

    FSOTS_InvPickupResult Result;
    const bool bHandled = RequestPickup(Request, Result);
    bSuccess = Result.bSuccess;
    return bHandled;
}

FSOTS_InventoryOpReport USOTS_InventoryBridgeSubsystem::Op_HasItemByTag(AActor* Owner, FGameplayTag ItemTag, int32 Count) const
{
    FSOTS_InventoryOpReport Report = MakeBaseReport(Owner, ItemTag, Count);

    if (Count <= 0)
    {
        Report.Result = ESOTS_InventoryOpResult::Success;
        return Report;
    }

    if (!ItemTag.IsValid())
    {
        Report.Result = ESOTS_InventoryOpResult::InvalidTag;
        Report.DebugReason = TEXT("ItemTag invalid");
        return Report;
    }

    UObject* Provider = GetResolvedProvider(Owner);
    if (!Provider)
    {
        Report.Result = ESOTS_InventoryOpResult::ProviderMissing;
        Report.DebugReason = TEXT("Provider missing");
        return Report;
    }

    if (ISOTS_InventoryProvider* ProviderInterface = Cast<ISOTS_InventoryProvider>(Provider))
    {
        if (!ProviderInterface->IsInventoryReady())
        {
            Report.Result = ESOTS_InventoryOpResult::ProviderNotReady;
            Report.DebugReason = TEXT("Provider not ready");
            return Report;
        }

        int32 Qty = 0;
        const bool bHas = ProviderInterface->HasItemByTag(ItemTag, Count, Qty);
        Report.ActualQty = Qty;
        Report.Result = bHas
            ? ESOTS_InventoryOpResult::Success
            : (Qty > 0 ? ESOTS_InventoryOpResult::NotEnoughQuantity : ESOTS_InventoryOpResult::ItemNotFound);
        return Report;
    }

    if (Provider->GetClass()->ImplementsInterface(USOTS_InventoryProviderInterface::StaticClass()))
    {
        const int32 Qty = ISOTS_InventoryProviderInterface::Execute_GetItemCountByTag(Provider, ItemTag);
        Report.ActualQty = Qty;
        const bool bHas = ISOTS_InventoryProviderInterface::Execute_HasItemByTag(Provider, ItemTag, Count);
        Report.Result = bHas
            ? ESOTS_InventoryOpResult::Success
            : (Qty > 0 ? ESOTS_InventoryOpResult::NotEnoughQuantity : ESOTS_InventoryOpResult::ItemNotFound);
        return Report;
    }

    Report.Result = ESOTS_InventoryOpResult::InternalError;
    Report.DebugReason = TEXT("Unknown provider type");
    return Report;
}

FSOTS_InventoryOpReport USOTS_InventoryBridgeSubsystem::HasItemByTag_WithReport(AActor* Owner, FGameplayTag ItemTag, int32 Count) const
{
    FSOTS_InventoryOpReport Report = Op_HasItemByTag(Owner, ItemTag, Count);
    LogOpFailureIfNeeded(Report);
    return Report;
}

FSOTS_InventoryOpReport USOTS_InventoryBridgeSubsystem::Op_TryConsumeItemByTag(AActor* Owner, FGameplayTag ItemTag, int32 Count)
{
    FSOTS_InventoryOpReport Report = MakeBaseReport(Owner, ItemTag, Count);

    if (Count <= 0)
    {
        Report.Result = ESOTS_InventoryOpResult::InvalidTag;
        Report.DebugReason = TEXT("Quantity <= 0");
        return Report;
    }

    if (!ItemTag.IsValid())
    {
        Report.Result = ESOTS_InventoryOpResult::InvalidTag;
        Report.DebugReason = TEXT("ItemTag invalid");
        return Report;
    }

    UObject* Provider = GetResolvedProvider(Owner);
    if (!Provider)
    {
        Report.Result = ESOTS_InventoryOpResult::ProviderMissing;
        Report.DebugReason = TEXT("Provider missing");
        return Report;
    }

    if (ISOTS_InventoryProvider* ProviderInterface = Cast<ISOTS_InventoryProvider>(Provider))
    {
        if (!ProviderInterface->IsInventoryReady())
        {
            Report.Result = ESOTS_InventoryOpResult::ProviderNotReady;
            Report.DebugReason = TEXT("Provider not ready");
            return Report;
        }
        int32 Consumed = 0;
        const bool bOk = ProviderInterface->TryConsumeItemByTag(ItemTag, Count, Consumed);
        Report.ActualQty = Consumed;
        Report.Result = bOk
            ? ESOTS_InventoryOpResult::Success
            : (Consumed > 0 ? ESOTS_InventoryOpResult::ConsumeRejected : ESOTS_InventoryOpResult::NotEnoughQuantity);
        return Report;
    }

    if (Provider->GetClass()->ImplementsInterface(USOTS_InventoryProviderInterface::StaticClass()))
    {
        const bool bOk = ISOTS_InventoryProviderInterface::Execute_TryConsumeItemByTag(Provider, ItemTag, Count);
        Report.Result = bOk ? ESOTS_InventoryOpResult::Success : ESOTS_InventoryOpResult::NotEnoughQuantity;
        return Report;
    }

    Report.Result = ESOTS_InventoryOpResult::InternalError;
    Report.DebugReason = TEXT("Unknown provider type");
    return Report;
}

FSOTS_InventoryOpReport USOTS_InventoryBridgeSubsystem::TryConsumeItemByTag_WithReport(AActor* Owner, FGameplayTag ItemTag, int32 Count)
{
    FSOTS_InventoryOpReport Report = Op_TryConsumeItemByTag(Owner, ItemTag, Count);
    LogOpFailureIfNeeded(Report);
    return Report;
}

FSOTS_InventoryOpReport USOTS_InventoryBridgeSubsystem::Op_AddItemByTag(AActor* Owner, FGameplayTag ItemTag, int32 Count)
{
    FSOTS_InventoryOpReport Report = MakeBaseReport(Owner, ItemTag, Count);

    if (Count <= 0 || !ItemTag.IsValid())
    {
        Report.Result = ESOTS_InventoryOpResult::InvalidTag;
        Report.DebugReason = TEXT("Invalid input");
        return Report;
    }

    UObject* Provider = GetResolvedProvider(Owner);
    if (!Provider)
    {
        Report.Result = ESOTS_InventoryOpResult::ProviderMissing;
        return Report;
    }

    if (ISOTS_InventoryProvider* ProviderInterface = Cast<ISOTS_InventoryProvider>(Provider))
    {
        if (!ProviderInterface->IsInventoryReady())
        {
            Report.Result = ESOTS_InventoryOpResult::ProviderNotReady;
            Report.DebugReason = TEXT("Provider not ready");
            return Report;
        }

        const bool bOk = ProviderInterface->AddItemByTag(ItemTag, Count);
        Report.Result = bOk ? ESOTS_InventoryOpResult::Success : ESOTS_InventoryOpResult::AddRejected;
        Report.ActualQty = bOk ? Count : 0;
        return Report;
    }

    if (Provider->GetClass()->ImplementsInterface(USOTS_InventoryProviderInterface::StaticClass()))
    {
        Report.Result = ESOTS_InventoryOpResult::InternalError;
        Report.DebugReason = TEXT("Legacy provider lacks add by tag");
        return Report;
    }

    Report.Result = ESOTS_InventoryOpResult::InternalError;
    return Report;
}

FSOTS_InventoryOpReport USOTS_InventoryBridgeSubsystem::AddItemByTag_WithReport(AActor* Owner, FGameplayTag ItemTag, int32 Count)
{
    FSOTS_InventoryOpReport Report = Op_AddItemByTag(Owner, ItemTag, Count);
    LogOpFailureIfNeeded(Report);
    return Report;
}

FSOTS_InventoryOpReport USOTS_InventoryBridgeSubsystem::Op_RemoveItemByTag(AActor* Owner, FGameplayTag ItemTag, int32 Count)
{
    FSOTS_InventoryOpReport Report = MakeBaseReport(Owner, ItemTag, Count);

    if (Count <= 0 || !ItemTag.IsValid())
    {
        Report.Result = ESOTS_InventoryOpResult::InvalidTag;
        Report.DebugReason = TEXT("Invalid input");
        return Report;
    }

    UObject* Provider = GetResolvedProvider(Owner);
    if (!Provider)
    {
        Report.Result = ESOTS_InventoryOpResult::ProviderMissing;
        return Report;
    }

    if (ISOTS_InventoryProvider* ProviderInterface = Cast<ISOTS_InventoryProvider>(Provider))
    {
        if (!ProviderInterface->IsInventoryReady())
        {
            Report.Result = ESOTS_InventoryOpResult::ProviderNotReady;
            Report.DebugReason = TEXT("Provider not ready");
            return Report;
        }

        int32 Consumed = 0;
        const bool bOk = ProviderInterface->TryConsumeItemByTag(ItemTag, Count, Consumed);
        Report.ActualQty = Consumed;
        Report.Result = bOk ? ESOTS_InventoryOpResult::Success : ESOTS_InventoryOpResult::RemoveRejected;
        return Report;
    }

    if (Provider->GetClass()->ImplementsInterface(USOTS_InventoryProviderInterface::StaticClass()))
    {
        const bool bOk = ISOTS_InventoryProviderInterface::Execute_TryConsumeItemByTag(Provider, ItemTag, Count);
        Report.ActualQty = bOk ? Count : 0;
        Report.Result = bOk ? ESOTS_InventoryOpResult::Success : ESOTS_InventoryOpResult::RemoveRejected;
        return Report;
    }

    Report.Result = ESOTS_InventoryOpResult::InternalError;
    return Report;
}

FSOTS_InventoryOpReport USOTS_InventoryBridgeSubsystem::RemoveItemByTag_WithReport(AActor* Owner, FGameplayTag ItemTag, int32 Count)
{
    FSOTS_InventoryOpReport Report = Op_RemoveItemByTag(Owner, ItemTag, Count);
    LogOpFailureIfNeeded(Report);
    return Report;
}

FSOTS_InventoryOpReport USOTS_InventoryBridgeSubsystem::Op_GetEquippedItem(AActor* Owner) const
{
    FSOTS_InventoryOpReport Report = MakeBaseReport(Owner, FGameplayTag(), 0);

    UObject* Provider = GetResolvedProvider(Owner);
    if (!Provider)
    {
        Report.Result = ESOTS_InventoryOpResult::ProviderMissing;
        return Report;
    }

    FGameplayTag Equipped;
    bool bOk = false;

    if (ISOTS_InventoryProvider* ProviderInterface = Cast<ISOTS_InventoryProvider>(Provider))
    {
        bOk = ProviderInterface->GetEquippedItemTag(Equipped);
    }
    else if (Provider->GetClass()->ImplementsInterface(USOTS_InventoryProviderInterface::StaticClass()))
    {
        bOk = ISOTS_InventoryProviderInterface::Execute_GetEquippedItemTag(Provider, Equipped);
    }

    Report.ItemTag = Equipped;
    Report.Result = bOk ? ESOTS_InventoryOpResult::Success : ESOTS_InventoryOpResult::ItemNotFound;
    return Report;
}

FSOTS_InventoryOpReport USOTS_InventoryBridgeSubsystem::GetEquippedItemTag_WithReport(AActor* Owner) const
{
    FSOTS_InventoryOpReport Report = Op_GetEquippedItem(Owner);
    LogOpFailureIfNeeded(Report);
    return Report;
}
int32 USOTS_InventoryBridgeSubsystem::CountItemsOnInvComponent(UInvSP_InventoryComponent* InvComp, FGameplayTag ItemTag) const
{
    if (!InvComp || !ItemTag.IsValid())
    {
        return 0;
    }

    const FName TagName = ItemTag.GetTagName();
    int32 TotalCount = 0;

    for (const FSOTS_SerializedItem& Item : InvComp->GetCarriedItems())
    {
        if (Item.ItemId == TagName)
        {
            TotalCount += Item.Quantity;
        }
    }

    return TotalCount;
}

bool USOTS_InventoryBridgeSubsystem::ConsumeItemsOnInvComponent(UInvSP_InventoryComponent* InvComp, FGameplayTag ItemTag, int32 Count)
{
    if (!InvComp || !ItemTag.IsValid() || Count <= 0)
    {
        return false;
    }

    TArray<FSOTS_SerializedItem> Items = InvComp->GetCarriedItems();
    const FName TagName = ItemTag.GetTagName();

    int32 Remaining = Count;
    for (FSOTS_SerializedItem& Item : Items)
    {
        if (Remaining <= 0)
        {
            break;
        }

        if (Item.ItemId != TagName || Item.Quantity <= 0)
        {
            continue;
        }

        const int32 RemoveAmount = FMath::Min(Item.Quantity, Remaining);
        Item.Quantity -= RemoveAmount;
        Remaining -= RemoveAmount;
    }

    if (Remaining > 0)
    {
        return false;
    }

    Items.RemoveAll([](const FSOTS_SerializedItem& Item)
    {
        return Item.ItemId.IsNone() || Item.Quantity <= 0;
    });

    InvComp->ClearCarriedItems();
    for (const FSOTS_SerializedItem& Item : Items)
    {
        InvComp->AddCarriedItem(Item.ItemId, Item.Quantity);
    }

    return true;
}

UActorComponent* USOTS_InventoryBridgeSubsystem::FindPlayerCarriedInventoryComponent() const
{
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (APawn* Pawn = PC->GetPawn())
            {
                return Pawn->FindComponentByClass<UInvSP_InventoryComponent>();
            }
        }
    }
    return nullptr;
}

UActorComponent* USOTS_InventoryBridgeSubsystem::FindPlayerStashInventoryComponent() const
{
    return FindPlayerCarriedInventoryComponent();
}

void USOTS_InventoryBridgeSubsystem::BuildProfileData(FSOTS_InventoryProfileData& OutData) const
{
    OutData.CarriedItems.Reset();
    OutData.StashItems.Reset();
    OutData.QuickSlots.Reset();

    if (UActorComponent* CarriedComp = FindPlayerCarriedInventoryComponent())
    {
        ExtractItemsFromInventoryComponent(CarriedComp, OutData.CarriedItems);
    }

    if (UActorComponent* StashComp = FindPlayerStashInventoryComponent())
    {
        ExtractItemsFromInventoryComponent(StashComp, OutData.StashItems, true);
    }

    ExtractQuickSlots(OutData.QuickSlots);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bValidateInventorySnapshotOnBuild)
    {
        TArray<FString> Warnings;
        TArray<FString> Errors;
        ValidateInventorySnapshot(OutData, Warnings, Errors);
        if (bDebugLogSnapshotValidation)
        {
            for (const FString& Warn : Warnings)
            {
                UE_LOG(LogTemp, Warning, TEXT("[SOTS_INV] BuildProfileData warning: %s"), *Warn);
            }
            for (const FString& Err : Errors)
            {
                UE_LOG(LogTemp, Warning, TEXT("[SOTS_INV] BuildProfileData error: %s"), *Err);
            }
        }
    }
#endif
}

void USOTS_InventoryBridgeSubsystem::ApplyProfileData(const FSOTS_InventoryProfileData& InData)
{
    TArray<FString> Warnings;
    TArray<FString> Errors;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bValidateInventorySnapshotOnApply)
    {
        ValidateInventorySnapshot(InData, Warnings, Errors);
        if (bDebugLogSnapshotValidation)
        {
            for (const FString& Warn : Warnings)
            {
                UE_LOG(LogTemp, Warning, TEXT("[SOTS_INV] ApplyProfileData warning: %s"), *Warn);
            }
            for (const FString& Err : Errors)
            {
                UE_LOG(LogTemp, Warning, TEXT("[SOTS_INV] ApplyProfileData error: %s"), *Err);
            }
        }
        if (!Errors.IsEmpty())
        {
            return;
        }
    }
#endif

    if (!IsInventoryProviderReady())
    {
        if (bEnableDeferredProfileApply)
        {
            PendingSnapshot = InData;
            bHasPendingSnapshot = true;
            PendingApplyRetries = 0;
            if (UWorld* World = GetWorld())
            {
                World->GetTimerManager().SetTimer(
                    DeferredApplyTimerHandle,
                    this,
                    &USOTS_InventoryBridgeSubsystem::TryApplyPendingSnapshot,
                    DeferredApplyRetryIntervalSeconds,
                    true);
            }
            return;
        }
        return;
    }

    if (UActorComponent* CarriedComp = FindPlayerCarriedInventoryComponent())
    {
        ClearInventoryComponent(CarriedComp);
        ApplyItemsToInventoryComponent(CarriedComp, InData.CarriedItems);
    }

    if (UActorComponent* StashComp = FindPlayerStashInventoryComponent())
    {
        ClearInventoryComponent(StashComp, true);
        ApplyItemsToInventoryComponent(StashComp, InData.StashItems, true);
    }

    ApplyQuickSlots(InData.QuickSlots);

    CachedCarriedItems = InData.CarriedItems;
    CachedStashItems = InData.StashItems;
    CachedQuickSlots = InData.QuickSlots;

    bHasPendingSnapshot = false;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(DeferredApplyTimerHandle);
    }
}

void USOTS_InventoryBridgeSubsystem::BuildProfileSnapshot(FSOTS_ProfileSnapshot& InOutSnapshot)
{
    BuildProfileData(InOutSnapshot.Inventory);
}

void USOTS_InventoryBridgeSubsystem::ApplyProfileSnapshot(const FSOTS_ProfileSnapshot& Snapshot)
{
    ApplyProfileData(Snapshot.Inventory);
}

void USOTS_InventoryBridgeSubsystem::ClearAllInventory()
{
    if (UActorComponent* CarriedComp = FindPlayerCarriedInventoryComponent())
    {
        ClearInventoryComponent(CarriedComp);
        CachedCarriedItems.Reset();
    }

    if (UActorComponent* StashComp = FindPlayerStashInventoryComponent())
    {
        ClearInventoryComponent(StashComp, true);
        CachedStashItems.Reset();
    }

    ApplyQuickSlots({});
    CachedQuickSlots.Reset();
}

void USOTS_InventoryBridgeSubsystem::AddCarriedItemById(FName ItemId, int32 Quantity)
{
    if (ItemId.IsNone() || Quantity <= 0)
    {
        return;
    }

    if (UActorComponent* CarriedComp = FindPlayerCarriedInventoryComponent())
    {
        TArray<FSOTS_SerializedItem> Temp;
        Temp.Add({ ItemId, Quantity });
        ApplyItemsToInventoryComponent(CarriedComp, Temp);
    }

    CachedCarriedItems.Add({ ItemId, Quantity });
}

void USOTS_InventoryBridgeSubsystem::AddStashItemById(FName ItemId, int32 Quantity)
{
    if (ItemId.IsNone() || Quantity <= 0)
    {
        return;
    }

    if (UActorComponent* StashComp = FindPlayerStashInventoryComponent())
    {
        TArray<FSOTS_SerializedItem> Temp;
        Temp.Add({ ItemId, Quantity });
        ApplyItemsToInventoryComponent(StashComp, Temp, true);
    }

    CachedStashItems.Add({ ItemId, Quantity });
}

void USOTS_InventoryBridgeSubsystem::SetQuickSlotItem(int32 SlotIndex, FName ItemId)
{
    if (SlotIndex < 0)
    {
        return;
    }

    TArray<FSOTS_ItemSlotBinding> Bindings;
    ExtractQuickSlots(Bindings);

    bool bFound = false;
    for (FSOTS_ItemSlotBinding& Binding : Bindings)
    {
        if (Binding.SlotIndex == SlotIndex)
        {
            Binding.ItemId = ItemId;
            bFound = true;
            break;
        }
    }

    if (!bFound)
    {
        Bindings.Add({ SlotIndex, ItemId });
    }

    ApplyQuickSlots(Bindings);
    CachedQuickSlots = Bindings;
}

void USOTS_InventoryBridgeSubsystem::ClearQuickSlots()
{
    ApplyQuickSlots({});
    CachedQuickSlots.Reset();
}

bool USOTS_InventoryBridgeSubsystem::HasItemByTag(AActor* Owner, FGameplayTag ItemTag, int32 Count) const
{
    FSOTS_InventoryOpReport Report = HasItemByTag_WithReport(Owner, ItemTag, Count);
    return Report.Result == ESOTS_InventoryOpResult::Success;
}

int32 USOTS_InventoryBridgeSubsystem::GetItemCountByTag(AActor* Owner, FGameplayTag ItemTag) const
{
    FSOTS_InventoryOpReport Report = HasItemByTag_WithReport(Owner, ItemTag, 0);
    return Report.ActualQty;
}

bool USOTS_InventoryBridgeSubsystem::TryConsumeItemByTag(AActor* Owner, FGameplayTag ItemTag, int32 Count)
{
    FSOTS_InventoryOpReport Report = TryConsumeItemByTag_WithReport(Owner, ItemTag, Count);
    return Report.Result == ESOTS_InventoryOpResult::Success;
}

bool USOTS_InventoryBridgeSubsystem::GetEquippedItemTag(AActor* Owner, FGameplayTag& OutItemTag) const
{
    FSOTS_InventoryOpReport Report = GetEquippedItemTag_WithReport(Owner);
    if (Report.Result == ESOTS_InventoryOpResult::Success)
    {
        OutItemTag = Report.ItemTag;
        return true;
    }
    return false;
}

void USOTS_InventoryBridgeSubsystem::ExtractItemsFromInventoryComponent(UActorComponent* Comp, TArray<FSOTS_SerializedItem>& OutItems, bool bStash) const
{
    OutItems.Reset();

    if (UInvSP_InventoryComponent* InvComp = Cast<UInvSP_InventoryComponent>(Comp))
    {
        if (bStash)
        {
            OutItems.Append(InvComp->GetStashItems());
        }
        else
        {
            OutItems.Append(InvComp->GetCarriedItems());
        }
    }
}

void USOTS_InventoryBridgeSubsystem::ClearInventoryComponent(UActorComponent* Comp, bool bStash) const
{
    if (UInvSP_InventoryComponent* InvComp = Cast<UInvSP_InventoryComponent>(Comp))
    {
        if (bStash)
        {
            InvComp->ClearStashItems();
        }
        else
        {
            InvComp->ClearCarriedItems();
        }
    }
}

void USOTS_InventoryBridgeSubsystem::ApplyItemsToInventoryComponent(UActorComponent* Comp, const TArray<FSOTS_SerializedItem>& Items, bool bStash) const
{
    if (UInvSP_InventoryComponent* InvComp = Cast<UInvSP_InventoryComponent>(Comp))
    {
        if (bStash)
        {
            InvComp->ClearStashItems();
        }
        else
        {
            InvComp->ClearCarriedItems();
        }

        for (const FSOTS_SerializedItem& Item : Items)
        {
            if (Item.ItemId.IsNone() || Item.Quantity <= 0)
            {
                continue;
            }

            if (bStash)
            {
                InvComp->AddStashItem(Item.ItemId, Item.Quantity);
            }
            else
            {
                InvComp->AddCarriedItem(Item.ItemId, Item.Quantity);
            }
        }
    }
}

void USOTS_InventoryBridgeSubsystem::ExtractQuickSlots(TArray<FSOTS_ItemSlotBinding>& OutBindings) const
{
    OutBindings.Reset();

    if (UActorComponent* Comp = FindPlayerCarriedInventoryComponent())
    {
        if (UInvSP_InventoryComponent* InvComp = Cast<UInvSP_InventoryComponent>(Comp))
        {
            OutBindings.Append(InvComp->GetQuickSlots());
        }
    }
}

int32 USOTS_InventoryBridgeSubsystem::GetItemCountByTags(const FGameplayTagContainer& Tags, bool bStash) const
{
    if (Tags.IsEmpty())
    {
        return 0;
    }

    TArray<FSOTS_SerializedItem> Items;
    if (UActorComponent* Comp = bStash ? FindPlayerStashInventoryComponent() : FindPlayerCarriedInventoryComponent())
    {
        ExtractItemsFromInventoryComponent(Comp, Items, bStash);
    }
    else
    {
        Items = bStash ? CachedStashItems : CachedCarriedItems;
    }

    int32 TotalCount = 0;
    for (const FGameplayTag& Tag : Tags)
    {
        if (!Tag.IsValid())
        {
            continue;
        }

        const FName TagName = Tag.GetTagName();

        for (const FSOTS_SerializedItem& Item : Items)
        {
            if (Item.ItemId == TagName)
            {
                TotalCount += Item.Quantity;
            }
        }
    }

    return TotalCount;
}

void USOTS_InventoryBridgeSubsystem::ApplyQuickSlots(const TArray<FSOTS_ItemSlotBinding>& InBindings) const
{
    if (UActorComponent* Comp = FindPlayerCarriedInventoryComponent())
    {
        if (UInvSP_InventoryComponent* InvComp = Cast<UInvSP_InventoryComponent>(Comp))
        {
            InvComp->ClearQuickSlots();
            for (const FSOTS_ItemSlotBinding& Binding : InBindings)
            {
                InvComp->SetQuickSlot(Binding.SlotIndex, Binding.ItemId);
            }
        }
    }
}

int32 USOTS_InventoryBridgeSubsystem::GetCarriedItemCountByTags(const FGameplayTagContainer& Tags) const
{
    return GetItemCountByTags(Tags, false);
}

bool USOTS_InventoryBridgeSubsystem::ConsumeCarriedItemsByTags(const FGameplayTagContainer& Tags, int32 Count)
{
    return ConsumeItemsByTagsInternal(Tags, Count, false);
}

bool USOTS_InventoryBridgeSubsystem::ConsumeItemsByTagsInternal(const FGameplayTagContainer& Tags, int32 Count, bool bStash)
{
    if (Count <= 0 || Tags.IsEmpty())
    {
        return false;
    }

    const int32 Available = GetItemCountByTags(Tags, bStash);
    if (Available < Count)
    {
        return false;
    }

    TArray<FSOTS_SerializedItem> Items;
    if (UActorComponent* Comp = bStash ? FindPlayerStashInventoryComponent() : FindPlayerCarriedInventoryComponent())
    {
        ExtractItemsFromInventoryComponent(Comp, Items, bStash);
    }
    else
    {
        Items = bStash ? CachedStashItems : CachedCarriedItems;
    }

    int32 Remaining = Count;
    for (const FGameplayTag& Tag : Tags)
    {
        if (Remaining <= 0)
        {
            break;
        }

        if (!Tag.IsValid())
        {
            continue;
        }

        const FName TagName = Tag.GetTagName();
        for (FSOTS_SerializedItem& Item : Items)
        {
            if (Item.ItemId != TagName || Item.Quantity <= 0)
            {
                continue;
            }

            const int32 RemoveAmount = FMath::Min(Item.Quantity, Remaining);
            Item.Quantity -= RemoveAmount;
            Remaining -= RemoveAmount;

            if (Remaining <= 0)
            {
                break;
            }
        }
    }

    Items.RemoveAll([](const FSOTS_SerializedItem& Item)
    {
        return Item.ItemId.IsNone() || Item.Quantity <= 0;
    });

    if (UActorComponent* Comp = bStash ? FindPlayerStashInventoryComponent() : FindPlayerCarriedInventoryComponent())
    {
        ApplyItemsToInventoryComponent(Comp, Items, bStash);
    }

    if (bStash)
    {
        CachedStashItems = Items;
    }
    else
    {
        CachedCarriedItems = Items;
    }

    return true;
}

void USOTS_InventoryBridgeSubsystem::ValidateInventorySnapshot(const FSOTS_InventoryProfileData& InData, TArray<FString>& OutWarnings, TArray<FString>& OutErrors) const
{
    auto ValidateStackArray = [&](const TArray<FSOTS_SerializedItem>& Items, const TCHAR* Label)
    {
        TSet<FName> SeenIds;
        for (const FSOTS_SerializedItem& Item : Items)
        {
            if (Item.ItemId.IsNone())
            {
                OutErrors.Add(FString::Printf(TEXT("%s contains None ItemId."), Label));
                continue;
            }
            if (Item.Quantity <= 0)
            {
                OutErrors.Add(FString::Printf(TEXT("%s item %s has non-positive quantity %d."), Label, *Item.ItemId.ToString(), Item.Quantity));
            }
            if (SeenIds.Contains(Item.ItemId))
            {
                OutWarnings.Add(FString::Printf(TEXT("%s has duplicate ItemId %s; merge recommended."), Label, *Item.ItemId.ToString()));
            }
            SeenIds.Add(Item.ItemId);
        }
    };

    ValidateStackArray(InData.CarriedItems, TEXT("CarriedItems"));
    ValidateStackArray(InData.StashItems, TEXT("StashItems"));

    for (const FSOTS_ItemSlotBinding& Binding : InData.QuickSlots)
    {
        if (Binding.SlotIndex < 0)
        {
            OutWarnings.Add(TEXT("QuickSlot binding has negative SlotIndex."));
        }
        if (Binding.ItemId.IsNone())
        {
            continue;
        }
        const bool bFoundInCarried = InData.CarriedItems.ContainsByPredicate([&](const FSOTS_SerializedItem& Item){ return Item.ItemId == Binding.ItemId; });
        const bool bFoundInStash = InData.StashItems.ContainsByPredicate([&](const FSOTS_SerializedItem& Item){ return Item.ItemId == Binding.ItemId; });
        if (!bFoundInCarried && !bFoundInStash)
        {
            OutWarnings.Add(FString::Printf(TEXT("QuickSlot references ItemId %s not present in snapshot items."), *Binding.ItemId.ToString()));
        }
    }
}

void USOTS_InventoryBridgeSubsystem::TryApplyPendingSnapshot()
{
    if (!bHasPendingSnapshot)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(DeferredApplyTimerHandle);
        }
        return;
    }

    if (IsInventoryProviderReady())
    {
        ApplyProfileData(PendingSnapshot);
        bHasPendingSnapshot = false;
        return;
    }

    PendingApplyRetries++;
    if (PendingApplyRetries >= DeferredApplyMaxRetries)
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        UE_LOG(LogTemp, Warning, TEXT("[SOTS_INV] Deferred inventory apply timed out."));
#endif
        bHasPendingSnapshot = false;
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(DeferredApplyTimerHandle);
        }
    }
}
