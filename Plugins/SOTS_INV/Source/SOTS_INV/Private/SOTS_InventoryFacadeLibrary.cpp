#include "SOTS_InventoryFacadeLibrary.h"

#include "SOTS_InventoryBridgeSubsystem.h"

namespace
{
    USOTS_InventoryBridgeSubsystem* GetBridge(const UObject* WorldContextObject)
    {
        return USOTS_InventoryBridgeSubsystem::Get(WorldContextObject);
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
        if (Bridge->bDebugLogInventoryOpFailures && OutReport.Result != ESOTS_InventoryOpResult::Success)
        {
            OutReport.DebugReason += FString::Printf(TEXT(" AbilityTag=%s"), *AbilityTag.ToString());
        }
        MaybeLogFacadeFailure(Bridge->bDebugLogInventoryOpFailures, TEXT("TryConsumeItemForAbility"), OutReport);
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
        if (Bridge->bDebugLogInventoryOpFailures && OutReport.Result != ESOTS_InventoryOpResult::Success)
        {
            OutReport.DebugReason += FString::Printf(TEXT(" SkillTag=%s"), *SkillTag.ToString());
        }
        MaybeLogFacadeFailure(Bridge->bDebugLogInventoryOpFailures, TEXT("HasItemForSkillGate"), OutReport);
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
        MaybeLogFacadeFailure(Bridge->bDebugLogInventoryOpFailures, TEXT("GetEquippedItemTag_ForContext"), OutReport);
#endif
        return OutReport.Result == ESOTS_InventoryOpResult::Success;
    }
    OutReport.Result = ESOTS_InventoryOpResult::ProviderMissing;
    return false;
}

static FSOTS_InventoryOpReport RunUIHelper(const UObject* WorldContextObject, TFunctionRef<bool(USOTS_InventoryProvider*)> Fn)
{
    FSOTS_InventoryOpReport Report;
    Report.RequestId = FGuid::NewGuid();

    if (USOTS_InventoryBridgeSubsystem* Bridge = GetBridge(WorldContextObject))
    {
        UObject* ProviderObj = Bridge->GetResolvedProvider(nullptr);
        if (!ProviderObj)
        {
            Report.Result = ESOTS_InventoryOpResult::ProviderMissing;
            return Report;
        }

        if (!ProviderObj->GetClass()->ImplementsInterface(USOTS_InventoryProvider::StaticClass()))
        {
            Report.Result = ESOTS_InventoryOpResult::InternalError;
            Report.DebugReason = TEXT("Provider lacks UI hooks");
            return Report;
        }

        ISOTS_InventoryProvider* Provider = Cast<ISOTS_InventoryProvider>(ProviderObj);
        if (!Provider || !ISOTS_InventoryProvider::Execute_IsInventoryReady(ProviderObj))
        {
            Report.Result = ESOTS_InventoryOpResult::ProviderNotReady;
            return Report;
        }

        const bool bOk = Fn(Provider);
        Report.Result = bOk ? ESOTS_InventoryOpResult::Success : ESOTS_InventoryOpResult::LockedOrBlocked;
        return Report;
    }

    Report.Result = ESOTS_InventoryOpResult::ProviderMissing;
    return Report;
}

bool USOTS_InventoryFacadeLibrary::OpenInventoryUI_WithReport(const UObject* WorldContextObject, FSOTS_InventoryOpReport& OutReport)
{
    OutReport = RunUIHelper(WorldContextObject, [](ISOTS_InventoryProvider* Provider)
    {
        return Provider->OpenInventoryUI();
    });
    if (USOTS_InventoryBridgeSubsystem* Bridge = GetBridge(WorldContextObject))
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        MaybeLogFacadeFailure(Bridge->bDebugLogInventoryOpFailures, TEXT("OpenInventoryUI_WithReport"), OutReport);
#endif
    }
    return OutReport.Result == ESOTS_InventoryOpResult::Success;
}

bool USOTS_InventoryFacadeLibrary::CloseInventoryUI_WithReport(const UObject* WorldContextObject, FSOTS_InventoryOpReport& OutReport)
{
    OutReport = RunUIHelper(WorldContextObject, [](ISOTS_InventoryProvider* Provider)
    {
        return Provider->CloseInventoryUI();
    });
    if (USOTS_InventoryBridgeSubsystem* Bridge = GetBridge(WorldContextObject))
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        MaybeLogFacadeFailure(Bridge->bDebugLogInventoryOpFailures, TEXT("CloseInventoryUI_WithReport"), OutReport);
#endif
    }
    return OutReport.Result == ESOTS_InventoryOpResult::Success;
}

bool USOTS_InventoryFacadeLibrary::ToggleInventoryUI_WithReport(const UObject* WorldContextObject, FSOTS_InventoryOpReport& OutReport)
{
    OutReport = RunUIHelper(WorldContextObject, [](ISOTS_InventoryProvider* Provider)
    {
        return Provider->ToggleInventoryUI();
    });
    if (USOTS_InventoryBridgeSubsystem* Bridge = GetBridge(WorldContextObject))
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        MaybeLogFacadeFailure(Bridge->bDebugLogInventoryOpFailures, TEXT("ToggleInventoryUI_WithReport"), OutReport);
#endif
    }
    return OutReport.Result == ESOTS_InventoryOpResult::Success;
}
