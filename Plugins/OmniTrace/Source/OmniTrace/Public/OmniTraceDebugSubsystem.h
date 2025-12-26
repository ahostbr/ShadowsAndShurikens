#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OmniTraceDebugTypes.h"

#include "OmniTraceDebugSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOmniTrace, Log, All);

UCLASS()
class OMNITRACE_API UOmniTraceDebugSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    void SetLastKEMTrace(const FSOTS_OmniTraceKEMDebugRecord& InRecord);
    const FSOTS_OmniTraceKEMDebugRecord& GetLastKEMTrace() const { return LastKEMTrace; }

	void ClearLastKEMTrace() { LastKEMTrace = {}; }

    UFUNCTION(Exec)
    void OmniTrace_DrawLastKEMTrace();

private:
    FSOTS_OmniTraceKEMDebugRecord LastKEMTrace;
};
