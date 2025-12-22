#include "Components/SOTS_AbilityComponent.h"

#include "Abilities/SOTS_AbilityBase.h"
#include "Subsystems/SOTS_AbilityRegistrySubsystem.h"
#include "Subsystems/SOTS_AbilitySubsystem.h"
#include "SOTS_GAS_AbilityRequirementLibrary.h"
#include "Subsystems/SOTS_AbilityFXSubsystem.h"
#include "SOTS_GAS_Plugin.h"
#include "SOTS_FXManagerSubsystem.h"
#include "SOTS_UIAbilityLibrary.h"
#include "SOTS_InventoryBridgeSubsystem.h"
#include "SOTS_InventoryTypes.h"
#include "SOTS_SkillTreeSubsystem.h"
#include "SOTS_TagLibrary.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace SOTSAbilityUIHelpers
{
	static FString GetActivationResultName(E_SOTS_AbilityActivationResult Result)
	{
		if (const UEnum* Enum = StaticEnum<E_SOTS_AbilityActivationResult>())
		{
			return Enum->GetNameStringByValue(static_cast<int64>(Result));
		}
		return TEXT("Unknown");
	}

	static void NotifyAbilityEvent(UAC_SOTS_Abilitys* OwnerComponent, FGameplayTag AbilityTag, bool bSuccess, E_SOTS_AbilityActivationResult Result)
	{
		if (!OwnerComponent)
		{
			return;
		}

		const FString Reason = SOTSAbilityUIHelpers::GetActivationResultName(Result);
		USOTS_UIAbilityLibrary::NotifyAbilityEvent(OwnerComponent, AbilityTag, bSuccess, FText::FromString(Reason));
	}
}

namespace
{
    static FString ResultToString(E_SOTS_AbilityActivateResult Result)
    {
        if (const UEnum* Enum = StaticEnum<E_SOTS_AbilityActivateResult>())
        {
            return Enum->GetNameStringByValue(static_cast<int64>(Result));
        }
        return TEXT("Unknown");
    }

    static E_SOTS_AbilityActivationResult MapReportToLegacy(const FSOTS_AbilityActivateReport& Report)
    {
        switch (Report.Result)
        {
            case E_SOTS_AbilityActivateResult::Success:
                return E_SOTS_AbilityActivationResult::Success;
            case E_SOTS_AbilityActivateResult::CooldownActive:
                return E_SOTS_AbilityActivationResult::Failed_Cooldown;
            case E_SOTS_AbilityActivateResult::NotEnoughCharges:
                return E_SOTS_AbilityActivationResult::Failed_ChgDepleted;
            case E_SOTS_AbilityActivateResult::MissingRequiredItem:
                return E_SOTS_AbilityActivationResult::Failed_InventoryGate;
            case E_SOTS_AbilityActivateResult::AbilityLocked:
            case E_SOTS_AbilityActivateResult::SkillTreeMissing:
            case E_SOTS_AbilityActivateResult::SkillNotUnlocked:
            case E_SOTS_AbilityActivateResult::SkillPrereqNotMet:
                return E_SOTS_AbilityActivationResult::Failed_SkillGate;
            case E_SOTS_AbilityActivateResult::BlockedByState:
                return E_SOTS_AbilityActivationResult::Failed_OwnerTags;
            default:
                return E_SOTS_AbilityActivationResult::Failed_CustomCondition;
        }
    }

    static FString InventoryResultToString(ESOTS_InventoryOpResult Result)
    {
        if (const UEnum* Enum = StaticEnum<ESOTS_InventoryOpResult>())
        {
            return Enum->GetNameStringByValue(static_cast<int64>(Result));
        }
        return TEXT("Unknown");
    }
}

namespace SOTSAbilityInventoryCosts
{
    static void GatherValidTags(const FGameplayTagContainer& Tags, TArray<FGameplayTag>& OutTags)
    {
        OutTags.Reset();
        for (const FGameplayTag& Tag : Tags)
        {
            if (Tag.IsValid())
            {
                OutTags.Add(Tag);
            }
        }

        OutTags.Sort([](const FGameplayTag& A, const FGameplayTag& B)
        {
            return A.ToString() < B.ToString();
        });
    }

    static FSOTS_InventoryOpReport MakeFallbackReport(AActor* Owner, const FGameplayTag& ItemTag, int32 Count, ESOTS_InventoryOpResult Result, const FString& Reason)
    {
        FSOTS_InventoryOpReport Report;
        Report.OwnerActor = Owner;
        Report.ItemTag = ItemTag;
        Report.RequestedQty = Count;
        Report.Result = Result;
        Report.RequestId = FGuid::NewGuid();
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        Report.DebugReason = Reason;
#endif
        return Report;
    }

    static void MaybeLogInventoryReport(bool bEnabled, const TCHAR* Prefix, const FSOTS_InventoryOpReport& Report)
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (!bEnabled)
        {
            return;
        }

        UE_LOG(LogSOTSGAS, Log, TEXT("[AbilityComponent][Inventory] %s Tag=%s Qty=%d Result=%s Debug=%s"),
               Prefix,
               *Report.ItemTag.ToString(),
               Report.RequestedQty,
               *InventoryResultToString(Report.Result),
               *Report.DebugReason);
#endif
    }

    static FSOTS_InventoryOpReport HasRequiredItem(
        UAC_SOTS_Abilitys* Component,
        AActor* Owner,
        const FGameplayTagContainer& Tags,
        int32 Count,
        E_SOTS_AbilityInventoryTagMatchMode MatchMode,
        bool bDebugLog)
    {
        TArray<FGameplayTag> ValidTags;
        GatherValidTags(Tags, ValidTags);
        if (ValidTags.Num() == 0)
        {
            return MakeFallbackReport(Owner, FGameplayTag(), Count, ESOTS_InventoryOpResult::InvalidTag, TEXT("No inventory tag configured"));
        }

        if (USOTS_InventoryBridgeSubsystem* Bridge = USOTS_InventoryBridgeSubsystem::Get(Component))
        {
            FSOTS_InventoryOpReport LastReport;

            if (MatchMode == E_SOTS_AbilityInventoryTagMatchMode::AllOf)
            {
                for (const FGameplayTag& Tag : ValidTags)
                {
                    LastReport = Bridge->HasItemByTag_WithReport(Owner, Tag, Count);
                    MaybeLogInventoryReport(bDebugLog, TEXT("HasItem(All)"), LastReport);
                    if (LastReport.Result != ESOTS_InventoryOpResult::Success)
                    {
                        return LastReport;
                    }
                }
                return LastReport;
            }

            for (const FGameplayTag& Tag : ValidTags)
            {
                LastReport = Bridge->HasItemByTag_WithReport(Owner, Tag, Count);
                MaybeLogInventoryReport(bDebugLog, TEXT("HasItem(Any)"), LastReport);
                if (LastReport.Result == ESOTS_InventoryOpResult::Success)
                {
                    return LastReport;
                }
            }

            return LastReport;
        }

        return MakeFallbackReport(Owner, ValidTags[0], Count, ESOTS_InventoryOpResult::ProviderMissing, TEXT("Inventory bridge missing"));
    }

    static FSOTS_InventoryOpReport ConsumeItem(
        UAC_SOTS_Abilitys* Component,
        AActor* Owner,
        const FGameplayTagContainer& Tags,
        int32 Count,
        E_SOTS_AbilityInventoryTagMatchMode MatchMode,
        bool bDebugLog)
    {
        TArray<FGameplayTag> ValidTags;
        GatherValidTags(Tags, ValidTags);
        if (ValidTags.Num() == 0)
        {
            return MakeFallbackReport(Owner, FGameplayTag(), Count, ESOTS_InventoryOpResult::InvalidTag, TEXT("No inventory tag configured"));
        }

        if (USOTS_InventoryBridgeSubsystem* Bridge = USOTS_InventoryBridgeSubsystem::Get(Component))
        {
            FSOTS_InventoryOpReport LastReport;

            if (MatchMode == E_SOTS_AbilityInventoryTagMatchMode::AllOf)
            {
                for (const FGameplayTag& Tag : ValidTags)
                {
                    LastReport = Bridge->HasItemByTag_WithReport(Owner, Tag, Count);
                    MaybeLogInventoryReport(bDebugLog, TEXT("HasItem(All)"), LastReport);
                    if (LastReport.Result != ESOTS_InventoryOpResult::Success)
                    {
                        return LastReport;
                    }
                }

                for (const FGameplayTag& Tag : ValidTags)
                {
                    LastReport = Bridge->TryConsumeItemByTag_WithReport(Owner, Tag, Count);
                    MaybeLogInventoryReport(bDebugLog, TEXT("Consume(All)"), LastReport);
                    if (LastReport.Result != ESOTS_InventoryOpResult::Success)
                    {
                        return LastReport;
                    }
                }

                return LastReport;
            }

            for (const FGameplayTag& Tag : ValidTags)
            {
                LastReport = Bridge->TryConsumeItemByTag_WithReport(Owner, Tag, Count);
                MaybeLogInventoryReport(bDebugLog, TEXT("Consume(Any)"), LastReport);
                if (LastReport.Result == ESOTS_InventoryOpResult::Success)
                {
                    return LastReport;
                }
            }

            return LastReport;
        }

        return MakeFallbackReport(Owner, ValidTags[0], Count, ESOTS_InventoryOpResult::ProviderMissing, TEXT("Inventory bridge missing"));
    }

    static E_SOTS_AbilityActivateResult MapToAbilityResult(const FSOTS_InventoryOpReport& Report)
    {
        switch (Report.Result)
        {
            case ESOTS_InventoryOpResult::Success:
                return E_SOTS_AbilityActivateResult::Success;
            case ESOTS_InventoryOpResult::ProviderMissing:
            case ESOTS_InventoryOpResult::ProviderNotReady:
            case ESOTS_InventoryOpResult::InvalidTag:
            case ESOTS_InventoryOpResult::ItemNotFound:
            case ESOTS_InventoryOpResult::NotEnoughQuantity:
                return E_SOTS_AbilityActivateResult::MissingRequiredItem;
            case ESOTS_InventoryOpResult::ConsumeRejected:
            case ESOTS_InventoryOpResult::RemoveRejected:
            case ESOTS_InventoryOpResult::LockedOrBlocked:
                return E_SOTS_AbilityActivateResult::CostApplyFailed;
            default:
                return E_SOTS_AbilityActivateResult::InternalError;
        }
    }
}

namespace SOTSSkillGate
{
    enum class ESkillGateResult : uint8
    {
        Allowed,
        SkillTreeMissing,
        NotUnlocked,
        MissingPrereq,
        InvalidDefinition
    };

    struct FSkillGateReport
    {
        ESkillGateResult Result = ESkillGateResult::InvalidDefinition;
        FGameplayTag BlockingSkillTag;
        FString DebugReason;

        bool IsAllowed() const { return Result == ESkillGateResult::Allowed; }
    };

    static USOTS_SkillTreeSubsystem* ResolveSkillTreeSubsystem(const UAC_SOTS_Abilitys* Component)
    {
        if (!Component)
        {
            return nullptr;
        }

        if (const UWorld* World = Component->GetWorld())
        {
            if (UGameInstance* GI = World->GetGameInstance())
            {
                return GI->GetSubsystem<USOTS_SkillTreeSubsystem>();
            }
        }

        return nullptr;
    }

    static FSkillGateReport Evaluate(const UAC_SOTS_Abilitys* Component, const F_SOTS_AbilityDefinition& Def)
    {
        FSkillGateReport Report;

        // If no gating is configured, allow.
        if (Def.SkillGateMode == E_SOTS_AbilitySkillGateMode::None || Def.RequiredSkillTags.Num() == 0)
        {
            Report.Result = ESkillGateResult::Allowed;
            return Report;
        }

        if (!Component || !Component->GetOwner())
        {
            Report.Result = ESkillGateResult::SkillTreeMissing;
            Report.DebugReason = TEXT("No ability component owner");
            return Report;
        }

        if (USOTS_SkillTreeSubsystem* SkillTree = ResolveSkillTreeSubsystem(Component))
        {
            for (const FGameplayTag& SkillTag : Def.RequiredSkillTags)
            {
                if (!SkillTag.IsValid())
                {
                    Report.Result = ESkillGateResult::InvalidDefinition;
                    Report.BlockingSkillTag = SkillTag;
                    Report.DebugReason = TEXT("Invalid skill tag in definition");
                    return Report;
                }

                if (!SkillTree->HasSkillTag(SkillTag))
                {
                    Report.Result = ESkillGateResult::NotUnlocked;
                    Report.BlockingSkillTag = SkillTag;
                    Report.DebugReason = TEXT("Required skill not unlocked");
                    return Report;
                }
            }

            Report.Result = ESkillGateResult::Allowed;
            return Report;
        }

        Report.Result = ESkillGateResult::SkillTreeMissing;
        Report.DebugReason = TEXT("SkillTree subsystem missing");
        return Report;
    }

    static E_SOTS_AbilityActivateResult MapToActivationResult(const FSkillGateReport& Report)
    {
        switch (Report.Result)
        {
            case ESkillGateResult::Allowed:
                return E_SOTS_AbilityActivateResult::Success;
            case ESkillGateResult::SkillTreeMissing:
                return E_SOTS_AbilityActivateResult::SkillTreeMissing;
            case ESkillGateResult::NotUnlocked:
                return E_SOTS_AbilityActivateResult::SkillNotUnlocked;
            case ESkillGateResult::MissingPrereq:
                return E_SOTS_AbilityActivateResult::SkillPrereqNotMet;
            case ESkillGateResult::InvalidDefinition:
            default:
                return E_SOTS_AbilityActivateResult::InternalError;
        }
    }

    static void MaybeLogSkillGate(bool bEnabled, const TCHAR* Prefix, FGameplayTag AbilityTag, const FSkillGateReport& Report)
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (!bEnabled)
        {
            return;
        }

        UE_LOG(LogSOTSGAS, Log, TEXT("[AbilityComponent][SkillGate] %s Ability=%s Skill=%s Result=%d Reason=%s"),
               Prefix,
               *AbilityTag.ToString(),
               *Report.BlockingSkillTag.ToString(),
               static_cast<int32>(Report.Result),
               *Report.DebugReason);
#endif
    }
}

namespace SOTSAbilityInventory
{
    static int32 CountItemsByTags(const TArray<FSOTS_SerializedItem>& Items, const FGameplayTagContainer& Tags)
    {
        if (Tags.IsEmpty())
        {
            return 0;
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

    static bool ConsumeItemsByTags(TArray<FSOTS_SerializedItem>& Items, const FGameplayTagContainer& Tags, int32 Count)
    {
        if (Count <= 0)
        {
            return false;
        }

        if (CountItemsByTags(Items, Tags) < Count)
        {
            return false;
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

        return true;
    }
}

UAC_SOTS_Abilitys::UAC_SOTS_Abilitys()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UAC_SOTS_Abilitys::BeginPlay()
{
    Super::BeginPlay();

    if (!AbilitySubsystem)
    {
        AbilitySubsystem = USOTS_AbilitySubsystem::Get(this);
    }
}

USOTS_AbilityRegistrySubsystem* UAC_SOTS_Abilitys::GetRegistry() const
{
    if (const UWorld* World = GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            return GI->GetSubsystem<USOTS_AbilityRegistrySubsystem>();
        }
    }
    return nullptr;
}

bool UAC_SOTS_Abilitys::InternalGetDefinition(FGameplayTag AbilityTag, F_SOTS_AbilityDefinition& OutDef) const
{
    if (!AbilityTag.IsValid())
    {
        return false;
    }

    if (USOTS_AbilityRegistrySubsystem* Registry = GetRegistry())
    {
        return Registry->GetAbilityDefinitionByTag(AbilityTag, OutDef);
    }

    return false;
}

float UAC_SOTS_Abilitys::GetWorldTime() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetTimeSeconds() : 0.0f;
}

bool UAC_SOTS_Abilitys::PassesOwnerTagGate(const F_SOTS_AbilityDefinition& Def) const
{
    const AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return Def.RequiredOwnerTags.IsEmpty() && Def.BlockedOwnerTags.IsEmpty();
    }

    if (!Def.RequiredOwnerTags.IsEmpty())
    {
        const bool bHasAll = USOTS_TagLibrary::ActorHasAllTags(this, OwnerActor, Def.RequiredOwnerTags);
        if (!bHasAll)
        {
            return false;
        }
    }

    if (!Def.BlockedOwnerTags.IsEmpty())
    {
        const bool bHasAny = USOTS_TagLibrary::ActorHasAnyTag(this, OwnerActor, Def.BlockedOwnerTags);
        if (bHasAny)
        {
            return false;
        }
    }

    return true;
}

int32 UAC_SOTS_Abilitys::QueryInventoryItemCount(const FGameplayTagContainer& Tags, E_SOTS_AbilityInventoryTagMatchMode MatchMode) const
{
    if (Tags.IsEmpty())
    {
        return 0;
    }

    if (USOTS_InventoryBridgeSubsystem* Bridge = USOTS_InventoryBridgeSubsystem::Get(this))
    {
        if (MatchMode == E_SOTS_AbilityInventoryTagMatchMode::AllOf)
        {
            const AActor* OwnerActor = GetOwner();
            int32 MinCount = INT32_MAX;
            bool bHasValidTag = false;

            for (const FGameplayTag& Tag : Tags)
            {
                if (!Tag.IsValid())
                {
                    continue;
                }

                bHasValidTag = true;
                const FSOTS_InventoryOpReport Report = Bridge->HasItemByTag_WithReport(const_cast<AActor*>(OwnerActor), Tag, 1);
                MinCount = FMath::Min(MinCount, Report.ActualQty);
            }

            return bHasValidTag ? MinCount : 0;
        }

        return Bridge->GetCarriedItemCountByTags(Tags);
    }

    return 0;
}

bool UAC_SOTS_Abilitys::HasSufficientCharges(const F_SOTS_AbilityDefinition& Def) const
{
    const F_SOTS_AbilityRuntimeState* State = RuntimeStates.Find(Def.AbilityTag);

    switch (Def.ChargeMode)
    {
        case E_SOTS_AbilityChargeMode::InternalOnly:
        {
            // MaxCharges <= 0 means infinite uses
            if (Def.MaxCharges <= 0)
            {
                return true;
            }

            const int32 Internal = State ? State->CurrentCharges : Def.MaxCharges;
            return Internal > 0;
        }

        case E_SOTS_AbilityChargeMode::InventoryLinked:
        {
            const int32 Count = QueryInventoryItemCount(Def.RequiredInventoryTags, Def.InventoryTagMatchMode);
            return Count > 0;
        }

        case E_SOTS_AbilityChargeMode::Hybrid:
        {
            const int32 InventoryCount = QueryInventoryItemCount(Def.RequiredInventoryTags, Def.InventoryTagMatchMode);

            // MaxCharges <= 0 means infinite internal charges; only inventory gates us
            if (Def.MaxCharges <= 0)
            {
                return InventoryCount > 0;
            }

            const int32 InternalCharges = State ? State->CurrentCharges : Def.MaxCharges;
            return InternalCharges > 0 && InventoryCount > 0;
        }

        default:
            break;
    }

    return true;
}


void UAC_SOTS_Abilitys::ConsumeChargesOnActivation(const F_SOTS_AbilityDefinition& Def)
{
    F_SOTS_AbilityRuntimeState* State = RuntimeStates.Find(Def.AbilityTag);
    if (!State)
    {
        State = &RuntimeStates.FindOrAdd(Def.AbilityTag);
        State->Handle.AbilityTag = Def.AbilityTag;
        State->Handle.InternalId = NextInternalHandleId++;
        State->Handle.bIsValid   = true;

        // Initialise internal charges only if finite
        if (Def.ChargeMode == E_SOTS_AbilityChargeMode::InternalOnly ||
            Def.ChargeMode == E_SOTS_AbilityChargeMode::Hybrid)
        {
            if (Def.MaxCharges > 0)
            {
                State->CurrentCharges = Def.MaxCharges;
            }
        }
    }

    switch (Def.ChargeMode)
    {
        case E_SOTS_AbilityChargeMode::InternalOnly:
        {
            // Only decrement if finite
            if (Def.MaxCharges > 0 && State->CurrentCharges > 0)
            {
                --State->CurrentCharges;
            }
            break;
        }

        case E_SOTS_AbilityChargeMode::InventoryLinked:
            break;

        case E_SOTS_AbilityChargeMode::Hybrid:
        {
            // Finite internal + inventory
            if (Def.MaxCharges > 0 && State->CurrentCharges > 0)
            {
                --State->CurrentCharges;
            }
            break;
        }
    }

    // Cooldown still applies even if internal charges are infinite
    if (State)
    {
        const float WorldTime = GetWorldTime();
        const float Cooldown  = Def.CooldownSeconds;
        State->CooldownEndTime = (Cooldown > 0.0f ? WorldTime + Cooldown : 0.0f);
    }
}


bool UAC_SOTS_Abilitys::GrantAbility(FGameplayTag AbilityTag, const F_SOTS_AbilityGrantOptions& Options, F_SOTS_AbilityHandle& OutHandle)
{
    OutHandle = F_SOTS_AbilityHandle();

    F_SOTS_AbilityDefinition Def;
    if (!InternalGetDefinition(AbilityTag, Def))
    {
        return false;
    }

    const bool bEnforceGrantGate = (Def.SkillGateMode == E_SOTS_AbilitySkillGateMode::RequireForGrant ||
                                    Def.SkillGateMode == E_SOTS_AbilitySkillGateMode::RequireForBoth);
    if (bEnforceGrantGate)
    {
        const SOTSSkillGate::FSkillGateReport GateReport = SOTSSkillGate::Evaluate(this, Def);
        SOTSSkillGate::MaybeLogSkillGate(bDebugLogAbilitySkillGates, TEXT("Grant"), AbilityTag, GateReport);
        if (!GateReport.IsAllowed())
        {
            return false;
        }
    }

    F_SOTS_AbilityRuntimeState& State = RuntimeStates.FindOrAdd(AbilityTag);
    if (!State.Handle.bIsValid)
    {
        State.Handle.AbilityTag = AbilityTag;
        State.Handle.InternalId = NextInternalHandleId++;
        State.Handle.bIsValid   = true;
    }

    // Only finite internal charges are stored here
    State.CurrentCharges = 0;
    if (Def.ChargeMode == E_SOTS_AbilityChargeMode::InternalOnly ||
        Def.ChargeMode == E_SOTS_AbilityChargeMode::Hybrid)
    {
        if (Def.MaxCharges > 0)
        {
            State.CurrentCharges = (Options.InitialCharges > 0 ? Options.InitialCharges : Def.MaxCharges);
        }
        // MaxCharges <= 0 => infinite; we leave CurrentCharges at 0 and ignore it in HasSufficientCharges
    }

    if (Options.bStartOnCooldown)
    {
        const float WorldTime = GetWorldTime();
        const float Cooldown  = (Options.OverrideCooldownSeconds >= 0.0f)
                                ? Options.OverrideCooldownSeconds
                                : Def.CooldownSeconds;

        State.CooldownEndTime = (Cooldown > 0.0f ? WorldTime + Cooldown : 0.0f);
    }
    else
    {
        State.CooldownEndTime = 0.0f;
    }

    OutHandle = State.Handle;

    // Instance the ability object
    if (Def.AbilityClass)
    {
        USOTS_AbilityBase* AbilityInstance = nullptr;
        if (USOTS_AbilityBase** Found = AbilityInstances.Find(AbilityTag))
        {
            AbilityInstance = *Found;
        }

        if (!AbilityInstance)
        {
            AbilityInstance = NewObject<USOTS_AbilityBase>(this, Def.AbilityClass);
            if (AbilityInstance)
            {
                AbilityInstances.Add(AbilityTag, AbilityInstance);
            }
        }

        if (AbilityInstance)
        {
            AbilityInstance->Initialize(this, Def, State.Handle);
        }
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bDebugLogAbilityGrants)
    {
        UE_LOG(LogSOTSGAS, Log,
               TEXT("[AbilityComponent] Granted ability '%s' to '%s' (HandleId=%d)."),
               *AbilityTag.ToString(),
               *GetOwner()->GetName(),
               State.Handle.InternalId);
    }
#endif

    BroadcastAbilityListChanged();
    BroadcastAbilityStateChanged(AbilityTag);

    return true;
}


bool UAC_SOTS_Abilitys::RevokeAbilityByTag(FGameplayTag AbilityTag)
{
    const bool bHadRuntimeState = RuntimeStates.Contains(AbilityTag);

    if (USOTS_AbilityBase** Found = AbilityInstances.Find(AbilityTag))
    {
        if (USOTS_AbilityBase* Ability = *Found)
        {
            Ability->OnAbilityRemoved();
        }
        AbilityInstances.Remove(AbilityTag);
    }

    const int32 RemovedCount = RuntimeStates.Remove(AbilityTag);

    if (RemovedCount > 0 || bHadRuntimeState)
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bDebugLogAbilityGrants)
        {
            UE_LOG(LogSOTSGAS, Log,
                   TEXT("[AbilityComponent] Revoked ability '%s' from '%s'."),
                   *AbilityTag.ToString(),
                   *GetOwner()->GetName());
        }
#endif

        BroadcastAbilityListChanged();
        BroadcastAbilityStateChanged(AbilityTag);
    }

    return RemovedCount > 0;
}

bool UAC_SOTS_Abilitys::TryActivateAbilityByTag(FGameplayTag AbilityTag, const F_SOTS_AbilityActivationContext& Context, E_SOTS_AbilityActivationResult& OutResult)
{
    const FSOTS_AbilityActivateReport Report = ProcessActivationRequest({ AbilityTag, Context, /*bCommit*/ true });

    switch (Report.Result)
    {
        case E_SOTS_AbilityActivateResult::Success:
            OutResult = E_SOTS_AbilityActivationResult::Success;
            return true;
        case E_SOTS_AbilityActivateResult::CooldownActive:
            OutResult = E_SOTS_AbilityActivationResult::Failed_Cooldown;
            break;
        case E_SOTS_AbilityActivateResult::NotEnoughCharges:
            OutResult = E_SOTS_AbilityActivationResult::Failed_ChgDepleted;
            break;
        case E_SOTS_AbilityActivateResult::MissingRequiredItem:
            OutResult = E_SOTS_AbilityActivationResult::Failed_InventoryGate;
            break;
        case E_SOTS_AbilityActivateResult::AbilityLocked:
            OutResult = E_SOTS_AbilityActivationResult::Failed_SkillGate;
            break;
        case E_SOTS_AbilityActivateResult::BlockedByState:
            OutResult = E_SOTS_AbilityActivationResult::Failed_OwnerTags;
            break;
        default:
            OutResult = E_SOTS_AbilityActivationResult::Failed_CustomCondition;
            break;
    }

    return false;
}

FSOTS_AbilityActivateReport UAC_SOTS_Abilitys::TryActivateAbilityByTag_WithReport(FGameplayTag AbilityTag, const F_SOTS_AbilityActivationContext& Context)
{
    return ProcessActivationRequest({ AbilityTag, Context, /*bCommit*/ true });
}

FSOTS_AbilityActivateReport UAC_SOTS_Abilitys::ProcessActivationRequest(const FSOTS_AbilityActivationParams& Params)
{
    FSOTS_AbilityActivateReport Report;
    Report.AbilityTag = Params.AbilityTag;
    Report.TimestampSeconds = GetWorldTime();

    if (!Params.AbilityTag.IsValid())
    {
        Report.Result = E_SOTS_AbilityActivateResult::AbilityNotFound;
        Report.DebugReason = TEXT("Invalid ability tag");
        if (Params.bCommit)
        {
            BroadcastActivationFailed(Report);
        }
        return Report;
    }

    if (!GetOwner())
    {
        Report.Result = E_SOTS_AbilityActivateResult::InvalidOwner;
        Report.DebugReason = TEXT("Missing owner");
        if (Params.bCommit)
        {
            BroadcastActivationFailed(Report);
        }
        return Report;
    }

    USOTS_AbilityRegistrySubsystem* Registry = GetRegistry();
    if (!Registry || !Registry->IsAbilityRegistryReady())
    {
        Report.Result = E_SOTS_AbilityActivateResult::RegistryNotReady;
        Report.DebugReason = TEXT("Ability registry not ready");
        if (Params.bCommit)
        {
            BroadcastActivationFailed(Report);
        }
        return Report;
    }

    F_SOTS_AbilityDefinition Def;
    if (!Registry->GetAbilityDefinitionByTag(Params.AbilityTag, Def))
    {
        Report.Result = E_SOTS_AbilityActivateResult::AbilityNotFound;
        Report.DebugReason = TEXT("Definition not found in registry");
        if (Params.bCommit)
        {
            BroadcastActivationFailed(Report);
        }
        return Report;
    }

    FText FailureReason;
    if (!EvaluateAbilityRequirementsForDefinition(Def, &FailureReason))
    {
        Report.Result = E_SOTS_AbilityActivateResult::AbilityLocked;
        Report.DebugReason = FailureReason.IsEmpty() ? TEXT("Requirements failed") : FailureReason.ToString();

        if (Params.bCommit && Def.FXTag_OnFailRequirements.IsValid())
        {
            USOTS_GAS_AbilityRequirementLibrary::TriggerFailureFX(GetOwner(), Def.FXTag_OnFailRequirements);
        }

        if (Params.bCommit)
        {
            BroadcastActivationFailed(Report);
        }
        return Report;
    }

    bool bIsOnCooldown = false;
    float RemainingCooldown = 0.0f;
    IsAbilityOnCooldown(Params.AbilityTag, bIsOnCooldown, RemainingCooldown);
    if (bIsOnCooldown)
    {
        Report.Result = E_SOTS_AbilityActivateResult::CooldownActive;
        Report.DebugReason = FString::Printf(TEXT("Cooldown active (%.2fs remaining)"), RemainingCooldown);
        Report.RemainingCooldown = RemainingCooldown;
        if (Params.bCommit)
        {
            BroadcastActivationFailed(Report);
        }
        return Report;
    }

    if (!HasSufficientCharges(Def))
    {
        Report.Result = E_SOTS_AbilityActivateResult::NotEnoughCharges;
        Report.DebugReason = TEXT("Insufficient charges");
        if (Params.bCommit)
        {
            BroadcastActivationFailed(Report);
        }
        return Report;
    }

    const bool bHasInventoryRequirement = (Def.InventoryMode != E_SOTS_AbilityInventoryMode::None) && (Def.RequiredInventoryTags.Num() > 0);
    if (bHasInventoryRequirement)
    {
        const int32 RequiredCount = 1; // current authored abilities use single-item requirements
        const FSOTS_InventoryOpReport InventoryCheck = SOTSAbilityInventoryCosts::HasRequiredItem(
            this,
            GetOwner(),
            Def.RequiredInventoryTags,
            RequiredCount,
            Def.InventoryTagMatchMode,
            bDebugLogAbilityInventoryCosts);
        const E_SOTS_AbilityActivateResult InventoryResult = SOTSAbilityInventoryCosts::MapToAbilityResult(InventoryCheck);

        if (InventoryResult != E_SOTS_AbilityActivateResult::Success)
        {
            Report.Result = InventoryResult;
            Report.DebugReason = FString::Printf(TEXT("Inventory require %s x%d -> %s (%s)"),
                                                 *InventoryCheck.ItemTag.ToString(),
                                                 InventoryCheck.RequestedQty,
                                                 *ResultToString(InventoryResult),
                                                 *InventoryCheck.DebugReason);
            if (Params.bCommit)
            {
                BroadcastActivationFailed(Report);
            }
            return Report;
        }
    }

    if (!PassesOwnerTagGate(Def))
    {
        Report.Result = E_SOTS_AbilityActivateResult::BlockedByState;
        Report.DebugReason = TEXT("Blocked by owner tags/state");
        if (Params.bCommit)
        {
            BroadcastActivationFailed(Report);
        }
        return Report;
    }

    if (Def.SkillGateMode == E_SOTS_AbilitySkillGateMode::RequireForActivate ||
        Def.SkillGateMode == E_SOTS_AbilitySkillGateMode::RequireForBoth)
    {
        const SOTSSkillGate::FSkillGateReport GateReport = SOTSSkillGate::Evaluate(this, Def);
        const E_SOTS_AbilityActivateResult GateResult = SOTSSkillGate::MapToActivationResult(GateReport);
        SOTSSkillGate::MaybeLogSkillGate(bDebugLogAbilitySkillGates, TEXT("Activate"), Params.AbilityTag, GateReport);

        if (GateResult != E_SOTS_AbilityActivateResult::Success)
        {
            Report.Result = GateResult;
            Report.BlockingSkillTag = GateReport.BlockingSkillTag;
            Report.DebugReason = FString::Printf(TEXT("Skill gate failed (%s): %s"),
                                                 *GateReport.BlockingSkillTag.ToString(),
                                                 *GateReport.DebugReason);
            if (Params.bCommit)
            {
                BroadcastActivationFailed(Report);
            }
            return Report;
        }
    }

    if (!Params.bCommit)
    {
        Report.Result = E_SOTS_AbilityActivateResult::Success;
        return Report;
    }

    F_SOTS_AbilityRuntimeState& State = RuntimeStates.FindOrAdd(Params.AbilityTag);
    if (!State.Handle.bIsValid)
    {
        State.Handle.AbilityTag = Params.AbilityTag;
        State.Handle.InternalId = NextInternalHandleId++;
        State.Handle.bIsValid   = true;
    }

    if (bHasInventoryRequirement && Def.InventoryMode == E_SOTS_AbilityInventoryMode::RequireAndConsume)
    {
        const int32 ConsumeCount = 1;
        const FSOTS_InventoryOpReport ConsumeReport = SOTSAbilityInventoryCosts::ConsumeItem(
            this,
            GetOwner(),
            Def.RequiredInventoryTags,
            ConsumeCount,
            Def.InventoryTagMatchMode,
            bDebugLogAbilityInventoryCosts);
        const E_SOTS_AbilityActivateResult ConsumeResult = SOTSAbilityInventoryCosts::MapToAbilityResult(ConsumeReport);

        if (ConsumeResult != E_SOTS_AbilityActivateResult::Success)
        {
            Report.Result = ConsumeResult;
            Report.DebugReason = FString::Printf(TEXT("Inventory consume %s x%d -> %s (%s)"),
                                                 *ConsumeReport.ItemTag.ToString(),
                                                 ConsumeReport.RequestedQty,
                                                 *ResultToString(ConsumeResult),
                                                 *ConsumeReport.DebugReason);
            BroadcastActivationFailed(Report);
            return Report;
        }
    }

    // Commit: consume resources and spin up the instance.
    ConsumeChargesOnActivation(Def);
    State.ActivationId = FGuid::NewGuid();
    State.bIsActive    = true;

    // Compute remaining charges post-consumption.
    int32 RemainingCharges = 0;
    int32 MaxCharges = 0;
    GetAbilityCharges(Params.AbilityTag, RemainingCharges, MaxCharges);
    Report.RemainingCharges = RemainingCharges;
    Report.MaxCharges = MaxCharges;

    float CooldownRemaining = 0.0f;
    bool bCooldownNow = false;
    IsAbilityOnCooldown(Params.AbilityTag, bCooldownNow, CooldownRemaining);
    Report.RemainingCooldown = CooldownRemaining;
    Report.ActivationId = State.ActivationId;
    Report.Result = E_SOTS_AbilityActivateResult::Success;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bDebugLogAbilityActivationStarts)
    {
        UE_LOG(LogSOTSGAS, Log, TEXT("[AbilityComponent] Activation start %s (%s)"),
               *Params.AbilityTag.ToString(), *State.ActivationId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
    }
#endif

    USOTS_AbilityBase* AbilityInstance = AbilityInstances.FindRef(Params.AbilityTag);
    if (!AbilityInstance)
    {
        AbilityInstance = USOTS_AbilityBase::GetAbilityInstance(this, Def, State.Handle);
        AbilityInstances.Add(Params.AbilityTag, AbilityInstance);
    }

    OnAbilityActivated.Broadcast(Params.AbilityTag, State.Handle);
    OnAbilityActivatedWithContext.Broadcast(Params.AbilityTag, State.Handle, Params.Context);
    BroadcastActivationStarted(Report);

    if (AbilityInstance)
    {
        AbilityInstance->K2_ActivateAbility(Params.Context);
    }

    if (USOTS_AbilityFXSubsystem* FXSubsystem = USOTS_AbilityFXSubsystem::Get(this))
    {
        if (Def.FXTag_OnActivate.IsValid())
        {
            FXSubsystem->TriggerAbilityFX(Def.FXTag_OnActivate, Params.AbilityTag, GetOwner());
        }
    }

    SOTSAbilityUIHelpers::NotifyAbilityEvent(this, Params.AbilityTag, /*bActivated*/ true, E_SOTS_AbilityActivationResult::Success);
    BroadcastAbilityStateChanged(Params.AbilityTag);

    return Report;
}

void UAC_SOTS_Abilitys::CancelAllAbilities()
{
    for (auto& Kvp : RuntimeStates)
    {
        const FGameplayTag& AbilityTag = Kvp.Key;
        F_SOTS_AbilityRuntimeState& State = Kvp.Value;

        if (State.bIsActive)
        {
            FinalizeActivation(AbilityTag, State, E_SOTS_AbilityEndReason::Cancelled);
        }
    }
}

void UAC_SOTS_Abilitys::FinalizeActivation(FGameplayTag AbilityTag, F_SOTS_AbilityRuntimeState& State, E_SOTS_AbilityEndReason EndReason)
{
    if (USOTS_AbilityBase** FoundAbility = AbilityInstances.Find(AbilityTag))
    {
        if (USOTS_AbilityBase* Ability = *FoundAbility)
        {
            Ability->K2_EndAbility();
        }
    }

    const FGuid ActivationId = State.ActivationId;
    State.bIsActive = false;
    State.ActivationId.Invalidate();

    OnAbilityEnded.Broadcast(AbilityTag, State.Handle);
    BroadcastActivationEnded(AbilityTag, ActivationId, EndReason);
    BroadcastAbilityStateChanged(AbilityTag);
}

void UAC_SOTS_Abilitys::GetAbilityCharges(FGameplayTag AbilityTag, int32& OutCurrentCharges, int32& OutMaxCharges) const
{
    OutCurrentCharges = 0;
    OutMaxCharges     = 0;

    F_SOTS_AbilityDefinition Def;
    if (!const_cast<UAC_SOTS_Abilitys*>(this)->InternalGetDefinition(AbilityTag, Def))
    {
        return;
    }

    const F_SOTS_AbilityRuntimeState* State = RuntimeStates.Find(AbilityTag);

    switch (Def.ChargeMode)
    {
        case E_SOTS_AbilityChargeMode::InternalOnly:
        {
            if (Def.MaxCharges <= 0)
            {
                // Infinite: UI can treat Max <= 0 as ∞
                OutCurrentCharges = 0;
                OutMaxCharges     = 0;
            }
            else
            {
                const int32 Internal = State ? State->CurrentCharges : Def.MaxCharges;
                OutCurrentCharges    = Internal;
                OutMaxCharges        = Def.MaxCharges;
            }
            break;
        }

        case E_SOTS_AbilityChargeMode::InventoryLinked:
        {
            const int32 Count = QueryInventoryItemCount(Def.RequiredInventoryTags, Def.InventoryTagMatchMode);
            OutCurrentCharges = Count;
            OutMaxCharges     = Count;
            break;
        }

        case E_SOTS_AbilityChargeMode::Hybrid:
        {
            const int32 Count    = QueryInventoryItemCount(Def.RequiredInventoryTags, Def.InventoryTagMatchMode);
            const int32 Internal = (Def.MaxCharges > 0)
                                   ? (State ? State->CurrentCharges : Def.MaxCharges)
                                   : INT32_MAX; // treat internal as infinite if MaxCharges <= 0

            const int32 Effective = FMath::Min(Internal, Count);

            OutCurrentCharges = Effective;
            OutMaxCharges     = (Def.MaxCharges > 0 ? Def.MaxCharges : Count);
            break;
        }
    }
}


void UAC_SOTS_Abilitys::IsAbilityOnCooldown(FGameplayTag AbilityTag, bool& bOutIsOnCooldown, float& OutRemainingTime) const
{
    bOutIsOnCooldown = false;
    OutRemainingTime = 0.0f;

    const F_SOTS_AbilityRuntimeState* State = RuntimeStates.Find(AbilityTag);
    if (!State)
    {
        return;
    }

    const float WorldTime = GetWorldTime();
    if (State->CooldownEndTime > 0.0f && WorldTime < State->CooldownEndTime)
    {
        bOutIsOnCooldown = true;
        OutRemainingTime = State->CooldownEndTime - WorldTime;
    }
}

bool UAC_SOTS_Abilitys::RemoveAbility(FGameplayTag AbilityTag)
{
    return RevokeAbilityByTag(AbilityTag);
}

bool UAC_SOTS_Abilitys::HasAbility(FGameplayTag AbilityTag) const
{
    return RuntimeStates.Contains(AbilityTag);
}

void UAC_SOTS_Abilitys::GetKnownAbilities(TArray<FGameplayTag>& OutAbilityTags) const
{
    OutAbilityTags.Reset();
    RuntimeStates.GetKeys(OutAbilityTags);
}

bool UAC_SOTS_Abilitys::CanActivateAbility(FGameplayTag AbilityTag, FText& OutFailureReason) const
{
    OutFailureReason = FText::GetEmpty();

    const FSOTS_AbilityActivateReport Report = const_cast<UAC_SOTS_Abilitys*>(this)->ProcessActivationRequest({ AbilityTag, F_SOTS_AbilityActivationContext(), /*bCommit*/ false });

    if (Report.Result == E_SOTS_AbilityActivateResult::Success)
    {
        return true;
    }

    switch (Report.Result)
    {
        case E_SOTS_AbilityActivateResult::AbilityNotFound:
            OutFailureReason = FText::FromString(TEXT("Ability definition not found."));
            break;
        case E_SOTS_AbilityActivateResult::InvalidOwner:
            OutFailureReason = FText::FromString(TEXT("Invalid ability owner."));
            break;
        case E_SOTS_AbilityActivateResult::RegistryNotReady:
            OutFailureReason = FText::FromString(TEXT("Ability registry not ready."));
            break;
        case E_SOTS_AbilityActivateResult::CooldownActive:
            OutFailureReason = FText::FromString(TEXT("Ability is on cooldown."));
            break;
        case E_SOTS_AbilityActivateResult::NotEnoughCharges:
            OutFailureReason = FText::FromString(TEXT("No charges remaining."));
            break;
        case E_SOTS_AbilityActivateResult::MissingRequiredItem:
            OutFailureReason = FText::FromString(TEXT("Required inventory items missing."));
            break;
        case E_SOTS_AbilityActivateResult::BlockedByState:
            OutFailureReason = FText::FromString(TEXT("Ability blocked by owner tags."));
            break;
        case E_SOTS_AbilityActivateResult::AbilityLocked:
            OutFailureReason = FText::FromString(TEXT("Required skills not unlocked."));
            break;
        case E_SOTS_AbilityActivateResult::SkillTreeMissing:
            OutFailureReason = FText::FromString(TEXT("Skill tree subsystem unavailable."));
            break;
        case E_SOTS_AbilityActivateResult::SkillNotUnlocked:
            OutFailureReason = FText::FromString(TEXT("Required skills not unlocked."));
            break;
        case E_SOTS_AbilityActivateResult::SkillPrereqNotMet:
            OutFailureReason = FText::FromString(TEXT("Skill prerequisites not met."));
            break;
        default:
            OutFailureReason = FText::FromString(Report.DebugReason);
            break;
    }

    return false;
}

bool UAC_SOTS_Abilitys::ActivateAbility(FGameplayTag AbilityTag, const F_SOTS_AbilityActivationContext& Context, FText& OutFailureReason)
{
    OutFailureReason = FText::GetEmpty();

    const FSOTS_AbilityActivateReport Report = ProcessActivationRequest({ AbilityTag, Context, /*bCommit*/ true });

    if (Report.Result == E_SOTS_AbilityActivateResult::Success)
    {
        return true;
    }

    switch (Report.Result)
    {
        case E_SOTS_AbilityActivateResult::CooldownActive:
            OutFailureReason = FText::FromString(TEXT("Ability is on cooldown."));
            break;
        case E_SOTS_AbilityActivateResult::NotEnoughCharges:
            OutFailureReason = FText::FromString(TEXT("No charges remaining."));
            break;
        case E_SOTS_AbilityActivateResult::MissingRequiredItem:
            OutFailureReason = FText::FromString(TEXT("Required inventory items missing."));
            break;
        case E_SOTS_AbilityActivateResult::BlockedByState:
            OutFailureReason = FText::FromString(TEXT("Ability blocked by owner tags."));
            break;
        case E_SOTS_AbilityActivateResult::AbilityLocked:
            OutFailureReason = FText::FromString(TEXT("Required skills not unlocked."));
            break;
        case E_SOTS_AbilityActivateResult::SkillTreeMissing:
            OutFailureReason = FText::FromString(TEXT("Skill tree subsystem unavailable."));
            break;
        case E_SOTS_AbilityActivateResult::SkillNotUnlocked:
            OutFailureReason = FText::FromString(TEXT("Required skills not unlocked."));
            break;
        case E_SOTS_AbilityActivateResult::SkillPrereqNotMet:
            OutFailureReason = FText::FromString(TEXT("Skill prerequisites not met."));
            break;
        case E_SOTS_AbilityActivateResult::RegistryNotReady:
            OutFailureReason = FText::FromString(TEXT("Ability registry not ready."));
            break;
        case E_SOTS_AbilityActivateResult::InvalidOwner:
            OutFailureReason = FText::FromString(TEXT("Invalid ability owner."));
            break;
        case E_SOTS_AbilityActivateResult::AbilityNotFound:
            OutFailureReason = FText::FromString(TEXT("Ability definition not found."));
            break;
        default:
            OutFailureReason = FText::FromString(Report.DebugReason);
            break;
    }

    return false;
}

bool UAC_SOTS_Abilitys::ForceGrantAbility(FGameplayTag AbilityTag, const F_SOTS_AbilityGrantOptions& Options, F_SOTS_AbilityHandle& OutHandle)
{
    // Simply bypass skill gating by temporarily disabling the skill gate mode.
    OutHandle = F_SOTS_AbilityHandle();

    F_SOTS_AbilityDefinition Def;
    if (!InternalGetDefinition(AbilityTag, Def))
    {
        return false;
    }

    const E_SOTS_AbilitySkillGateMode OriginalMode = Def.SkillGateMode;
    Def.SkillGateMode = E_SOTS_AbilitySkillGateMode::None;

    if (!GrantAbility(AbilityTag, Options, OutHandle))
    {
        return false;
    }

    // Restore the authored mode on the registry copy so future grants behave as authored.
    if (USOTS_AbilityRegistrySubsystem* Registry = GetRegistry())
    {
        F_SOTS_AbilityDefinition RegistryDef;
        if (Registry->GetAbilityDefinitionByTag(AbilityTag, RegistryDef))
        {
            RegistryDef.SkillGateMode = OriginalMode;
            Registry->RegisterAbilityDefinition(RegistryDef);
        }
    }

    return true;
}

bool UAC_SOTS_Abilitys::ForceRemoveAbility(FGameplayTag AbilityTag)
{
    return RevokeAbilityByTag(AbilityTag);
}

void UAC_SOTS_Abilitys::GetSerializedState(F_SOTS_AbilityComponentSaveData& OutData) const
{
    OutData.Snapshot = BuildRuntimeSnapshot();

    if (bValidateAbilitySnapshotOnSave)
    {
        TArray<FString> Warnings;
        TArray<FString> Errors;
        ValidateAbilitySnapshot(OutData.Snapshot, Warnings, Errors);
        LogSnapshotMessages(TEXT("Save"), Warnings, Errors);
    }

    OutData.SavedAbilities.Reset();

    TArray<FGameplayTag> SortedAbilityTags;
    RuntimeStates.GetKeys(SortedAbilityTags);
    SortedAbilityTags.Sort([](const FGameplayTag& A, const FGameplayTag& B){ return A.ToString() < B.ToString(); });

    for (const FGameplayTag& AbilityTag : SortedAbilityTags)
    {
        F_SOTS_AbilityStateSnapshot Snapshot = BuildStateSnapshot(AbilityTag);
        OutData.SavedAbilities.Add(Snapshot);
    }
}

void UAC_SOTS_Abilitys::ApplySerializedState(const F_SOTS_AbilityComponentSaveData& InData)
{
    ApplySerializedState_WithReport(InData);
}

FSOTS_AbilityApplyReport UAC_SOTS_Abilitys::ApplySerializedState_WithReport(const F_SOTS_AbilityComponentSaveData& InData)
{
    FSOTS_AbilityRuntimeSnapshot SnapshotToApply = InData.Snapshot;

    if (SnapshotToApply.Entries.Num() == 0 && InData.SavedAbilities.Num() > 0)
    {
        SnapshotToApply = BuildRuntimeSnapshotFromLegacyArray(InData.SavedAbilities);
    }

    const FSOTS_AbilityApplyReport Report = ApplyRuntimeSnapshot(SnapshotToApply);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bDebugLogAbilitySnapshot)
    {
        UE_LOG(LogSOTSGAS, Log, TEXT("[AbilityComponent] ApplySerializedState Result=%d Applied=%d MissingDefs=%d Invalid=%d Reason=%s"),
               static_cast<int32>(Report.Result), Report.AppliedCount, Report.SkippedMissingDefs, Report.SkippedInvalidEntries, *Report.DebugReason);
    }
#endif

    return Report;
}

bool UAC_SOTS_Abilitys::EvaluateAbilityRequirementsForDefinition(const F_SOTS_AbilityDefinition& Def, FText* OutFailureReason) const
{
    const FSOTS_AbilityRequirements& Requirements = Def.AbilityRequirements;

    const bool bHasAuthoredRequirements =
        Requirements.AbilityTag.IsValid() ||
        !Requirements.RequiredSkillTags.IsEmpty() ||
        !Requirements.RequiredPlayerTags.IsEmpty() ||
        Requirements.MinStealthTier >= 0 ||
        Requirements.MaxStealthTier >= 0 ||
        Requirements.bDisallowWhenCompromised ||
        Requirements.MaxStealthScore01 >= 0.0f;

    if (!bHasAuthoredRequirements)
    {
        if (OutFailureReason)
        {
            *OutFailureReason = FText::GetEmpty();
        }
        return true;
    }

    const UObject* WorldContext = GetOwner() ? static_cast<const UObject*>(GetOwner())
                                             : static_cast<const UObject*>(this);

    if (OutFailureReason)
    {
        return USOTS_GAS_AbilityRequirementLibrary::EvaluateAbilityRequirementsWithReason(WorldContext, Requirements, *OutFailureReason);
    }

    return USOTS_GAS_AbilityRequirementLibrary::CanActivateAbilityWithRequirements(WorldContext, Requirements);
}

F_SOTS_AbilityStateSnapshot UAC_SOTS_Abilitys::BuildStateSnapshot(FGameplayTag AbilityTag) const
{
    F_SOTS_AbilityStateSnapshot Snapshot;
    Snapshot.AbilityTag = AbilityTag;

    const F_SOTS_AbilityRuntimeState* State = RuntimeStates.Find(AbilityTag);
    Snapshot.bIsActive      = State ? State->bIsActive : false;
    Snapshot.CurrentCharges = State ? State->CurrentCharges : 0;

    IsAbilityOnCooldown(AbilityTag, Snapshot.bIsOnCooldown, Snapshot.RemainingCooldown);

    return Snapshot;
}

FSOTS_AbilityRuntimeSnapshot UAC_SOTS_Abilitys::BuildRuntimeSnapshot() const
{
    FSOTS_AbilityRuntimeSnapshot Snapshot;
    Snapshot.SchemaVersion = 1;
    Snapshot.SavedAtTimeSeconds = GetWorldTime();

    TArray<FGameplayTag> SortedAbilityTags;
    RuntimeStates.GetKeys(SortedAbilityTags);
    SortedAbilityTags.Sort([](const FGameplayTag& A, const FGameplayTag& B){ return A.ToString() < B.ToString(); });

    for (const FGameplayTag& AbilityTag : SortedAbilityTags)
    {
        FSOTS_AbilityRuntimeEntry Entry;
        Entry.AbilityTag = AbilityTag;
        Entry.bGranted = true;

        const F_SOTS_AbilityRuntimeState* State = RuntimeStates.Find(AbilityTag);
        Entry.bWasActive = State ? State->bIsActive : false;
        Entry.CurrentCharges = State ? State->CurrentCharges : 0;

        bool bIsOnCooldown = false;
        float RemainingCooldown = 0.0f;
        IsAbilityOnCooldown(AbilityTag, bIsOnCooldown, RemainingCooldown);
        Entry.CooldownRemainingSeconds = bIsOnCooldown ? RemainingCooldown : 0.0f;

        Snapshot.Entries.Add(Entry);
    }

    return Snapshot;
}

FSOTS_AbilityRuntimeSnapshot UAC_SOTS_Abilitys::BuildRuntimeSnapshotFromLegacyArray(const TArray<F_SOTS_AbilityStateSnapshot>& LegacySnapshots) const
{
    FSOTS_AbilityRuntimeSnapshot Snapshot;
    Snapshot.SchemaVersion = 1;
    Snapshot.SavedAtTimeSeconds = GetWorldTime();

    for (const F_SOTS_AbilityStateSnapshot& Legacy : LegacySnapshots)
    {
        if (!Legacy.AbilityTag.IsValid())
        {
            continue;
        }

        FSOTS_AbilityRuntimeEntry Entry;
        Entry.AbilityTag = Legacy.AbilityTag;
        Entry.bGranted = true;
        Entry.bWasActive = Legacy.bIsActive;
        Entry.CurrentCharges = Legacy.CurrentCharges;
        Entry.CooldownRemainingSeconds = Legacy.bIsOnCooldown ? Legacy.RemainingCooldown : 0.0f;

        Snapshot.Entries.Add(Entry);
    }

    return Snapshot;
}

bool UAC_SOTS_Abilitys::ValidateAbilitySnapshot(const FSOTS_AbilityRuntimeSnapshot& Snapshot, TArray<FString>& OutWarnings, TArray<FString>& OutErrors) const
{
    if (Snapshot.SchemaVersion <= 0)
    {
        OutErrors.Add(TEXT("Snapshot schema version must be > 0."));
    }

    if (Snapshot.SchemaVersion > 1)
    {
        OutWarnings.Add(FString::Printf(TEXT("Snapshot schema version %d is newer than supported version 1. Attempting best-effort apply."), Snapshot.SchemaVersion));
    }

    TSet<FGameplayTag> SeenTags;
    for (const FSOTS_AbilityRuntimeEntry& Entry : Snapshot.Entries)
    {
        if (!Entry.AbilityTag.IsValid())
        {
            OutErrors.Add(TEXT("Snapshot entry has invalid ability tag."));
            continue;
        }

        if (SeenTags.Contains(Entry.AbilityTag))
        {
            OutWarnings.Add(FString::Printf(TEXT("Duplicate snapshot entry for %s. Last entry wins."), *Entry.AbilityTag.ToString()));
        }
        SeenTags.Add(Entry.AbilityTag);

        if (Entry.CurrentCharges < 0)
        {
            OutWarnings.Add(FString::Printf(TEXT("Snapshot charges clamped to 0 for %s (was %d)."), *Entry.AbilityTag.ToString(), Entry.CurrentCharges));
        }

        if (Entry.CooldownRemainingSeconds < 0.0f)
        {
            OutWarnings.Add(FString::Printf(TEXT("Snapshot cooldown clamped to 0 for %s (was %.2f)."), *Entry.AbilityTag.ToString(), Entry.CooldownRemainingSeconds));
        }
    }

    return OutErrors.IsEmpty();
}

FSOTS_AbilityApplyReport UAC_SOTS_Abilitys::ApplyRuntimeSnapshot(const FSOTS_AbilityRuntimeSnapshot& Snapshot)
{
    FSOTS_AbilityApplyReport Report;

    if (!GetRegistry())
    {
        Report.Result = ESOTS_AbilityApplyResult::DeferredRegistryNotReady;
        Report.DebugReason = TEXT("Ability registry not ready");
        return Report;
    }

    TArray<FString> Warnings;
    TArray<FString> Errors;
    if (bValidateAbilitySnapshotOnLoad && !ValidateAbilitySnapshot(Snapshot, Warnings, Errors))
    {
        Report.Result = ESOTS_AbilityApplyResult::ValidationFailed;
        Report.SkippedInvalidEntries = Errors.Num();
        Report.DebugReason = Errors.Num() > 0 ? Errors[0] : TEXT("Snapshot validation failed");
        LogSnapshotMessages(TEXT("Apply"), Warnings, Errors);
        return Report;
    }

    LogSnapshotMessages(TEXT("Apply"), Warnings, Errors);

    TMap<FGameplayTag, FSOTS_AbilityRuntimeEntry> DedupedEntries;
    for (const FSOTS_AbilityRuntimeEntry& Entry : Snapshot.Entries)
    {
        if (!Entry.AbilityTag.IsValid())
        {
            Report.SkippedInvalidEntries++;
            continue;
        }

        DedupedEntries.Add(Entry.AbilityTag, Entry); // last-wins determinism
    }

    TArray<FGameplayTag> SortedTags;
    DedupedEntries.GetKeys(SortedTags);
    SortedTags.Sort([](const FGameplayTag& A, const FGameplayTag& B){ return A.ToString() < B.ToString(); });

    RuntimeStates.Reset();

    const float WorldTime = GetWorldTime();

    for (const FGameplayTag& AbilityTag : SortedTags)
    {
        const FSOTS_AbilityRuntimeEntry& Entry = DedupedEntries[AbilityTag];

        if (!Entry.bGranted)
        {
            continue;
        }

        F_SOTS_AbilityDefinition Def;
        if (!InternalGetDefinition(AbilityTag, Def))
        {
            Report.SkippedMissingDefs++;
            continue;
        }

        F_SOTS_AbilityRuntimeState& State = RuntimeStates.FindOrAdd(AbilityTag);
        if (!State.Handle.bIsValid)
        {
            State.Handle.AbilityTag = AbilityTag;
            State.Handle.InternalId = NextInternalHandleId++;
            State.Handle.bIsValid   = true;
        }

        State.bIsActive = false; // clear mid-activation
        State.ActivationId.Invalidate();

        const int32 MaxCharges = (Def.ChargeMode == E_SOTS_AbilityChargeMode::InventoryLinked) ? INT32_MAX : Def.MaxCharges;
        const int32 ClampedCharges = FMath::Clamp(Entry.CurrentCharges, 0, MaxCharges > 0 ? MaxCharges : INT32_MAX);
        State.CurrentCharges = ClampedCharges;

        const float ClampedCooldown = FMath::Max(0.0f, Entry.CooldownRemainingSeconds);
        State.CooldownEndTime = ClampedCooldown > 0.0f ? WorldTime + ClampedCooldown : 0.0f;

        Report.AppliedCount++;
        BroadcastAbilityStateChanged(AbilityTag);
    }

    BroadcastAbilityListChanged();

    if (Report.AppliedCount == 0 && Report.SkippedMissingDefs == 0 && Report.SkippedInvalidEntries == 0)
    {
        Report.Result = ESOTS_AbilityApplyResult::NoOp;
    }
    else if (Report.SkippedMissingDefs > 0 || Report.SkippedInvalidEntries > 0)
    {
        Report.Result = ESOTS_AbilityApplyResult::PartialApplied;
    }
    else
    {
        Report.Result = ESOTS_AbilityApplyResult::Applied;
    }

    return Report;
}

void UAC_SOTS_Abilitys::LogSnapshotMessages(const TCHAR* Prefix, const TArray<FString>& Warnings, const TArray<FString>& Errors) const
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (!bDebugLogAbilitySnapshot)
    {
        return;
    }

    for (const FString& Warning : Warnings)
    {
        UE_LOG(LogSOTSGAS, Warning, TEXT("[AbilityComponent][%s] %s"), Prefix, *Warning);
    }

    for (const FString& Error : Errors)
    {
        UE_LOG(LogSOTSGAS, Error, TEXT("[AbilityComponent][%s] %s"), Prefix, *Error);
    }
#endif
}

void UAC_SOTS_Abilitys::BroadcastAbilityListChanged()
{
    OnAbilitiesChanged.Broadcast(this);
}

void UAC_SOTS_Abilitys::BroadcastAbilityStateChanged(FGameplayTag AbilityTag)
{
    F_SOTS_AbilityStateSnapshot Snapshot = BuildStateSnapshot(AbilityTag);
    OnAbilityStateChanged.Broadcast(AbilityTag, Snapshot);
}

void UAC_SOTS_Abilitys::BroadcastActivationStarted(const FSOTS_AbilityActivateReport& Report)
{
    OnSOTSAbilityActivationStarted.Broadcast(Report);
}

void UAC_SOTS_Abilitys::BroadcastActivationFailed(const FSOTS_AbilityActivateReport& Report)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bDebugLogAbilityActivationFailures)
    {
        UE_LOG(LogSOTSGAS, Warning, TEXT("[AbilityComponent] Activation failed %s (%s)"),
               *Report.AbilityTag.ToString(), *Report.DebugReason);
    }
#endif

    OnSOTSAbilityActivationFailed.Broadcast(Report);

    const E_SOTS_AbilityActivationResult LegacyResult = MapReportToLegacy(Report);
    OnAbilityFailed.Broadcast(Report.AbilityTag, LegacyResult);
    SOTSAbilityUIHelpers::NotifyAbilityEvent(this, Report.AbilityTag, /*bActivated*/ false, LegacyResult);
}

void UAC_SOTS_Abilitys::BroadcastActivationEnded(FGameplayTag AbilityTag, const FGuid& ActivationId, E_SOTS_AbilityEndReason EndReason)
{
    OnSOTSAbilityActivationEnded.Broadcast(AbilityTag, ActivationId, EndReason);
}

void UAC_SOTS_Abilitys::PushProfileStateToSubsystem()
{
    PushProfileStateToSubsystem_WithResult();
}

bool UAC_SOTS_Abilitys::PushProfileStateToSubsystem_WithResult()
{
    if (!AbilitySubsystem)
    {
        AbilitySubsystem = USOTS_AbilitySubsystem::Get(this);
    }

    if (!AbilitySubsystem)
    {
        return false;
    }

    FSOTS_AbilityProfileData Data;
    GetKnownAbilities(Data.GrantedAbilityTags);
    AbilitySubsystem->ApplyProfileData(Data);
    return true;
}

void UAC_SOTS_Abilitys::PullProfileStateFromSubsystem()
{
    PullProfileStateFromSubsystem_WithResult();
}

bool UAC_SOTS_Abilitys::PullProfileStateFromSubsystem_WithResult()
{
    if (!AbilitySubsystem)
    {
        AbilitySubsystem = USOTS_AbilitySubsystem::Get(this);
    }

    if (!AbilitySubsystem)
    {
        return false;
    }

    FSOTS_AbilityProfileData Data;
    AbilitySubsystem->BuildProfileData(Data);

    bool bAllGranted = true;
    for (const FGameplayTag& AbilityTag : Data.GrantedAbilityTags)
    {
        F_SOTS_AbilityHandle Handle;
        if (!GrantAbility(AbilityTag, F_SOTS_AbilityGrantOptions(), Handle))
        {
            bAllGranted = false;
        }
    }

    return bAllGranted;
}
