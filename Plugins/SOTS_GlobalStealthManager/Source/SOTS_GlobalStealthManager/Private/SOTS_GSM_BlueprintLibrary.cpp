#include "SOTS_GSM_BlueprintLibrary.h"
#include "SOTS_GlobalStealthManagerSubsystem.h"

float USOTS_GSM_BlueprintLibrary::SOTS_GetStealthScore(const UObject* WorldContextObject)
{
    if (const USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(WorldContextObject))
    {
        return GSM->GetCurrentStealthScore();
    }
    return 1.0f;
}

ESOTS_StealthTier USOTS_GSM_BlueprintLibrary::SOTS_GetStealthTier(const UObject* WorldContextObject)
{
    if (const USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(WorldContextObject))
    {
        return GSM->GetStealthTier();
    }
    return ESOTS_StealthTier::Hidden;
}

FGameplayTag USOTS_GSM_BlueprintLibrary::SOTS_GetStealthTierTag(const UObject* WorldContextObject)
{
    if (const USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(WorldContextObject))
    {
        return GSM->GetTierTag(GSM->GetStealthTier());
    }
    return FGameplayTag();
}

bool USOTS_GSM_BlueprintLibrary::SOTS_IsStealthTierAtLeast(const UObject* WorldContextObject, ESOTS_StealthTier Tier)
{
    if (const USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(WorldContextObject))
    {
        return static_cast<uint8>(GSM->GetStealthTier()) >= static_cast<uint8>(Tier);
    }
    return false;
}

FGameplayTag USOTS_GSM_BlueprintLibrary::SOTS_GetLastStealthReasonTag(const UObject* WorldContextObject)
{
    if (const USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(WorldContextObject))
    {
        return GSM->GetLastReasonTag();
    }
    return FGameplayTag();
}
