#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SOTS_UDSBridgeSubsystem.generated.h"

class USOTS_UDSBridgeConfig;

/**
 * Timer-driven bridge scaffold for UDS; SPINE1 focuses on discovery + telemetry only.
 */
UCLASS()
class SOTS_UDSBRIDGE_API USOTS_UDSBridgeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	// Config
	UPROPERTY()
	TObjectPtr<USOTS_UDSBridgeConfig> Config = nullptr;

	struct FSOTS_UDSBridgeState
	{
		bool bSnowy = false;
		bool bRaining = false;
		bool bDusty = false;
		bool bHasSunDir = false;
		FVector SunDirWS = FVector::ZeroVector;
	};

	// Cached refs
	TWeakObjectPtr<AActor> CachedUDSActor;
	TWeakObjectPtr<APawn> CachedPlayerPawn;
	TWeakObjectPtr<UActorComponent> CachedDLWEComponent;
	FSOTS_UDSBridgeState LastState;
	bool bLastPushedSunValid = false;
	FVector LastPushedSunDirWS = FVector::ZeroVector;

	bool bLastSnowApplied = false;
	bool bLastRainApplied = false;
	bool bLastDustApplied = false;
	TWeakObjectPtr<UObject> LastSettingsApplied;

	bool bWarnedNoUDS = false;
	bool bWarnedNoDLWE = false;
	bool bWarnedNoGSM = false;
	bool bWarnedNoDLWESurface = false;

	// Timers
	FTimerHandle UpdateTimerHandle;
	double NextTelemetryTimeSeconds = 0.0;

	// Core loop
	void TickBridge();
	void RefreshCachedRefs(bool bForce);

	// Discovery helpers
	AActor* FindUDSActor();
	APawn* GetPlayerPawn();
	UActorComponent* FindDLWEComponent(APawn* Pawn);
	bool ValidateDLWEComponent(UActorComponent* DLWEComp, FString& OutInfo);
	void PushSunDirToGSM(const FSOTS_UDSBridgeState& State);
	void ApplyDLWEPolicy(const FSOTS_UDSBridgeState& State);

	// Debug
	void EmitTelemetry(double NowSeconds);

	// Config helpers
	float GetJitteredInterval() const;
	void EnsureConfig();
	bool TryReadBoolProperty(UObject* Obj, FName PropName, bool& OutValue) const;
	bool TryReadBoolByPath(UObject* Obj, const FString& Path, bool& OutValue) const;
	bool TryCallVectorFunction(UObject* Obj, FName FuncName, FVector& OutVec) const;
	UObject* TryGetObjectProperty(UObject* Obj, FName PropName) const;
	bool ExtractWeatherFlags(AActor* UDSActor, FSOTS_UDSBridgeState& OutState) const;
	bool ExtractSunDirectionWS(AActor* UDSActor, FVector& OutDirWS, bool& bValid) const;
	bool TryReadRotatorProperty(UObject* Obj, FName PropName, FRotator& OutRot) const;
	bool CallDLWE_BoolFunction(UObject* DLWEComp, FName FuncName, bool bValue) const;
	bool CallDLWE_SettingsFunction(UObject* DLWEComp, FName FuncName, UObject* SettingsObj) const;
};
