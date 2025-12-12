#include "SOTS_HUDSubsystem.h"

#include "Math/UnrealMathUtility.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void USOTS_HUDSubsystem::SetHealthPercent(float InPercent)
{
	const float Clamped = FMath::Clamp(InPercent, 0.f, 1.f);
	if (!FMath::IsNearlyEqual(HealthPercent, Clamped))
	{
		HealthPercent = Clamped;
		OnHealthPercentChanged.Broadcast(HealthPercent);
	}
}

USOTS_HUDSubsystem* USOTS_HUDSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	if (const UWorld* World = WorldContextObject->GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSubsystem<USOTS_HUDSubsystem>();
		}
	}

	return nullptr;
}

void USOTS_HUDSubsystem::SetDetectionLevel(float InLevel)
{
	const float Clamped = FMath::Clamp(InLevel, 0.f, 1.f);
	if (!FMath::IsNearlyEqual(DetectionLevel, Clamped))
	{
		DetectionLevel = Clamped;
		OnDetectionLevelChanged.Broadcast(DetectionLevel);
	}
}

void USOTS_HUDSubsystem::SetObjectiveText(const FString& InText)
{
	if (!ObjectiveText.Equals(InText, ESearchCase::CaseSensitive))
	{
		ObjectiveText = InText;
		OnObjectiveTextChanged.Broadcast(ObjectiveText);
	}
}
