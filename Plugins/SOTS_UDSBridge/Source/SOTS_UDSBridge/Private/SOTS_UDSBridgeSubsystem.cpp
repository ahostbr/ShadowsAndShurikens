#include "SOTS_UDSBridgeSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "SOTS_UDSBridgeConfig.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_UDSBridge, Log, All);

void USOTS_UDSBridgeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	EnsureConfig();

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

	CachedUDSActor = nullptr;
	CachedPlayerPawn = nullptr;
	CachedDLWEComponent = nullptr;
	Config = nullptr;

	Super::Deinitialize();
}

void USOTS_UDSBridgeSubsystem::TickBridge()
{
	if (!Config)
	{
		EnsureConfig();
	}

	RefreshCachedRefs(false);

	FSOTS_UDSBridgeState State;
	if (CachedUDSActor.IsValid())
	{
		ExtractWeatherFlags(CachedUDSActor.Get(), State);

		bool bHasSun = false;
		FVector SunDir;
		ExtractSunDirectionWS(CachedUDSActor.Get(), SunDir, bHasSun);
		State.bHasSunDir = bHasSun;
		State.SunDirWS = bHasSun ? SunDir : FVector::ZeroVector;
	}
	else if (Config && Config->bEnableBridgeWarnings && !bWarnedNoUDS)
	{
		UE_LOG(LogSOTS_UDSBridge, Warning, TEXT("UDSBridge: UDS actor not found; weather/sun data unavailable."));
		bWarnedNoUDS = true;
	}

	LastState = State;

	PushSunDirToGSM(State);
	ApplyDLWEPolicy(State);

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
	if (bForce || !CachedUDSActor.IsValid())
	{
		CachedUDSActor = FindUDSActor();
	}

	APawn* Pawn = CachedPlayerPawn.IsValid() ? CachedPlayerPawn.Get() : GetPlayerPawn();
	if (Pawn != CachedPlayerPawn.Get())
	{
		CachedPlayerPawn = Pawn;
		CachedDLWEComponent = nullptr;
	}

	if (!CachedDLWEComponent.IsValid() && Pawn)
	{
		CachedDLWEComponent = FindDLWEComponent(Pawn);
		if (CachedDLWEComponent.IsValid() && Config && Config->bEnableBridgeTelemetry)
		{
			FString Info;
			ValidateDLWEComponent(CachedDLWEComponent.Get(), Info);
			UE_LOG(LogSOTS_UDSBridge, Log, TEXT("UDSBridge DLWE component discovered: %s"), *Info);
		}
		else if (!CachedDLWEComponent.IsValid() && Config && Config->bEnableBridgeWarnings && !bWarnedNoDLWE)
		{
			UE_LOG(LogSOTS_UDSBridge, Warning, TEXT("UDSBridge: DLWE component not found on player pawn."));
			bWarnedNoDLWE = true;
		}
	}
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
	const bool bHasPawn = CachedPlayerPawn.IsValid();
	const bool bHasDLWE = CachedDLWEComponent.IsValid();

	UE_LOG(LogSOTS_UDSBridge, Log, TEXT("UDSBridge telemetry: UDS=%s Pawn=%s DLWE=%s Snow=%d Rain=%d Dust=%d SunValid=%d Dir=%s"),
		bHasUDS ? TEXT("OK") : TEXT("NO"),
		bHasPawn ? TEXT("OK") : TEXT("NO"),
		bHasDLWE ? TEXT("OK") : TEXT("NO"),
		LastState.bSnowy ? 1 : 0,
		LastState.bRaining ? 1 : 0,
		LastState.bDusty ? 1 : 0,
		LastState.bHasSunDir ? 1 : 0,
		*LastState.SunDirWS.ToString());
}

float USOTS_UDSBridgeSubsystem::GetJitteredInterval() const
{
	const float Base = Config ? Config->UpdateIntervalSeconds : 0.35f;
	const float Jitter = Config ? Config->IntervalJitterSeconds : 0.05f;
	return FMath::Max(0.01f, Base + FMath::FRandRange(-Jitter, Jitter));
}

void USOTS_UDSBridgeSubsystem::EnsureConfig()
{
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
		if (Fn->GetReturnProperty() && Fn->GetReturnProperty()->IsA(FStructProperty::StaticClass()))
		{
			struct FVectorReturnParams
			{
				FVector ReturnValue;
			};

			FVectorReturnParams Params;
			Obj->ProcessEvent(Fn, &Params);
			OutVec = Params.ReturnValue;
			return true;
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

	TArray<FString> Missing;
	auto HasFunc = [DLWEComp](FName FuncName)
	{
		return FuncName.IsValid() && DLWEComp->FindFunction(FuncName) != nullptr;
	};

	const bool bSnowFunc = HasFunc(Config->DLWE_EnableSnow_Function);
	const bool bRainFunc = HasFunc(Config->DLWE_EnableRain_Function);
	const bool bDustFunc = HasFunc(Config->DLWE_EnableDust_Function);

	if (!bSnowFunc) Missing.Add(Config->DLWE_EnableSnow_Function.ToString());
	if (!bRainFunc) Missing.Add(Config->DLWE_EnableRain_Function.ToString());
	if (!bDustFunc) Missing.Add(Config->DLWE_EnableDust_Function.ToString());

	OutInfo = FString::Printf(TEXT("DLWEComp=%s SnowFunc=%d RainFunc=%d DustFunc=%d Missing=%s"),
		*DLWEComp->GetClass()->GetName(),
		bSnowFunc ? 1 : 0,
		bRainFunc ? 1 : 0,
		bDustFunc ? 1 : 0,
		*FString::Join(Missing, TEXT(",")));

	return true;
}
