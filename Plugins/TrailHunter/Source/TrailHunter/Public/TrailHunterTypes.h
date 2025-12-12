// Copyright 2024 CAS. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine.h"
#include "TrailHunterTypes.generated.h"

UENUM(BlueprintType)
enum class ETH_VisionType: uint8
{
	NORMAL			UMETA(DisplayName = "Normal"),
	HUNTER			UMETA(DisplayName = "Hunter"),
	SUPER			UMETA(DisplayName = "Super"),
	SUPER_OTHER		UMETA(DisplayName = "Super, Other trails")
};

UENUM(BlueprintType)
enum class ETH_TrailType: uint8
{
	ACTIVE			  UMETA(DisplayName = "Active"),
	NORMAL			  UMETA(DisplayName = "Normal"),
	ACTIVE_OTHER      UMETA(DisplayName = "Active, Other trails"),
};

USTRUCT(BlueprintType)
struct FTH_QueueTrail
{
	GENERATED_USTRUCT_BODY()
	
	FVector location = FVector::ZeroVector;
	float traceRadius = 0.f;
	float trailDepth = 0.f;
	float trailPower = 0.f;
	float trailRadius = 0.f;
	bool bFarTrail = false;

	FTH_QueueTrail()
	{
	}
};

USTRUCT(BlueprintType)
struct FTH_Result
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	AActor *trailOwner = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	class ATrailHunterActor *supportActor = nullptr;
	

	FTH_Result()
	{
	}
};


USTRUCT(BlueprintType)
struct FTH_Parameters
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters", meta=(ClampMin = "0"))
	int maxTrailCount = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters", meta=(ClampMin = "0"))
	float lifeTimeSecond = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	float decalFadeScreenSize = 0.005f;

	FTH_Parameters()
	{
	}
};

USTRUCT()
struct FTH_PaintTrailStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FVector2D trailLocation = FVector2D::ZeroVector;
	UPROPERTY()
	float trailRadius = 0.f;
	UPROPERTY()
	float trailDepth = 0.f;
	UPROPERTY()
	float trailPower = 0.f;
	UPROPERTY()
	bool bIsFarTrail = false;

	FTH_PaintTrailStruct()
	{
	}
};

USTRUCT()
struct FTH_VisualTrailStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	bool bSpawned = false;
	UPROPERTY()
	class ATrailHunterVisualActor* visualActor = nullptr;

	FTH_VisualTrailStruct()
	{
	}
};

USTRUCT()
struct FTH_BaseStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	class UTrailHunterComponent* trailHunterComponent = nullptr;
	UPROPERTY()
	class ATrailHunterActor *trailHunterActor = nullptr;
	UPROPERTY()
	TArray<FVector> trailLocationArr;
	UPROPERTY()
	TArray<class UDecalComponent*> decalsArr;
	UPROPERTY()
	TArray<double> lifeTimeArr;
	UPROPERTY()
	TArray<double> spawnTimeArr;
	UPROPERTY()
	TArray<class UMaterialInstanceDynamic*> decalMatInstancesArr;
	UPROPERTY()
	TArray<FTH_VisualTrailStruct> visualArr;
	UPROPERTY()
	ETH_VisionType visionType = ETH_VisionType::NORMAL;
	UPROPERTY()
	ETH_TrailType trailType = ETH_TrailType::NORMAL;
	UPROPERTY()
	TArray<bool> bFarCalculatedArr;

	FTH_BaseStruct()
	{
	}
};

USTRUCT()
struct FTH_OnlyDeformationStruct
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY()
	FVector trailLocation = FVector::ZeroVector;
	UPROPERTY()
	double lifeTime = 0.f;
	UPROPERTY()
	double spawnTime = 0.f;
	UPROPERTY()
	bool bFarCalculated = false;

	UPROPERTY()
	float distanceTraceRadius = 10.f;
	UPROPERTY()
	float distanceTrailPower = 1.2f;
	UPROPERTY()
	float distanceTrailRadius = 0.0022f;
	UPROPERTY()
	float distanceTrailDepth = 1.0f;

	FTH_OnlyDeformationStruct()
	{
	}
};

USTRUCT(BlueprintType)
struct FTH_DecalParameters
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	FColor normalTrailColor = FColor::Black;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	float normalEmissive = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	FColor activeTrailColor = FColor::Green;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	float activeEmissive = 25.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	FColor otherTrailColor = FColor::Turquoise;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	float otherEmissive = 25.f;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	// FColor hunterTrailColor = FColor::White;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Hunter | Parameters")
	// float hunterEmissive = 1.f;

	FTH_DecalParameters()
	{
	}
};