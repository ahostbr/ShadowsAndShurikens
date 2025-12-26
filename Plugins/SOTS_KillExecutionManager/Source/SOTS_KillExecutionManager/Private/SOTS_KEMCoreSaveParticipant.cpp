#include "SOTS_KEMCoreSaveParticipant.h"

#include "SOTS_KEMCoreBridgeSettings.h"
#include "SOTS_KEM_ManagerSubsystem.h"
#include "SOTS_KillExecutionManagerModule.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

namespace
{
    USOTS_KEMManagerSubsystem* ResolveKEMSubsystemFromAnyWorld()
    {
        if (!GEngine)
        {
            return nullptr;
        }

        for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
        {
            UWorld* World = WorldContext.World();
            if (!World)
            {
                continue;
            }

            UGameInstance* GameInstance = World->GetGameInstance();
            if (!GameInstance)
            {
                continue;
            }

            return GameInstance->GetSubsystem<USOTS_KEMManagerSubsystem>();
        }

        return nullptr;
    }
}

FName FKEM_SaveParticipant::GetParticipantId() const
{
    return TEXT("KEM");
}

FSOTS_SaveParticipantStatus FKEM_SaveParticipant::QuerySaveStatus(const FSOTS_SaveRequestContext& Ctx) const
{
    FSOTS_SaveParticipantStatus Status;

    const USOTS_KEMCoreBridgeSettings* Settings = GetDefault<USOTS_KEMCoreBridgeSettings>();
    if (!Settings || !Settings->bEnableSOTSCoreSaveParticipantBridge)
    {
        return Status;
    }

    USOTS_KEMManagerSubsystem* Subsystem = ResolveKEMSubsystemFromAnyWorld();
    if (!Subsystem)
    {
        if (Settings->bEnableSOTSCoreBridgeVerboseLogs)
        {
            UE_LOG(LogSOTSKEM, Verbose, TEXT("KEM CoreBridge: SaveParticipant query could not resolve subsystem. Allowing save. Reason=%s"), *Ctx.Reason);
        }

        return Status;
    }

    if (Subsystem->IsSaveBlocked())
    {
        Status.bCanSave = false;
        Status.BlockReason = TEXT("KEM: Save blocked");

        if (Settings->bEnableSOTSCoreBridgeVerboseLogs)
        {
            UE_LOG(LogSOTSKEM, Verbose, TEXT("KEM CoreBridge: Save blocked. Profile=%s Slot=%s Auto=%d Reason=%s"), *Ctx.ProfileId, *Ctx.SlotId, Ctx.bIsAutoSave ? 1 : 0, *Ctx.Reason);
        }
    }

    return Status;
}
