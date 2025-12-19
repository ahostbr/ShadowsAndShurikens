#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_BPGenTypes.h"
#include "SOTS_BPGenDiscovery.generated.h"

/**
 * Node discovery utilities (SPINE_A): enumerate Blueprint actions and return stable spawner descriptors.
 */
UCLASS()
class SOTS_BLUEPRINTGEN_API USOTS_BPGenDiscovery : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Discover available Blueprint nodes with stable spawner descriptors. */
	UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen|Discovery")
	static FSOTS_BPGenNodeDiscoveryResult DiscoverNodesWithDescriptors(
		const UObject* WorldContextObject,
		const FString& BlueprintAssetPath,
		const FString& SearchText,
		int32 MaxResults = 200,
		bool bIncludePins = false);
};
