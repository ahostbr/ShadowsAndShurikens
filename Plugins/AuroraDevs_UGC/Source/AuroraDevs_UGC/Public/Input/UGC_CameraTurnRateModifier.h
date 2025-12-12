// Copyright(c) Aurora Devs 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputModifiers.h"
#include "UGC_CameraTurnRateModifier.generated.h"

/**
 *
 */
UCLASS()
class AURORADEVS_UGC_API UUGC_CameraTurnRateModifier : public UInputModifier
{
    GENERATED_BODY()
public:
    virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime) override;

private:
    TObjectPtr<class AUGC_PlayerCameraManager> PlayerCameraManager;
};