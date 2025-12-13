#include "SOTS_UDSBridgeSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "SOTS_TrailBreadcrumb.h"
#include "SOTS_GlobalStealthManagerSubsystem.h"
#include "SOTS_UDSBridgeConfig.h"
#include "SOTS_UDSBridgeSettings.h"
#include "UObject/Class.h"
#include "TimerManager.h"
#include "Perception/AISense_Hearing.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_UDSBridge, Log, All);

namespace
{
	constexpr double WarningThrottleSeconds = 10.0;
	constexpr float SunDirCompareTolerance = 0.001f;
}

void USOTS_UDSBridgeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	EnsureConfig();

	ForceRefreshConsoleCmd = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("SOTS.UDSBridge.Refresh"),
		TEXT("Forces SOTS_UDSBridge to rescan UDS actor, player pawn, and DLWE component, then reapply policy."),
		FConsoleCommandDelegate::CreateUObject(this, &USOTS_UDSBridgeSubsystem::Console_ForceRefresh),
		ECVF_Default);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(UpdateTimerHandle, this, &USOTS_UDSBridgeSubsystem::TickBridge, GetJitteredInterval(), true);
	}
}

void USOTS_UDSBridgeSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateTimerHandle);
	}

	if (ForceRefreshConsoleCmd)
	{
		IConsoleManager::Get().UnregisterConsoleObject(ForceRefreshConsoleCmd);
		ForceRefreshConsoleCmd = nullptr;
	}

	CachedUDSActor = nullptr;
	CachedPlayerPawn = nullptr;
	CachedDLWEComponent = nullptr;
	CachedGSMSubsystem = nullptr;
	Config = nullptr;

	Super::Deinitialize();
}

void USOTS_UDSBridgeSubsystem::Console_ForceRefresh()
{
	RefreshCachedRefs(true);
	TickBridge();

	const FString UDS = CachedUDSActor.IsValid() ? CachedUDSActor->GetName() : TEXT("None");
	const FString Pawn = CachedPlayerPawn.IsValid() ? CachedPlayerPawn->GetName() : TEXT("None");
	const FString DLWE = CachedDLWEComponent.IsValid() ? CachedDLWEComponent->GetName() : TEXT("None");

	UE_LOG(LogSOTS_UDSBridge, Log, TEXT("UDSBridge refresh: UDS=%s Pawn=%s DLWE=%s"), *UDS, *Pawn, *DLWE);
}

void USOTS_UDSBridgeSubsystem::TickBridge()
{
	EnsureConfig();

	RefreshCachedRefs(false);
	RefreshCachedGSM();

	FSOTS_UDSBridgeState State;
	BuildStateFromCaches(State);
	LastState = State;

	PushSunDirToGSM(State);
	ApplyDLWEPolicy(State);
	TrySpawnTrailBreadcrumb(State);

	if (UWorld* World = GetWorld())
	{
		const double Now = World->GetTimeSeconds();
		EmitTelemetry(Now);

		// Re-jitter the interval each tick.
		World->GetTimerManager().SetTimer(UpdateTimerHandle, this, &USOTS_UDSBridgeSubsystem::TickBridge, GetJitteredInterval(), false);
	}
}

void USOTS_UDSBridgeSubsystem::RefreshCachedRefs(bool bForce)
{
	const double Now = GetWorldTimeSecondsSafe();

	if (bForce || !CachedUDSActor.IsValid())
	{
		CachedUDSActor = FindUDSActor();
		if (!CachedUDSActor.IsValid())
		{
			EmitWarningOnce(TEXT("UDSBridge: UDS actor not discovered."), bWarnedNoUDS, bWarnedMissingUDSOnce, Now);
		}
	}

	APawn* Pawn = CachedPlayerPawn.IsValid() ? CachedPlayerPawn.Get() : GetPlayerPawn();
	if (Pawn != CachedPlayerPawn.Get())
	{
		CachedPlayerPawn = Pawn;
		CachedDLWEComponent = nullptr;
		bValidatedDLWEFunctions = false;
	}

	if (!CachedDLWEComponent.IsValid() && Pawn)
	{
		CachedDLWEComponent = FindDLWEComponent(Pawn);
		bValidatedDLWEFunctions = false;

		if (CachedDLWEComponent.IsValid() && Config && Config->bEnableBridgeTelemetry)
		{
			FString Info;
			ValidateDLWEComponent(CachedDLWEComponent.Get(), Info);
			UE_LOG(LogSOTS_UDSBridge, Log, TEXT("UDSBridge DLWE component discovered: %s"), *Info);
		}
		else if (!CachedDLWEComponent.IsValid())
		{
			EmitWarningOnce(TEXT("UDSBridge: DLWE component not found on player pawn."), bWarnedNoDLWE, bWarnedMissingDLWEOnce, Now);
		}
	}
}

void USOTS_UDSBridgeSubsystem::RefreshCachedGSM()
{
	if (!CachedGSMSubsystem.IsValid())
	{
		CachedGSMSubsystem = GetGSMSubsystem();
	}
}

void USOTS_UDSBridgeSubsystem::BuildStateFromCaches(FSOTS_UDSBridgeState& OutState)
{
	OutState = FSOTS_UDSBridgeState();

	if (!Config)
	{
		return;
	}

	if (CachedUDSActor.IsValid())
	{
		ExtractWeatherFlags(CachedUDSActor.Get(), OutState);

		bool bHasSun = false;
		FVector SunDir;
		ExtractSunDirectionWS(CachedUDSActor.Get(), SunDir, bHasSun);
		OutState.bHasSunDir = bHasSun;
		OutState.SunDirWS = bHasSun ? SunDir : FVector::ZeroVector;
	}
	else
	{
		const double Now = GetWorldTimeSecondsSafe();
		EmitWarningOnce(TEXT("UDSBridge: UDS actor not found; weather/sun data unavailable."), bWarnedNoUDS, bWarnedMissingUDSOnce, Now);
	}
}

bool USOTS_UDSBridgeSubsystem::GetBridgeStateSnapshot(
	bool& bOutHasUDS,
	bool& bOutHasDLWE,
	bool& bOutHasGSM,
	bool& bOutSnowy,
	bool& bOutRaining,
	bool& bOutDusty,
	bool& bOutHasSunDir,
	FVector& OutSunDirWS)
{
	EnsureConfig();
	RefreshCachedRefs(false);
	RefreshCachedGSM();

	FSOTS_UDSBridgeState State;
	BuildStateFromCaches(State);
	LastState = State;

	bOutHasUDS = CachedUDSActor.IsValid();
	bOutHasDLWE = CachedDLWEComponent.IsValid();
	bOutHasGSM = CachedGSMSubsystem.IsValid();
	bOutSnowy = State.bSnowy;
	bOutRaining = State.bRaining;
	bOutDusty = State.bDusty;
	bOutHasSunDir = State.bHasSunDir;
	OutSunDirWS = State.bHasSunDir ? State.SunDirWS : FVector::ZeroVector;

	return Config != nullptr;
}

void USOTS_UDSBridgeSubsystem::ForceRefreshAndApply()
{
	EnsureConfig();
	RefreshCachedRefs(true);
	RefreshCachedGSM();

	FSOTS_UDSBridgeState State;
	BuildStateFromCaches(State);
	LastState = State;

	PushSunDirToGSM(State);
	ApplyDLWEPolicy(State);
}

AActor* USOTS_UDSBridgeSubsystem::FindUDSActor()
{
	if (!Config)
	{
		return nullptr;
	}

	if (Config->UDSActorSoftRef.IsValid())
	{
		return Config->UDSActorSoftRef.Get();
	}

	if (Config->UDSActorSoftRef.IsNull() == false)
	{
		return Config->UDSActorSoftRef.LoadSynchronous();
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Tag-based discovery
	if (Config->UDSActorTag != NAME_None)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(Config->UDSActorTag))
			{
				return *It;
			}
		}
	}

	// Name contains discovery
	if (!Config->UDSActorNameContains.IsEmpty())
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->GetName().Contains(Config->UDSActorNameContains))
			{
				return *It;
			}
		}
	}

	return nullptr;
}

APawn* USOTS_UDSBridgeSubsystem::GetPlayerPawn()
{
	if (UWorld* World = GetWorld())
	{
		return World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr;
	}

	return nullptr;
}

UActorComponent* USOTS_UDSBridgeSubsystem::FindDLWEComponent(APawn* Pawn)
{
	if (!Pawn || !Config)
	{
		return nullptr;
	}

	const FString& NameContains = Config->DLWEComponentNameContains;
	if (NameContains.IsEmpty())
	{
		return nullptr;
	}

	const TInlineComponentArray<UActorComponent*> Components(Pawn);
	for (UActorComponent* Component : Components)
	{
		if (Component && Component->GetName().Contains(NameContains))
		{
			return Component;
		}
	}

	return nullptr;
}

void USOTS_UDSBridgeSubsystem::EmitTelemetry(double NowSeconds)
{
	if (!Config || !Config->bEnableBridgeTelemetry)
	{
		return;
	}

	if (NowSeconds < NextTelemetryTimeSeconds)
	{
		return;
	}

	NextTelemetryTimeSeconds = NowSeconds + Config->TelemetryIntervalSeconds;

	const bool bHasUDS = CachedUDSActor.IsValid();
	const bool bHasDLWE = CachedDLWEComponent.IsValid();
	const bool bHasGSM = CachedGSMSubsystem.IsValid();

	const FString ApplyModeStr = [this]()
	{
		switch (ActiveApplyMode)
		{
		case EDLWEApplyMode::SettingsAsset: return FString(TEXT("Settings"));
		case EDLWEApplyMode::ToggleFunctions: return FString(TEXT("Toggles"));
		default: return FString(TEXT("None"));
		}
	}();

	const bool bPuddlesOn = Config && Config->bEnablePuddlesWhenRaining && LastState.bRaining;
	const FString SettingsName = LastSettingsApplied.IsValid() ? LastSettingsApplied->GetName() : TEXT("None");

	FString Telemetry = FString::Printf(TEXT("UDSBridge | UDS=%s DLWE=%s GSM=%s | Snow=%d Rain=%d Dust=%d | SunDirValid=%d | AppliedMode=%s | SnowOn=%d PudOn=%d DustOn=%d | Settings=%s"),
		bHasUDS ? TEXT("OK") : TEXT("NO"),
		bHasDLWE ? TEXT("OK") : TEXT("NO"),
		bHasGSM ? TEXT("OK") : TEXT("NO"),
		LastState.bSnowy ? 1 : 0,
		LastState.bRaining ? 1 : 0,
		LastState.bDusty ? 1 : 0,
		LastState.bHasSunDir ? 1 : 0,
		*ApplyModeStr,
		bLastSnowApplied ? 1 : 0,
		bPuddlesOn ? 1 : 0,
		bLastDustApplied ? 1 : 0,
		*SettingsName);

	if (Config->bEnableVerboseTelemetry)
	{
		Telemetry += FString::Printf(TEXT(" | SunDirWS=(%.2f,%.2f,%.2f) | UDSActor=%s"),
			LastState.SunDirWS.X,
			LastState.SunDirWS.Y,
			LastState.SunDirWS.Z,
			bHasUDS ? *CachedUDSActor->GetName() : TEXT("None"));
	}

	UE_LOG(LogSOTS_UDSBridge, Log, TEXT("%s"), *Telemetry);
}

float USOTS_UDSBridgeSubsystem::GetJitteredInterval() const
{
	const float Base = Config ? Config->UpdateIntervalSeconds : 0.35f;
	const float Jitter = Config ? Config->IntervalJitterSeconds : 0.05f;
	return FMath::Max(0.01f, Base + FMath::FRandRange(-Jitter, Jitter));
}

void USOTS_UDSBridgeSubsystem::EnsureConfig()
{
	if (Config)
	{
		return;
	}

	const USOTS_UDSBridgeSettings* Settings = GetDefault<USOTS_UDSBridgeSettings>();
	if (Settings)
	{
		if (Settings->DefaultBridgeConfig.IsValid())
		{
			Config = Settings->DefaultBridgeConfig.Get();
		}
		else if (Settings->DefaultBridgeConfig.ToSoftObjectPath().IsValid())
		{
			Config = Settings->DefaultBridgeConfig.LoadSynchronous();
		}
	}

	if (!Config)
	{
		Config = NewObject<USOTS_UDSBridgeConfig>(this, USOTS_UDSBridgeConfig::StaticClass());
	}
}

bool USOTS_UDSBridgeSubsystem::TryReadBoolProperty(UObject* Obj, FName PropName, bool& OutValue) const
{
	if (!Obj || !PropName.IsValid())
	{
		return false;
	}

	if (const FBoolProperty* BoolProp = FindFProperty<FBoolProperty>(Obj->GetClass(), PropName))
	{
		OutValue = BoolProp->GetPropertyValue_InContainer(Obj);
		return true;
	}

	return false;
}

bool USOTS_UDSBridgeSubsystem::TryReadBoolByPath(UObject* Obj, const FString& Path, bool& OutValue) const
{
	if (!Obj || Path.IsEmpty())
	{
		return false;
	}

	TArray<FString> Segments;
	Path.ParseIntoArray(Segments, TEXT("."));
	if (Segments.Num() == 0)
	{
		return false;
	}

	void* Container = Obj;
	UStruct* Struct = Obj->GetClass();

	for (int32 Index = 0; Index < Segments.Num(); ++Index)
	{
		const FString& Segment = Segments[Index];
		if (!Struct)
		{
			return false;
		}

		if (FProperty* Prop = FindFProperty<FProperty>(Struct, FName(*Segment)))
		{
			const bool bLast = Index == Segments.Num() - 1;
			if (bLast)
			{
				if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
				{
					OutValue = BoolProp->GetPropertyValue_InContainer(Container);
					return true;
				}
				return false;
			}

			if (FStructProperty* StructProp = CastField<FStructProperty>(Prop))
			{
				Container = StructProp->ContainerPtrToValuePtr<void>(Container);
				Struct = StructProp->Struct;
				continue;
			}

			if (FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(Prop))
			{
				UObject* NextObj = ObjProp->GetObjectPropertyValue_InContainer(Container);
				Container = NextObj;
				Struct = NextObj ? NextObj->GetClass() : nullptr;
				continue;
			}

			return false;
		}
		else
		{
			return false;
		}
	}

	return false;
}

bool USOTS_UDSBridgeSubsystem::TryCallVectorFunction(UObject* Obj, FName FuncName, FVector& OutVec) const
{
	if (!Obj || !FuncName.IsValid())
	{
		return false;
	}

	if (UFunction* Fn = Obj->FindFunction(FuncName))
	{
		if (const FProperty* ReturnProp = Fn->GetReturnProperty())
		{
			if (const FStructProperty* StructProp = CastField<FStructProperty>(ReturnProp))
			{
				if (StructProp->Struct == TBaseStructure<FVector>::Get())
				{
					struct FVectorReturnParams
					{
						FVector ReturnValue;
					};

					FVectorReturnParams Params{FVector::ZeroVector};
					Obj->ProcessEvent(Fn, &Params);
					OutVec = Params.ReturnValue;
					return true;
				}
			}
		}
	}

	return false;
}

UObject* USOTS_UDSBridgeSubsystem::TryGetObjectProperty(UObject* Obj, FName PropName) const
{
	if (!Obj || !PropName.IsValid())
	{
		return nullptr;
	}

	if (FObjectPropertyBase* ObjProp = FindFProperty<FObjectPropertyBase>(Obj->GetClass(), PropName))
	{
		return ObjProp->GetObjectPropertyValue_InContainer(Obj);
	}

	return nullptr;
}

bool USOTS_UDSBridgeSubsystem::ExtractWeatherFlags(AActor* UDSActor, FSOTS_UDSBridgeState& OutState) const
{
	if (!UDSActor || !Config)
	{
		return false;
	}

	bool bFoundAny = false;
	bool Temp = false;

	if (TryReadBoolProperty(UDSActor, Config->Weather_bSnowy_Property, Temp) || TryReadBoolByPath(UDSActor, Config->WeatherPath_Snowy, Temp))
	{
		OutState.bSnowy = Temp;
		bFoundAny = true;
	}

	if (TryReadBoolProperty(UDSActor, Config->Weather_bRaining_Property, Temp) || TryReadBoolByPath(UDSActor, Config->WeatherPath_Raining, Temp))
	{
		OutState.bRaining = Temp;
		bFoundAny = true;
	}

	if (TryReadBoolProperty(UDSActor, Config->Weather_bDusty_Property, Temp) || TryReadBoolByPath(UDSActor, Config->WeatherPath_Dusty, Temp))
	{
		OutState.bDusty = Temp;
		bFoundAny = true;
	}

	return bFoundAny;
}

bool USOTS_UDSBridgeSubsystem::TryReadRotatorProperty(UObject* Obj, FName PropName, FRotator& OutRot) const
{
	if (!Obj || !PropName.IsValid())
	{
		return false;
	}

	if (FStructProperty* StructProp = FindFProperty<FStructProperty>(Obj->GetClass(), PropName))
	{
		if (StructProp->Struct == TBaseStructure<FRotator>::Get())
		{
			OutRot = *StructProp->ContainerPtrToValuePtr<FRotator>(Obj);
			return true;
		}
	}

	return false;
}

bool USOTS_UDSBridgeSubsystem::ExtractSunDirectionWS(AActor* UDSActor, FVector& OutDirWS, bool& bValid) const
{
	bValid = false;
	OutDirWS = FVector::ZeroVector;

	if (!UDSActor || !Config)
	{
		return false;
	}

	// Try function first
	if (Config->SunDirectionFunctionName.IsValid())
	{
		FVector Dir;
		if (TryCallVectorFunction(UDSActor, Config->SunDirectionFunctionName, Dir))
		{
			if (!Dir.IsNearlyZero())
			{
				OutDirWS = Dir.GetSafeNormal();
				bValid = true;
				return true;
			}
		}
	}

	// Try directional light reference
	if (UObject* LightObj = TryGetObjectProperty(UDSActor, Config->SunLightActorProperty))
	{
		if (AActor* LightActor = Cast<AActor>(LightObj))
		{
			FVector Forward = LightActor->GetActorForwardVector();
			if (Config->bInvertLightForwardVector)
			{
				Forward *= -1.f;
			}

			if (!Forward.IsNearlyZero())
			{
				OutDirWS = Forward.GetSafeNormal();
				bValid = true;
				return true;
			}
		}
	}

	// Try rotator property
	if (Config->SunWorldRotationProperty.IsValid())
	{
		FRotator Rot;
		if (TryReadRotatorProperty(UDSActor, Config->SunWorldRotationProperty, Rot))
		{
			FVector Forward = Rot.Vector();
			if (Config->bInvertLightForwardVector)
			{
				Forward *= -1.f;
			}

			if (!Forward.IsNearlyZero())
			{
				OutDirWS = Forward.GetSafeNormal();
				bValid = true;
				return true;
			}
		}
	}

	return false;
}

bool USOTS_UDSBridgeSubsystem::ValidateDLWEComponent(UActorComponent* DLWEComp, FString& OutInfo)
{
	if (!DLWEComp || !Config)
	{
		return false;
	}

	bHasSnowToggle = false;
	bHasRainToggle = false;
	bHasDustToggle = false;
	bHasSettingsSurface = false;

	UpdateApplyMode(DLWEComp);

	TArray<FString> Missing;

	if (!bHasSnowToggle) Missing.Add(Config->DLWE_EnableSnow_Function.ToString());
	if (!bHasRainToggle) Missing.Add(Config->DLWE_EnableRain_Function.ToString());
	if (!bHasDustToggle) Missing.Add(Config->DLWE_EnableDust_Function.ToString());

	OutInfo = FString::Printf(TEXT("DLWEComp=%s Mode=%s SnowFunc=%d RainFunc=%d DustFunc=%d Missing=%s"),
		*DLWEComp->GetClass()->GetName(),
		ActiveApplyMode == EDLWEApplyMode::SettingsAsset ? TEXT("Settings") : (ActiveApplyMode == EDLWEApplyMode::ToggleFunctions ? TEXT("Toggles") : TEXT("None")),
		bHasSnowToggle ? 1 : 0,
		bHasRainToggle ? 1 : 0,
		bHasDustToggle ? 1 : 0,
		*FString::Join(Missing, TEXT(",")));

	bValidatedDLWEFunctions = true;
	return true;
}

void USOTS_UDSBridgeSubsystem::PushSunDirToGSM(const FSOTS_UDSBridgeState& State)
{
	USOTS_GlobalStealthManagerSubsystem* GSM = GetGSMSubsystem();
	const double Now = GetWorldTimeSecondsSafe();

	if (!GSM)
	{
		EmitWarningOnce(TEXT("UDSBridge: GlobalStealthManager subsystem missing; cannot push sun direction."), bWarnedNoGSM, bWarnedMissingGSMOnce, Now);
		return;
	}

	if (!State.bHasSunDir)
	{
		if (bLastPushedSunValid)
		{
			GSM->SetDominantDirectionalLightDirectionWS(FVector::ZeroVector, false);
			bLastPushedSunValid = false;
			LastPushedSunDirWS = FVector::ZeroVector;
		}
		return;
	}

	if (!bLastPushedSunValid || !State.SunDirWS.Equals(LastPushedSunDirWS, SunDirCompareTolerance))
	{
		GSM->SetDominantDirectionalLightDirectionWS(State.SunDirWS, true);
		bLastPushedSunValid = true;
		LastPushedSunDirWS = State.SunDirWS;
	}
}

void USOTS_UDSBridgeSubsystem::ApplyDLWEPolicy(const FSOTS_UDSBridgeState& State)
{
	const double Now = GetWorldTimeSecondsSafe();

	if (!Config)
	{
		return;
	}

	UActorComponent* DLWEComp = CachedDLWEComponent.Get();
	if (!DLWEComp)
	{
		EmitWarningOnce(TEXT("UDSBridge: DLWE component missing; cannot apply policy."), bWarnedNoDLWE, bWarnedMissingDLWEOnce, Now);
		return;
	}

	if (!bValidatedDLWEFunctions)
	{
		FString Info;
		ValidateDLWEComponent(DLWEComp, Info);
	}

	if (ActiveApplyMode == EDLWEApplyMode::None)
	{
		EmitWarningOnce(TEXT("UDSBridge: No DLWE apply surface available (settings or toggles)."), bWarnedNoDLWESurface, bWarnedMissingDLWESurfaceOnce, Now);
		return;
	}

	const bool bSnowOn = Config->bEnableSnowInteractionWhenSnowy && State.bSnowy;
	const bool bRainOn = Config->bEnablePuddlesWhenRaining && State.bRaining;
	const bool bDustOn = Config->bEnableDustInteractionWhenDusty && State.bDusty;

	if (ActiveApplyMode == EDLWEApplyMode::SettingsAsset)
	{
		UObject* DesiredSettings = nullptr;
		if (bSnowOn && Config->DLWE_Settings_Snow.IsValid())
		{
			DesiredSettings = Config->DLWE_Settings_Snow.LoadSynchronous();
		}
		else if (bRainOn && Config->DLWE_Settings_Rain.IsValid())
		{
			DesiredSettings = Config->DLWE_Settings_Rain.LoadSynchronous();
		}
		else if (bDustOn && Config->DLWE_Settings_Dust.IsValid())
		{
			DesiredSettings = Config->DLWE_Settings_Dust.LoadSynchronous();
		}
		else if (Config->DLWE_Settings_Clear.IsValid())
		{
			DesiredSettings = Config->DLWE_Settings_Clear.LoadSynchronous();
		}

		if (!DesiredSettings)
		{
			EmitWarningOnce(TEXT("UDSBridge: DLWE settings asset not provided for current weather."), bWarnedNoDLWESurface, bWarnedMissingDLWESurfaceOnce, Now);
			return;
		}

		if (CallDLWE_SettingsFunction(DLWEComp, Config->DLWE_Func_SetInteractionSettings, DesiredSettings))
		{
			LastSettingsApplied = DesiredSettings;
			bLastSnowApplied = bSnowOn;
			bLastRainApplied = bRainOn;
			bLastDustApplied = bDustOn;
		}
		return;
	}

	// Toggle-based surface
	if (bHasSnowToggle && bLastSnowApplied != bSnowOn)
	{
		CallDLWE_BoolFunction(DLWEComp, Config->DLWE_EnableSnow_Function, bSnowOn);
		bLastSnowApplied = bSnowOn;
	}

	if (bHasRainToggle && bLastRainApplied != bRainOn)
	{
		CallDLWE_BoolFunction(DLWEComp, Config->DLWE_EnableRain_Function, bRainOn);
		bLastRainApplied = bRainOn;
	}

	if (bHasDustToggle && bLastDustApplied != bDustOn)
	{
		CallDLWE_BoolFunction(DLWEComp, Config->DLWE_EnableDust_Function, bDustOn);
		bLastDustApplied = bDustOn;
	}
}

void USOTS_UDSBridgeSubsystem::UpdateApplyMode(UActorComponent* DLWEComp)
{
	ActiveApplyMode = EDLWEApplyMode::None;

	if (!Config || !DLWEComp)
	{
		return;
	}

	// Settings surface preferred if signature is safe
	if (Config->DLWE_Func_SetInteractionSettings.IsValid())
	{
		if (UFunction* Fn = DLWEComp->FindFunction(Config->DLWE_Func_SetInteractionSettings))
		{
			if (IsSingleObjectParamFunction(Fn))
			{
				ActiveApplyMode = EDLWEApplyMode::SettingsAsset;
				bHasSettingsSurface = true;
				return;
			}
			else
			{
				const double Now = GetWorldTimeSecondsSafe();
				EmitWarningOnce(TEXT("UDSBridge: DLWE settings function signature mismatch; skipping settings surface."), bWarnedNoDLWESurface, bWarnedMissingDLWESurfaceOnce, Now);
			}
		}
	}

	auto ValidateBoolToggle = [this, DLWEComp](FName FuncName, bool& bOutHasFlag)
	{
		if (!FuncName.IsValid())
		{
			return;
		}

		if (UFunction* Fn = DLWEComp->FindFunction(FuncName))
		{
			if (IsSingleBoolParamFunction(Fn))
			{
				bOutHasFlag = true;
			}
			else
			{
				const double Now = GetWorldTimeSecondsSafe();
				EmitWarningOnce(TEXT("UDSBridge: DLWE toggle signature mismatch; skipping call."), bWarnedNoDLWESurface, bWarnedMissingDLWESurfaceOnce, Now);
			}
		}
	};

	ValidateBoolToggle(Config->DLWE_EnableSnow_Function, bHasSnowToggle);
	ValidateBoolToggle(Config->DLWE_EnableRain_Function, bHasRainToggle);
	ValidateBoolToggle(Config->DLWE_EnableDust_Function, bHasDustToggle);

	if (bHasSnowToggle || bHasRainToggle || bHasDustToggle)
	{
		ActiveApplyMode = EDLWEApplyMode::ToggleFunctions;
	}
}

void USOTS_UDSBridgeSubsystem::EmitWarningOnce(const TCHAR* Message, bool& bWarnedFlag, bool& bWarnedOnceFlag, double NowSeconds)
{
	if (!Config || !Config->bEnableBridgeWarnings)
	{
		return;
	}

	if (bWarnedOnceFlag)
	{
		return;
	}

	if (!ShouldThrottleWarning(NowSeconds))
	{
		return;
	}

	UE_LOG(LogSOTS_UDSBridge, Warning, TEXT("%s"), Message);
	bWarnedFlag = true;
	bWarnedOnceFlag = true;
}

bool USOTS_UDSBridgeSubsystem::ShouldThrottleWarning(double NowSeconds)
{
	if (NowSeconds < NextWarningTimeSeconds)
	{
		return false;
	}

	NextWarningTimeSeconds = NowSeconds + WarningThrottleSeconds;
	return true;
}

USOTS_GlobalStealthManagerSubsystem* USOTS_UDSBridgeSubsystem::GetGSMSubsystem()
{
	if (CachedGSMSubsystem.IsValid())
	{
		return CachedGSMSubsystem.Get();
	}

	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			CachedGSMSubsystem = GI->GetSubsystem<USOTS_GlobalStealthManagerSubsystem>();
			return CachedGSMSubsystem.Get();
		}
	}

	return nullptr;
}

double USOTS_UDSBridgeSubsystem::GetWorldTimeSecondsSafe() const
{
	if (const UWorld* World = GetWorld())
	{
		return World->GetTimeSeconds();
	}

	return 0.0;
}

bool USOTS_UDSBridgeSubsystem::CallDLWE_BoolFunction(UObject* DLWEComp, FName FuncName, bool bValue) const
{
	if (!DLWEComp || !FuncName.IsValid())
	{
		return false;
	}

	if (UFunction* Fn = DLWEComp->FindFunction(FuncName))
	{
		if (!IsSingleBoolParamFunction(Fn))
		{
			return false;
		}

		struct FBoolParam
		{
			bool bEnabled = false;
		};

		FBoolParam Params;
		Params.bEnabled = bValue;
		DLWEComp->ProcessEvent(Fn, &Params);
		return true;
	}

	return false;
}

bool USOTS_UDSBridgeSubsystem::CallDLWE_SettingsFunction(UObject* DLWEComp, FName FuncName, UObject* SettingsObj) const
{
	if (!DLWEComp || !FuncName.IsValid())
	{
		return false;
	}

	if (UFunction* Fn = DLWEComp->FindFunction(FuncName))
	{
		if (!IsSingleObjectParamFunction(Fn))
		{
			return false;
		}

		struct FSettingsParam
		{
			UObject* Settings = nullptr;
		};

		FSettingsParam Params;
		Params.Settings = SettingsObj;
		DLWEComp->ProcessEvent(Fn, &Params);
		return true;
	}

	return false;
}

bool USOTS_UDSBridgeSubsystem::IsSingleBoolParamFunction(UFunction* Fn) const
{
	if (!Fn)
	{
		return false;
	}

	int32 ParamCount = 0;
	for (TFieldIterator<FProperty> It(Fn); It; ++It)
	{
		FProperty* Prop = *It;
		if (!Prop->HasAnyPropertyFlags(CPF_Parm) || Prop->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			continue;
		}

		++ParamCount;
		if (!Prop->IsA(FBoolProperty::StaticClass()))
		{
			return false;
		}
	}

	return ParamCount == 1;
}

bool USOTS_UDSBridgeSubsystem::IsSingleObjectParamFunction(UFunction* Fn) const
{
	if (!Fn)
	{
		return false;
	}

	int32 ParamCount = 0;
	for (TFieldIterator<FProperty> It(Fn); It; ++It)
	{
		FProperty* Prop = *It;
		if (!Prop->HasAnyPropertyFlags(CPF_Parm) || Prop->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			continue;
		}

		++ParamCount;
		if (!Prop->IsA(FObjectPropertyBase::StaticClass()))
		{
			return false;
		}
	}

	return ParamCount == 1;
}

void USOTS_UDSBridgeSubsystem::TrySpawnTrailBreadcrumb(const FSOTS_UDSBridgeState& State)
{
	if (!Config || !Config->bEnableTrailBreadcrumbs)
	{
		return;
	}

	if (!State.bSnowy)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(CachedPlayerPawn.Get());
	if (!Character)
	{
		return;
	}

	const UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
	if (!MoveComp || !MoveComp->IsMovingOnGround())
	{
		return;
	}

	const float Speed2D = Character->GetVelocity().Size2D();
	if (Speed2D < Config->BreadcrumbMinSpeed2D)
	{
		return;
	}

	const double Now = World->GetTimeSeconds();
	if ((Now - LastBreadcrumbSpawnTime) < Config->BreadcrumbInterval)
	{
		return;
	}

	const FVector ActorLoc = Character->GetActorLocation();
	if (!LastBreadcrumbLocation.IsNearlyZero() && FVector::Dist2D(ActorLoc, LastBreadcrumbLocation) < Config->BreadcrumbMinDistance)
	{
		return;
	}

	FVector SpawnLoc = ActorLoc;
	FHitResult Hit;
	const FVector TraceStart = ActorLoc;
	const FVector TraceEnd = TraceStart - FVector(0.f, 0.f, 200.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SOTS_TrailBreadcrumbTrace), false, Character);
	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		SpawnLoc = Hit.ImpactPoint;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ASOTS_TrailBreadcrumb* NewBreadcrumb = World->SpawnActor<ASOTS_TrailBreadcrumb>(ASOTS_TrailBreadcrumb::StaticClass(), SpawnLoc, FRotator::ZeroRotator, SpawnParams);
	if (!NewBreadcrumb)
	{
		return;
	}

	NewBreadcrumb->LifespanSeconds = Config->BreadcrumbLifespanSeconds;
	NewBreadcrumb->HearingLoudness = Config->BreadcrumbHearingLoudness;
	NewBreadcrumb->HearingMaxRange = Config->BreadcrumbHearingMaxRange;
	NewBreadcrumb->HearingTag = Config->BreadcrumbHearingTag;
	NewBreadcrumb->bEmitHearingEventOnSpawn = Config->bEmitBreadcrumbHearingEvent;

	NewBreadcrumb->Prev = BreadcrumbTail.Get();
	if (BreadcrumbTail.IsValid())
	{
		BreadcrumbTail->Next = NewBreadcrumb;
	}
	else
	{
		BreadcrumbHead = NewBreadcrumb;
	}
	BreadcrumbTail = NewBreadcrumb;

	AliveBreadcrumbCount = FMath::Max(0, AliveBreadcrumbCount + 1);
	LastBreadcrumbSpawnTime = Now;
	LastBreadcrumbLocation = SpawnLoc;

	NewBreadcrumb->OnDestroyed.AddDynamic(this, &USOTS_UDSBridgeSubsystem::OnBreadcrumbDestroyed);

	if (Config->bEnableBridgeTelemetry && Now >= NextTelemetryTimeSeconds)
	{
		UE_LOG(LogSOTS_UDSBridge, Log, TEXT("UDSBridge breadcrumb spawn | Loc=(%.1f,%.1f,%.1f) Alive=%d HeadSet=%d TailSet=%d"),
			SpawnLoc.X, SpawnLoc.Y, SpawnLoc.Z,
			AliveBreadcrumbCount,
			BreadcrumbHead.IsValid() ? 1 : 0,
			BreadcrumbTail.IsValid() ? 1 : 0);
	}

	while (AliveBreadcrumbCount > Config->BreadcrumbMaxAlive)
	{
		ASOTS_TrailBreadcrumb* Head = BreadcrumbHead.Get();
		if (Head)
		{
			Head->Destroy();
		}
		else
		{
			BreadcrumbTail = nullptr;
			AliveBreadcrumbCount = 0;
			break;
		}
	}
}

void USOTS_UDSBridgeSubsystem::OnBreadcrumbDestroyed(AActor* DestroyedActor)
{
	ASOTS_TrailBreadcrumb* Breadcrumb = Cast<ASOTS_TrailBreadcrumb>(DestroyedActor);
	if (!Breadcrumb)
	{
		return;
	}

	if (BreadcrumbHead.Get() == Breadcrumb)
	{
		BreadcrumbHead = Breadcrumb->Next;
	}

	if (BreadcrumbTail.Get() == Breadcrumb)
	{
		BreadcrumbTail = Breadcrumb->Prev;
	}

	AliveBreadcrumbCount = FMath::Max(0, AliveBreadcrumbCount - 1);
}
