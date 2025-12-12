// Copyright 2024 CAS. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrailHunterTypes.h"
#include "Components/ActorComponent.h"
#include "TrailHunterComponent.generated.h"


UCLASS(Blueprintable)
class TRAILHUNTER_API UTrailHunterComponent : public UActorComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FModeChanged);

public:
	
	// Sets default values for this component's properties
	UTrailHunterComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Broadcasts notification
	void BroadcastOnModeChanged() const;
	// Delegate
	UPROPERTY(BlueprintAssignable)
	FModeChanged OnModeChanged;

	UFUNCTION(BlueprintCallable, Category = "Trail Hunter | Parameters")
	void AddNewTrail(class UMaterialInterface* trailDecalMaterial, FTransform decalTransform, FVector decalSize, FVector trailLocation);
	UFUNCTION(BlueprintCallable, Category = "Trail Hunter | Parameters")
	void AddCustomPaint(FVector trailLocation = FVector::ZeroVector, float trailPower = 1.2f, float trailRadius = 0.0022f, float trailDepth = 1.0f, float lifeTimeSecond = 1800.f);
	UFUNCTION(BlueprintCallable, Category = "Trail Hunter | Parameters")
	void SetVisionType(ETH_VisionType visionType);
	UFUNCTION(BlueprintCallable, Category = "Trail Hunter | Parameters")
	void SetTrailType(ETH_TrailType trailType);

	UFUNCTION(BlueprintPure, Category = "Trail Hunter | Parameters")
	void GetTrailHunterMode(ETH_VisionType &visionType, ETH_TrailType &trailType) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	FTH_Parameters trailParameters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	bool bDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	TSubclassOf<class ATrailHunterActor> trailHunterActor = nullptr;

protected:
	
	// Called when the game starts
	virtual void BeginPlay() override;

	void RegisterDelayTimer();

private:

	UPROPERTY()
	FTimerHandle registerHandle;
	UPROPERTY()
	float registerDelayRate = 0.25f;
	
	UPROPERTY()
	class ATrailHunterDirector *trailHunterDirector = nullptr;
	UPROPERTY()
	ETH_VisionType currentVisionType;
	UPROPERTY()
	ETH_TrailType currentTrailType;
		
};
