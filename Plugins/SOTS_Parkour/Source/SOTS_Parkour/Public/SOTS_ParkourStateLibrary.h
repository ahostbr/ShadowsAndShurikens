#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_ParkourTypes.h"
#include "SOTS_ParkourStateLibrary.generated.h"

class AActor;
class USOTS_ParkourComponent;

/**
 * Blueprint-facing helpers for querying and driving SOTS parkour state.
 *
 * This mirrors the role of the original CGF Blueprint glue:
 *  - “Get ParkourComp from Character”
 *  - “Is Parkour Active?”
 *  - “Try Perform Parkour (from BP)”
 *
 * Centralising these helpers here keeps the component itself focused
 * on runtime logic while Blueprints get a clean, discoverable surface.
 */
UCLASS()
class SOTS_PARKOUR_API USOTS_ParkourStateLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Return the SOTS_Parkour component on the given actor, if any. */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour", meta = (DefaultToSelf = "Actor"))
	static USOTS_ParkourComponent* GetParkourComponentFromActor(AActor* Actor);

	/** Read the current parkour state for the given actor (Idle / Entering / Active / Exiting). */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|State", meta = (DefaultToSelf = "Actor"))
	static ESOTS_ParkourState GetParkourStateFromActor(AActor* Actor);

	/** Convenience: true when the actor's parkour state is Active. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|State", meta = (DefaultToSelf = "Actor"))
	static bool IsParkourActive(AActor* Actor);

	/**
	 * Try to perform parkour on the given actor:
	 *  - Finds the SOTS_Parkour component (if present).
	 *  - Calls TryPerformParkour().
	 *
	 * Safe to call from input graphs; does nothing if the component is missing.
	 */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour", meta = (DefaultToSelf = "Actor"))
	static void TryPerformParkourOnActor(AActor* Actor);
};
