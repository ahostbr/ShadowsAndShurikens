#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SOTS_GlobalStealthTypes.h"
#include "SOTS_GAS_DebugLibrary.h"
#include "SOTS_PlayerStealthDebugWidget.generated.h"

class USOTS_GlobalStealthManagerSubsystem;

/**
 * Base C++ widget for player stealth debug. Blueprint widgets such as
 * WBP_SOTS_PlayerStealthDebug should subclass this and implement
 * OnStealthDebugDataUpdated to drive their visuals.
 */
UCLASS(Abstract, Blueprintable)
class SOTS_GAS_PLUGIN_API USOTS_PlayerStealthDebugWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    // Cached data pulled from the global stealth manager each tick.
    UPROPERTY(BlueprintReadOnly, Category="Stealth")
    FSOTS_PlayerStealthState CachedPlayerState;

    UPROPERTY(BlueprintReadOnly, Category="Stealth")
    FSOTS_StealthScoreBreakdown CachedBreakdown;

    UPROPERTY(BlueprintReadOnly, Category="Stealth")
    ESOTSStealthDebugMode CurrentDebugMode = ESOTSStealthDebugMode::Off;

    // Blueprint override point for updating text/bars based on cached data.
    UFUNCTION(BlueprintImplementableEvent, Category="Stealth")
    void OnStealthDebugDataUpdated();
};

