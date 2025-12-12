#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SOTS_GlobalStealthTypes.h"
#include "SOTS_GAS_DebugLibrary.h"
#include "SOTS_DragonStealthDebugWidget.generated.h"

class USOTS_GlobalStealthManagerSubsystem;

/**
 * Base C++ widget for dragon stealth debug. Blueprint widgets such as
 * WBP_SOTS_DragonStealthDebug should subclass this and use the cached
 * data to drive progress bars and labels.
 */
UCLASS(Abstract, Blueprintable)
class SOTS_GAS_PLUGIN_API USOTS_DragonStealthDebugWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    // For now we share the same global player/dragon state until a separate
    // dragon state exists. Widgets can add dragon-specific data in Blueprint.
    UPROPERTY(BlueprintReadOnly, Category="Stealth")
    FSOTS_PlayerStealthState CachedStealthState;

    UPROPERTY(BlueprintReadOnly, Category="Stealth")
    FSOTS_StealthScoreBreakdown CachedBreakdown;

    UPROPERTY(BlueprintReadOnly, Category="Stealth")
    ESOTSStealthDebugMode CurrentDebugMode = ESOTSStealthDebugMode::Off;

    UFUNCTION(BlueprintImplementableEvent, Category="Stealth")
    void OnStealthDebugDataUpdated();
};

