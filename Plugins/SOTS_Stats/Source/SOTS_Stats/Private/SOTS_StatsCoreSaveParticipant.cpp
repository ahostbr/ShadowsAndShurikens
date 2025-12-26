#include "SOTS_StatsCoreSaveParticipant.h"

#include "SOTS_ProfileTypes.h"
#include "SOTS_StatsCoreBridgeSettings.h"
#include "SOTS_StatsLibrary.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_StatsCoreBridge, Log, All);

namespace
{
    const USOTS_StatsCoreBridgeSettings* GetSettings()
    {
        return GetDefault<USOTS_StatsCoreBridgeSettings>();
    }

    bool IsBridgeEnabled(const USOTS_StatsCoreBridgeSettings* Settings)
    {
        return Settings && Settings->bEnableSOTSCoreSaveParticipantBridge;
    }

    bool ShouldLogVerbose(const USOTS_StatsCoreBridgeSettings* Settings)
    {
        return Settings && Settings->bEnableSOTSCoreBridgeVerboseLogs;
    }

    APawn* ResolvePrimaryPawnFromAnyWorld()
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

            if (ULocalPlayer* LocalPlayer = GameInstance->GetFirstGamePlayer())
            {
                if (APlayerController* PC = LocalPlayer->GetPlayerController(World))
                {
                    return PC->GetPawn();
                }
            }

            if (APlayerController* PC = World->GetFirstPlayerController())
            {
                return PC->GetPawn();
            }
        }

        return nullptr;
    }

    bool SerializeCharacterStateToBytes(const FSOTS_CharacterStateData& Snapshot, TArray<uint8>& OutBytes)
    {
        OutBytes.Reset();

        FMemoryWriter Writer(OutBytes, true);
        FSOTS_CharacterStateData::StaticStruct()->SerializeItem(Writer, const_cast<FSOTS_CharacterStateData*>(&Snapshot), nullptr);

        return OutBytes.Num() > 0;
    }

    bool DeserializeCharacterStateFromBytes(const TArray<uint8>& InBytes, FSOTS_CharacterStateData& OutSnapshot)
    {
        if (InBytes.Num() <= 0)
        {
            return false;
        }

        FMemoryReader Reader(InBytes, true);
        FSOTS_CharacterStateData::StaticStruct()->SerializeItem(Reader, &OutSnapshot, nullptr);
        return true;
    }
}

FName FStats_SaveParticipant::GetParticipantId() const
{
    return TEXT("Stats");
}

FSOTS_SaveParticipantStatus FStats_SaveParticipant::QuerySaveStatus(const FSOTS_SaveRequestContext& Ctx) const
{
    FSOTS_SaveParticipantStatus Status;

    const USOTS_StatsCoreBridgeSettings* Settings = GetSettings();
    if (!IsBridgeEnabled(Settings))
    {
        return Status;
    }

    if (ShouldLogVerbose(Settings))
    {
        UE_LOG(LogSOTS_StatsCoreBridge, Verbose, TEXT("Stats CoreBridge: QuerySaveStatus allow (Profile=%s Slot=%s Auto=%d Reason=%s)"), *Ctx.ProfileId, *Ctx.SlotId, Ctx.bIsAutoSave ? 1 : 0, *Ctx.Reason);
    }

    return Status;
}

bool FStats_SaveParticipant::BuildSaveFragment(const FSOTS_SaveRequestContext& Ctx, FSOTS_SavePayloadFragment& Out)
{
    const USOTS_StatsCoreBridgeSettings* Settings = GetSettings();
    if (!IsBridgeEnabled(Settings))
    {
        return false;
    }

    APawn* Pawn = ResolvePrimaryPawnFromAnyWorld();
    if (!Pawn)
    {
        if (ShouldLogVerbose(Settings))
        {
            UE_LOG(LogSOTS_StatsCoreBridge, Verbose, TEXT("Stats CoreBridge: BuildSaveFragment could not resolve pawn. Profile=%s Slot=%s"), *Ctx.ProfileId, *Ctx.SlotId);
        }
        return false;
    }

    FSOTS_CharacterStateData Snapshot;
    USOTS_StatsLibrary::SnapshotActorStats(Pawn, Snapshot);

    TArray<uint8> Bytes;
    if (!SerializeCharacterStateToBytes(Snapshot, Bytes))
    {
        if (ShouldLogVerbose(Settings))
        {
            UE_LOG(LogSOTS_StatsCoreBridge, Verbose, TEXT("Stats CoreBridge: BuildSaveFragment produced empty payload for %s"), *GetNameSafe(Pawn));
        }
        return false;
    }

    Out.FragmentId = TEXT("Stats.Snapshot");
    Out.Data = MoveTemp(Bytes);

    if (ShouldLogVerbose(Settings))
    {
        UE_LOG(LogSOTS_StatsCoreBridge, Verbose, TEXT("Stats CoreBridge: BuildSaveFragment ok Pawn=%s Bytes=%d"), *GetNameSafe(Pawn), Out.Data.Num());
    }

    return true;
}

bool FStats_SaveParticipant::ApplySaveFragment(const FSOTS_SaveRequestContext& Ctx, const FSOTS_SavePayloadFragment& In)
{
    const USOTS_StatsCoreBridgeSettings* Settings = GetSettings();
    if (!IsBridgeEnabled(Settings))
    {
        return false;
    }

    if (In.FragmentId != TEXT("Stats.Snapshot"))
    {
        if (ShouldLogVerbose(Settings))
        {
            UE_LOG(LogSOTS_StatsCoreBridge, Verbose, TEXT("Stats CoreBridge: ApplySaveFragment ignored FragmentId=%s"), *In.FragmentId.ToString());
        }
        return false;
    }

    APawn* Pawn = ResolvePrimaryPawnFromAnyWorld();
    if (!Pawn)
    {
        if (ShouldLogVerbose(Settings))
        {
            UE_LOG(LogSOTS_StatsCoreBridge, Verbose, TEXT("Stats CoreBridge: ApplySaveFragment could not resolve pawn. Profile=%s Slot=%s"), *Ctx.ProfileId, *Ctx.SlotId);
        }
        return false;
    }

    FSOTS_CharacterStateData Snapshot;
    if (!DeserializeCharacterStateFromBytes(In.Data, Snapshot))
    {
        if (ShouldLogVerbose(Settings))
        {
            UE_LOG(LogSOTS_StatsCoreBridge, Verbose, TEXT("Stats CoreBridge: ApplySaveFragment could not deserialize payload for %s"), *GetNameSafe(Pawn));
        }
        return false;
    }

    USOTS_StatsLibrary::ApplySnapshotToActor(Pawn, Snapshot);

    if (ShouldLogVerbose(Settings))
    {
        UE_LOG(LogSOTS_StatsCoreBridge, Verbose, TEXT("Stats CoreBridge: ApplySaveFragment ok Pawn=%s Bytes=%d"), *GetNameSafe(Pawn), In.Data.Num());
    }

    return true;
}
