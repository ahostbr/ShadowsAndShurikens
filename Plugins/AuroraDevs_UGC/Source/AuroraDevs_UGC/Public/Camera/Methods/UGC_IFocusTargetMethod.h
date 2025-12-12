// Copyright(c) Aurora Devs 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "UGC_IFocusTargetMethod.generated.h"

/**
 * Function which retrieves the target we want the camera to look at. (Uses the Strategy Design Pattern)
 */
UCLASS(abstract, Category = "UGC|Methods", EditInlineNew, Blueprintable)
class AURORADEVS_UGC_API UUGC_IFocusTargetMethod : public UObject
{
    GENERATED_BODY()
public:
    /*
     * Get the location of the target we want the camera to look at.
     * @param   Owner               The owner of the camera.
     * @param   OwnerLocation       The world location of the owner of camera.
     * @param   ViewPointLocation   Camera's location.
     * @param   ViewPointRotation   Camera's rotation.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "UGC|Methods")
    AActor* GetTargetLocation(class AActor* InOwner, FVector OwnerLocation, FVector ViewPointLocation, FRotator ViewPointRotation, FVector& OutTargetLocation);

private:
    /** Getter for the cached world pointer, will return null if the actor is not actually spawned in a level */
    virtual UWorld* GetWorld() const override;
};