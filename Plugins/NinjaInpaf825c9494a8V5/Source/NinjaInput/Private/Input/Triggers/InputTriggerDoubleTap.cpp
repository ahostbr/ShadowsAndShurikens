// Ninja Bear Studio Inc., all rights reserved.
#include "Input/Triggers/InputTriggerDoubleTap.h"

#include "EnhancedPlayerInput.h"
#include "InputTriggers.h"

UInputTriggerDoubleTap::UInputTriggerDoubleTap(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bAffectedByTimeDilation = false;
	TapReleaseTimeThreshold = 0.2f;
	TapIntervalThreshold = 0.5f;
	HeldDuration = 0.0f;
}

ETriggerEventsSupported UInputTriggerDoubleTap::GetSupportedTriggerEvents() const
{
	return ETriggerEventsSupported::Ongoing;
}

ETriggerState UInputTriggerDoubleTap::UpdateState_Implementation(const UEnhancedPlayerInput* PlayerInput,
	const FInputActionValue ModifiedValue, const float DeltaTime)
{
	ETriggerState State = ETriggerState::None;
	
	const float LastHeldDuration = HeldDuration;
	HeldDuration = CalculateHeldDuration(PlayerInput, DeltaTime);

	const bool bIsPressed = IsActuated(LastValue);
	const bool bIsReleased = !IsActuated(ModifiedValue);

	if (bIsPressed && LastHeldDuration < TapReleaseTimeThreshold)
	{
		if (Taps == 0)
		{
			State = ETriggerState::Ongoing;
			Taps = 1;
			LastTapTime = 0.f;
		}
		else if (Taps == 1 && LastTapTime <= TapIntervalThreshold)
		{
			State = ETriggerState::Triggered;
			Taps = 0;
			LastTapTime = 0.f;
		}
	}
	else if (HeldDuration >= TapReleaseTimeThreshold)
	{
		// Player held input too long.
		State = ETriggerState::None;
		HeldDuration = 0.f;
	}

	// Update time since last tap if needed.
	if (Taps > 0)
	{
		LastTapTime += DeltaTime;

		// Reset if exceeded max interval
		if (LastTapTime > TapIntervalThreshold)
		{
			Taps = 0;
			LastTapTime = 0.f;
		}
	}

	// Reset held duration if released.
	if (bIsReleased)
	{
		HeldDuration = 0.f;
	}

	return State;
}

float UInputTriggerDoubleTap::CalculateHeldDuration(const UEnhancedPlayerInput* const PlayerInput, const float DeltaTime) const
{
	// We may not have a PlayerInput object during automation tests, so default to 1.0f if we don't have one.
	// This will mean that TimeDilation has no effect.
	const float TimeDilation = PlayerInput ? PlayerInput->GetEffectiveTimeDilation() : 1.0f;
	
	// Calculates the new held duration, applying time dilation if desired
	return HeldDuration + (!bAffectedByTimeDilation ? DeltaTime : DeltaTime * TimeDilation);	
}

FString UInputTriggerDoubleTap::GetDebugState() const
{
	if (Taps > 0 || HeldDuration > 0.f)
	{
		return FString::Printf(TEXT("Taps: %d, Held: %.2f, SinceLastTap: %.2f"), Taps, HeldDuration, LastTapTime);
	}

	return FString();
}