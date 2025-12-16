#pragma once

#include "CoreMinimal.h"
#include "SOTS_InputDeviceTypes.generated.h"

UENUM(BlueprintType)
enum class ESOTS_InputDevice : uint8
{
    Unknown = 0,
    KBM,
    Gamepad
};
