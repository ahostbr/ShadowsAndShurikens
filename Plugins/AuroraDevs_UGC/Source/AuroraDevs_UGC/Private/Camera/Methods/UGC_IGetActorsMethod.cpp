// Copyright(c) Aurora Devs 2024. All Rights Reserved.

#include "Camera/Methods/UGC_IGetActorsMethod.h"
#include "Engine/World.h"

void UUGC_IGetActorsMethod::GetActors_Implementation(AActor* InOwner, FVector OwnerLocation, FVector ViewPointLocation, FRotator ViewPointRotation, TArray<AActor*>& OutActors)
{
}

UWorld* UUGC_IGetActorsMethod::GetWorld() const
{
    if (GWorld && GWorld->IsGameWorld() && GWorld->HasBegunPlay())
    {
        return GWorld;
    }
    return nullptr;
}