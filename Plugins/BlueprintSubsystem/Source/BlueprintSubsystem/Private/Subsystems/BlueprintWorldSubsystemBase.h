// Created by Satheesh (ryanjon2040). Copyright 2024.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BlueprintWorldSubsystemBase.generated.h"

class UBlueprintWorldSubsystem;

UCLASS(NotBlueprintType, HideDropdown)
class UBlueprintWorldSubsystemBase : public UWorldSubsystem
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TMap<UClass*, TObjectPtr<UBlueprintWorldSubsystem>> CreatedSubsystems;

public:

	UFUNCTION(BlueprintPure, Category = BlueprintWorldSubsystem, meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "TargetClass", CompactNodeTitle = "BLUEPRINT WORLD SUBSYSTEM"))
	static UBlueprintWorldSubsystem* GetBlueprintSubsystemOfClass(const UObject* WorldContextObject, const TSubclassOf<UBlueprintWorldSubsystem> TargetClass);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void PostInitialize() override;
	virtual void OnWorldComponentsUpdated(UWorld& World) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void UpdateStreamingState() override;
	virtual void Deinitialize() override;
};
