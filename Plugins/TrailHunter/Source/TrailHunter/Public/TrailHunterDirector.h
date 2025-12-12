// Copyright 2024 CAS. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrailHunterTypes.h"
#include "GameFramework/Actor.h"
#include "TrailHunterDirector.generated.h"

UCLASS()
class TRAILHUNTER_API ATrailHunterDirector : public AActor
{
	GENERATED_BODY()
	
public:
	
	// Sets default values for this actor's properties
	ATrailHunterDirector();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// void RegisterTrailHunterComponent(class UTrailHunterComponent* trailHunterComponent);

	void AddNewTrail(class UTrailHunterComponent* trailHunterComponent, class UMaterialInterface* trailDecalMaterial, FTransform decalTransform, FVector decalSize, FVector trailLocation);

	void AddOnlyDeformationTrail(FTH_OnlyDeformationStruct deformationStruct, bool bRenderInstant = false);
	
	void RegisterNewTrail(const FVector &location, const float &radius, const float &depth, const float &power, const float &trailRadius, bool bFarTrail = false);
	
	bool IsPointInPaintRadius(const FVector &location) const;
	
	void SetTrackComponent(class UTrailHunterComponent *trackComponent, bool bTack);
	
	void SetVisionType(ETH_VisionType visionType, class UTrailHunterComponent* thComp);

	void SetTrailType(ETH_TrailType trailType, class UTrailHunterComponent* thComp);

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	// TEnumAsByte<ECollisionChannel> trailCollision = ECC_Destructible;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	FTH_DecalParameters decalColorParameters;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters", meta=(ClampMin = "0.1"))
	float trailHunterWorkerRate = 0.5f;
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters", meta=(ClampMin = "0.1"))
	float trailSideFade = 0.05f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters", meta=(ClampMin = "0.1", ClampMax = "7000"))
	float maxDistanceToVisualActor = 1000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters", meta=(ClampMin = "1"))
	int spawnActorPerFrame = 5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters", meta=(ClampMin = "1"))
	int paintTrailPerFrame = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	bool bOptimizeDistantTrails = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters", meta=(ClampMin = 0.1), meta=(EditCondition="bPaintFarTrailByDistance"))
	float maxDistanceBetweenTrail = 10.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | System")
	class UMaterialInterface *trailPaint = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | System")
	class UMaterialInterface *trailMerge = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | System")
	class UMaterialInterface *trailHistory = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | System")
	class UTextureRenderTarget2D* trailsRenderTarget = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | System")
	class UMaterialParameterCollection* mpcTrailHunter = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Debug")
	bool bIsDebug = false;

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Work")
	// bool bEnableNearTrail = true;

	UPROPERTY()
	class UTrailHunterComponent* mainPlayerTHComponent = nullptr;

protected:
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);

	void TrailWorkerTimer();

	void ChangeDecalType(class UMaterialInstanceDynamic* dynamicMaterial, ETH_TrailType trailType) const;
	void CalculateLocation();
	void InitializeTrailDirector();

	void CalculateRenderTick(float &deltaTime);
	void RenderTrails();

	void DistantTrailsVisualTimer();

	void QueueSpawn();

	void QueueTrail();

private:

	UPROPERTY()
	TArray<FTH_BaseStruct> trailsArr;
	UPROPERTY()
	TArray<FTH_QueueTrail> queueTrailsArr;
	UPROPERTY()
	TArray<FTH_OnlyDeformationStruct> trailsDeformationArr;

	UPROPERTY()
	FTimerHandle workerHandle;

	UPROPERTY()
	ETH_VisionType activeVisionType = ETH_VisionType::NORMAL;
	UPROPERTY()
	ETH_TrailType activeTrailType = ETH_TrailType::NORMAL;

	UPROPERTY()
	bool bIsLocationCalculated = false;
	UPROPERTY()
	float locationCalculateHelper = 0.f;
	UPROPERTY()
	FVector currentLocation = FVector::ZeroVector;
	UPROPERTY()
	FVector historyLocation = FVector::ZeroVector;
	UPROPERTY()
	float maxPaintDistance = 8192.f;
	UPROPERTY()
	float maxPaintDistanceSquare = 0.f;
	UPROPERTY()
	float maxDistanceMultiply = 0.f;
	UPROPERTY()
	float maxPaintDistanceOffset = 0.f;
	UPROPERTY()
	float maxPaintDistanceOffsetSquare = 0.f;
	UPROPERTY()
	float trailVisualDistanceSquare = 0.f;
	UPROPERTY()
	float trailVisualDistanceOffsetSquare = 0.f;
	UPROPERTY()
	TArray<FTH_PaintTrailStruct> paintTrailArr;
	UPROPERTY()
	float trailsAttenuation = 0.05f;
	UPROPERTY()
	class UMaterialInstanceDynamic* trailPaintInstance = nullptr;
	UPROPERTY()
	class UMaterialInstanceDynamic* trailHistoryMergeInstance = nullptr;
	UPROPERTY()
	class UMaterialInstanceDynamic* trailHistoryCopyInstance = nullptr;
	UPROPERTY()
	class UTextureRenderTarget2D* currentRenderTarget = nullptr;
	UPROPERTY()
	class UTextureRenderTarget2D* historyRenderTarget = nullptr;
	UPROPERTY()
	bool bFinishRemoveVisual = true;
	UPROPERTY()
	int currentSpawnCount = 0;	
	UPROPERTY()
	int currentTrailCount = 0;
	
	UPROPERTY()
	class UTrailHunterComponent *currentTrackComponent = nullptr;
	UPROPERTY()
	bool bIsTrackComponent = false;
	UPROPERTY()
	bool bShowSelfVisionActor = false;
	UPROPERTY()
	float maxDistanceBetweenTrailSquare = 0.f;
	

};
