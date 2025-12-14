#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SOTS_BodyDragTypes.h"
#include "SOTS_BodyDragConfigDA.generated.h"

UCLASS(BlueprintType)
class USOTS_BodyDragConfigDA : public UDataAsset
{
    GENERATED_BODY()

public:
    /** Configurable body drag settings (KO vs Dead). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|BodyDrag")
    FSOTS_BodyDragConfig Config;
};
