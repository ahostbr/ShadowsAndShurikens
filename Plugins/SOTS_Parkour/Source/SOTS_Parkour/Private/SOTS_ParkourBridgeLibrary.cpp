#include "SOTS_ParkourBridgeLibrary.h"

#include "SOTS_ParkourComponent.h"

namespace
{
	template<typename TEnum>
	FName SOTS_Parkour_EnumToNameLabel(TEnum Value)
	{
		if (const UEnum* EnumObj = StaticEnum<TEnum>())
		{
			return EnumObj->GetNameByValue(static_cast<int64>(Value));
		}
		return NAME_None;
	}
}

FName USOTS_ParkourBridgeLibrary::GetParkourStateLabel(const USOTS_ParkourComponent* ParkourComponent)
{
	if (!IsValid(ParkourComponent))
	{
		return NAME_None;
	}

	const ESOTS_ParkourState State = ParkourComponent->GetParkourState();
	return SOTS_Parkour_EnumToNameLabel(State);
}

FName USOTS_ParkourBridgeLibrary::GetParkourActionLabel(const USOTS_ParkourComponent* ParkourComponent)
{
	if (!IsValid(ParkourComponent))
	{
		return NAME_None;
	}

	const FSOTS_ParkourResult& Result = ParkourComponent->GetLastResult();
	return SOTS_Parkour_EnumToNameLabel(Result.Action);
}

FName USOTS_ParkourBridgeLibrary::GetParkourResultTypeLabel(const USOTS_ParkourComponent* ParkourComponent)
{
	if (!IsValid(ParkourComponent))
	{
		return NAME_None;
	}

	const FSOTS_ParkourResult& Result = ParkourComponent->GetLastResult();
	return SOTS_Parkour_EnumToNameLabel(Result.ResultType);
}

FName USOTS_ParkourBridgeLibrary::GetParkourClimbStyleLabel(const USOTS_ParkourComponent* ParkourComponent)
{
	if (!IsValid(ParkourComponent))
	{
		return NAME_None;
	}

	const FSOTS_ParkourResult& Result = ParkourComponent->GetLastResult();
	return SOTS_Parkour_EnumToNameLabel(Result.ClimbStyle);
}

bool USOTS_ParkourBridgeLibrary::HasValidParkourResult(const USOTS_ParkourComponent* ParkourComponent)
{
	if (!IsValid(ParkourComponent))
	{
		return false;
	}

	const FSOTS_ParkourResult& Result = ParkourComponent->GetLastResult();

	// Treat either legacy or new validity flags as sufficient for “has result”.
	if (Result.bHasResult || Result.bIsValid)
	{
		return true;
	}

	// As a small extra guard, also consider any non-None coarse result type.
	if (Result.ResultType != ESOTS_ParkourResultType::None)
	{
		return true;
	}

	return false;
}

TArray<FSOTS_ParkourWarpRuntimeTarget> USOTS_ParkourBridgeLibrary::GetParkourWarpTargets(const USOTS_ParkourComponent* ParkourComponent)
{
	TArray<FSOTS_ParkourWarpRuntimeTarget> Out;
	if (!IsValid(ParkourComponent))
	{
		return Out;
	}

	Out = ParkourComponent->GetLastResult().WarpTargets;
	return Out;
}
