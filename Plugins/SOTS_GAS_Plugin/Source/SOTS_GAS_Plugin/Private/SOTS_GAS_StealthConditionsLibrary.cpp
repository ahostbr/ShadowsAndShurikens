#include "SOTS_GAS_StealthConditionsLibrary.h"

#include "Subsystems/SOTS_GAS_StealthBridgeSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

namespace
{
    FSOTS_StealthScoreBreakdown GetBreakdownFromBridge(const UObject* WorldContextObject)
    {
        if (!WorldContextObject)
        {
            return FSOTS_StealthScoreBreakdown();
        }

        if (const UWorld* World = WorldContextObject->GetWorld())
        {
            if (UGameInstance* GI = World->GetGameInstance())
            {
                if (USOTS_GAS_StealthBridgeSubsystem* Bridge =
                        GI->GetSubsystem<USOTS_GAS_StealthBridgeSubsystem>())
                {
                    return Bridge->GetCurrentStealthBreakdown();
                }
            }
        }

        return FSOTS_StealthScoreBreakdown();
    }
}

USOTS_GAS_StealthBridgeSubsystem* USOTS_GAS_StealthConditionsLibrary::GetBridge(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    if (const UWorld* World = WorldContextObject->GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            return GI->GetSubsystem<USOTS_GAS_StealthBridgeSubsystem>();
        }
    }

    return nullptr;
}

bool USOTS_GAS_StealthConditionsLibrary::IsPlayerHidden(const UObject* WorldContextObject)
{
    const FSOTS_StealthScoreBreakdown Breakdown = GetBreakdownFromBridge(WorldContextObject);
    const ESOTS_StealthTier Tier =
        static_cast<ESOTS_StealthTier>(Breakdown.StealthTier);

    return Tier == ESOTS_StealthTier::Hidden;
}

bool USOTS_GAS_StealthConditionsLibrary::IsPlayerInLowProfileStealth(const UObject* WorldContextObject)
{
    const FSOTS_StealthScoreBreakdown Breakdown = GetBreakdownFromBridge(WorldContextObject);
    const ESOTS_StealthTier Tier =
        static_cast<ESOTS_StealthTier>(Breakdown.StealthTier);

    // Low-profile: treat Hidden and Cautious as safe-ish stealth.
    return Tier == ESOTS_StealthTier::Hidden ||
           Tier == ESOTS_StealthTier::Cautious;
}

bool USOTS_GAS_StealthConditionsLibrary::CanUseLoudAbility(const UObject* WorldContextObject)
{
    const FSOTS_StealthScoreBreakdown Breakdown = GetBreakdownFromBridge(WorldContextObject);
    const ESOTS_StealthTier Tier =
        static_cast<ESOTS_StealthTier>(Breakdown.StealthTier);

    // Example rule: disallow "loud" abilities only when fully compromised.
    // Design can adjust this later without changing the API surface.
    return Tier != ESOTS_StealthTier::Compromised;
}

int32 USOTS_GAS_StealthConditionsLibrary::GetCurrentStealthTier(const UObject* WorldContextObject)
{
    if (USOTS_GAS_StealthBridgeSubsystem* Bridge = GetBridge(WorldContextObject))
    {
        return Bridge->GetCurrentStealthTier();
    }

    return 0;
}

float USOTS_GAS_StealthConditionsLibrary::GetCurrentStealthScore01(const UObject* WorldContextObject)
{
    if (USOTS_GAS_StealthBridgeSubsystem* Bridge = GetBridge(WorldContextObject))
    {
        return Bridge->GetCurrentStealthScore();
    }

    return 0.0f;
}

