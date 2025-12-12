#include "SOTS_SteamSubsystemBase.h"
#include "SOTS_SteamLog.h"
#include "Engine/Engine.h"
#include "SOTS_SteamSettings.h"

void USOTS_SteamSubsystemBase::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LogVerbose(TEXT("initialized"));
}

void USOTS_SteamSubsystemBase::Deinitialize()
{
    LogVerbose(TEXT("deinitialized"));
    Super::Deinitialize();
}

void USOTS_SteamSubsystemBase::LogVerbose(const FString& Message) const
{
    if (!USOTS_SteamSettings::IsVerboseLoggingEnabled())
    {
        return;
    }

    UE_LOG(LogSOTS_Steam, Verbose, TEXT("[%s] %s"), *GetClass()->GetName(), *Message);
}

void USOTS_SteamSubsystemBase::LogWarning(const FString& Message) const
{
    UE_LOG(LogSOTS_Steam, Warning, TEXT("[%s] %s"), *GetClass()->GetName(), *Message);
}
