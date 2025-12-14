#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_ParkourTypes.h"
#include "SOTS_ParkourVerticalTraceLibrary.generated.h"

class ACharacter;
class UCharacterMovementComponent;

/**
 * Trace-related helper math that mirrors the CGF Blueprint functions.
 *
 * SPINE V3_23: vertical wall detect start-height logic.
 *
 * C++ counterpart to CGF "Get Vertical Wall Detect Start Heigh" for the
 * Parkour.State.NotBusy path:
 *   ReturnValue = ActorLocation.Z - SelectFloat(40, 70, bPickA = IsFalling)
 *
 * Climb branch will be added when climb/free-hang wiring is complete.
 */
UCLASS()
class SOTS_PARKOUR_API USOTS_ParkourVerticalTraceLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * Compute the Z start height for a "vertical wall detect" trace.
	 *
	 * @param Character     Owning character.
	 * @param MoveComp      Movement component (for IsFalling).
	 * @param ParkourState  Current parkour state (Idle/Entering/Active/Exiting).
	 */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Trace")
	static float GetVerticalWallDetectStartHeight(
		const ACharacter* Character,
		const UCharacterMovementComponent* MoveComp,
		ESOTS_ParkourState ParkourState);
};
