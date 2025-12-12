// Copyright 2024 CAS. All Rights Reserved.


#include "TrailHunterDirector.h"

#include "TrailHunterActor.h"
#include "TrailHunterComponent.h"
#include "TrailHunterVisualActor.h"
#include "Components/DecalComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"

// Sets default values
ATrailHunterDirector::ATrailHunterDirector()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
}

// Called when the game starts or when spawned
void ATrailHunterDirector::BeginPlay()
{
	Super::BeginPlay();
	
	const AController* plrCtrl_ = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (plrCtrl_)
	{
		if (plrCtrl_->IsLocalController() && GetNetMode() != NM_DedicatedServer)
		{
			InitializeTrailDirector();
			GetWorld()->GetTimerManager().SetTimer(workerHandle, this, &ATrailHunterDirector::TrailWorkerTimer, trailHunterWorkerRate, true);
		}
	}
}

void ATrailHunterDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), trailsRenderTarget, FColor::Black);
	UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), historyRenderTarget, FColor::Black);
	UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), currentRenderTarget, FColor::Black);
}

// Called every frame
void ATrailHunterDirector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const AController* plrCtrl_ = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (plrCtrl_)
	{
		if (plrCtrl_->IsLocalController() && GetNetMode() != NM_DedicatedServer)
		{
			CalculateRenderTick(DeltaTime);
		}
	}

	QueueSpawn();
	QueueTrail();
}

void ATrailHunterDirector::RegisterNewTrail(const FVector& location, const float& radius, const float& depth, const float& power, const float& trailRadius, bool bFarTrail)
{
	if (!bIsLocationCalculated)
	{
		CalculateLocation();
	}

	FVector outSideV_ = location - currentLocation;
	float outSideVMax_ = FMath::Max(FMath::Abs(outSideV_.X), FMath::Abs(outSideV_.Y));
	if (outSideVMax_ - radius < maxPaintDistance)
	{
		FVector tempCalc_(FVector::ZeroVector);
		tempCalc_.X = outSideV_.X;
		tempCalc_.Y = outSideV_.Y;
		FVector2D trailLocation_ = UKismetMathLibrary::Conv_VectorToVector2D(tempCalc_ / maxDistanceMultiply + FVector(0.5f, 0.5f, 0.f));

		FTH_PaintTrailStruct paintTrailStruct_;
		paintTrailStruct_.trailLocation = trailLocation_;
		paintTrailStruct_.trailDepth = depth;
		paintTrailStruct_.trailRadius = trailRadius;
		paintTrailStruct_.trailPower = power;
		paintTrailStruct_.bIsFarTrail = bFarTrail;

		paintTrailArr.Add(paintTrailStruct_);
	}
}

bool ATrailHunterDirector::IsPointInPaintRadius(const FVector& location) const
{
	APlayerController* plrCtrl_ = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (plrCtrl_)
	{
		if (IsValid(plrCtrl_->GetPawn()))
		{
			if ((plrCtrl_->GetPawn()->GetActorLocation() - location).SizeSquared() < maxPaintDistanceOffsetSquare)
			{
				return true;
			}
		}
	}
	return false;
}

void ATrailHunterDirector::TrailWorkerTimer()
{
	for (int baseID_ = trailsArr.Num() - 1; baseID_ >= 0; --baseID_)
	{
		for (int trailID_ = trailsArr[baseID_].lifeTimeArr.Num() - 1; trailID_ >= 0; --trailID_)
		{
			if (trailsArr[baseID_].lifeTimeArr[trailID_] > 0.f)
			{
				if (GetWorld()->GetTimeSeconds() - trailsArr[baseID_].lifeTimeArr[trailID_] > trailsArr[baseID_].spawnTimeArr[trailID_])
				{
					if (IsValid(trailsArr[baseID_].decalsArr[trailID_]))
					{
						trailsArr[baseID_].decalsArr[trailID_]->DestroyComponent();
					}
					trailsArr[baseID_].decalsArr.RemoveAt(trailID_);
					trailsArr[baseID_].lifeTimeArr.RemoveAt(trailID_);
					trailsArr[baseID_].spawnTimeArr.RemoveAt(trailID_);
					trailsArr[baseID_].bFarCalculatedArr.RemoveAt(trailID_);

					trailsArr[baseID_].trailLocationArr.RemoveAt(trailID_);

					if (trailsArr[baseID_].visualArr[trailID_].bSpawned)
					{
						trailsArr[baseID_].visualArr[trailID_].bSpawned = false;
						if (trailsArr[baseID_].visualArr[trailID_].visualActor)
						{
							trailsArr[baseID_].visualArr[trailID_].visualActor->Destroy();
							trailsArr[baseID_].visualArr[trailID_].visualActor = nullptr;
						}
					}
					trailsArr[baseID_].visualArr.RemoveAt(trailID_);
				}
			}
		}

		if (trailsArr[baseID_].trailLocationArr.Num() == 0)
		{
			if (IsValid(trailsArr[baseID_].trailHunterActor))
			{
				trailsArr[baseID_].trailHunterActor->Destroy();
			}

			trailsArr.RemoveAt(baseID_);
		}
	}

	DistantTrailsVisualTimer();
}

void ATrailHunterDirector::ChangeDecalType(UMaterialInstanceDynamic* dynamicMaterial, ETH_TrailType trailType) const
{
	switch (trailType)
	{
	case ETH_TrailType::ACTIVE:
		dynamicMaterial->SetVectorParameterValue("Color", decalColorParameters.activeTrailColor);
		dynamicMaterial->SetScalarParameterValue("Emissive", decalColorParameters.activeEmissive);
		break;
	case ETH_TrailType::NORMAL:
		dynamicMaterial->SetVectorParameterValue("Color", decalColorParameters.normalTrailColor);
		dynamicMaterial->SetScalarParameterValue("Emissive", decalColorParameters.normalEmissive);
		break;
	case ETH_TrailType::ACTIVE_OTHER:
		dynamicMaterial->SetVectorParameterValue("Color", decalColorParameters.otherTrailColor);
		dynamicMaterial->SetScalarParameterValue("Emissive", decalColorParameters.otherEmissive);
		break;
	default:
		dynamicMaterial->SetVectorParameterValue("Color", decalColorParameters.normalTrailColor);
		dynamicMaterial->SetScalarParameterValue("Emissive", decalColorParameters.normalEmissive);
		break;
	}
}

void ATrailHunterDirector::CalculateLocation()
{
	APlayerController* plrCtrl_ = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (plrCtrl_)
	{
		FVector tempVec_ = plrCtrl_->PlayerCameraManager->GetCameraLocation() / locationCalculateHelper;
		FIntVector truncVec_ = UKismetMathLibrary::FTruncVector(tempVec_);
		FVector convertVec_ = UKismetMathLibrary::Conv_IntVectorToVector(truncVec_);
		convertVec_ *= locationCalculateHelper;
		currentLocation = convertVec_;

		//currentLocation = UKismetMathLibrary::Conv_IntVectorToVector(UKismetMathLibrary::FTruncVector(plrCtrl_->PlayerCameraManager->GetCameraLocation() / locationCalculateHelper)) * locationCalculateHelper;
		bIsLocationCalculated = true;
	}
}

void ATrailHunterDirector::InitializeTrailDirector()
{
	maxDistanceMultiply = maxPaintDistance * 2.f;
	maxPaintDistanceSquare = FMath::Square(maxPaintDistance);
	maxPaintDistanceOffset = maxPaintDistance * 0.9f;
	maxPaintDistanceOffsetSquare = FMath::Square(maxPaintDistanceOffset);
	trailVisualDistanceSquare = FMath::Square(maxDistanceToVisualActor);
	trailVisualDistanceOffsetSquare = FMath::Square(maxDistanceToVisualActor + maxDistanceToVisualActor * 0.1f);

	maxDistanceBetweenTrailSquare = FMath::Square(maxDistanceBetweenTrail);

	trailPaintInstance = UKismetMaterialLibrary::CreateDynamicMaterialInstance(GetWorld(), trailPaint);
	trailHistoryMergeInstance = UKismetMaterialLibrary::CreateDynamicMaterialInstance(GetWorld(), trailMerge);
	trailHistoryCopyInstance = UKismetMaterialLibrary::CreateDynamicMaterialInstance(GetWorld(), trailHistory);

	// Release resource.
	if (currentRenderTarget)
	{
		currentRenderTarget->ReleaseResource();
	}
	if (historyRenderTarget)
	{
		historyRenderTarget->ReleaseResource();
	}

	// Create additional render targets.
	if (trailsRenderTarget->SizeX == trailsRenderTarget->SizeY)
	{
		historyRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), trailsRenderTarget->SizeX,
		                                                                    trailsRenderTarget->SizeY, trailsRenderTarget->RenderTargetFormat, FColor::Black, false, false);

		currentRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), trailsRenderTarget->SizeX, trailsRenderTarget->SizeY,
		                                                                    trailsRenderTarget->RenderTargetFormat, FColor::Black, false, false);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error - trailsRenderTarget."));
	}

	// Set parameters and values.
	trailHistoryMergeInstance->SetTextureParameterValue("HistoryBuffer", historyRenderTarget);
	trailHistoryMergeInstance->SetTextureParameterValue("CurrentBuffer", currentRenderTarget);

	trailHistoryCopyInstance->SetTextureParameterValue("HistoryBuffer", trailsRenderTarget);
	trailHistoryCopyInstance->SetScalarParameterValue("Preservation", 5.0f);

	locationCalculateHelper = maxDistanceMultiply / trailsRenderTarget->SizeX;

	// Set Material Parameters Collection. 
	UKismetMaterialLibrary::SetScalarParameterValue(GetWorld(), mpcTrailHunter, "TrailScale", 1.f / maxDistanceMultiply);
	UKismetMaterialLibrary::SetScalarParameterValue(GetWorld(), mpcTrailHunter, "TrailSideFade", 1.f / FMath::Max(0.0001f, trailSideFade));
}

void ATrailHunterDirector::CalculateRenderTick(float& deltaTime)
{
	if (!bIsLocationCalculated)
	{
		CalculateLocation();
	}

	UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), currentRenderTarget, FColor::Black);

	if (!paintTrailArr.IsEmpty())
	{
		RenderTrails();
	}

	// Merge.
	UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), trailsRenderTarget, FColor::Black);
	//	trailHistoryMergeInstance->SetScalarParameterValue("Preservation", FMath::Clamp(1.f - (trailsAttenuation * deltaTime), 0.f,1.f));
	FLinearColor historyOffset_ = UKismetMathLibrary::Conv_VectorToLinearColor((historyLocation - currentLocation) / maxDistanceMultiply);
	trailHistoryMergeInstance->SetVectorParameterValue("HistoryOffset", historyOffset_);

	UCanvas* canvas_;
	FVector2D size_;
	FDrawToRenderTargetContext context_;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(GetWorld(), trailsRenderTarget, canvas_, size_, context_);

	canvas_->K2_DrawMaterial(trailHistoryMergeInstance, FVector2D(0.f, 0.f), size_, FVector2D(0.f, 0.f), FVector2D(1.f, 1.f), 0.f, FVector2D(0.5f, 0.5f));
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), context_);

	// Copy.
	UCanvas* canvasCopy_;
	FVector2D sizeCopy_;
	FDrawToRenderTargetContext contextCopy_;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(GetWorld(), historyRenderTarget, canvasCopy_, sizeCopy_, contextCopy_);

	canvasCopy_->K2_DrawMaterial(trailHistoryCopyInstance, FVector2D(0.f, 0.f), sizeCopy_, FVector2D(0.f, 0.f), FVector2D(1.f, 1.f), 0.f, FVector2D(0.5f, 0.5f));
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), contextCopy_);

	historyLocation = currentLocation;

	// Clear.
	if (!paintTrailArr.IsEmpty())
	{
		paintTrailArr.Empty();
	}

	bIsLocationCalculated = false;
	UKismetMaterialLibrary::SetVectorParameterValue(GetWorld(), mpcTrailHunter, "TrailTextureLocation", UKismetMathLibrary::Conv_VectorToLinearColor(currentLocation));


	// trailMergeInstance->SetScalarParameterValue("Preservation", 0.999999f);
	// trailMergeInstance->SetScalarParameterValue("Preservation", 1.f);
}

void ATrailHunterDirector::RenderTrails()
{
	UCanvas* canvas_;
	FVector2D size_;
	FDrawToRenderTargetContext context_;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(GetWorld(), currentRenderTarget, canvas_, size_, context_);
	
	for (int i = 0; i < paintTrailArr.Num(); ++i)
	{
		trailPaintInstance->SetScalarParameterValue("Depth", paintTrailArr[i].trailDepth);
		trailPaintInstance->SetScalarParameterValue("Power", paintTrailArr[i].trailPower);		

		FVector2D screenPosition_ = (paintTrailArr[i].trailLocation - FVector2D(paintTrailArr[i].trailRadius, paintTrailArr[i].trailRadius)) * size_;
		FVector2D screenSize_ = FVector2D(paintTrailArr[i].trailRadius, paintTrailArr[i].trailRadius) * 2.f * size_;

		FVector2D coordinateSize_;
		float rotation_;

		if (paintTrailArr[i].bIsFarTrail)
		{
			coordinateSize_ = FVector2D(1.f, 1.f);
			rotation_ = 0.f;
		}
		else
		{
			coordinateSize_ = FVector2D(FMath::RandRange(0.2f, 3.f), FMath::RandRange(0.2f, 3.f)); //FVector2D(1.f, 1.f);
			rotation_ = FMath::RandRange(0.f, 360.f); //0.f;//FMath::RandRange(0.f, 360.f);
		}
		
		canvas_->K2_DrawMaterial(trailPaintInstance, screenPosition_, screenSize_, FVector2D(0.f, 0.f), coordinateSize_, rotation_, FVector2D(0.5f, 0.5f));
	}

	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), context_);
}

void ATrailHunterDirector::SetVisionType(ETH_VisionType visionType, class UTrailHunterComponent* thComp)
{
	switch (visionType)
	{
	case ETH_VisionType::NORMAL:
		bIsTrackComponent = false;
		currentTrackComponent = nullptr;
		bShowSelfVisionActor = false;
		break;
	case ETH_VisionType::HUNTER:
		bIsTrackComponent = false;
		currentTrackComponent = nullptr;
		bShowSelfVisionActor = false;
		break;
	case ETH_VisionType::SUPER:
		bIsTrackComponent = false;
		currentTrackComponent = nullptr;
		bShowSelfVisionActor = true;
		break;
	case ETH_VisionType::SUPER_OTHER:
		bIsTrackComponent = false;
		currentTrackComponent = nullptr;
		bShowSelfVisionActor = false;
		break;
	default:
		bIsTrackComponent = false;
		currentTrackComponent = nullptr;
		bShowSelfVisionActor = false;
		break;
	}


	for (int baseID_ = 0; baseID_ < trailsArr.Num(); ++baseID_)
	{
		trailsArr[baseID_].visionType = visionType;
		for (int visID_ = 0; visID_ < trailsArr[baseID_].visualArr.Num(); ++visID_)
		{
			if (visionType != ETH_VisionType::SUPER || visionType != ETH_VisionType::SUPER_OTHER)
			{
				if (trailsArr[baseID_].visualArr[visID_].bSpawned)
				{
					trailsArr[baseID_].visualArr[visID_].bSpawned = false;
				}
			}
		}
	}
	activeVisionType = visionType;
	mainPlayerTHComponent = thComp;
}

void ATrailHunterDirector::SetTrailType(ETH_TrailType trailType, UTrailHunterComponent* thComp)
{
	activeTrailType = trailType;
	mainPlayerTHComponent = thComp;

	for (int baseID_ = 0; baseID_ < trailsArr.Num(); ++baseID_)
	{
		for (int decalID_ = 0; decalID_ < trailsArr[baseID_].visualArr.Num(); ++decalID_)
		{
			if (IsValid(trailsArr[baseID_].decalMatInstancesArr[decalID_]))
			{
				if (activeTrailType == ETH_TrailType::ACTIVE_OTHER)
				{
					if (trailsArr[baseID_].trailHunterComponent != mainPlayerTHComponent)
					{
						ChangeDecalType(trailsArr[baseID_].decalMatInstancesArr[decalID_], ETH_TrailType::ACTIVE_OTHER);
					}
					else
					{
						ChangeDecalType(trailsArr[baseID_].decalMatInstancesArr[decalID_], ETH_TrailType::NORMAL);
					}
				}
				else
				{
					ChangeDecalType(trailsArr[baseID_].decalMatInstancesArr[decalID_], activeTrailType);
				}
			}
		}
	}
}

void ATrailHunterDirector::DistantTrailsVisualTimer()
{
	APlayerController* plrCtrl_ = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!IsValid(plrCtrl_))
	{
		return;
	}
	if (!IsValid(plrCtrl_->GetPawn()))
	{
		return;
	}

	for (int baseID_ = trailsArr.Num() - 1; baseID_ >= 0; --baseID_)
	{
		for (int trailID_ = 0; trailID_ < trailsArr[baseID_].trailLocationArr.Num(); ++trailID_)
		{
			FTH_BaseStruct& baseStruct_ = trailsArr[baseID_];
			FVector& trailLocation_ = baseStruct_.trailLocationArr[trailID_];

			double trailDistance_ = (trailLocation_ - plrCtrl_->GetPawn()->GetActorLocation()).SizeSquared();

			if (trailDistance_ < maxPaintDistanceOffsetSquare)
			{
				if (!baseStruct_.bFarCalculatedArr[trailID_])
				{
					baseStruct_.bFarCalculatedArr[trailID_] = true;

					// Add trail to queue.
					FTH_QueueTrail newQueueTrail_;
					newQueueTrail_.location = trailLocation_;
					newQueueTrail_.traceRadius = 10.f;
					newQueueTrail_.trailDepth = 1.0f;
					newQueueTrail_.trailPower = 1.2f;
					newQueueTrail_.trailRadius = 0.0022f;
					newQueueTrail_.bFarTrail = true;
					queueTrailsArr.Add(newQueueTrail_);
				}
			}
			else
			{
				if (baseStruct_.bFarCalculatedArr[trailID_])
				{
					baseStruct_.bFarCalculatedArr[trailID_] = false;
				}
			}

			bool bStopShow_ = false;
			if (!bShowSelfVisionActor)
			{
				if (baseStruct_.trailHunterComponent == mainPlayerTHComponent)
				{
					bStopShow_ = true;
				}
			}

			if (bIsTrackComponent)
			{
				if (baseStruct_.trailHunterComponent != currentTrackComponent)
				{
					if (baseStruct_.visualArr[trailID_].bSpawned)
					{
						baseStruct_.visualArr[trailID_].bSpawned = false;
					}
					bStopShow_ = true;
				}
			}

			if (bStopShow_)
			{
				continue;
			}
			else if (activeVisionType == ETH_VisionType::SUPER || activeVisionType == ETH_VisionType::SUPER_OTHER)
			{
				bFinishRemoveVisual = false;
				if (trailDistance_ < trailVisualDistanceSquare)
				{
					if (!baseStruct_.visualArr[trailID_].bSpawned)
					{
						baseStruct_.visualArr[trailID_].bSpawned = true;
					}
				}
				else if (trailDistance_ > trailVisualDistanceOffsetSquare)
				{
					if (baseStruct_.visualArr[trailID_].bSpawned)
					{
						baseStruct_.visualArr[trailID_].bSpawned = false;
					}
				}
			}
			else if (!bFinishRemoveVisual)
			{
				if (activeVisionType == ETH_VisionType::NORMAL || activeVisionType == ETH_VisionType::HUNTER)
				{
					if (baseStruct_.visualArr[trailID_].bSpawned)
					{
						baseStruct_.visualArr[trailID_].bSpawned = false;
					}
				}
			}
		}
	}

	// Only deformation.
	for (int i = trailsDeformationArr.Num() - 1; i >= 0; --i)
	{
		if (trailsDeformationArr[i].lifeTime > 0.f)
		{
			if (GetWorld()->GetTimeSeconds() - trailsDeformationArr[i].lifeTime > trailsDeformationArr[i].spawnTime)
			{
				trailsDeformationArr.RemoveAt(i);
				continue;
			}
		}

		FTH_OnlyDeformationStruct& defStruct_(trailsDeformationArr[i]);
		double trailDistance_ = (defStruct_.trailLocation - plrCtrl_->GetPawn()->GetActorLocation()).SizeSquared();

		if (trailDistance_ < maxPaintDistanceOffsetSquare)
		{
			if (!defStruct_.bFarCalculated)
			{
				defStruct_.bFarCalculated = true;

				// Add trail to queue.
				FTH_QueueTrail newQueueTrail_;
				newQueueTrail_.location = defStruct_.trailLocation;
				newQueueTrail_.traceRadius = defStruct_.distanceTraceRadius;
				newQueueTrail_.trailDepth = defStruct_.distanceTrailDepth;
				newQueueTrail_.trailPower = defStruct_.distanceTrailPower;
				newQueueTrail_.trailRadius = defStruct_.distanceTrailRadius;
				newQueueTrail_.bFarTrail = true;
				queueTrailsArr.Add(newQueueTrail_);
			}
		}
		else
		{
			if (defStruct_.bFarCalculated)
			{
				defStruct_.bFarCalculated = false;
			}
		}
	}
}

void ATrailHunterDirector::QueueSpawn()
{
	bool bStopSpawn_(false);
	if (currentSpawnCount <= spawnActorPerFrame)
	{
		for (int baseID_ = trailsArr.Num() - 1; baseID_ >= 0; --baseID_)
		{
			for (int trailID_ = 0; trailID_ < trailsArr[baseID_].trailLocationArr.Num(); ++trailID_)
			{
				FTH_BaseStruct& baseStruct_ = trailsArr[baseID_];

				if (baseStruct_.visualArr[trailID_].bSpawned)
				{
					if (baseStruct_.visualArr[trailID_].visualActor == nullptr)
					{
						FVector& trailLocation_ = trailsArr[baseID_].trailLocationArr[trailID_];

						FActorSpawnParameters supportInfo_;
						supportInfo_.Owner = baseStruct_.trailHunterActor;
						FTransform supportTransform_ = FTransform::Identity;
						supportTransform_.SetLocation(trailLocation_);
						supportTransform_.SetScale3D(FVector(1.f, 1.f, 1.f));
						supportInfo_.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
						ATrailHunterVisualActor* spawnedActor_ = GetWorld()->SpawnActor<ATrailHunterVisualActor>(baseStruct_.trailHunterActor->visualActor, supportTransform_, supportInfo_);
						if (spawnedActor_)
						{
							spawnedActor_->thDirector = this;
							spawnedActor_->thComponent = baseStruct_.trailHunterComponent;
							baseStruct_.visualArr[trailID_].visualActor = spawnedActor_;
						}

						currentSpawnCount++;
						if (currentSpawnCount > spawnActorPerFrame)
						{
							bStopSpawn_ = true;
							currentSpawnCount = 0;
							break;
						}
					}
				}
			}
			if (bStopSpawn_)
			{
				break;
			}
		}
	}

	for (int baseID_ = trailsArr.Num() - 1; baseID_ >= 0; --baseID_)
	{
		for (int trailID_ = 0; trailID_ < trailsArr[baseID_].trailLocationArr.Num(); ++trailID_)
		{
			FTH_BaseStruct& baseStruct_ = trailsArr[baseID_];
			if (!baseStruct_.visualArr[trailID_].bSpawned)
			{
				if (baseStruct_.visualArr[trailID_].visualActor != nullptr)
				{
					baseStruct_.visualArr[trailID_].visualActor->Destroy();
					baseStruct_.visualArr[trailID_].visualActor = nullptr;
				}
			}
		}
	}
}

void ATrailHunterDirector::QueueTrail()
{
	if (currentTrailCount <= paintTrailPerFrame)
	{
		for (int i = queueTrailsArr.Num() - 1; i >= 0; --i)
		{
			RegisterNewTrail(queueTrailsArr[i].location, queueTrailsArr[i].traceRadius, queueTrailsArr[i].trailDepth, queueTrailsArr[i].trailPower, queueTrailsArr[i].trailRadius, queueTrailsArr[i].bFarTrail);
			queueTrailsArr.RemoveAt(i);

			currentTrailCount++;
			if (currentTrailCount > paintTrailPerFrame)
			{
				currentTrailCount = 0;
				break;
			}
		}
	}
}

void ATrailHunterDirector::SetTrackComponent(UTrailHunterComponent* trackComponent, bool bTack)
{
	if (trackComponent)
	{
		currentTrackComponent = trackComponent;
		bIsTrackComponent = bTack;
	}
	else
	{
		currentTrackComponent = nullptr;
		bIsTrackComponent = bTack;
	}
}

void ATrailHunterDirector::AddOnlyDeformationTrail(FTH_OnlyDeformationStruct deformationStruct, bool bRenderInstant)
{
	if (bOptimizeDistantTrails)
	{
		for (int i = 0; i < trailsDeformationArr.Num(); ++i)
		{
			if ((trailsDeformationArr[i].trailLocation - deformationStruct.trailLocation).SizeSquared() <= maxDistanceBetweenTrailSquare)
			{
				if (trailsDeformationArr[i].distanceTrailRadius < deformationStruct.distanceTrailRadius)
				{
					trailsDeformationArr[i] = deformationStruct;
				}
				if (bIsDebug)
				{
					GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("Similar trail found and updated"));
				}
				trailsDeformationArr[i].spawnTime = GetWorld()->GetTimeSeconds();
				return;
			}
		}
	}

	if (!bRenderInstant)
	{
		// Add new trail.
		deformationStruct.spawnTime = GetWorld()->GetTimeSeconds();
		if (IsPointInPaintRadius(deformationStruct.trailLocation))
		{
			deformationStruct.bFarCalculated = true;
		}
	}

	trailsDeformationArr.Add(deformationStruct);
}

void ATrailHunterDirector::AddNewTrail(UTrailHunterComponent* trailHunterComponent, UMaterialInterface* trailDecalMaterial, FTransform decalTransform, FVector decalSize, FVector trailLocation)
{
	FRotator decalRot_ = decalTransform.GetRotation().Rotator();
	bool bFindInstance_(false);

	for (int bID_ = 0; bID_ < trailsArr.Num(); ++bID_)
	{
		if (IsValid(trailsArr[bID_].trailHunterComponent) && IsValid(trailHunterComponent))
		{
			if (trailsArr[bID_].trailHunterComponent == trailHunterComponent)
			{
				bFindInstance_ = true;
				trailsArr[bID_].trailLocationArr.Add(trailLocation);
				trailsArr[bID_].lifeTimeArr.Add(trailHunterComponent->trailParameters.lifeTimeSecond);
				trailsArr[bID_].spawnTimeArr.Add(GetWorld()->GetTimeSeconds());

				FTH_VisualTrailStruct visualTrailStruct_;
				trailsArr[bID_].visualArr.Add(visualTrailStruct_);

				UMaterialInstanceDynamic* newDecalMatDynamic_ = UKismetMaterialLibrary::CreateDynamicMaterialInstance(GetWorld(), trailDecalMaterial);
				if (newDecalMatDynamic_)
				{
					if (activeTrailType == ETH_TrailType::ACTIVE_OTHER)
					{
						if (trailsArr[bID_].trailHunterComponent != mainPlayerTHComponent)
						{
							ChangeDecalType(newDecalMatDynamic_, ETH_TrailType::ACTIVE_OTHER);
						}
						else
						{
							ChangeDecalType(newDecalMatDynamic_, ETH_TrailType::NORMAL);
						}
					}
					else
					{
						ChangeDecalType(newDecalMatDynamic_, activeTrailType);
					}

					UDecalComponent* newDecal_ = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), newDecalMatDynamic_, decalSize, decalTransform.GetLocation(), decalRot_, 0.f);
					if (newDecal_)
					{
						trailsArr[bID_].decalsArr.Add(newDecal_);
						if (IsPointInPaintRadius(decalTransform.GetLocation()))
						{
							trailsArr[bID_].bFarCalculatedArr.Add(true);
						}
						else
						{
							trailsArr[bID_].bFarCalculatedArr.Add(false);
						}

						trailsArr[bID_].decalMatInstancesArr.Add(newDecalMatDynamic_);
						newDecal_->SetFadeScreenSize(trailHunterComponent->trailParameters.decalFadeScreenSize);
					}
				}

				// Remove trail if > max count.
				if (trailsArr[bID_].trailLocationArr.Num() > trailHunterComponent->trailParameters.maxTrailCount)
				{
					if (trailsArr[bID_].trailLocationArr.IsValidIndex(0))
					{
						if (IsValid(trailsArr[bID_].decalsArr[0]))
						{
							trailsArr[bID_].decalsArr[0]->DestroyComponent();
						}

						trailsArr[bID_].trailLocationArr.RemoveAt(0);
						trailsArr[bID_].decalsArr.RemoveAt(0);
						trailsArr[bID_].bFarCalculatedArr.RemoveAt(0);

						if (trailsArr[bID_].visualArr[0].bSpawned)
						{
							//trailsArr[bID_].visualArr[0].bSpawned = false;
							if (trailsArr[bID_].visualArr[0].visualActor)
							{
								trailsArr[bID_].visualArr[0].visualActor->Destroy();
								trailsArr[bID_].visualArr[0].visualActor = nullptr;
							}
						}
						trailsArr[bID_].visualArr.RemoveAt(0);

						if (trailsArr[bID_].lifeTimeArr.IsValidIndex(0))
						{
							trailsArr[bID_].lifeTimeArr.RemoveAt(0);
						}
						if (trailsArr[bID_].spawnTimeArr.IsValidIndex(0))
						{
							trailsArr[bID_].spawnTimeArr.RemoveAt(0);
						}
						if (trailsArr[bID_].decalMatInstancesArr.IsValidIndex(0))
						{
							trailsArr[bID_].decalMatInstancesArr.RemoveAt(0);
						}
					}
				}
				break;
			}
		}
	}

	if (!bFindInstance_)
	{
		FTH_BaseStruct newTrailHunterBase_;
		newTrailHunterBase_.trailLocationArr.Add(trailLocation);
		newTrailHunterBase_.trailHunterComponent = trailHunterComponent;
		//	newTrailHunterBase_.instanceComponent = newInstComp_;
		newTrailHunterBase_.lifeTimeArr.Add(trailHunterComponent->trailParameters.lifeTimeSecond);
		newTrailHunterBase_.spawnTimeArr.Add(GetWorld()->GetTimeSeconds());

		// Spawn Trail Hunter Support Actor.
		FActorSpawnParameters spawnInfo_;
		spawnInfo_.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		// Set transform.
		FTransform spawnTransform_(FTransform::Identity);
		spawnTransform_.SetLocation(trailHunterComponent->GetOwner()->GetActorLocation());
		ATrailHunterActor* newTrailSupportActor_ = GetWorld()->SpawnActor<ATrailHunterActor>(trailHunterComponent->trailHunterActor, spawnTransform_, spawnInfo_);
		if (newTrailSupportActor_)
		{
			newTrailHunterBase_.trailHunterActor = newTrailSupportActor_;
		}

		UMaterialInstanceDynamic* newDecalMatDynamic_ = UKismetMaterialLibrary::CreateDynamicMaterialInstance(GetWorld(), trailDecalMaterial);
		if (newDecalMatDynamic_)
		{
			newTrailHunterBase_.decalMatInstancesArr.Add(newDecalMatDynamic_);

			if (activeTrailType == ETH_TrailType::ACTIVE_OTHER)
			{
				if (newTrailHunterBase_.trailHunterComponent != mainPlayerTHComponent)
				{
					ChangeDecalType(newDecalMatDynamic_, ETH_TrailType::ACTIVE_OTHER);
				}
				else
				{
					ChangeDecalType(newDecalMatDynamic_, ETH_TrailType::NORMAL);
				}
			}
			else
			{
				ChangeDecalType(newDecalMatDynamic_, activeTrailType);
			}

			newTrailHunterBase_.visionType = activeVisionType;
			newTrailHunterBase_.trailType = activeTrailType;

			UDecalComponent* newDecal_ = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), newDecalMatDynamic_, decalSize, decalTransform.GetLocation(), decalRot_, 0.f);
			if (newDecal_)
			{
				newDecal_->SetFadeScreenSize(trailHunterComponent->trailParameters.decalFadeScreenSize);
				newTrailHunterBase_.decalsArr.Add(newDecal_);
				if (IsPointInPaintRadius(decalTransform.GetLocation()))
				{
					newTrailHunterBase_.bFarCalculatedArr.Add(true);
				}
				else
				{
					newTrailHunterBase_.bFarCalculatedArr.Add(false);
				}

				FTH_VisualTrailStruct visualTrailStruct_;
				newTrailHunterBase_.visualArr.Add(visualTrailStruct_);
			}
		}

		trailsArr.Add(newTrailHunterBase_);
	}
}
