#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "SOTS_TestGameMode.generated.h"

class AActor;

UCLASS()
class SHADOWSANDSHURIKENS_API ASOTS_TestGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ASOTS_TestGameMode();

    virtual void StartPlay() override;

protected:
    void LogSuiteStateOnStart();
};

UCLASS()
class SHADOWSANDSHURIKENS_API ASOTS_TestGameState : public AGameStateBase
{
    GENERATED_BODY()
};
