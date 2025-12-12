#include "Subsystems/SOTS_GAS_StealthBridgeSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

USOTS_GAS_StealthBridgeSubsystem::USOTS_GAS_StealthBridgeSubsystem()
    : GlobalStealthSubsystem(nullptr)
    , CachedStealthScore(0.0f)
    , CachedStealthTier(0)
{
}

void USOTS_GAS_StealthBridgeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    CachedStealthScore = 0.0f;
    CachedStealthTier = 0;
    GlobalStealthSubsystem = nullptr;
    CachedBreakdown = FSOTS_StealthScoreBreakdown();

    // Use the GSM static helper so we follow the same pattern as other systems.
    GlobalStealthSubsystem = USOTS_GlobalStealthManagerSubsystem::Get(this);

    if (GlobalStealthSubsystem)
    {
        // Prime the cached values from the current GSM state so callers have
        // something meaningful even before the first delegate broadcast.
        CachedBreakdown   = GlobalStealthSubsystem->GetCurrentStealthBreakdown();
        CachedStealthScore = CachedBreakdown.CombinedScore01;
        CachedStealthTier  = CachedBreakdown.StealthTier;

        GlobalStealthSubsystem->OnStealthLevelChanged.AddDynamic(
            this,
            &USOTS_GAS_StealthBridgeSubsystem::HandleStealthLevelChanged);
    }
}

void USOTS_GAS_StealthBridgeSubsystem::Deinitialize()
{
    if (GlobalStealthSubsystem)
    {
        GlobalStealthSubsystem->OnStealthLevelChanged.RemoveDynamic(
            this,
            &USOTS_GAS_StealthBridgeSubsystem::HandleStealthLevelChanged);

        GlobalStealthSubsystem = nullptr;
    }

    Super::Deinitialize();
}

void USOTS_GAS_StealthBridgeSubsystem::HandleStealthLevelChanged(
    ESOTSStealthLevel /*OldLevel*/,
    ESOTSStealthLevel NewLevel,
    float NewScore)
{
    if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(this))
    {
        CachedBreakdown    = GSM->GetCurrentStealthBreakdown();
        CachedStealthScore = CachedBreakdown.CombinedScore01;
        CachedStealthTier  = CachedBreakdown.StealthTier;
    }
    else
    {
        CachedStealthScore = NewScore;
        CachedStealthTier  = static_cast<int32>(NewLevel);
    }
}
