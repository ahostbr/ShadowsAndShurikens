#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "SOTS_BPGenTypes.h"
#include "SOTS_BPGenBuildCommandlet.generated.h"

/**
 * Commandlet entrypoint for BPGen jobs driven by DevTools.
 *
 * Usage (example):
 *
 *   UEEditor-Cmd.exe "ShadowsAndShurikens.uproject" ^
 *     -run=SOTS_BPGenBuildCommandlet ^
 *     -JobFile="E:/SAS/DevTools/bpgen_jobs/BPGEN_2025-12-03_001.json" ^
 *     -GraphSpecFile="E:/SAS/DevTools/bpgen_specs/BPGEN_2025-12-03_001_graph.json"
 *
 * The job JSON is expected to contain:
 * - "Function": FSOTS_BPGenFunctionDef-shaped object.
 * - Optional "StructsToCreate": array of FSOTS_BPGenStructDef-shaped objects.
 * - Optional "EnumsToCreate": array of FSOTS_BPGenEnumDef-shaped objects.
 * - Optional inline "GraphSpec": FSOTS_BPGenGraphSpec-shaped object (if no GraphSpecFile is provided).
 *
 * BRIDGE 1 keeps the schema simple and aligned with FSOTS_BPGen* types. 
 * BRIDGE 2 will introduce DevTools Python helpers and more detailed docs.
 */
UCLASS()
class SOTS_BLUEPRINTGEN_API USOTS_BPGenBuildCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	USOTS_BPGenBuildCommandlet();

	// Entry point for commandlet execution.
	virtual int32 Main(const FString& Params) override;
};
