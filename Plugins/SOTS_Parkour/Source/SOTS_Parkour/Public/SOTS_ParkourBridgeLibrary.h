#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_ParkourTypes.h"
#include "SOTS_ParkourBridgeLibrary.generated.h"

class USOTS_ParkourComponent;

/**
 * Lightweight bridge helpers so the rest of SOTS (TagManager, GSM, Stats, MissionDirector)
 * can query parkour state/action in a stable, label-based way without depending on internal
 * implementation details of USOTS_ParkourComponent.
 *
 * Intentionally very thin:
 *  - Turns ESOTS_ParkourState into an FName label.
 *  - Turns ESOTS_ParkourAction into an FName label.
 *  - Turns ESOTS_ParkourResultType into an FName label.
 *  - Turns ESOTS_ClimbStyle into an FName label.
 *  - Exposes a simple “has valid result?” bit.
 *
 * This keeps all the “what string/label should other systems see?” logic in one place.
 */
UCLASS()
class SOTS_PARKOUR_API USOTS_ParkourBridgeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Returns a simple FName label for the current high-level parkour state. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Bridge")
	static FName GetParkourStateLabel(const USOTS_ParkourComponent* ParkourComponent);

	/** Returns a simple FName label for the last classified parkour action, if any. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Bridge")
	static FName GetParkourActionLabel(const USOTS_ParkourComponent* ParkourComponent);

	/** Returns a simple FName label for the coarse result type (Mantle / Drop / None). */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Bridge")
	static FName GetParkourResultTypeLabel(const USOTS_ParkourComponent* ParkourComponent);

	/** Returns a simple FName label for the climb style (FreeHang / Braced / None). */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Bridge")
	static FName GetParkourClimbStyleLabel(const USOTS_ParkourComponent* ParkourComponent);

	/**
	 * True if the component currently has a valid classified parkour result.
	 *
	 * Uses both:
	 *  - Result.bHasResult  (legacy v0.2 path)
	 *  - Result.bIsValid    (new trace-driven path)
	 */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Bridge")
	static bool HasValidParkourResult(const USOTS_ParkourComponent* ParkourComponent);

	/** Returns all runtime warp targets (with start/end times) for AnimBP gating. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Bridge")
	static TArray<FSOTS_ParkourWarpRuntimeTarget> GetParkourWarpTargets(const USOTS_ParkourComponent* ParkourComponent);
};
