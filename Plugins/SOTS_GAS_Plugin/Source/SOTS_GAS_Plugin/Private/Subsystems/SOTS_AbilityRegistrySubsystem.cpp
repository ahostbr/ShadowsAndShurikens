#include "Subsystems/SOTS_AbilityRegistrySubsystem.h"
#include "SOTS_GAS_Plugin.h"

void USOTS_AbilityRegistrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    AbilityDefinitions.Reset();
    AbilityDefinitionAssets.Reset();

    UE_LOG(LogSOTSGAS, Log, TEXT("[AbilityRegistry] Initialized."));
}

void USOTS_AbilityRegistrySubsystem::Deinitialize()
{
    UE_LOG(LogSOTSGAS, Log, TEXT("[AbilityRegistry] Deinitialize. Clearing %d definitions."), AbilityDefinitions.Num());

    AbilityDefinitions.Reset();
    AbilityDefinitionAssets.Reset();

    Super::Deinitialize();
}

void USOTS_AbilityRegistrySubsystem::RegisterAbilityDefinition(const F_SOTS_AbilityDefinition& Definition)
{
    if (!Definition.AbilityTag.IsValid())
    {
        return;
    }

    AbilityDefinitions.FindOrAdd(Definition.AbilityTag) = Definition;
}

void USOTS_AbilityRegistrySubsystem::RegisterAbilityDefinitionDA(USOTS_AbilityDefinitionDA* DefinitionDA)
{
    if (!DefinitionDA)
    {
        return;
    }

    const F_SOTS_AbilityDefinition& Def = DefinitionDA->Ability;
    if (!Def.AbilityTag.IsValid())
    {
        return;
    }

    AbilityDefinitions.FindOrAdd(Def.AbilityTag) = Def;
    AbilityDefinitionAssets.FindOrAdd(Def.AbilityTag) = DefinitionDA;
}

void USOTS_AbilityRegistrySubsystem::RegisterAbilityDefinitionsFromArray(const TArray<USOTS_AbilityDefinitionDA*>& Definitions)
{
    for (USOTS_AbilityDefinitionDA* DA : Definitions)
    {
        RegisterAbilityDefinitionDA(DA);
    }
}

void USOTS_AbilityRegistrySubsystem::RegisterAbilityDefinitionsFromDataAsset(USOTS_AbilityDefinitionLibrary* Library)
{
    if (!Library)
    {
        return;
    }

    for (USOTS_AbilityDefinitionDA* DA : Library->Definitions)
    {
        RegisterAbilityDefinitionDA(DA);
    }
}

bool USOTS_AbilityRegistrySubsystem::GetAbilityDefinitionByTag(FGameplayTag AbilityTag, F_SOTS_AbilityDefinition& OutDefinition) const
{
    if (const F_SOTS_AbilityDefinition* Found = AbilityDefinitions.Find(AbilityTag))
    {
        OutDefinition = *Found;
        return true;
    }
    return false;
}

bool USOTS_AbilityRegistrySubsystem::GetAbilityDefinitionDAByTag(FGameplayTag AbilityTag, USOTS_AbilityDefinitionDA*& OutDefinitionDA) const
{
    if (USOTS_AbilityDefinitionDA* const* Found = AbilityDefinitionAssets.Find(AbilityTag))
    {
        OutDefinitionDA = *Found;
        return true;
    }
    return false;
}

void USOTS_AbilityRegistrySubsystem::GetAllAbilityDefinitions(TArray<F_SOTS_AbilityDefinition>& OutDefinitions) const
{
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
        UE_LOG(LogSOTSGAS, Warning,
               TEXT("[AbilityRegistry] EvaluateAbilityRequirementsForTag: No definition registered for tag '%s'."),
               *AbilityTag.ToString());

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
