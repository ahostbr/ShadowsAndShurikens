#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SOTS_ProfileSnapshotProvider.h"
#include "SOTS_ProfileTypes.h"
#include "SOTS_ProfileSubsystem.generated.h"

class AActor;
class APawn;
class UActorComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_ProfileRestoredSignature, const FSOTS_ProfileSnapshot&, Snapshot);

USTRUCT()
struct FSOTS_ProfileProviderEntry
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<UObject> Provider;

    UPROPERTY()
    int32 Priority = 0;

    UPROPERTY()
    uint64 Sequence = 0;
};

UCLASS()
class SOTS_PROFILESHARED_API USOTS_ProfileSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "SOTS|Profile")
    bool SaveProfile(const FSOTS_ProfileId& ProfileId, const FSOTS_ProfileSnapshot& Snapshot);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Profile")
    bool LoadProfile(const FSOTS_ProfileId& ProfileId, FSOTS_ProfileSnapshot& OutSnapshot);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Profile")
    void BuildSnapshotFromWorld(FSOTS_ProfileSnapshot& OutSnapshot);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Profile")
    void ApplySnapshotToWorld(const FSOTS_ProfileSnapshot& Snapshot);

    void RegisterProvider(UObject* Provider, int32 Priority);
    void UnregisterProvider(UObject* Provider);

    UFUNCTION(exec)
    void SOTS_DumpCurrentPlayerStatsSnapshot();

    // Fires after ApplySnapshotToWorld completes all provider restores.
    UPROPERTY(BlueprintAssignable, Category = "SOTS|Profile")
    FSOTS_ProfileRestoredSignature OnProfileRestored;

protected:
    FString GetSlotNameForProfile(const FSOTS_ProfileId& ProfileId) const;

    void GatherPlayerSnapshot(FSOTS_ProfileSnapshot& OutSnapshot) const;
    void RestorePlayerFromSnapshot(const FSOTS_ProfileSnapshot& Snapshot) const;
    void BuildOptionalStats(AActor* Actor, FSOTS_CharacterStateData& OutState) const;
    void ApplyOptionalStats(AActor* Actor, const FSOTS_CharacterStateData& InState) const;

    void InvokeProviderBuild(FSOTS_ProfileSnapshot& InOutSnapshot);
    void InvokeProviderApply(const FSOTS_ProfileSnapshot& Snapshot);
    void PruneInvalidProviders();
    void SortProviders();
    void UpdateProviderResolutionCache();
    UActorComponent* FindStatsComponent(AActor* Actor) const;
    bool TryGetPlayerPawn(APawn*& OutPawn) const;

private:
    UPROPERTY()
    TArray<FSOTS_ProfileProviderEntry> Providers;

    UPROPERTY()
    uint64 NextProviderSequence = 1;

    FString CachedPrimaryProviderName;
    int32 CachedPrimaryProviderPriority = 0;
    int32 CachedProviderCount = 0;
    bool bLoggedMissingProvidersOnce = false;
};
