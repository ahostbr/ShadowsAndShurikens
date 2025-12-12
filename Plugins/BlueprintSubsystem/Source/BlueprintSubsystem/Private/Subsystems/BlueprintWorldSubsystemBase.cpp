// Created by Satheesh (ryanjon2040). Copyright 2024.

#include "BlueprintWorldSubsystemBase.h"
#include "BlueprintSubsystemSettings.h"
#include "Logging/StructuredLog.h"
#include "Subsystems/BlueprintWorldSubsystem.h"
#include "Runtime/Engine/Public/UnrealEngine.h"

DEFINE_LOG_CATEGORY_STATIC(LogBlueprintWorldSubsystemBase, All, All);

UBlueprintWorldSubsystem* UBlueprintWorldSubsystemBase::GetBlueprintSubsystemOfClass(const UObject* WorldContextObject,
	const TSubclassOf<UBlueprintWorldSubsystem> TargetClass)
{
	if (const auto World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		const auto ThisSubsystem = World->GetSubsystem<UBlueprintWorldSubsystemBase>();
		return ThisSubsystem->CreatedSubsystems.FindRef(TargetClass);
	}

	return nullptr;
}

bool UBlueprintWorldSubsystemBase::ShouldCreateSubsystem(UObject* Outer) const
{
	if (UWorld* World = Cast<UWorld>(Outer))
	{
		if (World->IsGameWorld() && Super::ShouldCreateSubsystem(Outer))
		{
			const auto Settings = UBlueprintSubsystemSettings::Get();
			if (!Settings->IsBlueprintGameInstanceSubsystemsEnabled())
			{
				UE_LOG(LogBlueprintWorldSubsystemBase, Warning, TEXT("All Blueprint World Subsystems are disabled. None will be created."));
				return false;
			}

			return true;
		}
	}

	return false;
}

void UBlueprintWorldSubsystemBase::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const auto SubsystemSettings = UBlueprintSubsystemSettings::Get();
	
	TSet<TSoftClassPtr<UBlueprintWorldSubsystem>> OutSubsystemClasses;
	SubsystemSettings->GetWorldSubsystemClasses(OutSubsystemClasses);
	
	for (const auto& It : OutSubsystemClasses)
	{
		const auto CreatedSubsystem = UBlueprintWorldSubsystem::Create(this, It);
		if (IsValid(CreatedSubsystem))
		{
			CreatedSubsystems.Add(CreatedSubsystem->GetClass(), CreatedSubsystem);
		}
	}

	UE_LOGFMT(LogBlueprintWorldSubsystemBase, Log, "Total Blueprint World Subsystems initialized: {0}", CreatedSubsystems.Num());
}

void UBlueprintWorldSubsystemBase::PostInitialize()
{
	Super::PostInitialize();
	for (const auto& It : CreatedSubsystems)
	{
		It.Value->OnPostInitialize();
	}
}

void UBlueprintWorldSubsystemBase::OnWorldComponentsUpdated(UWorld& World)
{
	Super::OnWorldComponentsUpdated(World);
	for (const auto& It : CreatedSubsystems)
	{
		It.Value->OnWorldComponentsUpdated(&World);
	}
}

void UBlueprintWorldSubsystemBase::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	for (const auto& It : CreatedSubsystems)
	{
		It.Value->OnWorldBeginPlay(&InWorld);
	}
}

void UBlueprintWorldSubsystemBase::UpdateStreamingState()
{
	Super::UpdateStreamingState();
	for (const auto& It : CreatedSubsystems)
	{
		It.Value->OnUpdateStreamingState();
	}
}

void UBlueprintWorldSubsystemBase::Deinitialize()
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

	UE_LOGFMT(LogBlueprintWorldSubsystemBase, Log, "Total Blueprint World Subsystems deinitialized: {0}", CreatedSubsystems.Num());
}
