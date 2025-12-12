// Copyright 2024 CAS. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TrailHunterSceneComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TRAILHUNTER_API UTrailHunterSceneComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTrailHunterSceneComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Trail Hunter | Parameters")
	void ActivatePaint(bool bActivate);

	UFUNCTION(BlueprintPure, Category = "Trail Hunter | Parameters")
	void GetLastHitLocation(FVector &location, bool &hitBlock);
	UFUNCTION(BlueprintPure, Category = "Trail Hunter | Parameters")
	void GetLastDeformationLocation(FVector &location);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	TArray<TEnumAsByte<EObjectTypeQuery>> traceObjectTypesArr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters", meta=(ClampMin = 0.1))
	float trailStartOffset = 25.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters", meta=(ClampMin = 0.1))
	float trailEndOffset = 25.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters", meta=(ClampMin = 0.1))
	float traceRadius = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters", meta=(ClampMin = 0.01))
	float trailPower = 0.9f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters", meta=(ClampMin = 0.0001))
	float trailRadius = 0.0022f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters", meta=(ClampMin = 0.0001, ClampMax = 1))
	float trailDepth = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	float checkTrailRate = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	bool bPaintFarTrailByDistance = true;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters", meta=(ClampMin = 0.1), meta=(EditCondition="bPaintFarTrailByDistance"))
	// float distanceTraceRadius = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters", meta=(ClampMin = 0.01), meta=(EditCondition="bPaintFarTrailByDistance"))
	float distanceTrailPower = 1.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters", meta=(ClampMin = 0.0001), meta=(EditCondition="bPaintFarTrailByDistance"))
	float distanceTrailRadius = 0.0022f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters", meta=(ClampMin = 0.0001, ClampMax = 1), meta=(EditCondition="bPaintFarTrailByDistance"))
	float distanceTrailDepth = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters", meta=(EditCondition="bPaintFarTrailByDistance"))
	float distanceBetweenPaint = 50.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters", meta=(ClampMin = "0"), meta=(EditCondition="bPaintFarTrailByDistance"))
	float distanceLifeTimeSecond = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Debug")
	bool bDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	bool bActivatePaintOnBeginPlay = true;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void RegisterDelayTimer(); 

	FHitResult SphereTrace(const FVector& startV, const FVector& endV, float sphereRadius) const;

	void CheckTrailTimer();

	void PaintFarTrailByDistance();
	
private:	

	UPROPERTY()
	FTimerHandle registerHandle;
	UPROPERTY()
	float registerDelayRate = 0.25f;
	UPROPERTY()
	FTimerHandle checkTrailHandle;
	
	UPROPERTY()
	class ATrailHunterDirector *trailHunterDirector = nullptr;
	UPROPERTY()
	FVector lastHitLocation = FVector::ZeroVector;
	UPROPERTY()
	bool bLastHitBlock = false;
	UPROPERTY()
	FVector lastDeformationLocation = FVector::ZeroVector;
	UPROPERTY()
	FVector lastCheckLocation = FVector::Zero();
	UPROPERTY()
	float distanceBetweenPaintSquare = 0.0f;
	UPROPERTY()
	bool bActivatePaint = true;
		
};
