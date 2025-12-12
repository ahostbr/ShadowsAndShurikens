#include "Components/SOTS_AbilityComponent.h"

#include "Abilities/SOTS_AbilityBase.h"
#include "Subsystems/SOTS_AbilityRegistrySubsystem.h"
#include "Subsystems/SOTS_AbilitySubsystem.h"
#include "SOTS_GAS_AbilityRequirementLibrary.h"
#include "SOTS_GAS_Plugin.h"
#include "SOTS_FXManagerSubsystem.h"
#include "SOTS_UIAbilityLibrary.h"
#include "SOTS_InventoryBridgeSubsystem.h"
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

UAC_SOTS_Abilitys::UAC_SOTS_Abilitys()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UAC_SOTS_Abilitys::BeginPlay()
{
    Super::BeginPlay();

    // Attempt to locate the InvSP inventory component (BP_InventoryComponent) on the owning actor.
    AActor* OwnerActor = GetOwner();
    if (OwnerActor)
    {
        TArray<UActorComponent*> Components;
        OwnerActor->GetComponents(Components);

        for (UActorComponent* Component : Components)
        {
            if (!Component)
            {
                continue;
            }

            const FString ClassName = Component->GetClass()->GetName();
            if (ClassName.Contains(TEXT("BP_InventoryComponent")))
            {
                InventoryComponent = Component;
                break;
            }
        }
    }

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

bool UAC_SOTS_Abilitys::PassesOwnerTagGate(const F_SOTS_AbilityDefinition& /*Def*/) const
{
    // Hook into your global GameplayTag manager here if desired.
    return true;
}

bool UAC_SOTS_Abilitys::PassesSkillGate(const F_SOTS_AbilityDefinition& Def) const
{
    if (Def.SkillGateMode == E_SOTS_AbilitySkillGateMode::None || Def.RequiredSkillTags.Num() == 0)
    {
        return true;
    }

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return false;
    }

    if (!OwnerActor->GetClass()->ImplementsInterface(UBPI_SOTS_SkillTreeAccess::StaticClass()))
    {
        return false;
    }

    for (const FGameplayTag& SkillTag : Def.RequiredSkillTags)
    {
        const bool bUnlocked = IBPI_SOTS_SkillTreeAccess::Execute_IsSkillUnlocked(OwnerActor, SkillTag);
        if (!bUnlocked)
        {
            return false;
        }
    }

    return true;
}

int32 UAC_SOTS_Abilitys::QueryInventoryItemCount(const FGameplayTagContainer& Tags) const
{
    if (USOTS_InventoryBridgeSubsystem* Bridge = USOTS_InventoryBridgeSubsystem::Get(this))
    {
        return Bridge->GetCarriedItemCountByTags(Tags);
    }

    // Preferred path: let the owning actor implement the inventory interface.
    if (AActor* OwnerActor = GetOwner())
    {
        if (OwnerActor->GetClass()->ImplementsInterface(UBPI_SOTS_InventoryAccess::StaticClass()))
        {
            return IBPI_SOTS_InventoryAccess::Execute_GetInventoryItemCountByTags(OwnerActor, Tags);
        }
    }

    if (!InventoryComponent)
    {
        return 0;
    }

    // The owning project must implement a Blueprint function on the inventory component with the signature:
    //   Function GetTotalAmountByGameplayTag(ItemTag: GameplayTag) -> TotalAmount: int32
    static const FName FuncName_GetTotalAmountByGameplayTag(TEXT("GetTotalAmountByGameplayTag"));
    UFunction* GetTotalFunction = InventoryComponent->FindFunction(FuncName_GetTotalAmountByGameplayTag);
    if (!GetTotalFunction)
    {
        return 0;
    }

    int32 TotalCount = 0;

    TArray<FGameplayTag> TagArray;
    Tags.GetGameplayTagArray(TagArray);

    for (const FGameplayTag& Tag : TagArray)
    {
        struct FGetTotalAmountByGameplayTag_Params
        {
            FGameplayTag ItemTag;
            int32 TotalAmount;
        };

        FGetTotalAmountByGameplayTag_Params Params;
        Params.ItemTag = Tag;
        Params.TotalAmount = 0;

        InventoryComponent->ProcessEvent(GetTotalFunction, &Params);
        TotalCount += Params.TotalAmount;
    }

    return TotalCount;
}

bool UAC_SOTS_Abilitys::ConsumeInventoryItems(const FGameplayTagContainer& Tags, int32 Count) const
{
    if (USOTS_InventoryBridgeSubsystem* Bridge = USOTS_InventoryBridgeSubsystem::Get(this))
    {
        return Bridge->ConsumeCarriedItemsByTags(Tags, Count);
    }

    // Preferred path: let the owning actor implement the inventory interface.
    if (AActor* OwnerActor = GetOwner())
    {
        if (OwnerActor->GetClass()->ImplementsInterface(UBPI_SOTS_InventoryAccess::StaticClass()))
        {
            return IBPI_SOTS_InventoryAccess::Execute_ConsumeInventoryItemsByTags(OwnerActor, Tags, Count);
        }
    }

    if (!InventoryComponent || Count <= 0)
    {
        return false;
    }

    // The owning project must implement a Blueprint function on the inventory component with the signature:
    //   Function RemoveItemsByGameplayTag(ItemTag: GameplayTag, AmountToRemove: int32) -> RemovedAmount: int32, bSuccess: bool
    static const FName FuncName_RemoveItemsByGameplayTag(TEXT("RemoveItemsByGameplayTag"));
    UFunction* RemoveFunction = InventoryComponent->FindFunction(FuncName_RemoveItemsByGameplayTag);
    if (!RemoveFunction)
    {
        return false;
    }

    static const FName FuncName_GetTotalAmountByGameplayTag(TEXT("GetTotalAmountByGameplayTag"));
    UFunction* GetTotalFunction = InventoryComponent->FindFunction(FuncName_GetTotalAmountByGameplayTag);

    int32 RemainingToConsume = Count;

    TArray<FGameplayTag> TagArray;
    Tags.GetGameplayTagArray(TagArray);

    for (const FGameplayTag& Tag : TagArray)
    {
        if (RemainingToConsume <= 0)
        {
            break;
        }

        int32 AvailableForTag = 0;

        if (GetTotalFunction)
        {
            struct FGetTotalAmountByGameplayTag_Params
            {
                FGameplayTag ItemTag;
                int32 TotalAmount;
            };

            FGetTotalAmountByGameplayTag_Params GetParams;
            GetParams.ItemTag = Tag;
            GetParams.TotalAmount = 0;
            InventoryComponent->ProcessEvent(GetTotalFunction, &GetParams);
            AvailableForTag = GetParams.TotalAmount;
        }

        if (AvailableForTag <= 0)
        {
            continue;
        }

        const int32 ToRemoveForTag = FMath::Min(AvailableForTag, RemainingToConsume);

        struct FRemoveItemsByGameplayTag_Params
        {
            FGameplayTag ItemTag;
            int32 AmountToRemove;
            int32 RemovedAmount;
            bool bSuccess;
        };

        FRemoveItemsByGameplayTag_Params Params;
        Params.ItemTag = Tag;
        Params.AmountToRemove = ToRemoveForTag;
        Params.RemovedAmount = 0;
        Params.bSuccess = false;

        InventoryComponent->ProcessEvent(RemoveFunction, &Params);

        if (Params.bSuccess && Params.RemovedAmount > 0)
        {
            RemainingToConsume -= Params.RemovedAmount;
        }
    }

    return RemainingToConsume <= 0;
}

bool UAC_SOTS_Abilitys::PassesInventoryGate(const F_SOTS_AbilityDefinition& Def) const
{
    if (Def.InventoryMode == E_SOTS_AbilityInventoryMode::None)
    {
        return true;
    }

    if (Def.RequiredInventoryTags.Num() == 0)
    {
        return true;
    }

    const int32 Count = QueryInventoryItemCount(Def.RequiredInventoryTags);
    return Count > 0;
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
            const int32 Count = QueryInventoryItemCount(Def.RequiredInventoryTags);
            return Count > 0;
        }

        case E_SOTS_AbilityChargeMode::Hybrid:
        {
            const int32 InventoryCount = QueryInventoryItemCount(Def.RequiredInventoryTags);

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
        {
            if (Def.InventoryMode == E_SOTS_AbilityInventoryMode::RequireAndConsume)
            {
                ConsumeInventoryItems(Def.RequiredInventoryTags, 1);
            }
            break;
        }

        case E_SOTS_AbilityChargeMode::Hybrid:
        {
            // Finite internal + inventory
            if (Def.MaxCharges > 0 && State->CurrentCharges > 0)
            {
                --State->CurrentCharges;
            }

            if (Def.InventoryMode == E_SOTS_AbilityInventoryMode::RequireAndConsume)
            {
                ConsumeInventoryItems(Def.RequiredInventoryTags, 1);
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

    // Skill-gate on grant if required
    if (Def.SkillGateMode == E_SOTS_AbilitySkillGateMode::RequireForGrant ||
        Def.SkillGateMode == E_SOTS_AbilitySkillGateMode::RequireForBoth)
    {
        if (!PassesSkillGate(Def))
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

    UE_LOG(LogSOTSGAS, Log,
           TEXT("[AbilityComponent] Granted ability '%s' to '%s' (HandleId=%d)."),
           *AbilityTag.ToString(),
           *GetOwner()->GetName(),
           State.Handle.InternalId);

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
        UE_LOG(LogSOTSGAS, Log,
               TEXT("[AbilityComponent] Revoked ability '%s' from '%s'."),
               *AbilityTag.ToString(),
               *GetOwner()->GetName());

        BroadcastAbilityListChanged();
        BroadcastAbilityStateChanged(AbilityTag);
    }

    return RemovedCount > 0;
}

bool UAC_SOTS_Abilitys::TryActivateAbilityByTag(FGameplayTag AbilityTag, const F_SOTS_AbilityActivationContext& Context, E_SOTS_AbilityActivationResult& OutResult)
{
    OutResult = E_SOTS_AbilityActivationResult::Failed_CustomCondition;

    F_SOTS_AbilityDefinition Def;
    if (!InternalGetDefinition(AbilityTag, Def))
    {
        return false;
    }

    // Optional authored requirements on the definition (skill tags, player
    // tags, stealth state). These are evaluated via the shared requirement
    // library and are kept data-only so they can be reused by other systems.
    if (!EvaluateAbilityRequirementsForDefinition(Def, nullptr))
    {
        OutResult = E_SOTS_AbilityActivationResult::Failed_CustomCondition;
        OnAbilityFailed.Broadcast(AbilityTag, OutResult);

        UE_LOG(LogSOTSGAS, Log,
               TEXT("[AbilityComponent] Activation of '%s' on '%s' failed ability requirements."),
               *AbilityTag.ToString(),
               *GetOwner()->GetName());

        if (Def.FXTag_OnFailRequirements.IsValid())
        {
            if (USOTS_FXManagerSubsystem* FX = USOTS_FXManagerSubsystem::Get())
            {
                AActor* OwnerActor = GetOwner();
                AActor* InstigatorActor = Context.Instigator ? Context.Instigator : OwnerActor;
                AActor* TargetActor = Context.TargetActor;

                FVector Location = Context.TargetLocation;
                if (Location.IsNearlyZero())
                {
                    if (TargetActor)
                    {
                        Location = TargetActor->GetActorLocation();
                    }
                    else if (InstigatorActor)
                    {
                        Location = InstigatorActor->GetActorLocation();
                    }
                }

                const FRotator Rotation = InstigatorActor ? InstigatorActor->GetActorRotation() : FRotator::ZeroRotator;

                FX->TriggerFXByTag(OwnerActor, Def.FXTag_OnFailRequirements, InstigatorActor, TargetActor, Location, Rotation);
            }
        }

        SOTSAbilityUIHelpers::NotifyAbilityEvent(this, AbilityTag, false, OutResult);

        return false;
    }

    F_SOTS_AbilityRuntimeState& State = RuntimeStates.FindOrAdd(AbilityTag);
    if (!State.Handle.bIsValid)
    {
        State.Handle.AbilityTag = AbilityTag;
        State.Handle.InternalId = NextInternalHandleId++;
        State.Handle.bIsValid = true;
    }

    const float WorldTime = GetWorldTime();
        if (State.CooldownEndTime > 0.0f && WorldTime < State.CooldownEndTime)
        {
            OutResult = E_SOTS_AbilityActivationResult::Failed_Cooldown;
            OnAbilityFailed.Broadcast(AbilityTag, OutResult);

            UE_LOG(LogSOTSGAS, Log,
                   TEXT("[AbilityComponent] Activation of '%s' on '%s' failed: cooldown."),
                   *AbilityTag.ToString(),
                   *GetOwner()->GetName());
            SOTSAbilityUIHelpers::NotifyAbilityEvent(this, AbilityTag, false, OutResult);
            return false;
        }

        if (!HasSufficientCharges(Def))
        {
            OutResult = E_SOTS_AbilityActivationResult::Failed_ChgDepleted;
            OnAbilityFailed.Broadcast(AbilityTag, OutResult);

            UE_LOG(LogSOTSGAS, Log,
                   TEXT("[AbilityComponent] Activation of '%s' on '%s' failed: charges depleted."),
                   *AbilityTag.ToString(),
                   *GetOwner()->GetName());
            SOTSAbilityUIHelpers::NotifyAbilityEvent(this, AbilityTag, false, OutResult);
            return false;
        }

        if (!PassesOwnerTagGate(Def))
        {
            OutResult = E_SOTS_AbilityActivationResult::Failed_OwnerTags;
            OnAbilityFailed.Broadcast(AbilityTag, OutResult);

            UE_LOG(LogSOTSGAS, Log,
                   TEXT("[AbilityComponent] Activation of '%s' on '%s' failed: owner tag gate."),
                   *AbilityTag.ToString(),
                   *GetOwner()->GetName());
            SOTSAbilityUIHelpers::NotifyAbilityEvent(this, AbilityTag, false, OutResult);
            return false;
        }

    if (Def.SkillGateMode == E_SOTS_AbilitySkillGateMode::RequireForActivate ||
        Def.SkillGateMode == E_SOTS_AbilitySkillGateMode::RequireForBoth)
    {
        if (!PassesSkillGate(Def))
        {
            OutResult = E_SOTS_AbilityActivationResult::Failed_SkillGate;
            OnAbilityFailed.Broadcast(AbilityTag, OutResult);

            UE_LOG(LogSOTSGAS, Log,
                   TEXT("[AbilityComponent] Activation of '%s' on '%s' failed: skill gate."),
                   *AbilityTag.ToString(),
                   *GetOwner()->GetName());
            SOTSAbilityUIHelpers::NotifyAbilityEvent(this, AbilityTag, false, OutResult);
            return false;
        }
    }

    if (!PassesInventoryGate(Def))
    {
        OutResult = E_SOTS_AbilityActivationResult::Failed_InventoryGate;
        OnAbilityFailed.Broadcast(AbilityTag, OutResult);

        UE_LOG(LogSOTSGAS, Log,
               TEXT("[AbilityComponent] Activation of '%s' on '%s' failed: inventory gate."),
               *AbilityTag.ToString(),
               *GetOwner()->GetName());
        return false;
    }

    ConsumeChargesOnActivation(Def);

    State.bIsActive = true;
    OnAbilityActivated.Broadcast(AbilityTag, State.Handle);
    OnAbilityActivatedWithContext.Broadcast(AbilityTag, State.Handle, Context);

    if (USOTS_AbilityBase** FoundAbility = AbilityInstances.Find(AbilityTag))
    {
        if (USOTS_AbilityBase* Ability = *FoundAbility)
        {
            Ability->K2_ActivateAbility(Context);
        }
    }

    UE_LOG(LogSOTSGAS, Log,
           TEXT("[AbilityComponent] Activation of '%s' on '%s' succeeded."),
           *AbilityTag.ToString(),
           *GetOwner()->GetName());

        if (Def.FXTag_OnActivate.IsValid())
        {
            if (USOTS_FXManagerSubsystem* FX = USOTS_FXManagerSubsystem::Get())
            {
            AActor* OwnerActor = GetOwner();
            AActor* InstigatorActor = Context.Instigator ? Context.Instigator : OwnerActor;
            AActor* TargetActor = Context.TargetActor;

            FVector Location = Context.TargetLocation;
            if (Location.IsNearlyZero())
            {
                if (TargetActor)
                {
                    Location = TargetActor->GetActorLocation();
                }
                else if (InstigatorActor)
                {
                    Location = InstigatorActor->GetActorLocation();
                }
            }

            const FRotator Rotation = InstigatorActor ? InstigatorActor->GetActorRotation() : FRotator::ZeroRotator;

                FX->TriggerFXByTag(OwnerActor, Def.FXTag_OnActivate, InstigatorActor, TargetActor, Location, Rotation);
            }
        }

        SOTSAbilityUIHelpers::NotifyAbilityEvent(this, AbilityTag, true, E_SOTS_AbilityActivationResult::Success);

        BroadcastAbilityStateChanged(AbilityTag);

    OutResult = E_SOTS_AbilityActivationResult::Success;
    return true;
}

void UAC_SOTS_Abilitys::CancelAllAbilities()
{
    for (auto& Kvp : RuntimeStates)
    {
        const FGameplayTag& AbilityTag = Kvp.Key;
        F_SOTS_AbilityRuntimeState& State = Kvp.Value;

        if (State.bIsActive)
        {
            State.bIsActive = false;

            if (USOTS_AbilityBase** FoundAbility = AbilityInstances.Find(AbilityTag))
            {
                if (USOTS_AbilityBase* Ability = *FoundAbility)
                {
                    Ability->K2_EndAbility();
                }
            }

            OnAbilityEnded.Broadcast(AbilityTag, State.Handle);
            BroadcastAbilityStateChanged(AbilityTag);
        }
    }
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
            const int32 Count = QueryInventoryItemCount(Def.RequiredInventoryTags);
            OutCurrentCharges = Count;
            OutMaxCharges     = Count;
            break;
        }

        case E_SOTS_AbilityChargeMode::Hybrid:
        {
            const int32 Count    = QueryInventoryItemCount(Def.RequiredInventoryTags);
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

    if (!AbilityTag.IsValid())
    {
        OutFailureReason = FText::FromString(TEXT("Invalid ability tag."));
        return false;
    }

    F_SOTS_AbilityDefinition Def;
    if (!const_cast<UAC_SOTS_Abilitys*>(this)->InternalGetDefinition(AbilityTag, Def))
    {
        OutFailureReason = FText::FromString(TEXT("Ability definition not found."));
        return false;
    }

    // Requirements (skill / global tags / stealth).
    if (!EvaluateAbilityRequirementsForDefinition(Def, &OutFailureReason))
    {
        return false;
    }

    // Cooldown.
    bool bIsOnCooldown = false;
    float RemainingTime = 0.0f;
    IsAbilityOnCooldown(AbilityTag, bIsOnCooldown, RemainingTime);
    if (bIsOnCooldown)
    {
        OutFailureReason = FText::FromString(TEXT("Ability is on cooldown."));
        return false;
    }

    // Charges / inventory gates.
    if (!HasSufficientCharges(Def))
    {
        OutFailureReason = FText::FromString(TEXT("No charges remaining."));
        return false;
    }

    if (!PassesInventoryGate(Def))
    {
        OutFailureReason = FText::FromString(TEXT("Required inventory items missing."));
        return false;
    }

    if (!PassesOwnerTagGate(Def))
    {
        OutFailureReason = FText::FromString(TEXT("Ability blocked by owner tags."));
        return false;
    }

    if (Def.SkillGateMode == E_SOTS_AbilitySkillGateMode::RequireForActivate ||
        Def.SkillGateMode == E_SOTS_AbilitySkillGateMode::RequireForBoth)
    {
        if (!PassesSkillGate(Def))
        {
            OutFailureReason = FText::FromString(TEXT("Required skills not unlocked."));
            return false;
        }
    }

    return true;
}

bool UAC_SOTS_Abilitys::ActivateAbility(FGameplayTag AbilityTag, const F_SOTS_AbilityActivationContext& Context, FText& OutFailureReason)
{
    OutFailureReason = FText::GetEmpty();

    if (!CanActivateAbility(AbilityTag, OutFailureReason))
    {
        return false;
    }

    E_SOTS_AbilityActivationResult Result = E_SOTS_AbilityActivationResult::Failed_CustomCondition;
    if (!TryActivateAbilityByTag(AbilityTag, Context, Result))
    {
        if (Result == E_SOTS_AbilityActivationResult::Failed_Cooldown)
        {
            OutFailureReason = FText::FromString(TEXT("Ability is on cooldown."));
        }
        else if (Result == E_SOTS_AbilityActivationResult::Failed_ChgDepleted)
        {
            OutFailureReason = FText::FromString(TEXT("No charges remaining."));
        }
        else if (Result == E_SOTS_AbilityActivationResult::Failed_InventoryGate)
        {
            OutFailureReason = FText::FromString(TEXT("Required inventory items missing."));
        }
        else if (Result == E_SOTS_AbilityActivationResult::Failed_OwnerTags)
        {
            OutFailureReason = FText::FromString(TEXT("Ability blocked by owner tags."));
        }
        else if (Result == E_SOTS_AbilityActivationResult::Failed_SkillGate)
        {
            OutFailureReason = FText::FromString(TEXT("Required skills not unlocked."));
        }

        return false;
    }

    return true;
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
    OutData.SavedAbilities.Reset();

    for (const TPair<FGameplayTag, F_SOTS_AbilityRuntimeState>& Pair : RuntimeStates)
    {
        const FGameplayTag& AbilityTag = Pair.Key;
        F_SOTS_AbilityStateSnapshot Snapshot = BuildStateSnapshot(AbilityTag);
        OutData.SavedAbilities.Add(Snapshot);
    }
}

void UAC_SOTS_Abilitys::ApplySerializedState(const F_SOTS_AbilityComponentSaveData& InData)
{
    RuntimeStates.Reset();

    for (const F_SOTS_AbilityStateSnapshot& Snapshot : InData.SavedAbilities)
    {
        if (!Snapshot.AbilityTag.IsValid())
        {
            continue;
        }

        F_SOTS_AbilityDefinition Def;
        if (!InternalGetDefinition(Snapshot.AbilityTag, Def))
        {
            continue;
        }

        F_SOTS_AbilityRuntimeState& State = RuntimeStates.FindOrAdd(Snapshot.AbilityTag);
        if (!State.Handle.bIsValid)
        {
            State.Handle.AbilityTag = Snapshot.AbilityTag;
            State.Handle.InternalId = NextInternalHandleId++;
            State.Handle.bIsValid   = true;
        }

        State.bIsActive      = Snapshot.bIsActive;
        State.CurrentCharges = Snapshot.CurrentCharges;

        const float WorldTime = GetWorldTime();
        State.CooldownEndTime = (Snapshot.bIsOnCooldown && Snapshot.RemainingCooldown > 0.0f)
                                ? WorldTime + Snapshot.RemainingCooldown
                                : 0.0f;

        BroadcastAbilityStateChanged(Snapshot.AbilityTag);
    }

    BroadcastAbilityListChanged();
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

void UAC_SOTS_Abilitys::BroadcastAbilityListChanged()
{
    OnAbilitiesChanged.Broadcast(this);
}

void UAC_SOTS_Abilitys::BroadcastAbilityStateChanged(FGameplayTag AbilityTag)
{
    F_SOTS_AbilityStateSnapshot Snapshot = BuildStateSnapshot(AbilityTag);
    OnAbilityStateChanged.Broadcast(AbilityTag, Snapshot);
}

void UAC_SOTS_Abilitys::PushProfileStateToSubsystem()
{
    if (!AbilitySubsystem)
    {
        AbilitySubsystem = USOTS_AbilitySubsystem::Get(this);
    }

    if (!AbilitySubsystem)
    {
        return;
    }

    FSOTS_AbilityProfileData Data;
    GetKnownAbilities(Data.GrantedAbilityTags);
    AbilitySubsystem->ApplyProfileData(Data);
}

void UAC_SOTS_Abilitys::PullProfileStateFromSubsystem()
{
    if (!AbilitySubsystem)
    {
        AbilitySubsystem = USOTS_AbilitySubsystem::Get(this);
    }

    if (!AbilitySubsystem)
    {
        return;
    }

    FSOTS_AbilityProfileData Data;
    AbilitySubsystem->BuildProfileData(Data);

    for (const FGameplayTag& AbilityTag : Data.GrantedAbilityTags)
    {
        F_SOTS_AbilityHandle Handle;
        GrantAbility(AbilityTag, F_SOTS_AbilityGrantOptions(), Handle);
    }
}
