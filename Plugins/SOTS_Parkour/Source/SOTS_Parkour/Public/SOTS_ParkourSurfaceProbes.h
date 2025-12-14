#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_ParkourTypes.h"
#include "SOTS_ParkourSurfaceProbes.generated.h"

/**
 * Surface probe helpers that mirror the CGF Blueprint functions:
 *
 *   - Check Mantle Surface
 *   - Check Vault Surface
 *
 * These are thin, Blueprint-callable wrappers around:
 *   - USOTS_ParkourTraceMath::BuildMantleSurfaceTrace
 *   - USOTS_ParkourTraceMath::BuildVaultSurfaceTrace
 * plus a capsule sweep and optional debug drawing.
 *
 * Later SPINE passes will:
 *   - Wire these up directly in the C++ classification step.
 *   - Swap ECC_Visibility for the dedicated Parkour trace channel.
 */
UCLASS()
class SOTS_PARKOUR_API USOTS_ParkourSurfaceProbes : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * Run the CGF-equivalent mantle surface probe:
	 *  - Builds the mantle surface capsule settings from WarpTopPoint / CapsuleHalfHeight.
	 *  - Performs a single capsule sweep.
	 *  - Optionally draws debug geometry.
	 *
	 * Returns true if a valid blocking hit is found (OutHit is filled).
	 */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Trace", meta = (WorldContext = "WorldContextObject"))
	static bool ProbeMantleSurface(
		UObject* WorldContextObject,
		const FVector& WarpTopPoint,
		float CapsuleHalfHeight,
		FHitResult& OutHit,
		ECollisionChannel TraceChannel = ECC_Visibility,
		bool bDrawDebug = false,
		float DebugLifetime = 1.0f);

	/**
	 * Run the CGF-equivalent vault surface probe:
	 *  - Builds the vault surface capsule settings from WarpTopPoint / CapsuleHalfHeight.
	 *  - Performs a single capsule sweep.
	 *  - Optionally draws debug geometry.
	 *
	 * Returns true if a valid blocking hit is found (OutHit is filled).
	 */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Trace", meta = (WorldContext = "WorldContextObject"))
	static bool ProbeVaultSurface(
		UObject* WorldContextObject,
		const FVector& WarpTopPoint,
		float CapsuleHalfHeight,
		FHitResult& OutHit,
		ECollisionChannel TraceChannel = ECC_Visibility,
		bool bDrawDebug = false,
		float DebugLifetime = 1.0f);

	/** Drop hang primary sphere probe (radius 35). */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Trace", meta = (WorldContext = "WorldContextObject"))
	static bool ProbeDropHangPrimary(
		UObject* WorldContextObject,
		const FVector& Start,
		const FVector& End,
		FHitResult& OutHit,
		ECollisionChannel TraceChannel = ECC_Visibility,
		bool bDrawDebug = false,
		float DebugLifetime = 1.0f);

	/** Drop hang secondary sphere probe (radius 5). */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Trace", meta = (WorldContext = "WorldContextObject"))
	static bool ProbeDropHangSecondary(
		UObject* WorldContextObject,
		const FVector& Start,
		const FVector& End,
		FHitResult& OutHit,
		ECollisionChannel TraceChannel = ECC_Visibility,
		bool bDrawDebug = false,
		float DebugLifetime = 1.0f);

	/** Hop/vault confirmation capsule (radius 25, half-height provided). */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Trace", meta = (WorldContext = "WorldContextObject"))
	static bool ProbeCapsuleConfirm(
		UObject* WorldContextObject,
		const FVector& Start,
		const FVector& End,
		float Radius,
		float HalfHeight,
		FHitResult& OutHit,
		ECollisionChannel TraceChannel = ECC_Visibility,
		bool bDrawDebug = false,
		float DebugLifetime = 1.0f);

	/** TicTac side probes (radius 30, 5s debug). */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Trace", meta = (WorldContext = "WorldContextObject"))
	static bool ProbeTicTac(
		UObject* WorldContextObject,
		const FVector& Start,
		const FVector& End,
		FHitResult& OutHit,
		ECollisionChannel TraceChannel = ECC_Visibility,
		bool bDrawDebug = false,
		float DebugLifetime = 5.0f);
};
