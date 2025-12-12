// Copyright 2024 CAS. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrailHunterVisualActor.generated.h"

UCLASS()
class TRAILHUNTER_API ATrailHunterVisualActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATrailHunterVisualActor();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Trail Hunter | Parameters")
	void SetTrack();

	UPROPERTY()
	class ATrailHunterDirector *thDirector = nullptr;
	UPROPERTY()
	class UTrailHunterComponent *thComponent = nullptr;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:	
	

};
