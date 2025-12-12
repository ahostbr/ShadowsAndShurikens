// Copyright 2024 CAS. All Rights Reserved.

#include "TrailHunterSceneComponent.h"

#include "TrailHunterDirector.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values for this component's properties
UTrailHunterSceneComponent::UTrailHunterSceneComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


void UTrailHunterSceneComponent::GetLastHitLocation(FVector& location, bool& hitBlock)
{
	location = lastHitLocation;
	hitBlock = bLastHitBlock;
}

void UTrailHunterSceneComponent::GetLastDeformationLocation(FVector& location)
{
	location = lastDeformationLocation;
}

// Called when the game starts
void UTrailHunterSceneComponent::BeginPlay()
{
	Super::BeginPlay();

	bActivatePaint = bActivatePaintOnBeginPlay;

	const AController* plrCtrl_ = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (plrCtrl_)
	{
		if (plrCtrl_->IsLocalController() && GetNetMode() != NM_DedicatedServer)
		{
			distanceBetweenPaintSquare = FMath::Square(distanceBetweenPaint);
			lastCheckLocation = GetComponentLocation();
			GetWorld()->GetTimerManager().SetTimer(registerHandle, this, &UTrailHunterSceneComponent::RegisterDelayTimer, registerDelayRate, false);
		}
	}
}

void UTrailHunterSceneComponent::RegisterDelayTimer()
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
		GetWorld()->GetTimerManager().SetTimer(registerHandle, this, &UTrailHunterSceneComponent::RegisterDelayTimer, registerDelayRate, false);
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(registerHandle);
		GetWorld()->GetTimerManager().SetTimer(checkTrailHandle, this, &UTrailHunterSceneComponent::CheckTrailTimer, checkTrailRate, true);
	}
}

FHitResult UTrailHunterSceneComponent::SphereTrace(const FVector& startV, const FVector& endV, float sphereRadius) const
{
	FCollisionQueryParams traceParams_(TEXT("Trail Trace"), false);

	traceParams_.bReturnPhysicalMaterial = false;
	traceParams_.AddIgnoredActor(GetOwner());

	const FQuat actorQuat_ = GetOwner()->GetActorQuat();

	FHitResult hitResult_;
	GetWorld()->SweepSingleByObjectType(hitResult_, startV, endV, actorQuat_, traceObjectTypesArr, FCollisionShape::MakeSphere(sphereRadius), traceParams_);
	return hitResult_;
}

void UTrailHunterSceneComponent::CheckTrailTimer()
{
	if (!IsValid(trailHunterDirector) || !bActivatePaint)
	{
		return;
	}

	FVector compLoc_ = GetComponentLocation();
	FVector startCheck_ = compLoc_;
	FVector endCheck_ = compLoc_;
	endCheck_.Z -= trailEndOffset;
	startCheck_.Z += trailStartOffset;

	if (bDebug)
	{
		DrawDebugLine(GetWorld(), startCheck_, endCheck_, FColor::Green, false, checkTrailRate, 0.f, 1.f);
	}

	FHitResult hitResult_ = SphereTrace(startCheck_, endCheck_, traceRadius);
	if (hitResult_.bBlockingHit)
	{
		//	float depth_ = FMath::Max(0.f,(25.f + 10.f * 2.f) - hitResult_.Distance);
		trailHunterDirector->RegisterNewTrail(hitResult_.Location, trailRadius, trailDepth, trailPower, trailRadius);

		if (bDebug)
		{
			DrawDebugSphere(GetWorld(), hitResult_.Location, traceRadius, 8, FColor::Red, false, checkTrailRate, 0.f, 1.f);
		}

		bLastHitBlock = true;
		lastHitLocation = hitResult_.ImpactPoint;
		lastDeformationLocation = hitResult_.Location;
	}
	else
	{
		bLastHitBlock = false;
	}
}

void UTrailHunterSceneComponent::PaintFarTrailByDistance()
{
	if (IsValid(trailHunterDirector))
	{
		if ((lastCheckLocation - GetComponentLocation()).SizeSquared() > distanceBetweenPaintSquare)
		{
			lastCheckLocation = GetComponentLocation();

			FTH_OnlyDeformationStruct deformationStruct_;
			deformationStruct_.trailLocation = GetComponentLocation();
			deformationStruct_.distanceTraceRadius = 10.f;
			deformationStruct_.distanceTrailDepth = distanceTrailDepth;
			deformationStruct_.distanceTrailPower = distanceTrailPower;
			deformationStruct_.distanceTrailRadius = distanceTrailRadius;
			deformationStruct_.lifeTime = distanceLifeTimeSecond;
		
			trailHunterDirector->AddOnlyDeformationTrail(deformationStruct_);
		}
	}
}


// Called every frame
void UTrailHunterSceneComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bPaintFarTrailByDistance && bActivatePaint)
	{
		PaintFarTrailByDistance();
	}
}

void UTrailHunterSceneComponent::ActivatePaint(bool bActivate)
{
	bActivatePaint = bActivate;
}
