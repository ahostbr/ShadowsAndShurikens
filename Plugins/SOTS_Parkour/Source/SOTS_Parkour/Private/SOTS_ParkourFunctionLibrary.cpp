#include "SOTS_ParkourFunctionLibrary.h"

#include "Kismet/KismetMathLibrary.h"

namespace
{
FGameplayTag TagFromName(const TCHAR* TagName)
{
    return FGameplayTag::RequestGameplayTag(TagName, /*ErrorIfNotFound*/ false);
}

double SelectByTag(const FGameplayTag& Selection, const TPair<FGameplayTag, double>* Options, int32 OptionCount, double DefaultValue)
{
    for (int32 Index = 0; Index < OptionCount; ++Index)
    {
        if (Selection == Options[Index].Key)
        {
            return Options[Index].Value;
        }
    }
    return DefaultValue;
}

FGameplayTag SelectTagByTag(const FGameplayTag& Selection, const TPair<FGameplayTag, FGameplayTag>* Options, int32 OptionCount)
{
    for (int32 Index = 0; Index < OptionCount; ++Index)
    {
        if (Selection == Options[Index].Key)
        {
            return Options[Index].Value;
        }
    }
    return FGameplayTag();
}
} // namespace

FRotator USOTS_ParkourFunctionLibrary::ReverseRotation(const FRotator& InRotation)
{
    static const FRotator ReverseHalfTurn(0.0f, 180.0f, 0.0f);
    return UKismetMathLibrary::NormalizedDeltaRotator(InRotation, ReverseHalfTurn);
}

FRotator USOTS_ParkourFunctionLibrary::NormalReverseRotationZ(const FVector& InNormal)
{
    const FRotator NormalRot = UKismetMathLibrary::MakeRotFromX(InNormal);
    static const FRotator ReverseHalfTurn(0.0f, 180.0f, 0.0f);
    return UKismetMathLibrary::NormalizedDeltaRotator(NormalRot, ReverseHalfTurn);
}

double USOTS_ParkourFunctionLibrary::SelectClimbStyleFloat(const FGameplayTag& ClimbStyle, double Braced, double FreeHang)
{
    static const FGameplayTag TagBraced = TagFromName(TEXT("Parkour.ClimbStyle.Braced"));
    static const FGameplayTag TagFreeHang = TagFromName(TEXT("Parkour.ClimbStyle.FreeHang"));

    const TPair<FGameplayTag, double> Options[] = {
        {TagBraced, Braced},
        {TagFreeHang, FreeHang},
    };

    return SelectByTag(ClimbStyle, Options, UE_ARRAY_COUNT(Options), 0.0);
}

double USOTS_ParkourFunctionLibrary::SelectDirectionFloat(
    const FGameplayTag& Direction,
    double Forward,
    double Backward,
    double Left,
    double Right,
    double ForwardRight,
    double ForwardLeft,
    double BackwardRight,
    double BackwardLeft)
{
    static const FGameplayTag TagForward = TagFromName(TEXT("Parkour.Direction.Forward"));
    static const FGameplayTag TagBackward = TagFromName(TEXT("Parkour.Direction.Backward"));
    static const FGameplayTag TagLeft = TagFromName(TEXT("Parkour.Direction.Left"));
    static const FGameplayTag TagRight = TagFromName(TEXT("Parkour.Direction.Right"));
    static const FGameplayTag TagForwardRight = TagFromName(TEXT("Parkour.Direction.ForwardRight"));
    static const FGameplayTag TagForwardLeft = TagFromName(TEXT("Parkour.Direction.ForwardLeft"));
    static const FGameplayTag TagBackwardRight = TagFromName(TEXT("Parkour.Direction.BackwardRight"));
    static const FGameplayTag TagBackwardLeft = TagFromName(TEXT("Parkour.Direction.BackwardLeft"));

    const TPair<FGameplayTag, double> Options[] = {
        {TagForward, Forward},
        {TagBackward, Backward},
        {TagLeft, Left},
        {TagRight, Right},
        {TagForwardRight, ForwardRight},
        {TagForwardLeft, ForwardLeft},
        {TagBackwardRight, BackwardRight},
        {TagBackwardLeft, BackwardLeft},
    };

    return SelectByTag(Direction, Options, UE_ARRAY_COUNT(Options), 0.0);
}

double USOTS_ParkourFunctionLibrary::SelectParkourStateFloat(
    const FGameplayTag& ParkourState,
    double NotBusy,
    double Vault,
    double Mantle,
    double Climb)
{
    static const FGameplayTag TagNotBusy = TagFromName(TEXT("Parkour.State.NotBusy"));
    static const FGameplayTag TagVault = TagFromName(TEXT("Parkour.State.Vault"));
    static const FGameplayTag TagMantle = TagFromName(TEXT("Parkour.State.Mantle"));
    static const FGameplayTag TagClimb = TagFromName(TEXT("Parkour.State.Climb"));

    const TPair<FGameplayTag, double> Options[] = {
        {TagNotBusy, NotBusy},
        {TagVault, Vault},
        {TagMantle, Mantle},
        {TagClimb, Climb},
    };

    return SelectByTag(ParkourState, Options, UE_ARRAY_COUNT(Options), 0.0);
}

FGameplayTag USOTS_ParkourFunctionLibrary::SelectDirectionHopAction(
    const FGameplayTag& Direction,
    FGameplayTag Forward,
    FGameplayTag Left,
    FGameplayTag Right,
    FGameplayTag ForwardLeft,
    FGameplayTag ForwardRight,
    FGameplayTag Backward,
    FGameplayTag BackwardLeft,
    FGameplayTag BackwardRight)
{
    static const FGameplayTag TagForward = TagFromName(TEXT("Parkour.Direction.Forward"));
    static const FGameplayTag TagLeft = TagFromName(TEXT("Parkour.Direction.Left"));
    static const FGameplayTag TagRight = TagFromName(TEXT("Parkour.Direction.Right"));
    static const FGameplayTag TagForwardLeft = TagFromName(TEXT("Parkour.Direction.ForwardLeft"));
    static const FGameplayTag TagForwardRight = TagFromName(TEXT("Parkour.Direction.ForwardRight"));
    static const FGameplayTag TagBackward = TagFromName(TEXT("Parkour.Direction.Backward"));
    static const FGameplayTag TagBackwardLeft = TagFromName(TEXT("Parkour.Direction.BackwardLeft"));
    static const FGameplayTag TagBackwardRight = TagFromName(TEXT("Parkour.Direction.BackwardRight"));

    const TPair<FGameplayTag, FGameplayTag> Options[] = {
        {TagForward, Forward},
        {TagLeft, Left},
        {TagRight, Right},
        {TagForwardLeft, ForwardLeft},
        {TagForwardRight, ForwardRight},
        {TagBackward, Backward},
        {TagBackwardLeft, BackwardLeft},
        {TagBackwardRight, BackwardRight},
    };

    return SelectTagByTag(Direction, Options, UE_ARRAY_COUNT(Options));
}
