// Created by Satheesh (ryanjon2040). Copyright 2024.

#pragma once

#include "UObject/Object.h"
#include "Runtime/Engine/Public/Tickable.h"
#include "BlueprintGameInstanceSubsystem.generated.h"

class UBlueprintGameInstanceSubsystemBase;

UCLASS(Abstract, Blueprintable, BlueprintType)
class BLUEPRINTSUBSYSTEM_API UBlueprintGameInstanceSubsystem : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

	/** If this subsystem should be created at all. If set to false, this subsystem will not be created and no events will run. */
	UPROPERTY(EditDefaultsOnly, Category = BlueprintGameInstanceSubsystem)
	uint8 bEnabled : 1;

	/** If this subsystem should start with tick enabled. You can toggle this at runtime using SetTickEnabled function. */
	UPROPERTY(EditDefaultsOnly, Category = BlueprintGameInstanceSubsystem)
	uint8 bStartWithTickEnabled : 1;

	uint8 bIsAllowedToTick : 1;
	
	uint8 bHasBlueprintShouldCreateSubsystem : 1;
	uint8 bHasBlueprintOnInit : 1;
	uint8 bHasBlueprintOnTick : 1;
	uint8 bHasBlueprintOnDeInit : 1;

	mutable TWeakObjectPtr<UWorld> CachedWorld;
	TWeakObjectPtr<UBlueprintGameInstanceSubsystemBase> ParentSubsystem;

public:

	UBlueprintGameInstanceSubsystem();

	static UBlueprintGameInstanceSubsystem* Create(UBlueprintGameInstanceSubsystemBase* Owner, const TSoftClassPtr<UBlueprintGameInstanceSubsystem>& SoftClassPtr);

	virtual UWorld* GetWorld() const override;
	
	//~ Begin FTickableGameObject interface
	virtual UWorld* GetTickableGameObjectWorld() const override;
	virtual bool IsTickableInEditor() const { return false; }
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; } 
	virtual bool IsAllowedToTick() const override { return bIsAllowedToTick ; }
	virtual void Tick(float DeltaTime) override;
	TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UBlueprintGameInstanceSubsystem, STATGROUP_Tickables); }
	//~ End FTickableGameObject interface

	/** Toggle tick method. */
	UFUNCTION(BlueprintCallable, Category = BlueprintGameInstanceSubsystem)
	void SetTickEnabled(const bool bEnableTick);

	/** Is ticking currently enabled. If this returns true, that means Event tick is running. */
	UFUNCTION(BlueprintCallable, Category = BlueprintGameInstanceSubsystem)
	bool IsTickEnabled() const { return IsAllowedToTick(); }

	UFUNCTION(BlueprintPure, Category = BlueprintGameInstanceSubsystem)
	UGameInstance* GetGameInstance() const;
	
private:

	bool Internal_ShouldCreateSubsystem() const;
	void Internal_OnInit();

public:
	
	void OnDeInit();

	/** Determines if this subsystem should be created or not based on your condition.
	 * Returning false will not create the subsystem. You can optionally specify a reason too which will be displayed in the output log.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = BlueprintGameInstanceSubsystem, DisplayName = ShouldCreateSubsystem)
	bool K2_ShouldCreateSubsystem(FText& OutReason) const;

	/** Called once at the very beginning when the game application starts. */
	UFUNCTION(BlueprintImplementableEvent, Category = BlueprintGameInstanceSubsystem, DisplayName = OnInit)
	void K2_OnInit();

	/** Tick function that is executed each frame. Can be toggled at runtime by calling Set Tick Enabled function. */
	UFUNCTION(BlueprintImplementableEvent, Category = BlueprintGameInstanceSubsystem, DisplayName = OnTick)
	void K2_OnTick(const float& DeltaTime);

	/** Cleanup method. Called before the game application is exited. */
	UFUNCTION(BlueprintImplementableEvent, Category = BlueprintGameInstanceSubsystem, DisplayName = OnShutdown)
	void K2_OnDeInit();
};
