#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SOTS_ProfileTypes.h"
#include "SOTS_ProfileSaveGame.generated.h"

UCLASS()
class SOTS_PROFILESHARED_API USOTS_ProfileSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY()
    FSOTS_ProfileSnapshot Snapshot;
};
