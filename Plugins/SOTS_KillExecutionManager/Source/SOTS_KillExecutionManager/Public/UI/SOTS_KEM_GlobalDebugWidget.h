#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SOTS_KEM_Types.h"
#include "SOTS_GAS_DebugLibrary.h"
#include "SOTS_KEM_GlobalDebugWidget.generated.h"

class USOTS_KEMManagerSubsystem;

/**
 * Screen-space widget that shows recent KEM executions for debug purposes.
 * Blueprint widgets (e.g., WBP_SOTS_KEM_GlobalDebug) should subclass this
 * and bind to CachedRecords in OnKEMDebugRecordsUpdated.
 */
UCLASS(Abstract, Blueprintable)
class SOTS_KILLEXECUTIONMANAGER_API USOTS_KEM_GlobalDebugWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    UPROPERTY(BlueprintReadOnly, Category="KEM|Debug")
    TArray<FSOTS_KEMDebugRecord> CachedRecords;

    UPROPERTY(BlueprintReadOnly, Category="KEM|Debug")
    ESOTSStealthDebugMode CurrentDebugMode;

    UFUNCTION(BlueprintImplementableEvent, Category="KEM|Debug")
    void OnKEMDebugRecordsUpdated();
};
