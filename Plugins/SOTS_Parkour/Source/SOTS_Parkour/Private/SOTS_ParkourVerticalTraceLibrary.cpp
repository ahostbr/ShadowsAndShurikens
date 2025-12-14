#include "SOTS_ParkourVerticalTraceLibrary.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

float USOTS_ParkourVerticalTraceLibrary::GetVerticalWallDetectStartHeight(
	const ACharacter* Character,
	const UCharacterMovementComponent* MoveComp,
	ESOTS_ParkourState ParkourState)
{
	if (!Character || !MoveComp)
	{
		return 0.0f;
	}

	const FVector ActorLocation = Character->GetActorLocation();
	const bool bIsFalling = MoveComp->IsFalling();

	// CGF "NotBusy" behavior:
	//   ReturnValue = ActorLocation.Z - SelectFloat(40, 70, bPickA = IsFalling)
	const auto ComputeNotBusyValue = [&]() -> float
	{
		const float Offset = bIsFalling ? 40.0f : 70.0f;
		return ActorLocation.Z - Offset;
	};

	switch (ParkourState)
	{
	// Treat Idle/Entering/Exiting as the "NotBusy" bucket for now.
	case ESOTS_ParkourState::Idle:
	case ESOTS_ParkourState::Entering:
	case ESOTS_ParkourState::Exiting:
	default:
		return ComputeNotBusyValue();

	case ESOTS_ParkourState::Active:
		// TODO: When climb/free-hang state is fully mapped, port the CGF
		// "Parkour.State.Climb" branch here (uses climb anchor/arrow data).
		return ComputeNotBusyValue();
	}
}
