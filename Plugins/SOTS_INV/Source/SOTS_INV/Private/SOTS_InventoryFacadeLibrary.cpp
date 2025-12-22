#include "SOTS_InventoryFacadeLibrary.h"

#include "SOTS_InventoryBridgeSubsystem.h"
#include "Interfaces/SOTS_InventoryProviderInterface.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "UObject/SoftObjectPath.h"

class AActor;

namespace
{
    USOTS_InventoryBridgeSubsystem* GetBridge(const UObject* WorldContextObject)
    {
        return USOTS_InventoryBridgeSubsystem::Get(WorldContextObject);
    }

    UObject* FacadeResolveUIRouter(const UObject* WorldContextObject)
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

    void FacadeCallRouterNoParams(UObject* Router, const FName FuncName)
    {
        if (!Router)
        {
            return;
        }

        if (UFunction* Func = Router->FindFunction(FuncName))
        {
            Router->ProcessEvent(Func, nullptr);
        }
    }

    template <typename ParamsType>
    void FacadeCallRouterWithParams(UObject* Router, const FName FuncName, ParamsType& Params)
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

    enum class ESOTS_UIHelperAction
    {
        Open,
        Close,
        Toggle
    };

    FGameplayTag ResolveInventoryMenuTag()
    {
        static const FName InventoryNames[] = {
            FName(TEXT("UI.Menu.Inventory")),
            FName(TEXT("SAS.UI.InvSP.InventoryMenu"))
        };

        for (const FName& Name : InventoryNames)
        {
            const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(Name, false);
            if (Tag.IsValid())
            {
                return Tag;
            }
        }

        return FGameplayTag();
    }

    void NotifyExternalMenu(UObject* Router, const FGameplayTag& MenuIdTag, const FName FuncName)
    {
        if (!Router || !MenuIdTag.IsValid())
        {
            return;
        }

        struct FExternalMenuParams
        {
            FGameplayTag MenuIdTag;
        };

        FExternalMenuParams Params;
        Params.MenuIdTag = MenuIdTag;
        FacadeCallRouterWithParams(Router, FuncName, Params);
    }

    void DispatchInventoryUIAction(UObject* Router, ESOTS_UIHelperAction Action)
    {
        if (!Router)
        {
            return;
        }

        const FGameplayTag MenuTag = ResolveInventoryMenuTag();
        switch (Action)
        {
        case ESOTS_UIHelperAction::Open:
            FacadeCallRouterNoParams(Router, TEXT("RequestInvSP_OpenInventory"));
            NotifyExternalMenu(Router, MenuTag, TEXT("NotifyExternalMenuOpened"));
            break;
        case ESOTS_UIHelperAction::Close:
            FacadeCallRouterNoParams(Router, TEXT("RequestInvSP_CloseInventory"));
            NotifyExternalMenu(Router, MenuTag, TEXT("NotifyExternalMenuClosed"));
            break;
        case ESOTS_UIHelperAction::Toggle:
            FacadeCallRouterNoParams(Router, TEXT("RequestInvSP_ToggleInventory"));
            break;
        }
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    void MaybeLogFacadeFailure(bool bEnableLog, const TCHAR* HelperName, const FSOTS_InventoryOpReport& Report)
    {
        if (!bEnableLog || Report.Result == ESOTS_InventoryOpResult::Success)
        {
            return;
        }
        UE_LOG(LogTemp, Warning, TEXT("[SOTS_INV] %s failed: Result=%d Tag=%s Req=%d Actual=%d Reason=%s"),
            HelperName,
            static_cast<int32>(Report.Result),
            *Report.ItemTag.ToString(),
            Report.RequestedQty,
            Report.ActualQty,
            *Report.DebugReason);
    }
#endif

    UObject* ResolveProviderFromActor(AActor* Actor)
    {
        if (!Actor)
        {
            return nullptr;
        }

        if (Actor->GetClass()->ImplementsInterface(USOTS_InventoryProviderInterface::StaticClass()))
        {
            return Actor;
        }

        TInlineComponentArray<UActorComponent*> Components;
        Actor->GetComponents(Components);
        for (UActorComponent* Component : Components)
        {
            if (Component && Component->GetClass()->ImplementsInterface(USOTS_InventoryProviderInterface::StaticClass()))
            {
                return Component;
            }
        }

        return nullptr;
    }

    UObject* ResolveProvider(const UObject* WorldContextObject, AActor* Instigator)
    {
        return USOTS_InventoryFacadeLibrary::GetInventoryProviderFromWorldContext(WorldContextObject, Instigator);
    }
}

bool USOTS_InventoryFacadeLibrary::TryConsumeItemForAbility(const UObject* WorldContextObject,
                                                            FGameplayTag ItemTag,
                                                            int32 Quantity,
                                                            FGameplayTag AbilityTag,
                                                            FSOTS_InventoryOpReport& OutReport)
{
    OutReport = FSOTS_InventoryOpReport();
    if (USOTS_InventoryBridgeSubsystem* Bridge = GetBridge(WorldContextObject))
    {
        OutReport = Bridge->TryConsumeItemByTag_WithReport(nullptr, ItemTag, Quantity);
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (Bridge->ShouldLogInventoryOpFailures() && OutReport.Result != ESOTS_InventoryOpResult::Success)
        {
            OutReport.DebugReason += FString::Printf(TEXT(" AbilityTag=%s"), *AbilityTag.ToString());
        }
        MaybeLogFacadeFailure(Bridge->ShouldLogInventoryOpFailures(), TEXT("TryConsumeItemForAbility"), OutReport);
#endif
        return OutReport.Result == ESOTS_InventoryOpResult::Success;
    }
    OutReport.Result = ESOTS_InventoryOpResult::ProviderMissing;
    return false;
}

bool USOTS_InventoryFacadeLibrary::HasItemForSkillGate(const UObject* WorldContextObject,
                                                       FGameplayTag ItemTag,
                                                       int32 MinQuantity,
                                                       FGameplayTag SkillTag,
                                                       FSOTS_InventoryOpReport& OutReport)
{
    OutReport = FSOTS_InventoryOpReport();
    if (USOTS_InventoryBridgeSubsystem* Bridge = GetBridge(WorldContextObject))
    {
        OutReport = Bridge->HasItemByTag_WithReport(nullptr, ItemTag, MinQuantity);
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (Bridge->ShouldLogInventoryOpFailures() && OutReport.Result != ESOTS_InventoryOpResult::Success)
        {
            OutReport.DebugReason += FString::Printf(TEXT(" SkillTag=%s"), *SkillTag.ToString());
        }
        MaybeLogFacadeFailure(Bridge->ShouldLogInventoryOpFailures(), TEXT("HasItemForSkillGate"), OutReport);
#endif
        return OutReport.Result == ESOTS_InventoryOpResult::Success;
    }
    OutReport.Result = ESOTS_InventoryOpResult::ProviderMissing;
    return false;
}

bool USOTS_InventoryFacadeLibrary::GetEquippedItemTag_ForContext(const UObject* WorldContextObject,
                                                                 FGameplayTag SlotTag,
                                                                 FGameplayTag& OutEquippedTag,
                                                                 FSOTS_InventoryOpReport& OutReport)
{
    OutEquippedTag = FGameplayTag();
    OutReport = FSOTS_InventoryOpReport();
    if (USOTS_InventoryBridgeSubsystem* Bridge = GetBridge(WorldContextObject))
    {
        OutReport = Bridge->GetEquippedItemTag_WithReport(nullptr);
        OutEquippedTag = OutReport.ItemTag;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        MaybeLogFacadeFailure(Bridge->ShouldLogInventoryOpFailures(), TEXT("GetEquippedItemTag_ForContext"), OutReport);
#endif
        return OutReport.Result == ESOTS_InventoryOpResult::Success;
    }
    OutReport.Result = ESOTS_InventoryOpResult::ProviderMissing;
    return false;
}

static FSOTS_InventoryOpReport RunUIHelper(const UObject* WorldContextObject, const TCHAR* HelperName, ESOTS_UIHelperAction Action)
{
    FSOTS_InventoryOpReport Report;
    Report.RequestId = FGuid::NewGuid();
    Report.Result = ESOTS_InventoryOpResult::ProviderMissing;
    Report.DebugReason = FString::Printf(TEXT("%s failed (inventory provider missing)"), HelperName);

    if (USOTS_InventoryBridgeSubsystem* Bridge = GetBridge(WorldContextObject))
    {
        if (UObject* Router = FacadeResolveUIRouter(WorldContextObject))
        {
            DispatchInventoryUIAction(Router, Action);
            Report.Result = ESOTS_InventoryOpResult::Success;
            Report.DebugReason = FString();
            return Report;
        }

        Report.DebugReason = FString::Printf(TEXT("%s failed (UI router missing)"), HelperName);
        return Report;
    }

    Report.DebugReason = FString::Printf(TEXT("%s failed (inventory bridge missing)"), HelperName);
    return Report;
}

bool USOTS_InventoryFacadeLibrary::OpenInventoryUI_WithReport(const UObject* WorldContextObject, FSOTS_InventoryOpReport& OutReport)
{
    OutReport = RunUIHelper(WorldContextObject, TEXT("OpenInventoryUI_WithReport"), ESOTS_UIHelperAction::Open);
    if (USOTS_InventoryBridgeSubsystem* Bridge = GetBridge(WorldContextObject))
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        MaybeLogFacadeFailure(Bridge->ShouldLogInventoryOpFailures(), TEXT("OpenInventoryUI_WithReport"), OutReport);
#endif
    }
    return OutReport.Result == ESOTS_InventoryOpResult::Success;
}

bool USOTS_InventoryFacadeLibrary::CloseInventoryUI_WithReport(const UObject* WorldContextObject, FSOTS_InventoryOpReport& OutReport)
{
    OutReport = RunUIHelper(WorldContextObject, TEXT("CloseInventoryUI_WithReport"), ESOTS_UIHelperAction::Close);
    if (USOTS_InventoryBridgeSubsystem* Bridge = GetBridge(WorldContextObject))
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        MaybeLogFacadeFailure(Bridge->ShouldLogInventoryOpFailures(), TEXT("CloseInventoryUI_WithReport"), OutReport);
#endif
    }
    return OutReport.Result == ESOTS_InventoryOpResult::Success;
}

bool USOTS_InventoryFacadeLibrary::ToggleInventoryUI_WithReport(const UObject* WorldContextObject, FSOTS_InventoryOpReport& OutReport)
{
    OutReport = RunUIHelper(WorldContextObject, TEXT("ToggleInventoryUI_WithReport"), ESOTS_UIHelperAction::Toggle);
    if (USOTS_InventoryBridgeSubsystem* Bridge = GetBridge(WorldContextObject))
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        MaybeLogFacadeFailure(Bridge->ShouldLogInventoryOpFailures(), TEXT("ToggleInventoryUI_WithReport"), OutReport);
#endif
    }
    return OutReport.Result == ESOTS_InventoryOpResult::Success;
}

void USOTS_InventoryFacadeLibrary::RequestOpenInventoryUI(const UObject* WorldContextObject)
{
    if (UObject* Router = FacadeResolveUIRouter(WorldContextObject))
    {
        DispatchInventoryUIAction(Router, ESOTS_UIHelperAction::Open);
    }
}

void USOTS_InventoryFacadeLibrary::RequestCloseInventoryUI(const UObject* WorldContextObject)
{
    if (UObject* Router = FacadeResolveUIRouter(WorldContextObject))
    {
        DispatchInventoryUIAction(Router, ESOTS_UIHelperAction::Close);
    }
}

void USOTS_InventoryFacadeLibrary::RequestToggleInventoryUI(const UObject* WorldContextObject)
{
    if (UObject* Router = FacadeResolveUIRouter(WorldContextObject))
    {
        DispatchInventoryUIAction(Router, ESOTS_UIHelperAction::Toggle);
    }
}

void USOTS_InventoryFacadeLibrary::RequestRefreshInventoryUI(const UObject* WorldContextObject)
{
    if (UObject* Router = FacadeResolveUIRouter(WorldContextObject))
    {
        FacadeCallRouterNoParams(Router, TEXT("RequestInvSP_RefreshInventory"));
    }
}

void USOTS_InventoryFacadeLibrary::RequestSetShortcutMenuVisible(const UObject* WorldContextObject, bool bVisible)
{
    if (UObject* Router = FacadeResolveUIRouter(WorldContextObject))
    {
        struct FSetShortcutMenuVisibleParams
        {
            bool bVisible = false;
        };

        FSetShortcutMenuVisibleParams Params;
        Params.bVisible = bVisible;
        FacadeCallRouterWithParams(Router, TEXT("RequestInvSP_SetShortcutMenuVisible"), Params);
    }
}

FName USOTS_InventoryFacadeLibrary::ItemIdFromTag(FGameplayTag ItemTag)
{
    return ItemTag.GetTagName();
}

UObject* USOTS_InventoryFacadeLibrary::GetInventoryProviderFromWorldContext(const UObject* WorldContextObject, AActor* Instigator)
{
    if (UObject* FromInstigator = ResolveInventoryProviderFromControllerFirst(Instigator))
    {
        return FromInstigator;
    }

    if (const AActor* ContextActor = Cast<AActor>(WorldContextObject))
    {
        if (ContextActor != Instigator)
        {
            if (UObject* FromContextActor = ResolveInventoryProviderFromControllerFirst(const_cast<AActor*>(ContextActor)))
            {
                return FromContextActor;
            }
        }
    }

    if (USOTS_InventoryBridgeSubsystem* Bridge = GetBridge(WorldContextObject))
    {
        return Bridge->GetResolvedProvider_ForUI(Instigator);
    }

    return nullptr;
}

UObject* USOTS_InventoryFacadeLibrary::ResolveInventoryProviderFromControllerFirst(AActor* Instigator)
{
    if (!Instigator)
    {
        return nullptr;
    }

    if (AController* Controller = Instigator->GetInstigatorController())
    {
        if (UObject* FromController = ResolveProviderFromActor(Controller))
        {
            return FromController;
        }

        if (APawn* Pawn = Controller->GetPawn())
        {
            if (UObject* FromPawn = ResolveProviderFromActor(Pawn))
            {
                return FromPawn;
            }
        }
    }

    return ResolveProviderFromActor(Instigator);
}

bool USOTS_InventoryFacadeLibrary::RequestPickup_FromRequest(const UObject* WorldContextObject,
                                                             const FSOTS_InvPickupRequest& Request,
                                                             FSOTS_InvPickupResult& OutResult)
{
    OutResult = FSOTS_InvPickupResult();
    OutResult.ItemId = Request.ItemTag.GetTagName();

    if (USOTS_InventoryBridgeSubsystem* Bridge = GetBridge(WorldContextObject))
    {
        return Bridge->RequestPickup(Request, OutResult);
    }

    OutResult.FailReason = FText::FromString(TEXT("Inventory bridge missing"));
    return false;
}

bool USOTS_InventoryFacadeLibrary::RequestPickupSimple(const UObject* WorldContextObject,
                                                       AActor* Instigator,
                                                       AActor* PickupActor,
                                                       FGameplayTag ItemTag,
                                                       int32 Quantity,
                                                       bool& bSuccess)
{
    FSOTS_InvPickupRequest Request;
    Request.InstigatorActor = Instigator;
    Request.PickupActor = PickupActor;
    Request.ItemTag = ItemTag;
    Request.Quantity = Quantity;

    FSOTS_InvPickupResult Result;
    const bool bHandled = RequestPickup_FromRequest(WorldContextObject, Request, Result);
    bSuccess = Result.bSuccess;
    return bHandled;
}

bool USOTS_InventoryFacadeLibrary::RequestPickup(const UObject* WorldContextObject,
                                                 AActor* Instigator,
                                                 AActor* PickupActor,
                                                 FGameplayTag ItemTag,
                                                 int32 Quantity,
                                                 bool& bSuccess)
{
    return RequestPickupSimple(WorldContextObject, Instigator, PickupActor, ItemTag, Quantity, bSuccess);
}
