#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_BPGenTypes.h"
#include "SOTS_BPGenDescriptorTranslator.generated.h"

UCLASS()
class SOTS_BLUEPRINTGEN_API USOTS_BPGenDescriptorTranslator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Build a GraphNode spec from a discovery descriptor. NodeIdOverride is optional; when empty falls back to spawner key then display name. */
	UFUNCTION(BlueprintPure, Category = "SOTS|BPGen|Discovery")
	static FSOTS_BPGenGraphNode MakeGraphNodeFromDescriptor(const FSOTS_BPGenNodeSpawnerDescriptor& Descriptor, const FString& NodeIdOverride);

	/** Convenience link builder (schema-on by default). */
	UFUNCTION(BlueprintPure, Category = "SOTS|BPGen|Graph")
	static FSOTS_BPGenGraphLink MakeLinkSpec(const FString& FromNodeId, FName FromPinName, const FString& ToNodeId, FName ToPinName, bool bUseSchema);
};
