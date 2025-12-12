// Created by Satheesh (ryanjon2040). Copyright 2024.

#include "BlueprintGameInstanceSubsystemBase.h"
#include "Subsystems/BlueprintGameInstanceSubsystem.h"
#include "BlueprintSubsystemSettings.h"
#include "Logging/StructuredLog.h"
#include "Runtime/Engine/Classes/Engine/GameInstance.h"
#include "Runtime/Engine/Public/UnrealEngine.h"

DEFINE_LOG_CATEGORY_STATIC(LogBlueprintGameInstanceSubsystemBase, All, All);

UBlueprintGameInstanceSubsystem* UBlueprintGameInstanceSubsystemBase::GetBlueprintSubsystemOfClass(
	const UObject* WorldContextObject, const TSubclassOf<UBlueprintGameInstanceSubsystem> TargetClass)
{
	if (const auto World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		const auto ThisSubsystem = World->GetGameInstance()->GetSubsystem<UBlueprintGameInstanceSubsystemBase>();
		return ThisSubsystem->CreatedSubsystems.FindRef(TargetClass);
	}

	return nullptr;
}

bool UBlueprintGameInstanceSubsystemBase::ShouldCreateSubsystem(UObject* Outer) const
{
	const auto Settings = UBlueprintSubsystemSettings::Get();
	if (!Settings->IsBlueprintGameInstanceSubsystemsEnabled())
	{
		UE_LOG(LogBlueprintGameInstanceSubsystemBase, Warning, TEXT("All Blueprint Game Instance Subsystems are disabled in Project Settings. None will be created."));
		return false;
	}
	
	return Super::ShouldCreateSubsystem(Outer);
}

void UBlueprintGameInstanceSubsystemBase::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const auto SubsystemSettings = UBlueprintSubsystemSettings::Get();
	
	TSet<TSoftClassPtr<UBlueprintGameInstanceSubsystem>> OutSubsystemClasses;
	SubsystemSettings->GetGameInstanceSubsystemClasses(OutSubsystemClasses);
	
	for (const auto& It : OutSubsystemClasses)
	{
		const auto CreatedSubsystem = UBlueprintGameInstanceSubsystem::Create(this, It);
		if (IsValid(CreatedSubsystem))
		{
			CreatedSubsystems.Add(CreatedSubsystem->GetClass(), CreatedSubsystem);
		}
	}

	UE_LOGFMT(LogBlueprintGameInstanceSubsystemBase, Log, "Total Blueprint Game Instance Subsystems initialized: {0}", CreatedSubsystems.Num());
}

void UBlueprintGameInstanceSubsystemBase::Deinitialize()
{
	Super::Deinitialize();
	for (const auto& It : CreatedSubsystems)
	{
		It.Value->OnDeInit();
		It.Value->MarkAsGarbage();
	}

	CreatedSubsystems.Empty();
	CreatedSubsystems.Shrink();
	CreatedSubsystems.Compact();

	UE_LOGFMT(LogBlueprintGameInstanceSubsystemBase, Log, "Total Blueprint Game Instance Subsystems deinitialized: {0}", CreatedSubsystems.Num());
}
