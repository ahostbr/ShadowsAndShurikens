// Copyright 2024 CAS. All Rights Reserved.


#include "TrailHunterComponent.h"

#include "TrailHunterDirector.h"
#include "Kismet/GameplayStatics.h"
#include "Engine.h"


// Sets default values for this component's properties
UTrailHunterComponent::UTrailHunterComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

// Called when the game starts
void UTrailHunterComponent::BeginPlay()
{
	Super::BeginPlay();

	const AController* plrCtrl_ = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (plrCtrl_)
	{
		if (plrCtrl_->IsLocalController() && GetNetMode() != NM_DedicatedServer)
		{
			GetWorld()->GetTimerManager().SetTimer(registerHandle, this, &UTrailHunterComponent::RegisterDelayTimer, registerDelayRate, false);
		}
	}
}

void UTrailHunterComponent::RegisterDelayTimer()
{
	TArray<AActor*> allActorsArr_;
	UGameplayStatics::GetAllActorsOfClass(this, ATrailHunterDirector::StaticClass(), allActorsArr_);

	if (allActorsArr_.IsValidIndex(0))
	{
		if (allActorsArr_[0])
		{
			trailHunterDirector = Cast<ATrailHunterDirector>(allActorsArr_[0]);
		}
	}

	if (!IsValid(trailHunterDirector))
	{
		if (bDebug)
		{
			GEngine->AddOnScreenDebugMessage(-1, registerDelayRate, FColor::Red, TEXT("Error - Trail Hunter Director is not found in the scene."));
		}
		GetWorld()->GetTimerManager().SetTimer(registerHandle, this, &UTrailHunterComponent::RegisterDelayTimer, registerDelayRate, false);
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(registerHandle);
	}
}


// Called every frame
void UTrailHunterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UTrailHunterComponent::BroadcastOnModeChanged() const
{
	OnModeChanged.Broadcast();
}

void UTrailHunterComponent::AddNewTrail(UMaterialInterface* trailDecalMaterial, FTransform decalTransform, FVector decalSize, FVector trailLocation)
{
	if (IsValid(trailHunterDirector) && IsValid(trailDecalMaterial))
	{
		trailHunterDirector->AddNewTrail(this, trailDecalMaterial, decalTransform, decalSize, trailLocation);
	}
}

void UTrailHunterComponent::AddCustomPaint(FVector trailLocation, float trailPower, float trailRadius, float trailDepth, float lifeTimeSecond)
{
	if (IsValid(trailHunterDirector))
	{
		FTH_OnlyDeformationStruct deformationStruct_;
		deformationStruct_.trailLocation = trailLocation;
		deformationStruct_.distanceTraceRadius = 10.f;
		deformationStruct_.distanceTrailDepth = trailDepth;
		deformationStruct_.distanceTrailPower = trailPower;
		deformationStruct_.distanceTrailRadius = trailRadius;
		deformationStruct_.lifeTime = lifeTimeSecond;

		trailHunterDirector->AddOnlyDeformationTrail(deformationStruct_);
		trailHunterDirector->RegisterNewTrail(trailLocation, 10.f, trailDepth, trailPower, trailRadius, true);
	}
}

void UTrailHunterComponent::SetVisionType(ETH_VisionType visionType)
{
	if (IsValid(trailHunterDirector))
	{
		trailHunterDirector->SetVisionType(visionType, this);
		currentVisionType = visionType;
		BroadcastOnModeChanged();
	}
}

void UTrailHunterComponent::SetTrailType(ETH_TrailType trailType)
{
	if (IsValid(trailHunterDirector))
	{
		trailHunterDirector->SetTrailType(trailType, this);
		currentTrailType = trailType;
		BroadcastOnModeChanged();
	}
}

void UTrailHunterComponent::GetTrailHunterMode(ETH_VisionType& visionType, ETH_TrailType& trailType) const
{
	visionType = currentVisionType;
	trailType = currentTrailType;
}
