#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SOTS_KEM_Types.h"
#include "SOTS_GAS_DebugLibrary.h"
#include "SOTS_KEM_CandidateListWidget.generated.h"

/**
 * Screen-space widget that exposes the per-candidate scoring breakdown
 * for the most recent KEM selection. Blueprint widgets should subclass
 * this base and render CachedCandidates as a list/table.
 */
UCLASS(Abstract, Blueprintable)
class SOTS_KILLEXECUTIONMANAGER_API USOTS_KEM_CandidateListWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    UPROPERTY(BlueprintReadOnly, Category="KEM|Debug")
    TArray<FSOTS_KEMCandidateDebugRecord> CachedCandidates;

    UPROPERTY(BlueprintReadOnly, Category="KEM|Debug")
    ESOTSStealthDebugMode CurrentDebugMode;

    UFUNCTION(BlueprintImplementableEvent, Category="KEM|Debug")
    void OnCandidatesUpdated();
};
