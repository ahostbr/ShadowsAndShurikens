#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "SOTS_UDSBridgeSubsystem.generated.h"

class USOTS_UDSBridgeConfig;
class USOTS_GlobalStealthManagerSubsystem;
class IConsoleObject;
class ASOTS_TrailBreadcrumb;

USTRUCT(BlueprintType)
struct FSOTS_UDSObservedState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="SOTS|UDSBridge") bool bHasUDS = false;
	UPROPERTY(BlueprintReadOnly, Category="SOTS|UDSBridge") bool bHasDLWE = false;
	UPROPERTY(BlueprintReadOnly, Category="SOTS|UDSBridge") float SnowAmount01 = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="SOTS|UDSBridge") float Wetness01 = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="SOTS|UDSBridge") float TimeOfDay01 = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="SOTS|UDSBridge") bool bIsNight = false;
};

// DLWE application path chosen at runtime.
enum class EDLWEApplyMode : uint8
{
	None,
	SettingsAsset,
	ToggleFunctions
};

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

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUDSStateChanged, const FSOTS_UDSObservedState&, NewState);

	// BP/externally callable helpers
	bool GetBridgeStateSnapshot(
		bool& bOutHasUDS,
		bool& bOutHasDLWE,
		bool& bOutHasGSM,
		bool& bOutSnowy,
		bool& bOutRaining,
		bool& bOutDusty,
		bool& bOutHasSunDir,
		FVector& OutSunDirWS);

	UFUNCTION(BlueprintCallable, Category="SOTS|UDSBridge|State")
	FSOTS_UDSObservedState GetLastUDSState() const { return LastObservedState; }

	UFUNCTION(BlueprintCallable, Category="SOTS|UDSBridge|State")
	bool IsUDSBridgeActive() const;

	UPROPERTY(BlueprintAssignable, Category="SOTS|UDSBridge|State")
	FOnUDSStateChanged OnUDSStateChanged;

	void ForceRefreshAndApply();

private:
	// Console command entry point (registered during Initialize).
	void Console_ForceRefresh();

private:
	FSOTS_UDSObservedState LastObservedState;
	mutable bool bLoggedMissingUDSClassOnce = false;

	// Config
	UPROPERTY()
	TObjectPtr<USOTS_UDSBridgeConfig> Config = nullptr;

	/*
	 * Handoff notes for operators:
	 * - UDS discovery: prefer soft reference; fallback to tag/name contains.
	 * - Weather mapping: set bool property names or dot-paths for Snow/Rain/Dust and sun direction sources (function, light ref, or rotator).
	 * - DLWE mapping: either a settings asset surface (single UObject* function) or per-toggle bool functions.
	 * - This bridge only drives UDS DLWE_Interaction state; it does not draw effects or trails.
	 */

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
	bool bWarnedMissingUDSOnce = false;
	bool bWarnedMissingDLWEOnce = false;
	bool bWarnedMissingGSMOnce = false;
	bool bWarnedMissingDLWESurfaceOnce = false;

	// Timers
	FTimerHandle UpdateTimerHandle;
	double NextTelemetryTimeSeconds = 0.0;
	double NextWarningTimeSeconds = 0.0;
	EDLWEApplyMode ActiveApplyMode = EDLWEApplyMode::None;
	bool bHasSnowToggle = false;
	bool bHasRainToggle = false;
	bool bHasDustToggle = false;
	bool bHasSettingsSurface = false;
	bool bValidatedDLWEFunctions = false;
	TWeakObjectPtr<USOTS_GlobalStealthManagerSubsystem> CachedGSMSubsystem;
	IConsoleObject* ForceRefreshConsoleCmd = nullptr;

	// Core loop
	void TickBridge();
	void RefreshCachedRefs(bool bForce);
	void RefreshCachedGSM();
	void BuildStateFromCaches(FSOTS_UDSBridgeState& OutState);
	void PollObservedState();

	// Discovery helpers
	AActor* FindUDSActor();
	APawn* GetPlayerPawn();
	UActorComponent* FindDLWEComponent(AActor* Owner) const;
	bool TryReadDLWEState(UActorComponent* DLWEComp, FSOTS_UDSObservedState& OutState) const;
	bool TryReadFloatProperty(UObject* Obj, FName PropName, float& OutValue, bool bClamp01 = false) const;
	bool ValidateDLWEComponent(UActorComponent* DLWEComp, FString& OutInfo);
	void PushSunDirToGSM(const FSOTS_UDSBridgeState& State);
	void ApplyDLWEPolicy(const FSOTS_UDSBridgeState& State);
	void UpdateApplyMode(UActorComponent* DLWEComp);
	void TrySpawnTrailBreadcrumb(const FSOTS_UDSBridgeState& State);
	UFUNCTION()
	void OnBreadcrumbDestroyed(AActor* DestroyedActor);
	void DebugDrawBreadcrumbChain(double NowSeconds);

	// Debug
	void EmitTelemetry(double NowSeconds);
	void EmitWarningOnce(const TCHAR* Message, bool& bWarnedFlag, bool& bWarnedOnceFlag, double NowSeconds);
	bool ShouldThrottleWarning(double NowSeconds);
	USOTS_GlobalStealthManagerSubsystem* GetGSMSubsystem();
	double GetWorldTimeSecondsSafe() const;

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
	bool IsSingleBoolParamFunction(UFunction* Fn) const;
	bool IsSingleObjectParamFunction(UFunction* Fn) const;

	// Breadcrumb chain state
	TWeakObjectPtr<ASOTS_TrailBreadcrumb> BreadcrumbTail;
	TWeakObjectPtr<ASOTS_TrailBreadcrumb> BreadcrumbHead;
	FVector LastBreadcrumbLocation = FVector::ZeroVector;
	double LastBreadcrumbSpawnTime = 0.0;
	int32 AliveBreadcrumbCount = 0;
	double NextBreadcrumbDebugDrawTimeSeconds = 0.0;
};
