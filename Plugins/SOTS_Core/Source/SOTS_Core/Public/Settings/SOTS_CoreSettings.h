#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SOTS_CoreVersion.h"
#include "SOTS_CoreSettings.generated.h"

class FProperty;

UCLASS(Config=Game, DefaultConfig)
class SOTS_CORE_API USOTS_CoreSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    static const USOTS_CoreSettings* Get() { return GetDefault<USOTS_CoreSettings>(); }

    UPROPERTY(VisibleAnywhere, Config, Category = "SOTS|Core")
    int32 ConfigSchemaVersion = SOTS_CORE_CONFIG_SCHEMA_VERSION;

    UPROPERTY(EditAnywhere, Config, Category = "SOTS|Core")
    bool bEnableWorldDelegateBridge = false;

    UPROPERTY(EditAnywhere, Config, Category = "SOTS|Core")
    bool bEnableVerboseCoreLogs = false;

    UPROPERTY(EditAnywhere, Config, Category = "SOTS|Core")
    bool bSuppressDuplicateNotifications = true;

    UPROPERTY(EditAnywhere, Config, Category = "SOTS|Core")
    bool bEnableLifecycleListenerDispatch = false;

    UPROPERTY(EditAnywhere, Config, Category = "SOTS|Core")
    bool bEnableLifecycleDispatchLogs = false;

    UPROPERTY(EditAnywhere, Config, Category = "SOTS|Core")
    bool bEnableMapTravelBridge = false;

    UPROPERTY(EditAnywhere, Config, Category = "SOTS|Core")
    bool bEnableMapTravelLogs = false;

    UPROPERTY(EditAnywhere, Config, Category = "SOTS|Core")
    bool bEnablePrimaryIdentityCache = true;

    UPROPERTY(EditAnywhere, Config, Category = "SOTS|Core")
    bool bEnableSaveParticipantQueries = false;

    UPROPERTY(EditAnywhere, Config, Category = "SOTS|Core")
    bool bEnableSaveContractLogs = false;

    virtual void PostInitProperties() override;
    virtual void PostReloadConfig(FProperty* PropertyThatWasLoaded) override;
    virtual FName GetCategoryName() const override;
    virtual FName GetSectionName() const override;

private:
    void ApplyConfigMigrations();
};
