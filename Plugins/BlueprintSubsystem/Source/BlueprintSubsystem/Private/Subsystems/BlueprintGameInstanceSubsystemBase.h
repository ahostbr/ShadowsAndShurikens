// Created by Satheesh (ryanjon2040). Copyright 2024.

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "BlueprintGameInstanceSubsystemBase.generated.h"

class UBlueprintGameInstanceSubsystem;

UCLASS(NotBlueprintType, HideDropdown)
class UBlueprintGameInstanceSubsystemBase : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TMap<UClass*, TObjectPtr<UBlueprintGameInstanceSubsystem>> CreatedSubsystems;

public:

	UFUNCTION(BlueprintPure, Category = BlueprintGameInstanceSubsystem, meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "TargetClass", CompactNodeTitle = "BLUEPRINT GAME INSTANCE SUBSYSTEM"))
	static UBlueprintGameInstanceSubsystem* GetBlueprintSubsystemOfClass(const UObject* WorldContextObject, const TSubclassOf<UBlueprintGameInstanceSubsystem> TargetClass);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
};
