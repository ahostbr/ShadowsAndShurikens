#include "Subsystems/SOTS_AbilityRegistrySubsystem.h"
#include "SOTS_GAS_Plugin.h"

void USOTS_AbilityRegistrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    AbilityDefinitions.Reset();
    AbilityDefinitionAssets.Reset();

    bRegistryReady = false;
    BuildRegistryFromConfig();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bValidateAbilityRegistryOnInit)
    {
        ValidateRegistry();
    }
#endif

    bRegistryReady = true;
    OnAbilityRegistryReady.Broadcast();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bLogRegistryLifecycle)
    {
        UE_LOG(LogSOTSGAS, Log, TEXT("[AbilityRegistry] Initialized."));
    }
#endif
}

void USOTS_AbilityRegistrySubsystem::Deinitialize()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bLogRegistryLifecycle)
    {
        UE_LOG(LogSOTSGAS, Log, TEXT("[AbilityRegistry] Deinitialize. Clearing %d definitions."), AbilityDefinitions.Num());
    }
#endif

    AbilityDefinitions.Reset();
    AbilityDefinitionAssets.Reset();
    bRegistryReady = false;

    Super::Deinitialize();
}

void USOTS_AbilityRegistrySubsystem::RegisterAbilityDefinition(const F_SOTS_AbilityDefinition& Definition)
{
    RegisterAbilityDefinition_WithResult(Definition);
}

void USOTS_AbilityRegistrySubsystem::RegisterAbilityDefinitionDA(USOTS_AbilityDefinitionDA* DefinitionDA)
{
    RegisterAbilityDefinitionDA_WithResult(DefinitionDA);
}

void USOTS_AbilityRegistrySubsystem::RegisterAbilityDefinitionsFromArray(const TArray<USOTS_AbilityDefinitionDA*>& Definitions)
{
    RegisterAbilityDefinitionsFromArray_WithResult(Definitions);
}

void USOTS_AbilityRegistrySubsystem::RegisterAbilityDefinitionsFromDataAsset(USOTS_AbilityDefinitionLibrary* Library)
{
    RegisterAbilityDefinitionsFromDataAsset_WithResult(Library);
}

bool USOTS_AbilityRegistrySubsystem::RegisterAbilityDefinition_WithResult(const F_SOTS_AbilityDefinition& Definition)
{
    if (!Definition.AbilityTag.IsValid())
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bWarnOnInvalidAbilityDefs)
        {
            UE_LOG(LogSOTSGAS, Warning, TEXT("[AbilityRegistry] Skipping invalid ability definition with missing tag."));
        }
#endif
        return false;
    }

    if (AbilityDefinitions.Contains(Definition.AbilityTag))
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bWarnOnDuplicateAbilityTags)
        {
            UE_LOG(LogSOTSGAS, Warning, TEXT("[AbilityRegistry] Duplicate ability tag %s encountered; first definition kept."), *Definition.AbilityTag.ToString());
        }
#endif
        return false;
    }

    AbilityDefinitions.Add(Definition.AbilityTag, Definition);
    return true;
}

bool USOTS_AbilityRegistrySubsystem::RegisterAbilityDefinitionDA_WithResult(USOTS_AbilityDefinitionDA* DefinitionDA)
{
    if (!DefinitionDA)
    {
        return false;
    }

    const F_SOTS_AbilityDefinition& Def = DefinitionDA->Ability;
    if (!Def.AbilityTag.IsValid())
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bWarnOnInvalidAbilityDefs)
        {
            UE_LOG(LogSOTSGAS, Warning, TEXT("[AbilityRegistry] Definition asset %s has invalid AbilityTag; skipped."), *DefinitionDA->GetName());
        }
#endif
        return false;
    }

    if (AbilityDefinitions.Contains(Def.AbilityTag))
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bWarnOnDuplicateAbilityTags)
        {
            UE_LOG(LogSOTSGAS, Warning, TEXT("[AbilityRegistry] Duplicate ability tag %s in asset %s; first definition kept."), *Def.AbilityTag.ToString(), *DefinitionDA->GetName());
        }
#endif
        return false;
    }

    AbilityDefinitions.Add(Def.AbilityTag, Def);
    AbilityDefinitionAssets.FindOrAdd(Def.AbilityTag) = DefinitionDA;
    return true;
}

int32 USOTS_AbilityRegistrySubsystem::RegisterAbilityDefinitionsFromArray_WithResult(const TArray<USOTS_AbilityDefinitionDA*>& Definitions)
{
    int32 AddedCount = 0;
    for (USOTS_AbilityDefinitionDA* DA : Definitions)
    {
        if (RegisterAbilityDefinitionDA_WithResult(DA))
        {
            ++AddedCount;
        }
    }
    return AddedCount;
}

int32 USOTS_AbilityRegistrySubsystem::RegisterAbilityDefinitionsFromDataAsset_WithResult(USOTS_AbilityDefinitionLibrary* Library)
{
    if (!Library)
    {
        return 0;
    }

    return RegisterAbilityDefinitionsFromArray_WithResult(Library->Definitions);
}

bool USOTS_AbilityRegistrySubsystem::GetAbilityDefinitionByTag(FGameplayTag AbilityTag, F_SOTS_AbilityDefinition& OutDefinition) const
{
    if (!IsRegistryReadyChecked())
    {
        return false;
    }

    if (const F_SOTS_AbilityDefinition* Found = AbilityDefinitions.Find(AbilityTag))
    {
        OutDefinition = *Found;
        return true;
    }

    MaybeWarnMissingAbilityTag(AbilityTag);
    return false;
}

bool USOTS_AbilityRegistrySubsystem::GetAbilityDefinitionDAByTag(FGameplayTag AbilityTag, USOTS_AbilityDefinitionDA*& OutDefinitionDA) const
{
    if (!IsRegistryReadyChecked())
    {
        return false;
    }

    if (USOTS_AbilityDefinitionDA* const* Found = AbilityDefinitionAssets.Find(AbilityTag))
    {
        OutDefinitionDA = *Found;
        return true;
    }

    MaybeWarnMissingAbilityTag(AbilityTag);
    return false;
}

void USOTS_AbilityRegistrySubsystem::GetAllAbilityDefinitions(TArray<F_SOTS_AbilityDefinition>& OutDefinitions) const
{
    if (!IsRegistryReadyChecked())
    {
        OutDefinitions.Reset();
        return;
    }

    OutDefinitions.Reset();
    for (const auto& Kvp : AbilityDefinitions)
    {
        OutDefinitions.Add(Kvp.Value);
    }
}

bool USOTS_AbilityRegistrySubsystem::EvaluateAbilityRequirementsForTag(
    const UObject* WorldContextObject,
    FGameplayTag AbilityTag,
    FSOTS_AbilityRequirementCheckResult& OutResult) const
{
    OutResult = FSOTS_AbilityRequirementCheckResult();

    if (!AbilityTag.IsValid())
    {
        // Invalid tag -> treat as no authored requirements.
        OutResult.bMeetsAllRequirements = true;
        return true;
    }

    F_SOTS_AbilityDefinition Def;
    if (!GetAbilityDefinitionByTag(AbilityTag, Def))
    {
        OutResult.bMeetsAllRequirements = true;
        return true;
    }

    // If the definition has no authored requirements, treat as trivially true.
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
        OutResult.bMeetsAllRequirements = true;
        return true;
    }

    OutResult = USOTS_GAS_AbilityRequirementLibrary::EvaluateAbilityRequirements(WorldContextObject, Requirements);
    return true;
}

bool USOTS_AbilityRegistrySubsystem::CanActivateAbilityByTag(
    const UObject* WorldContextObject,
    FGameplayTag AbilityTag,
    FText& OutFailureReason) const
{
    FSOTS_AbilityRequirementCheckResult Result;
    if (!EvaluateAbilityRequirementsForTag(WorldContextObject, AbilityTag, Result))
    {
        OutFailureReason = FText::FromString(TEXT("Internal ability requirement evaluation error."));
        return false;
    }

    if (Result.bMeetsAllRequirements)
    {
        OutFailureReason = FText::GetEmpty();
        return true;
    }

    OutFailureReason = USOTS_GAS_AbilityRequirementLibrary::DescribeAbilityRequirementCheckResult(Result);
    return false;
}

void USOTS_AbilityRegistrySubsystem::BuildRegistryFromConfig()
{
    AbilityDefinitions.Reset();
    AbilityDefinitionAssets.Reset();

    for (const TSoftObjectPtr<USOTS_AbilityDefinitionLibrary>& LibraryPtr : AbilityDefinitionLibraries)
    {
        if (!LibraryPtr.IsValid() && !LibraryPtr.ToSoftObjectPath().IsValid())
        {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            if (bWarnOnMissingAbilityAssets)
            {
                UE_LOG(LogSOTSGAS, Warning, TEXT("[AbilityRegistry] Configured ability library reference is invalid; skipping."));
            }
#endif
            continue;
        }

        USOTS_AbilityDefinitionLibrary* Library = LibraryPtr.LoadSynchronous();
        if (!Library)
        {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            if (bWarnOnMissingAbilityAssets)
            {
                UE_LOG(LogSOTSGAS, Warning, TEXT("[AbilityRegistry] Failed to load ability library %s"), *LibraryPtr.ToSoftObjectPath().ToString());
            }
#endif
            continue;
        }

        RegisterAbilityDefinitionsFromDataAsset(Library);
    }

    for (const TSoftObjectPtr<USOTS_AbilityDefinitionDA>& DefinitionPtr : AdditionalAbilityDefinitions)
    {
        if (!DefinitionPtr.IsValid() && !DefinitionPtr.ToSoftObjectPath().IsValid())
        {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            if (bWarnOnMissingAbilityAssets)
            {
                UE_LOG(LogSOTSGAS, Warning, TEXT("[AbilityRegistry] Configured additional definition reference is invalid; skipping."));
            }
#endif
            continue;
        }

        USOTS_AbilityDefinitionDA* DefinitionDA = DefinitionPtr.LoadSynchronous();
        if (!DefinitionDA)
        {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            if (bWarnOnMissingAbilityAssets)
            {
                UE_LOG(LogSOTSGAS, Warning, TEXT("[AbilityRegistry] Failed to load ability definition %s"), *DefinitionPtr.ToSoftObjectPath().ToString());
            }
#endif
            continue;
        }

        RegisterAbilityDefinitionDA(DefinitionDA);
    }
}

void USOTS_AbilityRegistrySubsystem::ValidateDefinition(const F_SOTS_AbilityDefinition& Definition, TSet<FGameplayTag>& SeenTags) const
{
    if (!Definition.AbilityTag.IsValid())
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bWarnOnInvalidAbilityDefs)
        {
            UE_LOG(LogSOTSGAS, Warning, TEXT("[AbilityRegistry] Invalid ability definition missing tag."));
        }
#endif
        return;
    }

    if (SeenTags.Contains(Definition.AbilityTag))
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bWarnOnDuplicateAbilityTags)
        {
            UE_LOG(LogSOTSGAS, Warning, TEXT("[AbilityRegistry] Duplicate ability tag detected during validation: %s."), *Definition.AbilityTag.ToString());
        }
#endif
        return;
    }

    SeenTags.Add(Definition.AbilityTag);

    if (!Definition.AbilityClass)
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bWarnOnMissingAbilityAssets)
        {
            UE_LOG(LogSOTSGAS, Warning, TEXT("[AbilityRegistry] Ability %s has no AbilityClass set."), *Definition.AbilityTag.ToString());
        }
#endif
    }
}

void USOTS_AbilityRegistrySubsystem::ValidateRegistry() const
{
    TSet<FGameplayTag> SeenTags;

    for (const TPair<FGameplayTag, F_SOTS_AbilityDefinition>& Pair : AbilityDefinitions)
    {
        ValidateDefinition(Pair.Value, SeenTags);
    }
}

bool USOTS_AbilityRegistrySubsystem::IsRegistryReadyChecked() const
{
    if (bRegistryReady)
    {
        return true;
    }

    MaybeWarnRegistryNotReady();
    return false;
}

void USOTS_AbilityRegistrySubsystem::MaybeWarnRegistryNotReady() const
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (!bWarnOnRegistryNotReady)
    {
        return;
    }

    static bool bWarned = false;
    if (!bWarned)
    {
        bWarned = true;
        UE_LOG(LogSOTSGAS, Warning, TEXT("[AbilityRegistry] Registry not ready; requests will fail until init completes."));
    }
#endif
}

void USOTS_AbilityRegistrySubsystem::MaybeWarnMissingAbilityTag(FGameplayTag AbilityTag) const
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (!bWarnOnMissingAbilityTags)
    {
        return;
    }

    UE_LOG(LogSOTSGAS, Warning, TEXT("[AbilityRegistry] No definition registered for tag '%s' (registry size: %d)."), *AbilityTag.ToString(), AbilityDefinitions.Num());
#endif
}
