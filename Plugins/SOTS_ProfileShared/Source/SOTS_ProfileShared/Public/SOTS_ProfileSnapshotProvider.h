#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SOTS_ProfileTypes.h"
#include "SOTS_ProfileSnapshotProvider.generated.h"

UINTERFACE(BlueprintType)
class SOTS_PROFILESHARED_API USOTS_ProfileSnapshotProvider : public UInterface
{
    GENERATED_BODY()
};

class SOTS_PROFILESHARED_API ISOTS_ProfileSnapshotProvider
{
    GENERATED_BODY()

public:
    virtual void BuildProfileSnapshot(FSOTS_ProfileSnapshot& InOutSnapshot) = 0;
    virtual void ApplyProfileSnapshot(const FSOTS_ProfileSnapshot& Snapshot) = 0;
};
