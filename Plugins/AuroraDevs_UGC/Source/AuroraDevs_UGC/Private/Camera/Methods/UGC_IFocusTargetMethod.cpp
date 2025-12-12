// Copyright(c) Aurora Devs 2024. All Rights Reserved.

#include "Camera/Methods/UGC_IFocusTargetMethod.h"
#include "Engine/World.h"

AActor* UUGC_IFocusTargetMethod::GetTargetLocation_Implementation(AActor* InOwner, FVector OwnerLocation, FVector ViewPointLocation, FRotator ViewPointRotation, FVector& OutTargetLocation)
{
    return nullptr;
}

UWorld* UUGC_IFocusTargetMethod::GetWorld() const
{
    if (GWorld && GWorld->IsGameWorld() && GWorld->HasBegunPlay())
    {
        return GWorld;
    }
    return nullptr;
}