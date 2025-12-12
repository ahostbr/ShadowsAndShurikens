#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Subsystems/SubsystemCollection.h"
#include "SOTS_SteamSubsystemBase.generated.h"

UCLASS(Abstract)
class SOTS_STEAM_API USOTS_SteamSubsystemBase : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

protected:
    void LogVerbose(const FString& Message) const;
    void LogWarning(const FString& Message) const;
};
