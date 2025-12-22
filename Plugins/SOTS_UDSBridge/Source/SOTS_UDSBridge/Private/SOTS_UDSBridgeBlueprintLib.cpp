#include "SOTS_UDSBridgeBlueprintLib.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "SOTS_UDSBridgeSubsystem.h"

namespace
{
	UWorld* ResolveWorld(UObject* WorldContextObject)
	{
		return WorldContextObject ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	}
}

bool USOTS_UDSBridgeBlueprintLib::GetUDSBridgeState(
	UObject* WorldContextObject,
	bool& bOutHasUDS,
	bool& bOutHasDLWE,
	bool& bOutHasGSM,
	bool& bOutSnowy,
	bool& bOutRaining,
	bool& bOutDusty,
	bool& bOutHasSunDir,
	FVector& OutSunDirWS)
{
	UWorld* World = ResolveWorld(WorldContextObject);
	if (!World)
	{
		bOutHasUDS = false;
		bOutHasDLWE = false;
		bOutHasGSM = false;
		bOutSnowy = false;
		bOutRaining = false;
		bOutDusty = false;
		bOutHasSunDir = false;
		OutSunDirWS = FVector::ZeroVector;
		return false;
	}

	UGameInstance* GI = World->GetGameInstance();
	if (!GI)
	{
		bOutHasUDS = false;
		bOutHasDLWE = false;
		bOutHasGSM = false;
		bOutSnowy = false;
		bOutRaining = false;
		bOutDusty = false;
		bOutHasSunDir = false;
		OutSunDirWS = FVector::ZeroVector;
		return false;
	}

	USOTS_UDSBridgeSubsystem* Subsystem = GI->GetSubsystem<USOTS_UDSBridgeSubsystem>();
	if (!Subsystem)
	{
		bOutHasUDS = false;
		bOutHasDLWE = false;
		bOutHasGSM = false;
		bOutSnowy = false;
		bOutRaining = false;
		bOutDusty = false;
		bOutHasSunDir = false;
		OutSunDirWS = FVector::ZeroVector;
		return false;
	}

	return Subsystem->GetBridgeStateSnapshot(
		bOutHasUDS,
		bOutHasDLWE,
		bOutHasGSM,
		bOutSnowy,
		bOutRaining,
		bOutDusty,
		bOutHasSunDir,
		OutSunDirWS);
}

void USOTS_UDSBridgeBlueprintLib::ForceUDSBridgeRefresh(UObject* WorldContextObject)
{
	UWorld* World = ResolveWorld(WorldContextObject);
	if (!World)
	{
		return;
	}

	if (UGameInstance* GI = World->GetGameInstance())
	{
		if (USOTS_UDSBridgeSubsystem* Subsystem = GI->GetSubsystem<USOTS_UDSBridgeSubsystem>())
		{
			Subsystem->ForceRefreshAndApply();
		}
	}
}

bool USOTS_UDSBridgeBlueprintLib::GetRecentUDSBreadcrumbs(UObject* WorldContextObject, int32 MaxCount, TArray<FSOTS_UDSBreadcrumb>& OutBreadcrumbs)
{
	OutBreadcrumbs.Reset();

	UWorld* World = ResolveWorld(WorldContextObject);
	if (!World)
	{
		return false;
	}

	if (UGameInstance* GI = World->GetGameInstance())
	{
		if (USOTS_UDSBridgeSubsystem* Subsystem = GI->GetSubsystem<USOTS_UDSBridgeSubsystem>())
		{
			Subsystem->GetRecentBreadcrumbs(MaxCount, OutBreadcrumbs);
			return true;
		}
	}

	return false;
}
