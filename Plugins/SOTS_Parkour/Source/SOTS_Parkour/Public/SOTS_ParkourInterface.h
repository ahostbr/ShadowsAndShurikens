#pragma once

#include "CoreMinimal.h"
#include "SOTS_ParkourTypes.h"
#include "SOTS_ParkourInterface.generated.h"

/** Interface for gameplay actors that want parkour state/result notifications. Mirrors the original BP ParkourInterface. */
UINTERFACE(Blueprintable)
class SOTS_PARKOUR_API USOTS_ParkourInterface : public UInterface
{
	GENERATED_BODY()
};

class SOTS_PARKOUR_API ISOTS_ParkourInterface
{
	GENERATED_BODY()

public:
	/** Called when the parkour result is recomputed. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SOTS|Parkour")
	void OnParkourResultUpdated(const FSOTS_ParkourResult& Result);

	/** Called when the parkour state transitions. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SOTS|Parkour")
	void OnParkourStateChanged(ESOTS_ParkourState NewState);
};

/** Interface for AnimBlueprints that need parkour IK/warp data. Mirrors the original BP ParkourABPInterface. */
UINTERFACE(Blueprintable)
class SOTS_PARKOUR_API USOTS_ParkourABPInterface : public UInterface
{
	GENERATED_BODY()
};

class SOTS_PARKOUR_API ISOTS_ParkourABPInterface
{
	GENERATED_BODY()

public:
	/** Called when the parkour result is recomputed; provides hand IK and warp data. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SOTS|Parkour")
	void OnParkourResultUpdated(const FSOTS_ParkourResult& Result);

	/** Called when the parkour state transitions. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SOTS|Parkour")
	void OnParkourStateChanged(ESOTS_ParkourState NewState);

	/** Set the current parkour state tag (mirrors ParkourABPInterface.Set Parkour State). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SOTS|Parkour")
	void SetParkourStateTag(FGameplayTag StateTag);

	/** Set the current parkour action tag. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SOTS|Parkour")
	void SetParkourActionTag(FGameplayTag ActionTag);

	/** Set the current climb style tag. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SOTS|Parkour")
	void SetClimbStyleTag(FGameplayTag ClimbStyleTag);

	/** Update climb movement vector (used by ABP for offsets). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SOTS|Parkour")
	void SetClimbMovement(const FVector& ClimbMovement);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SOTS|Parkour")
	void SetLeftHandLedgeLocation(const FVector& Location);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SOTS|Parkour")
	void SetRightHandLedgeLocation(const FVector& Location);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SOTS|Parkour")
	void SetLeftHandLedgeRotation(const FRotator& Rotation);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SOTS|Parkour")
	void SetRightHandLedgeRotation(const FRotator& Rotation);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SOTS|Parkour")
	void SetLeftFootLocation(const FVector& Location);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SOTS|Parkour")
	void SetRightFootLocation(const FVector& Location);

	/** Toggle climb IK per hand, mirroring ReachLedgeIK notify (First/Second IK). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SOTS|Parkour")
	void SetLeftClimbIK(bool bEnable);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SOTS|Parkour")
	void SetRightClimbIK(bool bEnable);
};
