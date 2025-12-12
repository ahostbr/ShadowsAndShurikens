// Copyright 2021-2024 CAS. All Rights Reserved.


#include "NPCEyesPointsPro.h"


#include "Components/LightComponent.h"


// Sets default values for this component's properties
UNPCEyesPointsPro::UNPCEyesPointsPro()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

USceneComponent* UNPCEyesPointsPro::GetComponentForSight() const
{
	return sceneComponentRef;
}

TArray<FName> UNPCEyesPointsPro::GetPointsName() const
{
	return pointsNameArr;
}

TArray<USceneComponent*> UNPCEyesPointsPro::GetLights() const
{
	return lightComponentsArr;
}


// Called when the game starts
void UNPCEyesPointsPro::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void UNPCEyesPointsPro::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UNPCEyesPointsPro::SetSkeletalMesh_BP(USceneComponent* setSkeletalMesh)
{
	sceneComponentRef = setSkeletalMesh;
}

void UNPCEyesPointsPro::SetComponentForSight_BP(USceneComponent* setSceneComponent)
{
	sceneComponentRef = setSceneComponent;
}

void UNPCEyesPointsPro::AddLight(USceneComponent* newLight)
{
	if (newLight)
	{
		lightComponentsArr.AddUnique(newLight);
	}
}

TArray<USceneComponent*> UNPCEyesPointsPro::GetAllLights()
{
	return lightComponentsArr;
}

void UNPCEyesPointsPro::ClearLights()
{
	lightComponentsArr.Empty();
}

