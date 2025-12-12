// SOTS_TestActor.h
// Simple test actor to validate the DevTools pipeline + C++ compile

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SOTS_TestActor.generated.h"

UCLASS()
class SHADOWSANDSHURIKENS_API ASOTS_TestActor : public AActor
{
    GENERATED_BODY()

public:
    ASOTS_TestActor();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaSeconds) override;

    /** Message printed to log and (once) to the screen on BeginPlay */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Test")
    FString Message;

    /** If true, prints to the log every tick instead of just once on BeginPlay */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Test")
    bool bPrintEveryTick;
};
