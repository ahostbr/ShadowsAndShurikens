#pragma once

#include "CoreMinimal.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

class USOTS_CoreLifecycleSubsystem;

namespace SOTS_Core::Private
{
	inline USOTS_CoreLifecycleSubsystem* GetLifecycleSubsystem(const UObject* WorldContext)
	{
		if (!WorldContext)
		{
			return nullptr;
		}

		UWorld* World = WorldContext->GetWorld();
		if (!World)
		{
			return nullptr;
		}

		UGameInstance* GameInstance = World->GetGameInstance();
		if (!GameInstance)
		{
			return nullptr;
		}

		return GameInstance->GetSubsystem<USOTS_CoreLifecycleSubsystem>();
	}
}
