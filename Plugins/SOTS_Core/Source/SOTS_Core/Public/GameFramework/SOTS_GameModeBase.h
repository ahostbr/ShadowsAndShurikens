#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SOTS_GameModeBase.generated.h"

UCLASS()
class SOTS_CORE_API ASOTS_GameModeBase : public AGameModeBase
{
    GENERATED_BODY()

protected:
    virtual void StartPlay() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
};
