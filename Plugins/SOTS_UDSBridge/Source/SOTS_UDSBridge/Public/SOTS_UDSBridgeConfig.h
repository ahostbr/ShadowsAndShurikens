#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SOTS_UDSBridgeConfig.generated.h"

/**
 * Configuration for the UDS bridge scaffolding. Values are intentionally minimal in SPINE1.
 */
UCLASS(BlueprintType)
class SOTS_UDSBRIDGE_API USOTS_UDSBridgeConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// Timer cadence
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge")
	float UpdateIntervalSeconds = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge")
	float IntervalJitterSeconds = 0.05f;

	// Discovery hints
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|Discovery")
	FName UDSActorTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|Discovery")
	FString UDSActorNameContains;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|Discovery")
	TSoftObjectPtr<AActor> UDSActorSoftRef;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|Discovery")
	FString DLWEComponentNameContains = TEXT("DLWE_Interaction");

	// Debug telemetry
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|Debug")
	bool bEnableBridgeTelemetry = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|Debug")
	float TelemetryIntervalSeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|Debug")
	bool bEnableBridgeWarnings = true;

	// Weather property mappings (from UDS actor)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|UDS")
	FName Weather_bSnowy_Property = FName(TEXT("ED_Snowy"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|UDS")
	FName Weather_bRaining_Property = FName(TEXT("ED_Raining"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|UDS")
	FName Weather_bDusty_Property = FName(TEXT("ED_Dusty"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|UDS")
	FString WeatherPath_Snowy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|UDS")
	FString WeatherPath_Raining;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|UDS")
	FString WeatherPath_Dusty;

	// Sun direction mapping (via reflection)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|UDS|Sun")
	FName SunLightActorProperty = FName(TEXT("SunLight"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|UDS|Sun")
	FName SunDirectionFunctionName = FName(TEXT("GetSunDirection"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|UDS|Sun")
	FName SunWorldRotationProperty = FName(TEXT("Sun World Rotation"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|UDS|Sun")
	bool bUseForwardVectorAsLightDir = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|UDS|Sun")
	bool bInvertLightForwardVector = true;

	// DLWE reflection mapping
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|DLWE")
	FName DLWE_EnableSnow_Function = FName(TEXT("SetSnowEnabled"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|DLWE")
	FName DLWE_EnableRain_Function = FName(TEXT("SetRainEnabled"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|DLWE")
	FName DLWE_EnableDust_Function = FName(TEXT("SetDustEnabled"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|DLWE|Policy")
	bool bEnableSnowInteractionWhenSnowy = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|DLWE|Policy")
	bool bEnableDustInteractionWhenDusty = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|DLWE|Policy")
	bool bEnablePuddlesWhenRaining = true;

	// Optional settings swap surface
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|DLWE|Policy")
	FName DLWE_Func_SetInteractionSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|DLWE|Policy")
	TSoftObjectPtr<UObject> DLWE_Settings_Snow;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|DLWE|Policy")
	TSoftObjectPtr<UObject> DLWE_Settings_Clear;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|DLWE|Policy")
	TSoftObjectPtr<UObject> DLWE_Settings_Rain;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|UDSBridge|DLWE|Policy")
	TSoftObjectPtr<UObject> DLWE_Settings_Dust;
};
