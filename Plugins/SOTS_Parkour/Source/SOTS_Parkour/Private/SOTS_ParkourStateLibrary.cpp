#include "SOTS_ParkourStateLibrary.h"

#include "GameFramework/Actor.h"
#include "SOTS_ParkourComponent.h"
#include "SOTS_ParkourLog.h"

USOTS_ParkourComponent* USOTS_ParkourStateLibrary::GetParkourComponentFromActor(AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	// In CGF this was typically “GetComponentByClass(ParkourComp)”.
	// We mirror that here so existing Blueprint wiring stays intuitive.
	return Actor->FindComponentByClass<USOTS_ParkourComponent>();
}

ESOTS_ParkourState USOTS_ParkourStateLibrary::GetParkourStateFromActor(AActor* Actor)
{
	if (USOTS_ParkourComponent* ParkourComp = GetParkourComponentFromActor(Actor))
	{
		return ParkourComp->GetParkourState();
	}

	return ESOTS_ParkourState::Idle;
}

bool USOTS_ParkourStateLibrary::IsParkourActive(AActor* Actor)
{
	if (USOTS_ParkourComponent* ParkourComp = GetParkourComponentFromActor(Actor))
	{
		return ParkourComp->GetParkourState() == ESOTS_ParkourState::Active;
	}

	return false;
}

void USOTS_ParkourStateLibrary::TryPerformParkourOnActor(AActor* Actor)
{
	UE_LOG(LogSOTS_Parkour, Warning, TEXT("TryPerformParkourOnActor Actor=%s"), *GetNameSafe(Actor));

	if (USOTS_ParkourComponent* ParkourComp = GetParkourComponentFromActor(Actor))
	{
		UE_LOG(LogSOTS_Parkour, Warning, TEXT("TryPerformParkourOnActor forwarding to component %s"), *GetNameSafe(ParkourComp));
		ParkourComp->TryPerformParkour();
	}
	else
	{
		UE_LOG(LogSOTS_Parkour, Warning, TEXT("TryPerformParkourOnActor no SOTS_ParkourComponent found on Actor=%s"), *GetNameSafe(Actor));
	}
}
