// SOTS_ParkourFunctionLibrary.h
// Blueprint helpers ported from CGF Parkour function library snippets.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_ParkourFunctionLibrary.generated.h"

UCLASS()
class SOTS_PARKOUR_API USOTS_ParkourFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Mirror the CGF "Reverse Rotation" snippet: returns Delta(In, Rot(0,180,0)). */
    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Math")
    static FRotator ReverseRotation(const FRotator& InRotation);

    /** Build a rotator from a normal, then mirror yaw by 180 degrees. */
    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Math")
    static FRotator NormalReverseRotationZ(const FVector& InNormal);

    /** Select a float by climb style tag (Braced / FreeHang). */
    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Select")
    static double SelectClimbStyleFloat(const FGameplayTag& ClimbStyle, double Braced, double FreeHang);

    /** Select a float by direction tag. */
    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Select")
    static double SelectDirectionFloat(
        const FGameplayTag& Direction,
        double Forward,
        double Backward,
        double Left,
        double Right,
        double ForwardRight,
        double ForwardLeft,
        double BackwardRight,
        double BackwardLeft);

    /** Select a float by parkour state tag (NotBusy / Vault / Mantle / Climb). */
    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Select")
    static double SelectParkourStateFloat(
        const FGameplayTag& ParkourState,
        double NotBusy,
        double Vault,
        double Mantle,
        double Climb);

    /** Select the hop action tag that corresponds to the supplied direction tag. */
    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Select")
    static FGameplayTag SelectDirectionHopAction(
        const FGameplayTag& Direction,
        FGameplayTag Forward,
        FGameplayTag Left,
        FGameplayTag Right,
        FGameplayTag ForwardLeft,
        FGameplayTag ForwardRight,
        FGameplayTag Backward,
        FGameplayTag BackwardLeft,
        FGameplayTag BackwardRight);
};
