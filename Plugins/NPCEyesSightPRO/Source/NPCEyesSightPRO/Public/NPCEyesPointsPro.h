// Copyright 2021-2024 CAS. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NPCEyesPointsPro.generated.h"


UCLASS(Blueprintable)
class NPCEYESSIGHTPRO_API UNPCEyesPointsPro : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UNPCEyesPointsPro();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "NPC Eyes PRO")
	void SetSkeletalMesh_BP(class USceneComponent* setSkeletalMesh);

	UFUNCTION(BlueprintCallable, Category = "NPC Eyes PRO") // Set component with points for Sight.
	void SetComponentForSight_BP(class USceneComponent* setSceneComponent);

	UFUNCTION(BlueprintCallable, Category = "NPC Eyes PRO")
	void AddLight(class USceneComponent *newLight);
	UFUNCTION(BlueprintCallable, Category = "NPC Eyes PRO")
	TArray<class USceneComponent*> GetAllLights();
	UFUNCTION(BlueprintCallable, Category = "NPC Eyes PRO")
	void ClearLights();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPC Eyes PRO Parameters")
	TArray<FName> pointsNameArr;

	class USceneComponent* GetComponentForSight() const;

	TArray<FName> GetPointsName() const;

	TArray<class USceneComponent*> GetLights() const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:

	UPROPERTY()
	class USceneComponent* sceneComponentRef;

	// All manual added light components.
	UPROPERTY()
	TArray<class USceneComponent*> lightComponentsArr;

};
