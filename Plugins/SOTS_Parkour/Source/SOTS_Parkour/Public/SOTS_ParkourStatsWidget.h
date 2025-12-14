#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "SOTS_ParkourStatsWidgetInterface.h"
#include "SOTS_ParkourStatsWidget.generated.h"

/**
 * Base widget that implements the parkour stats interface so C++ can push tags
 * while Blueprint bindings pull formatted text.
 */
UCLASS(Abstract, Blueprintable)
class SOTS_PARKOUR_API USOTS_ParkourStatsWidget : public UUserWidget, public ISOTS_ParkourStatsWidgetInterface
{
    GENERATED_BODY()

public:
    // ISOTS_ParkourStatsWidgetInterface
    virtual void SetParkourStateTag_Implementation(FGameplayTag StateTag) override;
    virtual void SetParkourActionTag_Implementation(FGameplayTag ActionTag) override;
    virtual void SetParkourClimbStyleTag_Implementation(FGameplayTag ClimbStyleTag) override;

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|UI")
    FGameplayTag GetParkourStateTag() const { return ParkourStateTag; }

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|UI")
    FGameplayTag GetParkourActionTag() const { return ParkourActionTag; }

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|UI")
    FGameplayTag GetParkourClimbStyleTag() const { return ParkourClimbStyleTag; }

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|UI")
    FText GetParkourStateText() const;

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|UI")
    FText GetParkourActionText() const;

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|UI")
    FText GetParkourClimbStyleText() const;

protected:
    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|UI")
    FGameplayTag ParkourStateTag;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|UI")
    FGameplayTag ParkourActionTag;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|UI")
    FGameplayTag ParkourClimbStyleTag;

private:
    FText ToText(const FGameplayTag& Tag) const;
};
