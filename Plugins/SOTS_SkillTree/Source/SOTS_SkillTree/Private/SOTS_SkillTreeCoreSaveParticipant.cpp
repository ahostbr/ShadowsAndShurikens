#include "SOTS_SkillTreeCoreSaveParticipant.h"

#include "SOTS_ProfileTypes.h"
#include "SOTS_SkillTreeCoreBridgeSettings.h"
#include "SOTS_SkillTreeSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_SkillTreeCoreBridge, Log, All);

namespace
{
    const USOTS_SkillTreeCoreBridgeSettings* GetSettings()
    {
        return GetDefault<USOTS_SkillTreeCoreBridgeSettings>();
    }

    bool IsBridgeEnabled(const USOTS_SkillTreeCoreBridgeSettings* Settings)
    {
        return Settings && Settings->bEnableSOTSCoreSaveParticipantBridge;
    }

    bool ShouldLogVerbose(const USOTS_SkillTreeCoreBridgeSettings* Settings)
    {
        return Settings && Settings->bEnableSOTSCoreBridgeVerboseLogs;
    }

    USOTS_SkillTreeSubsystem* ResolveSkillTreeSubsystemFromAnyWorld()
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

            if (USOTS_SkillTreeSubsystem* Subsystem = GameInstance->GetSubsystem<USOTS_SkillTreeSubsystem>())
            {
                return Subsystem;
            }
        }

        return nullptr;
    }

    bool SerializeSkillTreeProfileDataToBytes(const FSOTS_SkillTreeProfileData& Snapshot, TArray<uint8>& OutBytes)
    {
        OutBytes.Reset();

        FMemoryWriter Writer(OutBytes, true);
        FSOTS_SkillTreeProfileData::StaticStruct()->SerializeItem(Writer, const_cast<FSOTS_SkillTreeProfileData*>(&Snapshot), nullptr);

        return OutBytes.Num() > 0;
    }

    bool DeserializeSkillTreeProfileDataFromBytes(const TArray<uint8>& InBytes, FSOTS_SkillTreeProfileData& OutSnapshot)
    {
        if (InBytes.Num() <= 0)
        {
            return false;
        }

        FMemoryReader Reader(InBytes, true);
        FSOTS_SkillTreeProfileData::StaticStruct()->SerializeItem(Reader, &OutSnapshot, nullptr);
        return true;
    }
}

FName FSkillTree_SaveParticipant::GetParticipantId() const
{
    return TEXT("SkillTree");
}

FSOTS_SaveParticipantStatus FSkillTree_SaveParticipant::QuerySaveStatus(const FSOTS_SaveRequestContext& Ctx) const
{
    FSOTS_SaveParticipantStatus Status;

    const USOTS_SkillTreeCoreBridgeSettings* Settings = GetSettings();
    if (!IsBridgeEnabled(Settings))
    {
        return Status;
    }

    if (ShouldLogVerbose(Settings))
    {
        UE_LOG(LogSOTS_SkillTreeCoreBridge, Verbose, TEXT("SkillTree CoreBridge: QuerySaveStatus allow (Profile=%s Slot=%s Auto=%d Reason=%s)"), *Ctx.ProfileId, *Ctx.SlotId, Ctx.bIsAutoSave ? 1 : 0, *Ctx.Reason);
    }

    return Status;
}

bool FSkillTree_SaveParticipant::BuildSaveFragment(const FSOTS_SaveRequestContext& Ctx, FSOTS_SavePayloadFragment& Out)
{
    const USOTS_SkillTreeCoreBridgeSettings* Settings = GetSettings();
    if (!IsBridgeEnabled(Settings))
    {
        return false;
    }

    USOTS_SkillTreeSubsystem* SkillTree = ResolveSkillTreeSubsystemFromAnyWorld();
    if (!SkillTree)
    {
        if (ShouldLogVerbose(Settings))
        {
            UE_LOG(LogSOTS_SkillTreeCoreBridge, Verbose, TEXT("SkillTree CoreBridge: BuildSaveFragment could not resolve subsystem. Profile=%s Slot=%s"), *Ctx.ProfileId, *Ctx.SlotId);
        }
        return false;
    }

    FSOTS_SkillTreeProfileData Snapshot;
    SkillTree->BuildProfileData(Snapshot);

    TArray<uint8> Bytes;
    if (!SerializeSkillTreeProfileDataToBytes(Snapshot, Bytes))
    {
        if (ShouldLogVerbose(Settings))
        {
            UE_LOG(LogSOTS_SkillTreeCoreBridge, Verbose, TEXT("SkillTree CoreBridge: BuildSaveFragment produced empty payload. Profile=%s Slot=%s"), *Ctx.ProfileId, *Ctx.SlotId);
        }
        return false;
    }

    Out.FragmentId = TEXT("SkillTree.State");
    Out.Data = MoveTemp(Bytes);

    if (ShouldLogVerbose(Settings))
    {
        UE_LOG(LogSOTS_SkillTreeCoreBridge, Verbose, TEXT("SkillTree CoreBridge: BuildSaveFragment ok Unlocked=%d Points=%d Bytes=%d"), Snapshot.UnlockedSkillNodes.Num(), Snapshot.UnspentSkillPoints, Out.Data.Num());
    }

    return true;
}

bool FSkillTree_SaveParticipant::ApplySaveFragment(const FSOTS_SaveRequestContext& Ctx, const FSOTS_SavePayloadFragment& In)
{
    const USOTS_SkillTreeCoreBridgeSettings* Settings = GetSettings();
    if (!IsBridgeEnabled(Settings))
    {
        return false;
    }

    if (In.FragmentId != TEXT("SkillTree.State"))
    {
        if (ShouldLogVerbose(Settings))
        {
            UE_LOG(LogSOTS_SkillTreeCoreBridge, Verbose, TEXT("SkillTree CoreBridge: ApplySaveFragment ignored FragmentId=%s"), *In.FragmentId.ToString());
        }
        return false;
    }

    USOTS_SkillTreeSubsystem* SkillTree = ResolveSkillTreeSubsystemFromAnyWorld();
    if (!SkillTree)
    {
        if (ShouldLogVerbose(Settings))
        {
            UE_LOG(LogSOTS_SkillTreeCoreBridge, Verbose, TEXT("SkillTree CoreBridge: ApplySaveFragment could not resolve subsystem. Profile=%s Slot=%s"), *Ctx.ProfileId, *Ctx.SlotId);
        }
        return false;
    }

    FSOTS_SkillTreeProfileData Snapshot;
    if (!DeserializeSkillTreeProfileDataFromBytes(In.Data, Snapshot))
    {
        if (ShouldLogVerbose(Settings))
        {
            UE_LOG(LogSOTS_SkillTreeCoreBridge, Verbose, TEXT("SkillTree CoreBridge: ApplySaveFragment could not deserialize payload. Profile=%s Slot=%s"), *Ctx.ProfileId, *Ctx.SlotId);
        }
        return false;
    }

    SkillTree->ApplyProfileData(Snapshot);

    const TArray<FSOTS_SkillTreeRuntimeState> RuntimeStates = SkillTree->GetAllRuntimeStates();
    for (const FSOTS_SkillTreeRuntimeState& State : RuntimeStates)
    {
        SkillTree->OnSkillTreeStateChanged.Broadcast(State);
    }

    if (ShouldLogVerbose(Settings))
    {
        UE_LOG(LogSOTS_SkillTreeCoreBridge, Verbose, TEXT("SkillTree CoreBridge: ApplySaveFragment ok Unlocked=%d Points=%d Bytes=%d"), Snapshot.UnlockedSkillNodes.Num(), Snapshot.UnspentSkillPoints, In.Data.Num());
    }

    return true;
}
