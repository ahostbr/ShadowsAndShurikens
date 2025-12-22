#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_UDSBridgeSubsystem.h"
#include "SOTS_UDSBridgeBlueprintLib.generated.h"

UCLASS()
class SOTS_UDSBRIDGE_API USOTS_UDSBridgeBlueprintLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SOTS|UDSBridge", meta = (WorldContext = "WorldContextObject"))
	static bool GetUDSBridgeState(
		UObject* WorldContextObject,
		bool& bOutHasUDS,
		bool& bOutHasDLWE,
		bool& bOutHasGSM,
		bool& bOutSnowy,
		bool& bOutRaining,
		bool& bOutDusty,
		bool& bOutHasSunDir,
		FVector& OutSunDirWS);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UDSBridge", meta = (WorldContext = "WorldContextObject"))
	static void ForceUDSBridgeRefresh(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "SOTS|UDSBridge", meta = (WorldContext = "WorldContextObject"))
	static bool GetRecentUDSBreadcrumbs(UObject* WorldContextObject, int32 MaxCount, TArray<FSOTS_UDSBreadcrumb>& OutBreadcrumbs);
};
