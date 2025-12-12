#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_ParkourMirrorLibrary.generated.h"

/**
 * Mirror helper backed by NewMirrorDataTable.json.
 * Provides left/right bone pairing lookups for retarget/IK/mirroring logic.
 */
UCLASS()
class SOTS_PARKOUR_API USOTS_ParkourMirrorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Return the mirrored bone name if known; otherwise returns the input or a heuristic swap (_l/_r). */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Mirror")
	static FName GetMirroredBoneName(FName BoneName);

	/** Return both sides of a bone pair; OutPrimary is the input, OutMirror is the partner. */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Mirror")
	static bool GetMirrorPair(FName BoneName, FName& OutPrimary, FName& OutMirror);

private:
	static bool LoadMirrorMap();
	static const FName* FindMirror(FName BoneName);
};
