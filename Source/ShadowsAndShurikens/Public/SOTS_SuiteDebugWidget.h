// SPDX-License-Identifier: MIT
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SOTS_SuiteDebugWidget.generated.h"

class USOTS_SuiteDebugSubsystem;

/**
 * Blueprint-friendly widget base that exposes per-subsystem debug strings
 * from `USOTS_SuiteDebugSubsystem`.
 */
UCLASS(Abstract, Blueprintable)
class SHADOWSANDSHURIKENS_API USOTS_SuiteDebugWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "SOTS|Debug")
    FString GetGSMStateText() const;

    UFUNCTION(BlueprintCallable, Category = "SOTS|Debug")
    FString GetPlayerStatsText() const;

    UFUNCTION(BlueprintCallable, Category = "SOTS|Debug")
    FString GetAbilityStateText() const;

    UFUNCTION(BlueprintCallable, Category = "SOTS|Debug")
    FString GetInventoryStateText() const;

    UFUNCTION(BlueprintCallable, Category = "SOTS|Debug")
    FString GetMissionStateText() const;

    UFUNCTION(BlueprintCallable, Category = "SOTS|Debug")
    FString GetMusicStateText() const;

    UFUNCTION(BlueprintCallable, Category = "SOTS|Debug")
    FString GetFXStateText() const;

    UFUNCTION(BlueprintCallable, Category = "SOTS|Debug")
    FString GetTagManagerStateText() const;

private:
    USOTS_SuiteDebugSubsystem* GetSuiteDebugSubsystem() const;
};
