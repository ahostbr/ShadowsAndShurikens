#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SOTS_HUDBase.generated.h"

UCLASS()
class SOTS_CORE_API ASOTS_HUDBase : public AHUD
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;
};
