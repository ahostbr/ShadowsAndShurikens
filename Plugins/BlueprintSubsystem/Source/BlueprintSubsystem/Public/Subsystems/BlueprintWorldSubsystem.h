// Created by Satheesh (ryanjon2040). Copyright 2024.

#pragma once

#include "Runtime/Engine/Public/Tickable.h"
#include "UObject/Object.h"
#include "BlueprintWorldSubsystem.generated.h"

class UBlueprintWorldSubsystemBase;

UCLASS(Abstract, Blueprintable, BlueprintType)
class BLUEPRINTSUBSYSTEM_API UBlueprintWorldSubsystem : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

	/** If this subsystem should be created at all. If set to false, this subsystem will not be created and no events will run. */
	UPROPERTY(EditDefaultsOnly, Category = BlueprintWorldSubsystem)
	uint8 bEnabled : 1;

	/** If this subsystem should start with tick enabled. You can toggle this at runtime using SetTickEnabled function. */
	UPROPERTY(EditDefaultsOnly, Category = BlueprintWorldSubsystem)
	uint8 bStartWithTickEnabled : 1;

	uint8 bIsAllowedToTick : 1;
	
	uint8 bHasBlueprintShouldCreateSubsystem : 1;
	uint8 bHasBlueprintOnPostInitialize : 1;
	uint8 bHasBlueprintOnInit : 1;
	uint8 bHasBlueprintOnWorldBeginPlay : 1;
	uint8 bHasBlueprintOnWorldComponentsUpdated : 1;
	uint8 bHasBlueprintOnUpdateStreamingState : 1;
	uint8 bHasBlueprintOnTick : 1;
	uint8 bHasBlueprintOnDeInit : 1;

	mutable TWeakObjectPtr<UWorld> CachedWorld;

public:

	UBlueprintWorldSubsystem();

	static UBlueprintWorldSubsystem* Create(UBlueprintWorldSubsystemBase* Owner, const TSoftClassPtr<UBlueprintWorldSubsystem>& SoftClassPtr);

	virtual UWorld* GetWorld() const override;
	
	//~ Begin FTickableGameObject interface
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
	virtual bool IsTickableInEditor() const { return false; }
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; } 
	virtual bool IsAllowedToTick() const override { return bIsAllowedToTick ; }
	virtual void Tick(float DeltaTime) override;
	TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UBlueprintWorldSubsystem, STATGROUP_Tickables); }
	//~ End FTickableGameObject interface

	/** Toggle tick method. */
	UFUNCTION(BlueprintCallable, Category = BlueprintWorldSubsystem)
	void SetTickEnabled(const bool bEnableTick);

	/** Is ticking currently enabled. If this returns true, that means Event tick is running. */
	UFUNCTION(BlueprintCallable, Category = BlueprintWorldSubsystem)
	bool IsTickEnabled() const { return IsAllowedToTick(); }
	
private:

	bool Internal_ShouldCreateSubsystem() const;
	void Internal_OnInit();

public:
	
	void OnPostInitialize();
	void OnWorldComponentsUpdated(UWorld* World);
	void OnWorldBeginPlay(UWorld* World);
	void OnUpdateStreamingState();
	void OnDeInit();

	/** Determines if this subsystem should be created or not based on your condition.
	 * Returning false will not create the subsystem. You can optionally specify a reason too which will be displayed in the output log.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = BlueprintWorldSubsystem, DisplayName = ShouldCreateSubsystem)
	bool K2_ShouldCreateSubsystem(FText& OutReason) const;

	/** Called once all UWorldSubsystems have been initialized. */
	UFUNCTION(BlueprintImplementableEvent, Category = BlueprintWorldSubsystem, DisplayName = OnPostInitialize)
	void K2_OnPostInitialize();

	/** Called once at each map load. For each map this subsystem is created when map loads. */
	UFUNCTION(BlueprintImplementableEvent, Category = BlueprintWorldSubsystem, DisplayName = OnInit)
	void K2_OnInit();

	/** Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors */
	UFUNCTION(BlueprintImplementableEvent, Category = BlueprintWorldSubsystem, DisplayName = OnWorldBeginPlay)
	void K2_OnWorldBeginPlay(UWorld* World);

	/** Called after world components (e.g. line batcher and all level components) have been updated */
	UFUNCTION(BlueprintImplementableEvent, Category = BlueprintWorldSubsystem, DisplayName = OnWorldComponentsUpdated)
	void K2_OnWorldComponentsUpdated(UWorld* World);

	/** Updates sub-system required streaming levels (called by world's UpdateStreamingState function) */
	UFUNCTION(BlueprintImplementableEvent, Category = BlueprintWorldSubsystem, DisplayName = OnUpdateStreamingState)
	void K2_OnUpdateStreamingState();

	/** Tick function that is executed each frame. Can be toggled at runtime by calling Set Tick Enabled function. */
	UFUNCTION(BlueprintImplementableEvent, Category = BlueprintWorldSubsystem, DisplayName = OnTick)
	void K2_OnTick(const float& DeltaTime);

	/** Cleanup method. Called before the map is unloaded. */
	UFUNCTION(BlueprintImplementableEvent, Category = BlueprintWorldSubsystem, DisplayName = OnShutdown)
	void K2_OnDeInit();
};
