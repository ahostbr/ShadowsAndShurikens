// Copyright(c) Aurora Devs 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UGC_PawnInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UUGC_PawnInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * Pawn Interface used to get the movement/rotation input values.
 */
class AURORADEVS_UGC_API IUGC_PawnInterface
{
    GENERATED_BODY()

public:
    // Get the value of the camera rotation input (usually the right thumbstick or the mouse).
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "UGC|Camera Interface")
    FRotator GetRotationInput() const;

    // Get the value of the movement input (usually WASD or the left thumbstick).
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "UGC|Camera Interface")
    FVector GetMovementInput() const;
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UUGC_PawnMovementInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * This interface should only be used with player classes NOT using the default Unreal Movement Component or components inheriting from it.
 * This interface can be used to query important movement properties (velocity, falling; etc.) from your custom movement components.
 */
class AURORADEVS_UGC_API IUGC_PawnMovementInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "UGC|Movement Interface")
    FVector GetOwnerVelocity() const;

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "UGC|Movement Interface")
    bool IsOwnerFalling() const;

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "UGC|Movement Interface")
    bool IsOwnerStrafing() const;

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "UGC|Movement Interface")
    bool IsOwnerMovingOnGround() const;
};