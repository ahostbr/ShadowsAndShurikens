#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SOTS_PlayerControllerBase.generated.h"

UCLASS()
class SOTS_CORE_API ASOTS_PlayerControllerBase : public APlayerController
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
};
