// SPDX-License-Identifier: MIT
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SOTS_GameplayTagManagerSubsystem.h"
#include "SOTS_MMSSSubsystem.h"
#include "SOTS_MissionDirectorSubsystem.h"
#include "SOTS_InventoryBridgeSubsystem.h"
#include "SOTS_GlobalStealthManagerSubsystem.h"
#include "InvSPInventoryComponent.h"
#include "SOTS_StatsComponent.h"
#include "Components/ActorComponent.h"
#include "SOTS_KillExecutionManagerKEMAnchorDebugWidget.h"
#include "SOTS_SuiteDebugSubsystem.generated.h"

class UWorld;
class AActor;

/**
 * A read-only game instance subsystem that aggregates state across SOTS subsystems
 * and provides concise debugging summaries for use in logs and Blueprint.
 *
 * This lives in the SOTS_Debug plugin and depends on several SOTS plugin modules.
 * Prefer `Get()` to obtain a pointer safely.
 */
UCLASS()
class SOTS_DEBUG_API USOTS_SuiteDebugSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** Helper to obtain this subsystem with a world context. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|Debug", meta = (WorldContext = "WorldContextObject"))
    static USOTS_SuiteDebugSubsystem* Get(const UObject* WorldContextObject);

    /** Returns a short summary of the global stealth manager state. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SOTS|Debug")
    FString GetGlobalStealthSummary() const;

    /** Returns a short summary of the mission director state. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SOTS|Debug")
    FString GetMissionDirectorSummary() const;

    /** Returns a short summary of the music subsystem state. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SOTS|Debug")
    FString GetMusicSubsystemSummary() const;

    /** Returns a short summary of the TagManager presence. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SOTS|Debug")
    FString GetTagManagerSummary() const;

    /** Returns a short summary of FX subsystem settings and counts. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SOTS|Debug")
    FString GetFXSummary() const;

    /** Returns a short summary of the player's Inventory state. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SOTS|Debug")
    FString GetInventorySummary() const;

    // DevTools: This summary is intended to quickly confirm that
    // USOTS_StatsComponent and ProfileSubsystem remain in sync for StatValues.
    /** Returns the player's stat summary (top N stats by value). */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SOTS|Debug")
    FString GetStatsSummary(int32 TopN = 5) const;

    /** Returns a short summary of abilities on the player (known tags count). */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SOTS|Debug")
    FString GetAbilitiesSummary() const;

    /** Dumps the combined suite state to the log (UE_LOG). */
    UFUNCTION(BlueprintCallable, Category = "SOTS|Debug")
    void DumpSuiteStateToLog() const;

    UFUNCTION(BlueprintCallable, Category = "SOTS|KEM|Debug")
    void ToggleKEMAnchorOverlay();

    UFUNCTION(BlueprintCallable, Category = "SOTS|KEM|Debug")
    void SetKEMAnchorOverlayEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SOTS|KEM|Debug")
    bool IsKEMAnchorOverlayVisible() const;

    UFUNCTION()
    void ShowKEMAnchorOverlay();

    UFUNCTION()
    void HideKEMAnchorOverlay();

protected:
    // Small helpers
    AActor* GetPlayerPawn() const;

    UPROPERTY(EditAnywhere, Category = "SOTS|KEM|Debug", meta = (DisplayName = "KEM Anchor Overlay"))
    TSubclassOf<USOTS_KillExecutionManagerKEMAnchorDebugWidget> KEMAnchorDebugWidgetClass;

private:
    TWeakObjectPtr<USOTS_KillExecutionManagerKEMAnchorDebugWidget> KEMAnchorDebugWidgetInstance;
};
