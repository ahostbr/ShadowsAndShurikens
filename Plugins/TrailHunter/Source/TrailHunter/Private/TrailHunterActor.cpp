// Copyright 2024 CAS. All Rights Reserved.


#include "TrailHunterActor.h"

// Sets default values
ATrailHunterActor::ATrailHunterActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

void ATrailHunterActor::SetTrailOwner(AActor* owner)
{
	if (IsValid(owner))
	{
		trailOwner = owner;
	}
}

// Called when the game starts or when spawned
void ATrailHunterActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATrailHunterActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

AActor* ATrailHunterActor::GetTrailOwner() const
{
	return trailOwner;
}

