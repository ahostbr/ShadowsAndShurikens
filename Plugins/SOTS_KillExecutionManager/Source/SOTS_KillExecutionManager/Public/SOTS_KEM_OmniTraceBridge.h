#pragma once

#include "CoreMinimal.h"
#include "SOTS_KEM_Types.h"
#include "SOTS_OmniTraceKEMPresetLibrary.h"

class USOTS_KEM_ExecutionDefinition;

/**
 * Thin C++ bridge between KEM's spawn backend and OmniTrace.
 * For now this is intentionally small and focused on a few
 * high-value patterns (e.g. Ground.Rear.Single).
 */
struct SOTS_KILLEXECUTIONMANAGER_API FSOTS_KEM_OmniTraceBridge
{
    FSOTS_KEM_OmniTraceBridge() = default;

private:
    struct FPositionPatternConfig
    {
        FName PatternLabel = TEXT("SOTS.KEM.Pattern.Ground.Unknown");
        FVector LocalDirection = FVector::ZeroVector;
        bool bUseGroundRelative = false;
        bool bUseVertical = false;
        bool bVerticalAbove = false;
        bool bVerticalBelow = false;
        float VerticalTraceDistance = 900.f;
        float PatternTraceDistance = 2000.f;
        UOmniTracePatternPreset* PatternPreset = nullptr;
        bool bHasKnownMapping = false;
    } PositionConfig;

public:
    // DevTools: Position-based OmniTrace patterns are keyed by PositionTag.
    // Stealth/KEM reports can aggregate warp results using those tags.
    void ConfigureForPositionTag(const FGameplayTag& PositionTag);

    void ConfigureForPresetEntry(const FSOTS_OmniTraceKEMPresetEntry* Entry);

    /** Compute warp information for a spawn-based execution.
     *
     *  @param SpawnConfig   SpawnActorConfig from the execution definition.
     *  @param Definition    Execution definition being executed.
     *  @param Context       Fully built execution context.
     *  @param InOutSpawnTransform  In: initial spawn transform guess. Out: refined helper spawn transform.
     *  @param OutResult     Filled with helper spawn + per-role warp targets if a solution is found.
     *
     *  @return true if OmniTrace produced a usable warp solution; false to fall back to existing behaviour.
     */
    bool ComputeWarpForSpawnExecution(
        const FSOTS_KEM_SpawnActorConfig& SpawnConfig,
        const USOTS_KEM_ExecutionDefinition* Definition,
        const FSOTS_ExecutionContext& Context,
        FTransform& InOutSpawnTransform,
        FSOTS_KEM_OmniTraceWarpResult& OutResult);
};
