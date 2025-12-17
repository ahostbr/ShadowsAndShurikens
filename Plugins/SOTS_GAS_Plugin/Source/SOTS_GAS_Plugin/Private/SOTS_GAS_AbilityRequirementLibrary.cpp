#include "SOTS_GAS_AbilityRequirementLibrary.h"

#include "SOTS_GAS_AbilityRequirementLibraryAsset.h"
#include "SOTS_GAS_SkillTreeLibrary.h"
#include "Subsystems/SOTS_GAS_StealthBridgeSubsystem.h"
#include "SOTS_TagLibrary.h"
#include "Subsystems/SOTS_AbilityFXSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Internationalization/Text.h"

namespace
{
    static USOTS_GAS_StealthBridgeSubsystem* GetStealthBridge_Requirement(const UObject* WorldContextObject)
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

        if (UGameInstance* GI = World->GetGameInstance())
        {
            return GI->GetSubsystem<USOTS_GAS_StealthBridgeSubsystem>();
        }

        return nullptr;
    }

    static const AActor* GetPlayerActor_Requirement(const UObject* WorldContextObject)
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

        APlayerController* PC = World->GetFirstPlayerController();
        return PC ? PC->GetPawn() : nullptr;
    }
}

bool USOTS_GAS_AbilityRequirementLibrary::CollectPlayerSkillTags(
    const UObject* WorldContextObject,
    FGameplayTagContainer& OutSkillTags)
{
    OutSkillTags = USOTS_GAS_SkillTreeLibrary::GetAllSkillTags(WorldContextObject);
    return true;
}

bool USOTS_GAS_AbilityRequirementLibrary::GetCurrentStealthState(
    const UObject* WorldContextObject,
    int32& OutTier,
    float& OutScore01)
{
    OutTier = 0;
    OutScore01 = 0.0f;

    if (USOTS_GAS_StealthBridgeSubsystem* Bridge = GetStealthBridge_Requirement(WorldContextObject))
    {
        const FSOTS_StealthScoreBreakdown Breakdown = Bridge->GetCurrentStealthBreakdown();
        OutTier    = Breakdown.StealthTier;
        OutScore01 = Breakdown.CombinedScore01;
        return true;
    }

    return false;
}

FSOTS_AbilityRequirementCheckResult USOTS_GAS_AbilityRequirementLibrary::EvaluateAbilityRequirementsFromLibrary(
    const UObject* WorldContextObject,
    USOTS_AbilityRequirementLibraryAsset* LibraryAsset,
    FGameplayTag AbilityTag)
{
    FSOTS_AbilityRequirementCheckResult Result;

    if (!LibraryAsset || !AbilityTag.IsValid())
    {
        // No data -> treat as allowed by default.
        Result.bMeetsAllRequirements = true;
        return Result;
    }

    for (const FSOTS_AbilityRequirements& Req : LibraryAsset->AbilityRequirements)
    {
        if (Req.AbilityTag == AbilityTag)
        {
            return EvaluateAbilityRequirements(WorldContextObject, Req);
        }
    }

    // No entry for this ability tag -> no authored requirements.
    Result.bMeetsAllRequirements = true;
    return Result;
}

FSOTS_AbilityRequirementCheckResult USOTS_GAS_AbilityRequirementLibrary::EvaluateAbilityRequirements(
    const UObject* WorldContextObject,
    const FSOTS_AbilityRequirements& Requirements)
{
    FSOTS_AbilityRequirementCheckResult Result;

    // 1) Skill tags (from skill trees)
    FGameplayTagContainer SkillTags;
    CollectPlayerSkillTags(WorldContextObject, SkillTags);

    if (!Requirements.RequiredSkillTags.IsEmpty())
    {
        if (!SkillTags.HasAll(Requirements.RequiredSkillTags))
        {
            Result.bMissingSkillTags = true;
        }
    }

    // 2) Player generic tags (optional, only if we have data)
    if (!Requirements.RequiredPlayerTags.IsEmpty())
    {
        const AActor* PlayerActor = GetPlayerActor_Requirement(WorldContextObject);
        const bool bHasAllRequiredTags =
            PlayerActor &&
            USOTS_TagLibrary::ActorHasAllTags(WorldContextObject, PlayerActor, Requirements.RequiredPlayerTags);

        if (!bHasAllRequiredTags)
        {
            Result.bMissingPlayerTags = true;
        }
    }

    // 3) Stealth info
    int32 CurrentTier = 0;
    float CurrentScore01 = 0.0f;
    GetCurrentStealthState(WorldContextObject, CurrentTier, CurrentScore01);

    if (Requirements.MinStealthTier >= 0 && CurrentTier < Requirements.MinStealthTier)
    {
        Result.bStealthTierTooLow = true;
    }

    if (Requirements.MaxStealthTier >= 0 && CurrentTier > Requirements.MaxStealthTier)
    {
        Result.bStealthTierTooHigh = true;
    }

    if (Requirements.MaxStealthScore01 >= 0.0f &&
        CurrentScore01 > Requirements.MaxStealthScore01)
    {
        Result.bStealthScoreTooHigh = true;
    }

    if (Requirements.bDisallowWhenCompromised)
    {
        const ESOTS_StealthTier TierEnum =
            static_cast<ESOTS_StealthTier>(CurrentTier);

        if (TierEnum == ESOTS_StealthTier::Compromised)
        {
            Result.bStealthTierTooHigh = true;
        }
    }

    Result.bMeetsAllRequirements =
        !Result.bMissingSkillTags &&
        !Result.bMissingPlayerTags &&
        !Result.bStealthTierTooLow &&
        !Result.bStealthTierTooHigh &&
        !Result.bStealthScoreTooHigh;

    return Result;
}

bool USOTS_GAS_AbilityRequirementLibrary::CanActivateAbilityWithRequirements(
    const UObject* WorldContextObject,
    const FSOTS_AbilityRequirements& Requirements)
{
    const FSOTS_AbilityRequirementCheckResult Check =
        EvaluateAbilityRequirements(WorldContextObject, Requirements);

    return Check.bMeetsAllRequirements;
}

bool USOTS_GAS_AbilityRequirementLibrary::EvaluateAbilityRequirementsWithReason(
    const UObject* WorldContextObject,
    const FSOTS_AbilityRequirements& Requirements,
    FText& OutFailureReason)
{
    const FSOTS_AbilityRequirementCheckResult Result =
        EvaluateAbilityRequirements(WorldContextObject, Requirements);

    if (Result.bMeetsAllRequirements)
    {
        OutFailureReason = FText::GetEmpty();
        return true;
    }

    OutFailureReason = DescribeAbilityRequirementCheckResult(Result);
    return false;
}

bool USOTS_GAS_AbilityRequirementLibrary::SOTS_CanActivateAbilityByTag(
    const UObject* WorldContextObject,
    USOTS_AbilityRequirementLibraryAsset* LibraryAsset,
    FGameplayTag AbilityTag,
    FText& OutFailureReason)
{
    FSOTS_AbilityRequirementCheckResult Result;
    FText DebugDescription;

    const bool bCanActivate =
        EvaluateAbilityFromLibraryWithReason(
            WorldContextObject,
            LibraryAsset,
            AbilityTag,
            Result,
            DebugDescription);

    OutFailureReason = bCanActivate ? FText::GetEmpty() : DebugDescription;
    return bCanActivate;
}

bool USOTS_GAS_AbilityRequirementLibrary::EvaluateAbilityFromLibraryWithReason(
    const UObject* WorldContextObject,
    USOTS_AbilityRequirementLibraryAsset* LibraryAsset,
    FGameplayTag AbilityTag,
    FSOTS_AbilityRequirementCheckResult& OutResult,
    FText& OutDebugDescription)
{
    // No asset or invalid tag -> treat as "no requirements".
    if (!LibraryAsset || !AbilityTag.IsValid())
    {
        OutResult = FSOTS_AbilityRequirementCheckResult();
        OutResult.bMeetsAllRequirements = true;
        OutDebugDescription = FText::FromString(TEXT("No requirements configured (treat as allowed)."));
        return true;
    }

    const FSOTS_AbilityRequirementCheckResult Result =
        EvaluateAbilityRequirementsFromLibrary(WorldContextObject, LibraryAsset, AbilityTag);

    OutResult = Result;
    OutDebugDescription = DescribeAbilityRequirementCheckResult(Result);

    return Result.bMeetsAllRequirements;
}

void USOTS_GAS_AbilityRequirementLibrary::TriggerFailureFX(const UObject* WorldContextObject, FGameplayTag FailureFXTag)
{
    if (!FailureFXTag.IsValid())
    {
        return;
    }

    if (USOTS_AbilityFXSubsystem* FXSubsystem = USOTS_AbilityFXSubsystem::Get(WorldContextObject))
    {
        FXSubsystem->TriggerAbilityFX(FailureFXTag, FGameplayTag(), nullptr);
    }
}

FText USOTS_GAS_AbilityRequirementLibrary::DescribeAbilityRequirementCheckResult(
    const FSOTS_AbilityRequirementCheckResult& Result)
{
    if (Result.bMeetsAllRequirements)
    {
        return FText::FromString(TEXT("All requirements met"));
    }

    TArray<FString> Parts;

    if (Result.bMissingSkillTags)
    {
        Parts.Add(TEXT("Missing required skill tags"));
    }

    if (Result.bMissingPlayerTags)
    {
        Parts.Add(TEXT("Missing required player tags"));
    }

    if (Result.bStealthTierTooLow)
    {
        Parts.Add(TEXT("Stealth tier too low"));
    }

    if (Result.bStealthTierTooHigh)
    {
        Parts.Add(TEXT("Stealth tier too high"));
    }

    if (Result.bStealthScoreTooHigh)
    {
        Parts.Add(TEXT("Stealth score too high"));
    }

    if (Parts.Num() == 0)
    {
        return FText::FromString(TEXT("Requirements not met"));
    }

    const FString Joined = FString::Join(Parts, TEXT("; "));
    return FText::FromString(Joined);
}
