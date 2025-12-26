#include "SOTS_GASCoreSaveParticipant.h"

#include "SOTS_GASCoreBridgeSettings.h"
#include "Subsystems/SOTS_AbilitySubsystem.h"

#include "SOTS_ProfileTypes.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogGASSaveBridge, Log, All);

namespace
{
	const USOTS_GASCoreBridgeSettings* GetSettings()
	{
		return USOTS_GASCoreBridgeSettings::Get();
	}

	bool IsBridgeEnabled(const USOTS_GASCoreBridgeSettings* Settings)
	{
		return Settings && Settings->bEnableSOTSCoreSaveParticipantBridge;
	}

	bool ShouldLogVerbose(const USOTS_GASCoreBridgeSettings* Settings)
	{
		return Settings && Settings->bEnableSOTSCoreBridgeVerboseLogs;
	}

	USOTS_AbilitySubsystem* ResolveAbilitySubsystemFromAnyWorld()
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

			UGameInstance* GI = World->GetGameInstance();
			if (!GI)
			{
				continue;
			}

			if (USOTS_AbilitySubsystem* AbilitySubsystem = GI->GetSubsystem<USOTS_AbilitySubsystem>())
			{
				return AbilitySubsystem;
			}
		}

		return nullptr;
	}

	bool SerializeAbilityProfileDataToBytes(const FSOTS_AbilityProfileData& In, TArray<uint8>& OutBytes)
	{
		OutBytes.Reset();
		FMemoryWriter Writer(OutBytes, true);
		FSOTS_AbilityProfileData::StaticStruct()->SerializeItem(Writer, (void*)&In, nullptr);
		return OutBytes.Num() > 0;
	}

	bool DeserializeAbilityProfileDataFromBytes(const TArray<uint8>& InBytes, FSOTS_AbilityProfileData& Out)
	{
		if (InBytes.Num() <= 0)
		{
			return false;
		}

		FMemoryReader Reader(InBytes, true);
		FSOTS_AbilityProfileData::StaticStruct()->SerializeItem(Reader, (void*)&Out, nullptr);
		return true;
	}
}

FName FGAS_SaveParticipant::GetParticipantId() const
{
	return TEXT("GAS");
}

bool FGAS_SaveParticipant::BuildSaveFragment(const FSOTS_SaveRequestContext& Ctx, FSOTS_SavePayloadFragment& Out)
{
	const USOTS_GASCoreBridgeSettings* Settings = GetSettings();
	if (!IsBridgeEnabled(Settings))
	{
		return false;
	}

	USOTS_AbilitySubsystem* AbilitySubsystem = ResolveAbilitySubsystemFromAnyWorld();
	if (!AbilitySubsystem)
	{
		return false;
	}

	FSOTS_AbilityProfileData ProfileData;
	AbilitySubsystem->BuildProfileData(ProfileData);

	TArray<uint8> Bytes;
	if (!SerializeAbilityProfileDataToBytes(ProfileData, Bytes))
	{
		return false;
	}

	Out.FragmentId = TEXT("GAS.State");
	Out.Data = MoveTemp(Bytes);

	if (ShouldLogVerbose(Settings))
	{
		UE_LOG(
			LogGASSaveBridge,
			Verbose,
			TEXT("GAS SaveBridge: Built fragment id=%s bytes=%d ProfileId=%s SlotId=%s Abilities=%d Ranks=%d Cooldowns=%d"),
			*Out.FragmentId.ToString(),
			Out.Data.Num(),
			*Ctx.ProfileId,
			*Ctx.SlotId,
			ProfileData.GrantedAbilityTags.Num(),
			ProfileData.AbilityRanks.Num(),
			ProfileData.CooldownsRemaining.Num());
	}

	return true;
}

bool FGAS_SaveParticipant::ApplySaveFragment(const FSOTS_SaveRequestContext& Ctx, const FSOTS_SavePayloadFragment& In)
{
	const USOTS_GASCoreBridgeSettings* Settings = GetSettings();
	if (!IsBridgeEnabled(Settings))
	{
		return false;
	}

	if (In.FragmentId != TEXT("GAS.State"))
	{
		return false;
	}

	USOTS_AbilitySubsystem* AbilitySubsystem = ResolveAbilitySubsystemFromAnyWorld();
	if (!AbilitySubsystem)
	{
		return false;
	}

	FSOTS_AbilityProfileData ProfileData;
	if (!DeserializeAbilityProfileDataFromBytes(In.Data, ProfileData))
	{
		return false;
	}

	AbilitySubsystem->ApplyProfileData(ProfileData);

	if (ShouldLogVerbose(Settings))
	{
		UE_LOG(
			LogGASSaveBridge,
			Verbose,
			TEXT("GAS SaveBridge: Applied fragment id=%s bytes=%d ProfileId=%s SlotId=%s Summary=%s"),
			*In.FragmentId.ToString(),
			In.Data.Num(),
			*Ctx.ProfileId,
			*Ctx.SlotId,
			*AbilitySubsystem->GetAbilityProfileSummary());
	}

	return true;
}
