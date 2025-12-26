#include "SOTS_INVCoreSaveParticipant.h"

#include "SOTS_INVCoreBridgeSettings.h"
#include "SOTS_InventoryBridgeSubsystem.h"
#include "SOTS_ProfileTypes.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_INVCoreBridge, Log, All);

namespace
{
    const USOTS_INVCoreBridgeSettings* GetSettings()
    {
        return GetDefault<USOTS_INVCoreBridgeSettings>();
    }

    bool IsBridgeEnabled(const USOTS_INVCoreBridgeSettings* Settings)
    {
        return Settings && Settings->bEnableSOTSCoreSaveParticipantBridge;
    }

    bool ShouldLogVerbose(const USOTS_INVCoreBridgeSettings* Settings)
    {
        return Settings && Settings->bEnableSOTSCoreBridgeVerboseLogs;
    }

    USOTS_InventoryBridgeSubsystem* ResolveInventorySubsystemFromAnyWorld()
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

            if (USOTS_InventoryBridgeSubsystem* Subsystem = GameInstance->GetSubsystem<USOTS_InventoryBridgeSubsystem>())
            {
                return Subsystem;
            }
        }

        return nullptr;
    }

    bool SerializeInventoryProfileDataToBytes(const FSOTS_InventoryProfileData& Snapshot, TArray<uint8>& OutBytes)
    {
        OutBytes.Reset();

        FMemoryWriter Writer(OutBytes, true);
        FSOTS_InventoryProfileData::StaticStruct()->SerializeItem(Writer, const_cast<FSOTS_InventoryProfileData*>(&Snapshot), nullptr);

        return OutBytes.Num() > 0;
    }

    bool DeserializeInventoryProfileDataFromBytes(const TArray<uint8>& InBytes, FSOTS_InventoryProfileData& OutSnapshot)
    {
        if (InBytes.Num() <= 0)
        {
            return false;
        }

        FMemoryReader Reader(InBytes, true);
        FSOTS_InventoryProfileData::StaticStruct()->SerializeItem(Reader, &OutSnapshot, nullptr);
        return true;
    }
}

FName FINV_SaveParticipant::GetParticipantId() const
{
    return TEXT("INV");
}

FSOTS_SaveParticipantStatus FINV_SaveParticipant::QuerySaveStatus(const FSOTS_SaveRequestContext& Ctx) const
{
    FSOTS_SaveParticipantStatus Status;

    const USOTS_INVCoreBridgeSettings* Settings = GetSettings();
    if (!IsBridgeEnabled(Settings))
    {
        return Status;
    }

    if (ShouldLogVerbose(Settings))
    {
        UE_LOG(LogSOTS_INVCoreBridge, Verbose, TEXT("INV CoreBridge: QuerySaveStatus allow (Profile=%s Slot=%s Auto=%d Reason=%s)"), *Ctx.ProfileId, *Ctx.SlotId, Ctx.bIsAutoSave ? 1 : 0, *Ctx.Reason);
    }

    return Status;
}

bool FINV_SaveParticipant::BuildSaveFragment(const FSOTS_SaveRequestContext& Ctx, FSOTS_SavePayloadFragment& Out)
{
    const USOTS_INVCoreBridgeSettings* Settings = GetSettings();
    if (!IsBridgeEnabled(Settings))
    {
        return false;
    }

    USOTS_InventoryBridgeSubsystem* Subsystem = ResolveInventorySubsystemFromAnyWorld();
    if (!Subsystem)
    {
        if (ShouldLogVerbose(Settings))
        {
            UE_LOG(LogSOTS_INVCoreBridge, Verbose, TEXT("INV CoreBridge: BuildSaveFragment could not resolve subsystem. Profile=%s Slot=%s"), *Ctx.ProfileId, *Ctx.SlotId);
        }
        return false;
    }

    FSOTS_InventoryProfileData Snapshot;
    Subsystem->BuildProfileData(Snapshot);

    TArray<uint8> Bytes;
    if (!SerializeInventoryProfileDataToBytes(Snapshot, Bytes))
    {
        if (ShouldLogVerbose(Settings))
        {
            UE_LOG(LogSOTS_INVCoreBridge, Verbose, TEXT("INV CoreBridge: BuildSaveFragment produced empty payload. Profile=%s Slot=%s"), *Ctx.ProfileId, *Ctx.SlotId);
        }
        return false;
    }

    Out.FragmentId = TEXT("INV.Inventory");
    Out.Data = MoveTemp(Bytes);

    if (ShouldLogVerbose(Settings))
    {
        UE_LOG(LogSOTS_INVCoreBridge, Verbose, TEXT("INV CoreBridge: BuildSaveFragment ok Bytes=%d"), Out.Data.Num());
    }

    return true;
}

bool FINV_SaveParticipant::ApplySaveFragment(const FSOTS_SaveRequestContext& Ctx, const FSOTS_SavePayloadFragment& In)
{
    const USOTS_INVCoreBridgeSettings* Settings = GetSettings();
    if (!IsBridgeEnabled(Settings))
    {
        return false;
    }

    if (In.FragmentId != TEXT("INV.Inventory"))
    {
        if (ShouldLogVerbose(Settings))
        {
            UE_LOG(LogSOTS_INVCoreBridge, Verbose, TEXT("INV CoreBridge: ApplySaveFragment ignored FragmentId=%s"), *In.FragmentId.ToString());
        }
        return false;
    }

    USOTS_InventoryBridgeSubsystem* Subsystem = ResolveInventorySubsystemFromAnyWorld();
    if (!Subsystem)
    {
        if (ShouldLogVerbose(Settings))
        {
            UE_LOG(LogSOTS_INVCoreBridge, Verbose, TEXT("INV CoreBridge: ApplySaveFragment could not resolve subsystem. Profile=%s Slot=%s"), *Ctx.ProfileId, *Ctx.SlotId);
        }
        return false;
    }

    FSOTS_InventoryProfileData Snapshot;
    if (!DeserializeInventoryProfileDataFromBytes(In.Data, Snapshot))
    {
        if (ShouldLogVerbose(Settings))
        {
            UE_LOG(LogSOTS_INVCoreBridge, Verbose, TEXT("INV CoreBridge: ApplySaveFragment could not deserialize payload. Profile=%s Slot=%s"), *Ctx.ProfileId, *Ctx.SlotId);
        }
        return false;
    }

    Subsystem->ApplyProfileData(Snapshot);

    if (ShouldLogVerbose(Settings))
    {
        UE_LOG(LogSOTS_INVCoreBridge, Verbose, TEXT("INV CoreBridge: ApplySaveFragment ok Bytes=%d"), In.Data.Num());
    }

    return true;
}
