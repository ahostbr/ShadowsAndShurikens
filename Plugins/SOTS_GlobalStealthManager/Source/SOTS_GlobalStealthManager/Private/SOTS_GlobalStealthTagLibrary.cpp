#include "SOTS_GlobalStealthTagLibrary.h"

#include "SOTS_GlobalStealthManagerSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "SOTS_TagLibrary.h"

bool USOTS_GlobalStealthTagLibrary::SOTS_IsPlayerHidden(const UObject* WorldContextObject)
{
    if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(WorldContextObject))
    {
        const ESOTS_StealthTier Tier = GSM->GetStealthTier();
        if (Tier == ESOTS_StealthTier::Hidden)
        {
            return true;
        }
    }

    // Fallback: consult the player pawn's tags via the central tag manager.
    const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
    if (!World)
    {
        return false;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    const AActor* PlayerActor = PC ? PC->GetPawn() : nullptr;
    if (!PlayerActor)
    {
        return false;
    }

    return USOTS_TagLibrary::ActorHasTagByName(
        WorldContextObject,
        PlayerActor,
        FName(TEXT("Player.Stealth.Hidden")));
}

int32 USOTS_GlobalStealthTagLibrary::SOTS_GetStealthTier(const UObject* WorldContextObject)
{
    if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(WorldContextObject))
    {
        return static_cast<int32>(GSM->GetStealthTier());
    }

    // Fallback: derive from tier tags, if present, using the player pawn.
    const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
    if (!World)
    {
        return 0;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    const AActor* PlayerActor = PC ? PC->GetPawn() : nullptr;
    if (!PlayerActor)
    {
        return 0;
    }

    for (int32 Index = 0; Index < 4; ++Index)
    {
        const FString TagName = FString::Printf(TEXT("Player.Stealth.Tier.%d"), Index);
        if (USOTS_TagLibrary::ActorHasTagByName(WorldContextObject, PlayerActor, FName(*TagName)))
        {
            return Index;
        }
    }

    return 0;
}
