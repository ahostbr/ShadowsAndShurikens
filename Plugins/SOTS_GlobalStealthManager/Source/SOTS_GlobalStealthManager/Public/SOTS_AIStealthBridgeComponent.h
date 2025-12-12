#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SOTS_AIStealthBridgeComponent.generated.h"

/**
 * Tiny bridge component intended to live on AI pawns or controllers.
 * It receives a normalized suspicion value from BAT/AIBT (or other AI logic)
 * and forwards it into the global stealth manager.
 */
UCLASS(ClassGroup=(SOTS), meta=(BlueprintSpawnableComponent))
class SOTS_GLOBALSTEALTHMANAGER_API USOTS_AIStealthBridgeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USOTS_AIStealthBridgeComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Suspicion01;

    UFUNCTION(BlueprintCallable, Category="Stealth")
    void SetSuspicion01(float In01);
};

