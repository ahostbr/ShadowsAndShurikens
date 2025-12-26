#include "SOTS_SteamSubsystemBase.h"
#include "SOTS_SteamLog.h"
#include "Engine/Engine.h"
#include "OnlineSubsystem.h"
#include "SOTS_SteamSettings.h"

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

void USOTS_SteamSubsystemBase::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    RefreshSteamAvailability();
    LogVerbose(TEXT("initialized"));
}

void USOTS_SteamSubsystemBase::Deinitialize()
{
    LogVerbose(TEXT("deinitialized"));
    CachedOnlineSubsystem = nullptr;
    ActiveSubsystemName = NAME_None;
    bSteamAvailable = false;
    bSteamAvailabilityChecked = false;
    bLoggedUnavailable = false;
    Super::Deinitialize();
}

bool USOTS_SteamSubsystemBase::IsSteamAvailable() const
{
    if (!bSteamAvailabilityChecked)
    {
        const_cast<USOTS_SteamSubsystemBase*>(this)->RefreshSteamAvailability();
    }

    return bSteamAvailable && CachedOnlineSubsystem != nullptr;
}

void USOTS_SteamSubsystemBase::ForceRefreshSteamAvailability()
{
    RefreshSteamAvailability();
}

IOnlineSubsystem* USOTS_SteamSubsystemBase::GetOnlineSubsystemSafe() const
{
    return IsSteamAvailable() ? CachedOnlineSubsystem : nullptr;
}

void USOTS_SteamSubsystemBase::RefreshSteamAvailability()
{
    bSteamAvailabilityChecked = true;
    bSteamAvailable = false;
    CachedOnlineSubsystem = nullptr;
    ActiveSubsystemName = NAME_None;

    const USOTS_SteamSettings* Settings = USOTS_SteamSettings::Get();
    bEnableSteam = Settings ? Settings->bEnableSteam : true;
    bRequireSteamSubsystem = Settings ? Settings->bRequireSteamSubsystem : false;
    PreferredSubsystemName = (Settings && !Settings->PreferredSubsystemName.IsNone())
        ? Settings->PreferredSubsystemName
        : FName(TEXT("STEAM"));

    if (!bEnableSteam)
    {
        return;
    }

    IOnlineSubsystem* Subsystem = nullptr;

    if (!PreferredSubsystemName.IsNone())
    {
        Subsystem = IOnlineSubsystem::Get(PreferredSubsystemName);
    }

    if (!Subsystem)
    {
        Subsystem = IOnlineSubsystem::Get();
    }

    if (!Subsystem)
    {
        if (bRequireSteamSubsystem && !bLoggedUnavailable)
        {
            LogWarning(TEXT("Steam required but no OnlineSubsystem is available; Steam features disabled."));
            bLoggedUnavailable = true;
        }
        return;
    }

    ActiveSubsystemName = Subsystem->GetSubsystemName();
    const bool bIsSteamSubsystem =
        ActiveSubsystemName.IsEqual(PreferredSubsystemName, ENameCase::IgnoreCase) ||
        ActiveSubsystemName.IsEqual(FName(TEXT("STEAM")), ENameCase::IgnoreCase) ||
        ActiveSubsystemName.IsEqual(FName(TEXT("Steam")), ENameCase::IgnoreCase);

    if (!bIsSteamSubsystem)
    {
        if (bRequireSteamSubsystem && !bLoggedUnavailable)
        {
            LogWarning(FString::Printf(TEXT("Steam required but OnlineSubsystem '%s' is active; Steam features disabled."),
                *ActiveSubsystemName.ToString()));
            bLoggedUnavailable = true;
        }
        return;
    }

    CachedOnlineSubsystem = Subsystem;
    bSteamAvailable = true;
    bLoggedUnavailable = false;
}
