#pragma once

#include "CoreMinimal.h"
#include "SOTS_BPGenTypes.h"
#include "SOTS_BPGenGraphResolver.generated.h"

class UBlueprint;
class UEdGraph;

UCLASS()
class SOTS_BLUEPRINTGEN_API USOTS_BPGenGraphResolver : public UObject
{
    GENERATED_BODY()

public:
    /** Load blueprint and resolve/create target graph according to GraphTarget. */
    static bool ResolveTargetGraph(UBlueprint*& OutBlueprint, UEdGraph*& OutGraph, const FSOTS_BPGenGraphTarget& GraphTarget, FString& OutError, FString& OutErrorCode);

private:
    static bool LoadBlueprint(const FSOTS_BPGenGraphTarget& GraphTarget, UBlueprint*& OutBlueprint, FString& OutError, FString& OutErrorCode);
    static bool ResolveFunctionGraph(UBlueprint* Blueprint, const FString& TargetName, bool bCreateIfMissing, UEdGraph*& OutGraph, FString& OutError, FString& OutErrorCode);
    static bool ResolveEventGraph(UBlueprint* Blueprint, const FString& TargetName, bool bCreateIfMissing, UEdGraph*& OutGraph, FString& OutError, FString& OutErrorCode);
    static bool ResolveConstructionScript(UBlueprint* Blueprint, const FString& TargetName, bool bCreateIfMissing, UEdGraph*& OutGraph, FString& OutError, FString& OutErrorCode);
    static bool ResolveMacroGraph(UBlueprint* Blueprint, const FString& TargetName, bool bCreateIfMissing, UEdGraph*& OutGraph, FString& OutError, FString& OutErrorCode);
    static bool ResolveWidgetBindingGraph(UBlueprint* Blueprint, const FString& TargetName, bool bCreateIfMissing, UEdGraph*& OutGraph, FString& OutError, FString& OutErrorCode);
    static FString NormalizeTargetType(const FString& InType);
};
