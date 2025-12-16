#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_InputDeviceTypes.h"
#include "SOTS_InputDeviceLibrary.generated.h"

UCLASS()
class SOTS_INPUT_API USOTS_InputDeviceLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "SOTS|Input")
    static ESOTS_InputDevice GetDeviceFromKey(const FKey& Key);

    UFUNCTION(BlueprintPure, Category = "SOTS|Input")
    static bool IsKBMKey(const FKey& Key);
};
