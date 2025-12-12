// Created by Satheesh (ryanjon2040). Copyright 2024.

#pragma once

#include "BlueprintSubsystemEditorModule.h"
#include "Factories/Factory.h"
#include "BlueprintSubsystemFactoryBase.generated.h"

UCLASS(Abstract)
class UBlueprintSubsystemFactoryBase : public UFactory
{
	GENERATED_BODY()

public:

	TSubclassOf<UObject> ParentClass;

	UBlueprintSubsystemFactoryBase();
	
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
};

UCLASS()
class UBlueprintGameInstanceSubsystemFactory : public UBlueprintSubsystemFactoryBase
{
	GENERATED_BODY()

public:

	UBlueprintGameInstanceSubsystemFactory();

	virtual FText GetDisplayName() const override { return INVTEXT("Game Instance Subsystem"); }
	virtual FText GetToolTip() const override { return INVTEXT("Blueprint Subsystem that is guaranteed to exist as long as your game is running. Assign this Blueprint in Project Settings -> Blueprint Subsystem Settings"); }
	virtual uint32 GetMenuCategories() const override { return FBlueprintSubsystemEditorModule::BlueprintSubsystemAssetCategory; }
	virtual FString GetDefaultNewAssetName() const override { return "NewBlueprintGameInstanceSubsystem"; }
};

UCLASS()
class UBlueprintWorldSubsystemFactory : public UBlueprintSubsystemFactoryBase
{
	GENERATED_BODY()

public:

	UBlueprintWorldSubsystemFactory();

	virtual FText GetDisplayName() const override { return INVTEXT("World Subsystem"); }
	virtual FText GetToolTip() const override { return INVTEXT("Blueprint Subsystem that is guaranteed to exist for each map. When map is unloaded this subsystem will also shutdown. Assign this Blueprint in Project Settings -> Blueprint Subsystem Settings"); }
	virtual uint32 GetMenuCategories() const override { return FBlueprintSubsystemEditorModule::BlueprintSubsystemAssetCategory; }
	virtual FString GetDefaultNewAssetName() const override { return "NewBlueprintWorldSubsystem"; }
};
