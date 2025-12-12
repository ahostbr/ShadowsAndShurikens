// Created by Satheesh (ryanjon2040). Copyright 2024.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "BlueprintSubsystemSettings.generated.h"

class UBlueprintWorldSubsystem;
class UBlueprintGameInstanceSubsystem;

UCLASS(Config=Game, DefaultConfig)
class UBlueprintSubsystemSettings : public UDeveloperSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Config, Category = BlueprintSubsystemSettings)
	uint8 bDisableAllBlueprintGameInstanceSubsystems : 1;

	UPROPERTY(EditAnywhere, Config, Category = BlueprintSubsystemSettings)
	uint8 bDisableAllBlueprintWorldSubsystems : 1;

	UPROPERTY(EditAnywhere, Config, Category = BlueprintSubsystemSettings, meta = (EditCondition = "!bDisableAllBlueprintGameInstanceSubsystems"))
	TSet<TSoftClassPtr<UBlueprintGameInstanceSubsystem>> GameInstanceSubsystemClasses;

	UPROPERTY(EditAnywhere, Config, Category = BlueprintSubsystemSettings, meta = (EditCondition = "!bDisableAllBlueprintWorldSubsystems"))
	TSet<TSoftClassPtr<UBlueprintWorldSubsystem>> WorldSubsystemClasses;

public:

	UBlueprintSubsystemSettings();

	static const UBlueprintSubsystemSettings* Get()	{ return GetDefault<UBlueprintSubsystemSettings>(); }

#if WITH_EDITORONLY_DATA
	virtual FText GetSectionText() const override { return INVTEXT("Settings"); }
	virtual FName GetCategoryName() const override { return FName("Blueprint Subsystem"); }
#endif

	FORCEINLINE bool IsBlueprintGameInstanceSubsystemsEnabled() const { return !bDisableAllBlueprintGameInstanceSubsystems; }
	FORCEINLINE bool IsBlueprintWorldSubsystemsEnabled() const { return !bDisableAllBlueprintWorldSubsystems; }

	FORCEINLINE void GetGameInstanceSubsystemClasses(TSet<TSoftClassPtr<UBlueprintGameInstanceSubsystem>>& OutSubsystemClasses) const
	{
		OutSubsystemClasses = GameInstanceSubsystemClasses;
	}

	FORCEINLINE void GetWorldSubsystemClasses(TSet<TSoftClassPtr<UBlueprintWorldSubsystem>>& OutSubsystemClasses) const
	{
		OutSubsystemClasses = WorldSubsystemClasses;
	}
};
