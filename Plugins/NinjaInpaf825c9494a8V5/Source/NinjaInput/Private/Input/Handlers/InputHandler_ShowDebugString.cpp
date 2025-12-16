// Ninja Bear Studio Inc., all rights reserved.
#include "Input/Handlers/InputHandler_ShowDebugString.h"

#include "Components/NinjaInputManagerComponent.h"
#include "Engine/Engine.h"

UInputHandler_ShowDebugString::UInputHandler_ShowDebugString()
{
	DisplayString = FString("Input received from '{pawn}': {action} = {value}");
	Duration = 3.f;
	Color = FColor::Emerald;
	TriggerEvents.Add(ETriggerEvent::Triggered);
}

void UInputHandler_ShowDebugString::HandleTriggeredEvent_Implementation(UNinjaInputManagerComponent* Manager,
	const FInputActionValue& Value, const UInputAction* InputAction, const float ElapsedTime) const
{
#if WITH_EDITOR
	if (GEngine && ensure(IsValid(Manager)))
	{
		const float ValueMagnitude = Value.GetMagnitude();
		const FString ActionName = GetNameSafe(InputAction);
		const FString PawnName = GetNameSafe(Manager->GetPawn());

		FString Resolved = DisplayString;
		Resolved.ReplaceInline(TEXT("{value}"), *FString::SanitizeFloat(ValueMagnitude), ESearchCase::IgnoreCase);
		Resolved.ReplaceInline(TEXT("{action}"), *ActionName, ESearchCase::IgnoreCase);
		Resolved.ReplaceInline(TEXT("{pawn}"), *PawnName, ESearchCase::IgnoreCase);
		
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, Duration, Color, Resolved);		
	}
#endif
}
