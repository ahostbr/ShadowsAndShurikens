#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SOTS_GlobalStealthTypes.h"
#include "SOTS_GAS_DebugLibrary.h"
#include "SOTS_StealthSummaryHUDWidget.generated.h"

/**
 * Screen-space summary HUD widget for global stealth state and
 * high-level enemy perception counts. Blueprint widgets such as
 * WBP_SOTS_StealthSummaryHUD should subclass this and bind to the
 * cached fields.
 */
UCLASS(Abstract, Blueprintable)
class SOTS_GAS_PLUGIN_API USOTS_StealthSummaryHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    UPROPERTY(BlueprintReadOnly, Category="Stealth")
    FSOTS_StealthScoreBreakdown CachedBreakdown;

    UPROPERTY(BlueprintReadOnly, Category="Stealth")
    int32 NumEnemiesUnaware = 0;

    UPROPERTY(BlueprintReadOnly, Category="Stealth")
    int32 NumEnemiesSuspicious = 0;

    UPROPERTY(BlueprintReadOnly, Category="Stealth")
    int32 NumEnemiesAlerted = 0;

    UPROPERTY(BlueprintReadOnly, Category="Stealth")
    ESOTSStealthDebugMode CurrentDebugMode = ESOTSStealthDebugMode::Off;

    UFUNCTION(BlueprintImplementableEvent, Category="Stealth")
    void OnStealthSummaryUpdated();
};

