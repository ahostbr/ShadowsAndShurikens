#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SOTS_CoreSettings.generated.h"

UCLASS(Config=Game, DefaultConfig)
class SOTS_CORE_API USOTS_CoreSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    static const USOTS_CoreSettings* Get() { return GetDefault<USOTS_CoreSettings>(); }

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

    virtual FName GetCategoryName() const override;
    virtual FName GetSectionName() const override;
};
