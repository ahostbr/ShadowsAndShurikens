#include "SOTS_InputDeviceLibrary.h"

ESOTS_InputDevice USOTS_InputDeviceLibrary::GetDeviceFromKey(const FKey& Key)
{
    if (Key.IsGamepadKey())
    {
        return ESOTS_InputDevice::Gamepad;
    }

    return ESOTS_InputDevice::KBM;
}

bool USOTS_InputDeviceLibrary::IsKBMKey(const FKey& Key)
{
    return !Key.IsGamepadKey();
}
