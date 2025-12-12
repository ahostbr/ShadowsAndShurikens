// Copyright 2024 CAS. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrailHunterActor.generated.h"

UCLASS()
class TRAILHUNTER_API ATrailHunterActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATrailHunterActor();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "Trail Hunter")
	AActor* GetTrailOwner() const;

	void SetTrailOwner(AActor* owner);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	TSubclassOf<class ATrailHunterVisualActor> visualActor;
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY()
	AActor* trailOwner = nullptr;

private:	
	

};
