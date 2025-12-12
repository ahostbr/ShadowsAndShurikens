// Created by Satheesh (ryanjon2040). Copyright 2024.

#include "Subsystems/BlueprintGameInstanceSubsystem.h"
#include "BlueprintGameInstanceSubsystemBase.h"
#include "BlueprintSubsystem/BlueprintSubsystemMacros.h"
#include "Logging/StructuredLog.h"
#include "Runtime/Engine/Classes/Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogBlueprintGameInstanceSubsystem, All, All);

UBlueprintGameInstanceSubsystem::UBlueprintGameInstanceSubsystem()
	: bEnabled(true)
	, bStartWithTickEnabled(false)
{
	IMPLEMENTED_IN_BP_LAMBDA();

	CHECK_IN_BP(UBlueprintGameInstanceSubsystem, ShouldCreateSubsystem);
	CHECK_IN_BP(UBlueprintGameInstanceSubsystem, OnInit);
	CHECK_IN_BP(UBlueprintGameInstanceSubsystem, OnTick);
	CHECK_IN_BP(UBlueprintGameInstanceSubsystem, OnDeInit);
}

UBlueprintGameInstanceSubsystem* UBlueprintGameInstanceSubsystem::Create(UBlueprintGameInstanceSubsystemBase* Owner,
	const TSoftClassPtr<UBlueprintGameInstanceSubsystem>& SoftClassPtr)
{
	if (!IsValid(Owner))
	{
		return nullptr;
	}

	if (SoftClassPtr.IsNull())
	{
		return nullptr;
	}

	auto ProxySubsystem = NewObject<UBlueprintGameInstanceSubsystem>(Owner, SoftClassPtr.LoadSynchronous(), NAME_None, RF_Transient);
	if (!ProxySubsystem->Internal_ShouldCreateSubsystem())
	{
		ProxySubsystem->MarkAsGarbage();
		ProxySubsystem = nullptr;
		return nullptr;
	}

	ProxySubsystem->ParentSubsystem = Owner;
	ProxySubsystem->Internal_OnInit();
	return ProxySubsystem;
}

UWorld* UBlueprintGameInstanceSubsystem::GetWorld() const
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

UWorld* UBlueprintGameInstanceSubsystem::GetTickableGameObjectWorld() const
{
	return GetWorld();
}

void UBlueprintGameInstanceSubsystem::Tick(float DeltaTime)
{
	if (bHasBlueprintOnTick)
	{
		K2_OnTick(DeltaTime);
	}
}

void UBlueprintGameInstanceSubsystem::SetTickEnabled(const bool bEnableTick)
{
	bIsAllowedToTick = bEnableTick;
}

UGameInstance* UBlueprintGameInstanceSubsystem::GetGameInstance() const
{
	return ParentSubsystem->GetGameInstance();
}

bool UBlueprintGameInstanceSubsystem::Internal_ShouldCreateSubsystem() const
{
	if (!bEnabled)
	{
		UE_LOGFMT(LogBlueprintGameInstanceSubsystem, Warning, "{0} is not enabled. Subsystem will not be created.", GetClass()->GetName());
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
		
			UE_LOGFMT(LogBlueprintGameInstanceSubsystem, Warning, "{0} create failed. Reason: {1}.", GetClass()->GetName(), OutReasonString);
			return false;
		}
	}
	
	return true;
}

void UBlueprintGameInstanceSubsystem::Internal_OnInit()
{
	if (bHasBlueprintOnInit)
	{
		UE_LOGFMT(LogBlueprintGameInstanceSubsystem, Log, "Calling OnInit on {0}", GetClass()->GetName());
		K2_OnInit();
	}
	SetTickEnabled(bStartWithTickEnabled);
}

void UBlueprintGameInstanceSubsystem::OnDeInit()
{
	if (bHasBlueprintOnDeInit)
	{
		UE_LOGFMT(LogBlueprintGameInstanceSubsystem, Log, "Calling OnShutdown on {0}", GetClass()->GetName());
		K2_OnDeInit();
	}
}
