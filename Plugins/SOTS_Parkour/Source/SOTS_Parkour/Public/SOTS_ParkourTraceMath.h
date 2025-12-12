#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_ParkourTypes.h"
#include "SOTS_ParkourTraceMath.generated.h"

/**
 * USOTS_ParkourTraceMath
 *
 * Pure math helpers for constructing FSOTS_ParkourCapsuleTraceSettings
 * from CGF-style warp points and capsule dimensions.
 *
 * These functions are the C++ mirror of the Blueprint graphs:
 *   - Check Mantle Surface
 *   - Check Vault Surface
 *
 * The numeric values used here (±8, +18, +5, Radius 25) come directly
 * from the original CGF Parkour graphs / captured screenshots.
 */
UCLASS()
class SOTS_PARKOUR_API USOTS_ParkourTraceMath : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * Build a capsule trace around the mantle warp top point.
	 *
	 * CGF parity notes:
	 *   - Uses WarpTopPoint as the base.
	 *   - Offsets the capsule center by CapsuleHalfHeight ± 8 on Z.
	 *   - Uses Radius = 25 (ParkourTraceChannel).
	 *
	 * This is used to confirm that there is valid surface around the
	 * mantle target location before committing to the action.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SOTS|Parkour|Trace")
	static void BuildMantleSurfaceTrace(
		const FVector& WarpTopPoint,
		float CapsuleHalfHeight,
		FSOTS_ParkourCapsuleTraceSettings& OutSettings
	);

	/**
	 * Build a capsule trace around the vault warp top point.
	 *
	 * CGF parity notes:
	 *   - Uses WarpTopPoint as the base.
	 *   - Uses CapsuleHalfHeight + 18 as the upper sample.
	 *   - Uses CapsuleHalfHeight + 5  as the lower sample.
	 *   - Radius is again fixed at 25.
	 *
	 * This is used to confirm that there is valid surface for the
	 * vault contact point.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SOTS|Parkour|Trace")
	static void BuildVaultSurfaceTrace(
		const FVector& WarpTopPoint,
		float CapsuleHalfHeight,
		FSOTS_ParkourCapsuleTraceSettings& OutSettings
	);

private:

	/** Small internal helper so both mantle/vault can share the same pattern. */
	static void BuildVerticalSegmentCapsule(
		const FVector& WarpTopPoint,
		float CapsuleHalfHeight,
		float StartZOffset,
		float EndZOffset,
		float Radius,
		FSOTS_ParkourCapsuleTraceSettings& OutSettings
	);
};
