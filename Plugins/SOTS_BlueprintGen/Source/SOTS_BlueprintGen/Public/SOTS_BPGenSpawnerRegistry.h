#pragma once

#include "CoreMinimal.h"
#include "BlueprintNodeSpawner.h"

class UBlueprint;
class UEdGraph;

struct SOTS_BLUEPRINTGEN_API FSOTS_BPGenResolvedSpawner
{
	FString SpawnerKey;
	TWeakObjectPtr<UBlueprintNodeSpawner> Spawner;
	FString DebugName;
	FString DebugCategory;
};

/**
 * Spawner registry for BPGen SPINE_B+. Supports function-path, variable, and node-class spawner keys.
 */
class SOTS_BLUEPRINTGEN_API FSOTS_BPGenSpawnerRegistry
{
public:
	/** Resolve or create a spawner for the given key. Currently supports function path keys. */
	static bool ResolveForContext(UBlueprint* Blueprint, UEdGraph* Graph, const FString& SpawnerKey, FSOTS_BPGenResolvedSpawner& OutResolved, FString& OutError);

	/** Prime the spawner cache from the action database (best effort). */
	static int32 PrimeCache(UBlueprint* Blueprint, int32 MaxItems, FString& OutError);

	/** Clear cached spawners. */
	static void Clear();
};
