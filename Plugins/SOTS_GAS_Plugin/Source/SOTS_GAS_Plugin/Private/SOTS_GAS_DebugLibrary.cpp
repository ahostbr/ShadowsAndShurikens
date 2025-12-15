#include "SOTS_GAS_DebugLibrary.h"

#include "SOTS_GAS_AbilityRequirementLibrary.h"
#include "SOTS_GAS_AbilityRequirementLibraryAsset.h"
#include "SOTS_GAS_SkillTreeLibrary.h"
#include "Subsystems/SOTS_GAS_StealthBridgeSubsystem.h"
#include "SOTS_GlobalStealthTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Logging/LogMacros.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/IConsoleManager.h"
#include "GameplayTagAssetInterface.h"

namespace
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    static TAutoConsoleVariable<int32> CVarSOTSStealthDebugMode(
        TEXT("sots.StealthDebugMode"),
        0,
        TEXT("Global stealth/perception debug mode. 0=Off, 1=Basic, 2=Advanced"),
        ECVF_Cheat);
#endif

    static USOTS_GAS_StealthBridgeSubsystem* GetStealthBridge_Debug(const UObject* WorldContextObject)
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

}

ESOTSStealthDebugMode USOTS_GAS_DebugLibrary::GetStealthDebugMode()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    const int32 Value = CVarSOTSStealthDebugMode.GetValueOnGameThread();
    const int32 Clamped = FMath::Clamp(Value, 0, 2);
    return static_cast<ESOTSStealthDebugMode>(Clamped);
#else
    return ESOTSStealthDebugMode::Off;
#endif
}

void USOTS_GAS_DebugLibrary::SetStealthDebugMode(ESOTSStealthDebugMode NewMode)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    CVarSOTSStealthDebugMode->Set(static_cast<int32>(NewMode));
#endif
}

void USOTS_GAS_DebugLibrary::LogCurrentStealthState(const UObject* WorldContextObject)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (USOTS_GAS_StealthBridgeSubsystem* Bridge = GetStealthBridge_Debug(WorldContextObject))
    {
        const FSOTS_StealthScoreBreakdown Breakdown = Bridge->GetCurrentStealthBreakdown();

        UE_LOG(LogTemp, Log,
               TEXT("[SOTS GAS Debug] Stealth: Light=%.2f Shadow=%.2f Vis=%.2f AISusp=%.2f Score=%.2f Tier=%d Modifier=%.2f"),
               Breakdown.Light01,
               Breakdown.Shadow01,
               Breakdown.Visibility01,
               Breakdown.AISuspicion01,
               Breakdown.CombinedScore01,
               Breakdown.StealthTier,
               Breakdown.ModifierMultiplier);
    }
    else
    {
        UE_LOG(LogTemp, Warning,
               TEXT("[SOTS GAS Debug] LogCurrentStealthState: Stealth bridge subsystem not available."));
    }
#endif
}

void USOTS_GAS_DebugLibrary::LogCurrentSkillAndPlayerTags(const UObject* WorldContextObject)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    const FGameplayTagContainer SkillTags =
        USOTS_GAS_SkillTreeLibrary::GetAllSkillTags(WorldContextObject);

    FGameplayTagContainer PlayerTags;
    if (WorldContextObject)
    {
        if (const UWorld* World = WorldContextObject->GetWorld())
        {
            if (APlayerController* PC = World->GetFirstPlayerController())
            {
                if (AActor* PlayerActor = PC->GetPawn())
                {
                    const IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(PlayerActor);
                    if (TagInterface)
                    {
                        TagInterface->GetOwnedGameplayTags(PlayerTags);
                    }
                }
            }
        }
    }

    UE_LOG(LogTemp, Log,
           TEXT("[SOTS GAS Debug] SkillTags count=%d, PlayerTags count=%d"),
           SkillTags.Num(),
           PlayerTags.Num());

    {
        FString SkillTagString;
        for (const FGameplayTag& Tag : SkillTags)
        {
            if (!SkillTagString.IsEmpty())
            {
                SkillTagString.Append(TEXT(", "));
            }
            SkillTagString.Append(Tag.ToString());
        }

        UE_LOG(LogTemp, Log,
               TEXT("[SOTS GAS Debug] SkillTags: %s"),
               SkillTagString.IsEmpty() ? TEXT("<none>") : *SkillTagString);
    }

    {
        FString PlayerTagString;
        for (const FGameplayTag& Tag : PlayerTags)
        {
            if (!PlayerTagString.IsEmpty())
            {
                PlayerTagString.Append(TEXT(", "));
            }
            PlayerTagString.Append(Tag.ToString());
        }

        UE_LOG(LogTemp, Log,
               TEXT("[SOTS GAS Debug] PlayerTags: %s"),
               PlayerTagString.IsEmpty() ? TEXT("<none>") : *PlayerTagString);
    }
#endif
}

void USOTS_GAS_DebugLibrary::LogAbilityRequirementsFromLibrary(
    const UObject* WorldContextObject,
    USOTS_AbilityRequirementLibraryAsset* LibraryAsset,
    FGameplayTag AbilityTag)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    FSOTS_AbilityRequirementCheckResult Result;
    FText DebugText;

    const bool bCanActivate =
        USOTS_GAS_AbilityRequirementLibrary::EvaluateAbilityFromLibraryWithReason(
            WorldContextObject,
            LibraryAsset,
            AbilityTag,
            Result,
            DebugText);

    UE_LOG(LogTemp, Log,
           TEXT("[SOTS GAS Debug] AbilityTag=%s CanActivate=%s Reason=%s"),
           *AbilityTag.ToString(),
           bCanActivate ? TEXT("true") : TEXT("false"),
           *DebugText.ToString());

    UE_LOG(LogTemp, Log,
           TEXT("[SOTS GAS Debug] Flags: MissingSkillTags=%s MissingPlayerTags=%s TierTooLow=%s TierTooHigh=%s ScoreTooHigh=%s"),
           Result.bMissingSkillTags ? TEXT("true") : TEXT("false"),
           Result.bMissingPlayerTags ? TEXT("true") : TEXT("false"),
           Result.bStealthTierTooLow ? TEXT("true") : TEXT("false"),
           Result.bStealthTierTooHigh ? TEXT("true") : TEXT("false"),
           Result.bStealthScoreTooHigh ? TEXT("true") : TEXT("false"));
#endif
}
