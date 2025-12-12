// Created by Satheesh (ryanjon2040). Copyright 2024.

#include "Subsystems/BlueprintWorldSubsystem.h"
#include "BlueprintWorldSubsystemBase.h"
#include "BlueprintSubsystem/BlueprintSubsystemMacros.h"
#include "Logging/StructuredLog.h"
#include "Runtime/Engine/Classes/Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogBlueprintWorldSubsystem, All, All);

UBlueprintWorldSubsystem::UBlueprintWorldSubsystem()
	: bEnabled(true)
	, bStartWithTickEnabled(false)
{
	IMPLEMENTED_IN_BP_LAMBDA();

	CHECK_IN_BP(UBlueprintWorldSubsystem, ShouldCreateSubsystem);
	CHECK_IN_BP(UBlueprintWorldSubsystem, OnPostInitialize);
	CHECK_IN_BP(UBlueprintWorldSubsystem, OnInit);
	CHECK_IN_BP(UBlueprintWorldSubsystem, OnWorldBeginPlay);
	CHECK_IN_BP(UBlueprintWorldSubsystem, OnWorldComponentsUpdated);
	CHECK_IN_BP(UBlueprintWorldSubsystem, OnUpdateStreamingState);
	CHECK_IN_BP(UBlueprintWorldSubsystem, OnTick);
	CHECK_IN_BP(UBlueprintWorldSubsystem, OnDeInit);
}

UBlueprintWorldSubsystem* UBlueprintWorldSubsystem::Create(UBlueprintWorldSubsystemBase* Owner,
	const TSoftClassPtr<UBlueprintWorldSubsystem>& SoftClassPtr)
{
	if (!IsValid(Owner))
	{
		return nullptr;
	}

	if (SoftClassPtr.IsNull())
	{
		return nullptr;
	}

	auto ProxySubsystem = NewObject<UBlueprintWorldSubsystem>(Owner, SoftClassPtr.LoadSynchronous(), NAME_None, RF_Transient);
	if (!ProxySubsystem->Internal_ShouldCreateSubsystem())
	{
		ProxySubsystem->MarkAsGarbage();
		ProxySubsystem = nullptr;
		return nullptr;
	}

	ProxySubsystem->Internal_OnInit();
	return ProxySubsystem;
}

UWorld* UBlueprintWorldSubsystem::GetWorld() const
{
	if (const auto LastWorld = CachedWorld.Get())
	{
		return LastWorld;
	}

	if (HasAllFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

	const UObject* Outer = GetOuter();
	while (Outer)
	{
		if (const auto World = Outer->GetWorld())
		{
			CachedWorld = World;
			return World;
		}

		Outer = Outer->GetOuter();
	}

	return nullptr;
}

void UBlueprintWorldSubsystem::Tick(float DeltaTime)
{
	if (bHasBlueprintOnTick)
	{
		K2_OnTick(DeltaTime);
	}
}

void UBlueprintWorldSubsystem::SetTickEnabled(const bool bEnableTick)
{
	bIsAllowedToTick = bEnableTick;
}

bool UBlueprintWorldSubsystem::Internal_ShouldCreateSubsystem() const
{
	if (!bEnabled)
	{
		UE_LOGFMT(LogBlueprintWorldSubsystem, Warning, "{0} is not enabled. Subsystem will not be created.", GetClass()->GetName());
		return false;
	}

	if (bHasBlueprintShouldCreateSubsystem)
	{
		FText OutReason;
		if (!K2_ShouldCreateSubsystem(OutReason))
		{
			FString OutReasonString = OutReason.ToString();
			if (OutReason.IsEmptyOrWhitespace())
			{
				OutReasonString = "No reason specified.";
			}
		
			UE_LOGFMT(LogBlueprintWorldSubsystem, Warning, "{0} create failed. Reason: {1}.", GetClass()->GetName(), OutReasonString);
			return false;
		}
	}
	
	return true;
}

void UBlueprintWorldSubsystem::Internal_OnInit()
{
	if (bHasBlueprintOnInit)
	{
		UE_LOGFMT(LogBlueprintWorldSubsystem, Log, "Calling OnInit on {0}", GetClass()->GetName());
		K2_OnInit();
	}
	
	SetTickEnabled(bStartWithTickEnabled);
}

void UBlueprintWorldSubsystem::OnPostInitialize()
{
	if (bHasBlueprintOnPostInitialize)
	{
		K2_OnPostInitialize();
	}
}

void UBlueprintWorldSubsystem::OnWorldComponentsUpdated(UWorld* World)
{
	if (bHasBlueprintOnWorldComponentsUpdated)
	{
		K2_OnWorldComponentsUpdated(World);
	}
}

void UBlueprintWorldSubsystem::OnWorldBeginPlay(UWorld* World)
{
	if (bHasBlueprintOnWorldBeginPlay)
	{
		K2_OnWorldBeginPlay(World);
	}
}

void UBlueprintWorldSubsystem::OnUpdateStreamingState()
{
	if (bHasBlueprintOnUpdateStreamingState)
	{
		K2_OnUpdateStreamingState();
	}
}

void UBlueprintWorldSubsystem::OnDeInit()
{
	if (bHasBlueprintOnDeInit)
	{
		UE_LOGFMT(LogBlueprintWorldSubsystem, Log, "Calling OnShutdown on {0}", GetClass()->GetName());
		K2_OnDeInit();
	}
}
