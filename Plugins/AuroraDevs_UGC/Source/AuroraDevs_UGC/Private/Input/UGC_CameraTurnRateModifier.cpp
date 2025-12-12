// Copyright(c) Aurora Devs 2024. All Rights Reserved.

#include "Input/UGC_CameraTurnRateModifier.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/UGC_PlayerCameraManager.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

FInputActionValue UUGC_CameraTurnRateModifier::ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime)
{
    EInputActionValueType ValueType = CurrentValue.GetValueType();
    if (ValueType == EInputActionValueType::Boolean)
    {
        return CurrentValue;
    }

    if (!PlayerCameraManager)
    {
        if (!PlayerInput->GetOuterAPlayerController() || !PlayerInput->GetOuterAPlayerController()->PlayerCameraManager)
        {
    #if ENABLE_DRAW_DEBUG
            // Debugging
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Red, FString::Printf(TEXT("UGC_CameraSlowDownInputModifier: Could not find Player Camera Manager to use Camera Slow Down constraint.")));
            }
    #endif
            return CurrentValue;
        }

        AUGC_PlayerCameraManager* PCManager = Cast<AUGC_PlayerCameraManager>(PlayerInput->GetOuterAPlayerController()->PlayerCameraManager);
        if (!PCManager)
        {
    #if ENABLE_DRAW_DEBUG
            // Debugging
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Red, FString::Printf(TEXT("UGC_CameraSlowDownInputModifier: Player's Camera Manager does not inherit from UGC_PlayerCameraManager. Angle Constraint cannot be used.")));
            }
    #endif
            return CurrentValue;
        }
        // Here we know that it's not null
        PlayerCameraManager = PCManager;
    }

    return CurrentValue.Get<FVector>() * PlayerCameraManager->GetCameraTurnRate();
}