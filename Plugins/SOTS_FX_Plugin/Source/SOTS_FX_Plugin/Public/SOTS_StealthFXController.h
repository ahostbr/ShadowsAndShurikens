#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SOTS_GlobalStealthTypes.h"
#include "SOTS_StealthFXController.generated.h"

class UWorld;
class UCurveFloat;

/**
 * Lightweight listener that translates global stealth state into FX parameters.
 * This object is intended to be owned/initialized by game code (e.g., GameInstance)
 * and uses existing SOTS_FX manager APIs to actually spawn/scale FX.
 */
UCLASS()
class SOTS_FX_PLUGIN_API USOTS_StealthFXController : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UWorld* World);

    // Curves to control FX strength (set from editor or BP).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="StealthFX")
    UCurveFloat* Curve_LightToAmbientFX = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="StealthFX")
    UCurveFloat* Curve_StealthToDangerFX = nullptr;

protected:
    void HandleStealthStateChanged(const FSOTS_PlayerStealthState& NewState);

private:
    // Tracks the last tier so we only fire FX when the tier actually changes.
    ESOTS_StealthTier LastStealthTier = ESOTS_StealthTier::Hidden;
};
