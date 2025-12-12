#include "SOTS_MMSSBPLibrary.h"
#include "SOTS_MMSSSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

USOTS_MMSSSubsystem* USOTS_MMSSBPLibrary::GetMusicManagerSubsystem(const UObject* WorldContextObject)
{
    if (!WorldContextObject || !GEngine)
    {
        return nullptr;
    }

    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if (!World)
    {
        return nullptr;
    }

    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
    {
        return nullptr;
    }

    return GI->GetSubsystem<USOTS_MMSSSubsystem>();
}

