// Copyright(c) Aurora Devs 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputModifiers.h"
#include "UGC_InputAccelerationModifier.generated.h"

/**
 * Apply an acceleration curve to your input so that it accelerates as time goes by. The given curve should be time-normalized, i.e., between 0 and 1.
 */
UCLASS(ClassGroup = "UGC Input Modifiers")
class AURORADEVS_UGC_API UUGC_InputAccelerationModifier : public UInputModifier
{
    GENERATED_BODY()
public:
    virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime) override;

public:
    // The time it takes to reach full speed.
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings, Config)
    float AccelerationTime = 1.f;

    // Apply an acceleration curve to your input so that it accelerates as time goes by. The given curve should be time-normalized, i.e., between 0 and 1.
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings, meta = (DisplayThumbnail = "false"))
    TObjectPtr<UCurveFloat> AccelerationCurve;

private:
    float Timer = 0.f;
};