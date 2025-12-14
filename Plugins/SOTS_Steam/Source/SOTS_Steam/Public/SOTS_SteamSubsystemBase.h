#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Subsystems/SubsystemCollection.h"
#include "SOTS_SteamSubsystemBase.generated.h"

class IOnlineSubsystem;

UCLASS(Abstract)
class SOTS_STEAM_API USOTS_SteamSubsystemBase : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam")
    bool IsSteamAvailable() const;

protected:
    void LogVerbose(const FString& Message) const;
    void LogWarning(const FString& Message) const;

    IOnlineSubsystem* GetOnlineSubsystemSafe() const;
    void RefreshSteamAvailability();

private:
    bool bSteamAvailabilityChecked = false;
    bool bSteamAvailable = false;
    bool bLoggedUnavailable = false;
    bool bRequireSteamSubsystem = false;
    bool bEnableSteam = true;
    FName PreferredSubsystemName = FName(TEXT("STEAM"));
    FName ActiveSubsystemName = NAME_None;
    IOnlineSubsystem* CachedOnlineSubsystem = nullptr;
};
