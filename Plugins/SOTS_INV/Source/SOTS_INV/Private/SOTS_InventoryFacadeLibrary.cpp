#include "SOTS_InventoryFacadeLibrary.h"

#include "SOTS_InventoryBridgeSubsystem.h"
#include "Interfaces/SOTS_InventoryProvider.h"
#include "Interfaces/SOTS_InventoryProviderInterface.h"

class AActor;

namespace
{
    USOTS_InventoryBridgeSubsystem* GetBridge(const UObject* WorldContextObject)
    {
        return USOTS_InventoryBridgeSubsystem::Get(WorldContextObject);
    }

    enum class ESOTS_UIHelperAction
    {
        Open,
        Close,
        Toggle
    };

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
        if (UObject* ProviderObj = Bridge->GetResolvedProvider_ForUI(nullptr))
        {
            if (ISOTS_InventoryProvider* Provider = Cast<ISOTS_InventoryProvider>(ProviderObj))
            {
                if (!Provider->IsInventoryReady())
                {
                    Report.Result = ESOTS_InventoryOpResult::ProviderNotReady;
                    Report.DebugReason = FString::Printf(TEXT("%s failed (provider not ready)"), HelperName);
                    return Report;
                }

                const bool bSuccess = [Action, Provider]()
                {
                    switch (Action)
                    {
                        case ESOTS_UIHelperAction::Open:
                            return Provider->OpenInventoryUI();
                        case ESOTS_UIHelperAction::Close:
                            return Provider->CloseInventoryUI();
                        case ESOTS_UIHelperAction::Toggle:
                            return Provider->ToggleInventoryUI();
                    }
                    return false;
                }();

                Report.Result = bSuccess ? ESOTS_InventoryOpResult::Success : ESOTS_InventoryOpResult::InternalError;
                Report.DebugReason = bSuccess
                    ? FString()
                    : FString::Printf(TEXT("%s failed (provider returned false)"), HelperName);
                Report.OwnerActor = Cast<AActor>(Provider->GetProviderObject());
                return Report;
            }

            Report.Result = ESOTS_InventoryOpResult::InternalError;
            Report.DebugReason = FString::Printf(TEXT("%s failed (resolved provider lacks ISOTS_InventoryProvider)"), HelperName);
            return Report;
        }

        Report.DebugReason = FString::Printf(TEXT("%s failed (inventory provider unresolved)"), HelperName);
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
