// Copyright 2024 CAS. All Rights Reserved.


#include "TrailHunterVisualActor.h"

#include "TrailHunterDirector.h"

// Sets default values
ATrailHunterVisualActor::ATrailHunterVisualActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ATrailHunterVisualActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATrailHunterVisualActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATrailHunterVisualActor::SetTrack()
{
	if (thDirector)
	{
		thDirector->SetTrackComponent(thComponent, true);
	}
}

